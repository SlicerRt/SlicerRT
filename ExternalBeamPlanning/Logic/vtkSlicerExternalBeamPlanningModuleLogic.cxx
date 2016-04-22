/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#include "vtkSlicerExternalBeamPlanningModuleLogic.h"

// SlicerRT includes
#include "PlmCommon.h"
#include "SlicerRtCommon.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkSlicerDoseCalculationEngine.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Plastimatch includes
#include "itk_image_accumulate.h"
#include "itk_image_create.h"
#include "itk_image_save.h"
#include "itk_image_scale.h"
#include "itk_image_stats.h"
#include "plm_image.h"
#include "plm_image_header.h"
#include "rpl_volume.h"
#include "rt_beam.h"
#include "rt_plan.h"
#include "string_util.h"

// CLI invocation
#include <qSlicerCLIModule.h>
#include <vtkSlicerCLIModuleLogic.h>

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkConeSource.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkPolyData.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkImageCast.h>
#include <vtkPiecewiseFunction.h>
#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkVolumeRayCastMapper.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkVolumeRayCastCompositeFunction.h>
#include <vtkVolumeTextureMapper3D.h>
#include <vtkVolumeTextureMapper2D.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageShiftScale.h>
#include <vtkImageExtractComponents.h>
#include <vtkTransform.h>
#include <vtkPolyDataMapper.h>
#include <vtkImageGradientMagnitude.h>
#include <vtkImageMathematics.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

//----------------------------------------------------------------------------
class vtkSlicerExternalBeamPlanningModuleLogic::vtkInternal
{
public:
  vtkInternal();

  vtkSlicerCLIModuleLogic* MatlabDoseCalculationModuleLogic;
  vtkSlicerDoseCalculationEngine* DoseEngine;

  Plm_image::Pointer plmRef;
  float TotalRx;
};

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkInternal::vtkInternal()
{
  this->MatlabDoseCalculationModuleLogic = 0;
  this->DoseEngine = vtkSlicerDoseCalculationEngine::New();
  this->TotalRx = 0.f;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerExternalBeamPlanningModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkSlicerExternalBeamPlanningModuleLogic()
{
  this->RTPlanNode = NULL;

  this->DRRImageSize[0] = 256;
  this->DRRImageSize[1] = 256;

  this->Internal = new vtkInternal;
}

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::~vtkSlicerExternalBeamPlanningModuleLogic()
{
  if (this->Internal->DoseEngine)
  {
    this->Internal->DoseEngine->Delete();
    this->Internal->DoseEngine = NULL;
  }
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetAndObserveRTPlanNode(vtkMRMLRTPlanNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->RTPlanNode, node);

  // Create an output dose volume for the plan if not yet present
  if (this->RTPlanNode && !this->RTPlanNode->GetDoseVolumeNode())
  {
    vtkSmartPointer<vtkMRMLScalarVolumeNode> newDoseVolume = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    std::string newDoseVolumeName = std::string(this->RTPlanNode->GetName()) + "_TotalDose";
    newDoseVolume->SetName(newDoseVolumeName.c_str());

    if (!this->GetMRMLScene())
    {
      vtkErrorMacro("SetAndObserveRTPlanNode: Invalid MRML scene!");
      return;
    }
    this->GetMRMLScene()->AddNode(newDoseVolume);
    this->RTPlanNode->SetAndObserveDoseVolumeNode(newDoseVolume);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node!");
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLRTPlanNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or input node!");
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLRTPlanNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLRTPlanNode *planNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
  if (node)
  {
    planNode = vtkMRMLRTPlanNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->RTPlanNode, planNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateBeamTransform(vtkMRMLRTBeamNode *beamNode)
{
  if (!this->GetMRMLScene() || !this->RTPlanNode)
  {
    vtkErrorMacro("UpdateBeamTransform: Invalid MRML scene or RT plan node!");
    return;
  }

  vtkSlicerBeamsModuleLogic::UpdateBeamTransform(this->GetMRMLScene(), beamNode);
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateBeamGeometryModel(char *beamName)
{
  if (!this->GetMRMLScene() || !this->RTPlanNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Invalid MRML scene or RT plan node!");
    return;
  }

  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->RTPlanNode->GetReferenceVolumeNode();

  // Make sure inputs are initialized
  if (!referenceVolumeNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Unable to access reference volume node!");
    return;
  }

  // Get beam node by name
  vtkMRMLRTBeamNode* beamNode = this->RTPlanNode->GetRTBeamNodeByName(beamName);
  if (!beamNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Unable to access beam node with name " << (beamName?beamName:"NULL"));
    return;
  }

  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = beamNode->GetIsocenterFiducialNode();
  if (!isocenterMarkupsNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Unable to access isocenter fiducial node!");
    return;
  }

  vtkSmartPointer<vtkPolyData> beamModelPolyData = NULL;
  vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = beamNode->GetMLCPositionDoubleArrayNode();
  if (MLCPositionDoubleArrayNode)
  {
    beamModelPolyData = vtkSlicerBeamsModuleLogic::CreateBeamPolyData(
        beamNode->GetX1Jaw(),
        beamNode->GetX2Jaw(), 
        beamNode->GetY1Jaw(),
        beamNode->GetY2Jaw(),
        beamNode->GetSAD(),
        beamNode->GetMLCPositionDoubleArrayNode()->GetArray());
  }
  else
  {
    beamModelPolyData = vtkSlicerBeamsModuleLogic::CreateBeamPolyData(
        beamNode->GetX1Jaw(),
        beamNode->GetX2Jaw(), 
        beamNode->GetY1Jaw(),
        beamNode->GetY2Jaw(),
        beamNode->GetSAD());
  }

  beamNode->SetAndObservePolyData(beamModelPolyData);
}

//---------------------------------------------------------------------------
bool vtkSlicerExternalBeamPlanningModuleLogic::ComputeTargetVolumeCenter(vtkMRMLRTBeamNode *beamNode, double* center)
{
  if (!this->GetMRMLScene() || !this->RTPlanNode)
  {
    vtkErrorMacro("ComputeTargetVolumeCenter: Invalid MRML scene or RT plan node!");
    return false;
  }
  if (!beamNode)
  {
    vtkErrorMacro("ComputeTargetVolumeCenter: Invalid beam node!");
    return false;
  }

  return beamNode->ComputeTargetVolumeCenter(center);
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetBeamIsocenterToTargetCenter(vtkMRMLRTBeamNode *beamNode)
{
  if (!beamNode)
  {
    vtkErrorMacro("SetBeamIsocenterToTargetCenter: Invalid beam node!");
    return;
  }

  beamNode->SetIsocenterToTargetCenter();
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkSlicerExternalBeamPlanningModuleLogic::AddBeam(vtkMRMLRTBeamNode *copyFrom)
{
  if (!this->GetMRMLScene() || !this->RTPlanNode)
  {
    vtkErrorMacro("AddBeam: Invalid MRML scene or RT plan node!");
    return 0;
  }

  // **** TODO GCS FIX --- this will inspect the radiation type, and then 
  // create the correct beam node using plugin logic

  // Create rtbeam node
  vtkSmartPointer<vtkMRMLRTProtonBeamNode> beamNode = vtkSmartPointer<vtkMRMLRTProtonBeamNode>::New();
  vtkMRMLRTProtonBeamNode* copyfromProtonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(copyFrom);
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
  // If template beam was given, clone into new beam
  if (copyFrom)
  {
    beamNode->Copy(copyFrom);
    protonNode->Copy(copyfromProtonNode);
  }
  

  // Add beam to scene
  this->GetMRMLScene()->AddNode(beamNode);

  // Attach model to beam
  vtkSlicerBeamsModuleLogic::AddDefaultModelToRTBeamNode(this->GetMRMLScene(), beamNode);

  // Attach beam to plan
  this->RTPlanNode->AddRTBeamNode(beamNode);

  return beamNode;
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RemoveBeam(vtkMRMLRTBeamNode *beam)
{
  if ( !this->GetMRMLScene() || !this->RTPlanNode )
  {
    vtkErrorMacro("RemoveBeam: Invalid MRML scene or RT plan node!");
    return;
  }

  // Remove the beam
  this->RTPlanNode->RemoveRTBeamNode(beam);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateDRR(char *beamName)
{
  if ( !this->GetMRMLScene() || !this->RTPlanNode )
  {
    vtkErrorMacro("RemoveBeam: Invalid MRML scene or RT plan node!");
    return;
  }

#if defined (commentout)
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->RTPlanNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    vtkErrorMacro("UpdateDRR: Failed to access reference volume node!");
    return;
  }

  // Get beam node by name
  vtkMRMLRTBeamNode* beamNode = this->RTPlanNode->GetRTBeamNodeByName(beamName);
  if (!beamNode)
  {
    vtkErrorMacro("UpdateDRR: Unable to access beam node with name " << (beamName?beamName:"NULL"));
    return;
  }

  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = beamNode->GetIsocenterFiducialNode();
  // vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = beamNode->GetMLCPositionDoubleArrayNode();
  if (!isocenterMarkupsNode) // || !MLCPositionDoubleArrayNode)
  {
    vtkErrorMacro("UpdateDRR: Inputs are not initialized!");
    return;
  }

  // Cast image data to uchar for faster rendering (this is for CT data only now)
  vtkSmartPointer<vtkImageShiftScale> cast = vtkSmartPointer<vtkImageShiftScale>::New();
  cast->SetInputData(referenceVolumeNode->GetImageData());
  cast->SetOutputScalarTypeToUnsignedChar();
  cast->SetShift(1000);
  cast->SetScale(255./2000.);
  cast->SetClampOverflow(1);
  cast->Update();

  // Create the renderer, render window 
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetOffScreenRendering(1);
  renWin->AddRenderer(renderer);

  // Create our volume and mapper
  // Currently use cpu ray casting as gpu ray casting and texturemapping 3d are not working on my computer
  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  vtkSmartPointer<vtkVolumeRayCastMapper> mapper = vtkSmartPointer<vtkVolumeRayCastMapper>::New();
  vtkSmartPointer<vtkVolumeRayCastCompositeFunction> compositeFunction = vtkSmartPointer<vtkVolumeRayCastCompositeFunction>::New();
  mapper->SetVolumeRayCastFunction(compositeFunction);
  //vtkSmartPointer<vtkVolumeTextureMapper3D> mapper = vtkSmartPointer<vtkVolumeTextureMapper3D>::New();
  //vtkSmartPointer<vtkVolumeTextureMapper2D> mapper = vtkSmartPointer<vtkVolumeTextureMapper2D>::New();
  //vtkSmartPointer<vtkGPUVolumeRayCastMapper> mapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
  mapper->SetInputData( cast->GetOutput() );
  mapper->SetBlendModeToComposite();
  volume->SetMapper( mapper );

  // Create our transfer function
  vtkSmartPointer<vtkColorTransferFunction> colorFun = vtkSmartPointer<vtkColorTransferFunction>::New();
  vtkSmartPointer<vtkPiecewiseFunction> opacityFun = vtkSmartPointer<vtkPiecewiseFunction>::New();

  // Create the property and attach the transfer functions
  vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->SetColor( colorFun );
  volumeProperty->SetScalarOpacity( opacityFun );
  volumeProperty->SetInterpolationTypeToLinear();

  colorFun->AddRGBPoint( 0, 0, 0, 0);
  colorFun->AddRGBPoint( 128, 0, 0, 0);
  colorFun->AddRGBPoint( 255, 1, 1, 1);

  opacityFun->AddPoint(0, 0 );
  opacityFun->AddPoint(128, 0 );
  opacityFun->AddPoint(255, 0.2);

  volumeProperty->ShadeOff();
  volumeProperty->SetScalarOpacityUnitDistance(0.8919);

  // connect up the volume to the property and the mapper
  volume->SetProperty( volumeProperty );

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateY(beamNode->GetGantryAngle());
  transform->RotateX(90);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  isocenterMarkupsNode->GetNthFiducialPosition(0,isoCenterPosition);
  transform2->Translate(-isoCenterPosition[0], -isoCenterPosition[1], -isoCenterPosition[2]);

  vtkSmartPointer<vtkTransform> transform3 = vtkSmartPointer<vtkTransform>::New();
  vtkSmartPointer<vtkMatrix4x4> IJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  referenceVolumeNode->GetIJKToRASMatrix(IJK2RASMatrix);
  transform3->SetMatrix(IJK2RASMatrix);

  transform->PreMultiply();
  transform->Concatenate(transform2);
  transform->Concatenate(transform3);
  volume->SetUserTransform( transform );

  // The usual rendering stuff.
  vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
  // fixed camera parameters for now
  camera->SetPosition(0, 0, 1000);
  camera->SetFocalPoint(0, 0, 0);
  camera->SetViewAngle(14.68);
  camera->SetClippingRange(100, 2000);
  camera->ParallelProjectionOff();

  // Add the volume to the scene
  renderer->AddVolume( volume );
  renderer->SetActiveCamera(camera);

  renWin->SetSize(this->DRRImageSize[0], this->DRRImageSize[1]);
  renWin->Render();

  // Capture and convert to 2D image
  vtkSmartPointer<vtkWindowToImageFilter> windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
  windowToImage->SetInput( renWin );

  vtkSmartPointer<vtkImageExtractComponents> extract = vtkSmartPointer<vtkImageExtractComponents>::New();
  extract->SetInputConnection( windowToImage->GetOutputPort() );
  extract->SetComponents(0);
  extract->Update();

  // Add the drr image to mrml scene
  vtkSmartPointer<vtkMRMLScalarVolumeNode> drrImageNode = beamNode->GetDRRVolumeNode();
  if (!drrImageNode)
  {
    drrImageNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    drrImageNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->AddNode(drrImageNode));
    std::string DRRImageNodeName = std::string(beamName) + std::string("_DRRImage");
    DRRImageNodeName = this->GetMRMLScene()->GenerateUniqueName(DRRImageNodeName);
    drrImageNode->SetName(DRRImageNodeName.c_str());
    beamNode->SetAndObserveDRRVolumeNode(drrImageNode);
  }
  drrImageNode->SetAndObserveImageData(extract->GetOutput());
  drrImageNode->SetOrigin(-128,-128,0);
  drrImageNode->SetSelectable(1);

  vtkSmartPointer<vtkMRMLSliceLogic> sliceLogic = this->GetApplicationLogic()->GetSliceLogicByLayoutName("Slice4");
  if (!sliceLogic)
  {
    vtkErrorMacro("UpdateDRR: Invalid sliceLogic for DRR viewer!");
    return;
  }

  // Transforms to place the DRR image aligned with the beam geometry
  vtkSmartPointer<vtkTransform> transformDRR = vtkSmartPointer<vtkTransform>::New();
  transformDRR->Identity();
  transformDRR->RotateZ(beamNode->GetGantryAngle());
  transformDRR->RotateX(-90);

  vtkSmartPointer<vtkTransform> transformDRR2 = vtkSmartPointer<vtkTransform>::New();
  transformDRR2->Identity();
  transformDRR2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transformDRR->PostMultiply();
  transformDRR->Concatenate(transformDRR2->GetMatrix());

  vtkSmartPointer<vtkMRMLLinearTransformNode> drrImageTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(drrImageNode->GetTransformNodeID()));
  if (!drrImageTransformNode)
  {
    drrImageTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    drrImageTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->AddNode(drrImageTransformNode));
    std::string DRRImageTransformNodeName = std::string(beamName) + std::string("_DRRImage_Transform");
    DRRImageTransformNodeName = this->GetMRMLScene()->GenerateUniqueName(DRRImageTransformNodeName);
    drrImageTransformNode->SetName(DRRImageTransformNodeName.c_str());
    drrImageNode->SetAndObserveTransformNodeID(drrImageTransformNode->GetID());
  }
  drrImageTransformNode->SetAndObserveMatrixTransformToParent(transformDRR->GetMatrix());

  // 
  vtkSmartPointer<vtkMRMLSliceNode> sliceNode = sliceLogic->GetSliceNode();
  vtkSmartPointer<vtkMRMLSliceCompositeNode> compositeSliceNode = sliceLogic->GetSliceCompositeNode();
  compositeSliceNode->SetBackgroundVolumeID(drrImageNode->GetID());

  //TODO: convert to segmentations
  // Get list of segments from the segmentation node
  vtkSmartPointer<vtkSegmentation> selectedSegmentation = this->RTPlanNode->GetTargetSegmentationNode()->GetSegmentation();
  vtkSegmentation::SegmentMap segmentMap = selectedSegmentation->GetSegments();

  // Compute DVH and get result nodes
  vtkSmartPointer<vtkImageData> mergedImageData = vtkSmartPointer<vtkImageData>::New();
  unsigned int numberOfContours = 0;
  
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
  {
    numberOfContours++;
    // Create the translucent contour object in BEV 
    // Create the renderer, render window 
    vtkSmartPointer<vtkRenderer> renderer2 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renWin2 = vtkSmartPointer<vtkRenderWindow>::New();
    renWin2->SetOffScreenRendering(1);
    renWin2->AddRenderer(renderer2);

    // Now we'll look at it. //TODO: This comment is meaningless
    vtkSmartPointer<vtkPolyDataMapper> contourPolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkPolyData> polydata = vtkPolyData::SafeDownCast(segmentIt->second->GetRepresentation(vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()));
    contourPolyDataMapper->SetInputData(polydata);
    //contourPolyDataMapper->SetScalarRange(0,255);
    vtkSmartPointer<vtkActor> contourPolyDataActor = vtkSmartPointer<vtkActor>::New();
    contourPolyDataActor->SetMapper(contourPolyDataMapper);
    contourPolyDataActor->GetProperty()->LightingOff();
    contourPolyDataActor->GetProperty()->SetColor(1.0,1.0,1.0);

    vtkSmartPointer<vtkTransform> transformContourBEV = vtkSmartPointer<vtkTransform>::New();
    transformContourBEV->Identity();
    transformContourBEV->RotateY(beamNode->GetGantryAngle());
    transformContourBEV->RotateX(90);

    vtkSmartPointer<vtkTransform> transformContourBEV2 = vtkSmartPointer<vtkTransform>::New();
    transformContourBEV2->Identity();
    transformContourBEV2->Translate(-isoCenterPosition[0], -isoCenterPosition[1], -isoCenterPosition[2]);

    transformContourBEV->PreMultiply();
    transformContourBEV->Concatenate(transformContourBEV2);
    contourPolyDataActor->SetUserTransform( transformContourBEV );

    // The usual rendering stuff.
    vtkSmartPointer<vtkCamera> camera2 = vtkSmartPointer<vtkCamera>::New();
    // fixed camera parameters for now
    camera2->SetPosition(0, 0, 1000);
    camera2->SetFocalPoint(0, 0, 0);
    camera2->SetViewAngle(14.68);
    camera2->SetClippingRange(100, 2000);
    camera2->ParallelProjectionOff();

    // Add the volume to the scene
    renderer2->AddActor(contourPolyDataActor);
    renderer2->SetActiveCamera(camera2);

    renWin2->SetSize(this->DRRImageSize[0], this->DRRImageSize[1]);
    renWin2->Render();

    // Capture and convert to 2D image
    vtkSmartPointer<vtkWindowToImageFilter> windowToImage2 = vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImage2->SetInput( renWin2 );

    vtkSmartPointer<vtkImageExtractComponents> extract2 = vtkSmartPointer<vtkImageExtractComponents>::New();
    extract2->SetInputConnection( windowToImage2->GetOutputPort() );
    extract2->SetComponents(0);
    extract2->Update();

    // Investigate when contour technique to use (gradient )
    vtkSmartPointer<vtkImageGradientMagnitude> magnitude = vtkSmartPointer<vtkImageGradientMagnitude>::New();
    magnitude->SetInputConnection( extract2->GetOutputPort() );
    magnitude->SetDimensionality(2);
    magnitude->Update();

    if (numberOfContours > 1)
    {
      vtkSmartPointer<vtkImageMathematics> addFilter = vtkSmartPointer<vtkImageMathematics>::New(); 
      addFilter->SetInput1Data(mergedImageData);
      addFilter->SetInput2Data(magnitude->GetOutput());
      addFilter->SetOperationToAdd();
      addFilter->Update();
      mergedImageData->DeepCopy(addFilter->GetOutput());
    }
    else
    {
      mergedImageData->DeepCopy(magnitude->GetOutput());
    }
  }

  // Add the contour BEV image to mrml scene
  vtkSmartPointer<vtkMRMLScalarVolumeNode> contourBEVImageNode = beamNode->GetContourBEVVolumeNode();
  if (!contourBEVImageNode)
  {
    contourBEVImageNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    contourBEVImageNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->AddNode(contourBEVImageNode));
    std::string contourBEVImageNodeName = std::string(beamName) + std::string("_ContourBEVImage");
    contourBEVImageNodeName = this->GetMRMLScene()->GenerateUniqueName(contourBEVImageNodeName);
    contourBEVImageNode->SetName(contourBEVImageNodeName.c_str());
  }
  beamNode->SetAndObserveContourBEVVolumeNode(contourBEVImageNode);

  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> volumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(contourBEVImageNode->GetDisplayNode());
  if (!volumeDisplayNode)
  {
    // Set default colormap to the loaded one if found or generated, or to rainbow otherwise
    volumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    this->GetMRMLScene()->AddNode(volumeDisplayNode);
  }
  volumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeGreen");
  volumeDisplayNode->SetScalarRange(0,255);
  volumeDisplayNode->AutoWindowLevelOff();
  volumeDisplayNode->SetWindow(64);
  volumeDisplayNode->SetLevel(128);

  contourBEVImageNode->SetAndObserveDisplayNodeID(volumeDisplayNode->GetID());
  contourBEVImageNode->SetAndObserveImageData(mergedImageData);
  contourBEVImageNode->SetOrigin(-128,-128,0);
  contourBEVImageNode->SetSelectable(1);

  contourBEVImageNode->SetAndObserveTransformNodeID(drrImageTransformNode->GetID());

  compositeSliceNode->SetForegroundVolumeID(contourBEVImageNode->GetID());
  compositeSliceNode->SetForegroundOpacity(0.3);

  // Transform ???
  vtkSmartPointer<vtkTransform> transformSlice = vtkSmartPointer<vtkTransform>::New();
  transformSlice->Identity();
  transformSlice->RotateZ(beamNode->GetGantryAngle());
  transformSlice->RotateX(-90);
  transformSlice->Update();

  sliceNode->SetOrientationToReformat();
  sliceNode->SetSliceToRAS(transformSlice->GetMatrix());
  sliceNode->UpdateMatrices();

  sliceLogic->FitSliceToAll();

#endif
}

//---------------------------------------------------------------------------
std::string vtkSlicerExternalBeamPlanningModuleLogic::ComputeDose(vtkMRMLRTBeamNode* beamNode)
{
  if (!this->RTPlanNode)
  {
    std::string errorMessage("Invalid RT plan node");
    vtkErrorMacro("ComputeDose: " << errorMessage);
    return errorMessage;
  }

  std::string computeDoseErrorMessage("");
  if (this->RTPlanNode->GetDoseEngine() == vtkMRMLRTPlanNode::Plastimatch)
  {
    computeDoseErrorMessage = this->ComputeDoseByPlastimatch(beamNode);
  }
  else if (this->RTPlanNode->GetDoseEngine() == vtkMRMLRTPlanNode::Matlab)
  {
    computeDoseErrorMessage = this->ComputeDoseByMatlab(beamNode);
  }
  else
  {
    computeDoseErrorMessage = "Unknown dose engine";
    vtkErrorMacro("ComputeDose: " << computeDoseErrorMessage);
  }

  // Add RT plan to the same branch where the reference volume is
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->RTPlanNode->GetReferenceVolumeNode();
  vtkMRMLSubjectHierarchyNode* referenceVolumeShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(referenceVolumeNode);
  if (referenceVolumeShNode)
  {
    vtkMRMLSubjectHierarchyNode* planShNode = this->RTPlanNode->GetPlanSubjectHierarchyNode();
    if (planShNode)
    {
      planShNode->SetParentNodeID(referenceVolumeShNode->GetParentNodeID());
    }
    else
    {
      vtkErrorMacro("ComputeDose: Failed to acctess RT plan subject hierarchy node, although it should always be available!");
    }
  }

  return computeDoseErrorMessage;
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkOrientedImageData> 
vtkSlicerExternalBeamPlanningModuleLogic::GetTargetLabelmap(vtkMRMLRTBeamNode* beamNode)
{
  vtkSmartPointer<vtkOrientedImageData> targetLabelmap;
  vtkMRMLSegmentationNode* targetSegmentationNode = beamNode->GetTargetSegmentationNode();
  if (!targetSegmentationNode)
  {
    vtkErrorMacro("GetTargetLabelmap: Failed to get target segmentation node");
    return targetLabelmap;
  }

  vtkSegmentation *segmentation = targetSegmentationNode->GetSegmentation();
  if (!segmentation)
  {
    vtkErrorMacro("GetTargetLabelmap: Failed to get segmentation");
    return targetLabelmap;
  }
  if (!beamNode->GetTargetSegmentID())
  {
    vtkErrorMacro("GetTargetLabelmap: No target segment specified");
    return targetLabelmap;
  }

  vtkSegment *segment = segmentation->GetSegment(beamNode->GetTargetSegmentID());
  if (!segment) 
  {
    vtkErrorMacro("GetTargetLabelmap: Failed to get segment");
    return targetLabelmap;
  }
  
  if (segmentation->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    targetLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
    targetLabelmap->DeepCopy( vtkOrientedImageData::SafeDownCast(
        segment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) ) );
  }
  else
  {
    // Need to convert
    targetLabelmap = vtkSmartPointer<vtkOrientedImageData>::Take(
      vtkOrientedImageData::SafeDownCast(
        vtkSlicerSegmentationsModuleLogic::CreateRepresentationForOneSegment(
          segmentation,
          beamNode->GetTargetSegmentID(),
          vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName())));
    if (!targetLabelmap.GetPointer())
    {
      std::string errorMessage("Failed to convert target segment into binary labelmap");
      vtkErrorMacro("GetTargetLabelmap: " << errorMessage);
      return targetLabelmap;
    }
  }

  // Apply parent transformation nodes if necessary
  if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(targetSegmentationNode, targetLabelmap))
  {
    std::string errorMessage("Failed to apply parent transformation to target segment!");
    vtkErrorMacro("GetTargetLabelmap: " << errorMessage);
    return targetLabelmap;
  }
  return targetLabelmap;
}

//---------------------------------------------------------------------------
std::string vtkSlicerExternalBeamPlanningModuleLogic::ComputeDoseByPlastimatch(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    std::string errorMessage("Invalid beam node");
    vtkErrorMacro("ComputeDoseByPlastimatch: " << errorMessage);
    return errorMessage;
  }
  if ( !this->GetMRMLScene() || !this->RTPlanNode )
  {
    std::string errorMessage("Invalid MRML scene or RT plan node");
    vtkErrorMacro("ComputeDoseByPlastimatch: " << errorMessage);
    return errorMessage;
  }
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->RTPlanNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    std::string errorMessage("Unable to access reference volume node");
    vtkErrorMacro("ComputeDoseByPlastimatch: " << errorMessage);
    return errorMessage;
  }
  vtkMRMLSubjectHierarchyNode* rtPlanShNode = this->RTPlanNode->GetPlanSubjectHierarchyNode();
  if (!rtPlanShNode)
  {
    std::string errorMessage("Unable to access RT plan hierarchy");
    vtkErrorMacro("ComputeDoseByPlastimatch: " << errorMessage);
    return errorMessage;
  }

  // Get a labelmap for the target
  vtkSmartPointer<vtkOrientedImageData> targetLabelmap = this->GetTargetLabelmap(beamNode);
  if (targetLabelmap == NULL)
  {
    std::string errorMessage("Failed to access target labelmap");
    vtkErrorMacro("ComputeDoseByPlastimatch: " << errorMessage);
    return errorMessage;
  }

  // Convert inputs to ITK images
  Plm_image::Pointer plmTgt = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(targetLabelmap);
  if (!plmTgt)
  {
    std::string errorMessage("Failed to convert reference segment labelmap");
    vtkErrorMacro("ComputeDoseByPlastimatch: " << errorMessage);
    return errorMessage;
  }

  // Reference code for setting the geometry of the segmentation rasterization
  // in case the default one (from DICOM) is not desired
#if defined (commentout)
  vtkSmartPointer<vtkMatrix4x4> doseIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  protonDoseVolumeNode->GetIJKToRASMatrix(doseIjkToRasMatrix);
  std::string doseGeometryString = vtkSegmentationConverter::SerializeImageGeometry(doseIjkToRasMatrix, protonDoseVolumeNode->GetImageData());
  segmentationCopy->SetConversionParameter( vtkSegmentationConverter::GetReferenceImageGeometryParameterName(),
    doseGeometryString );
#endif
  
  //plmTgt->print();

  double src[3];
  double isocenter[3] = { 0, 0, 0 };
  beamNode->GetIsocenterPosition(isocenter);
  isocenter[0] = -isocenter[0];
  isocenter[1] = -isocenter[1];

  // Adjust src according to gantry angle
  double ga_radians = 
    beamNode->GetGantryAngle() * M_PI / 180.;
  double src_dist = beamNode->GetSAD();
  src[0] = isocenter[0] + src_dist * sin(ga_radians);
  src[1] = isocenter[1] - src_dist * cos(ga_radians);
  src[2] = isocenter[2];

  double RxDose = this->RTPlanNode->GetRxDose();

  // Calculate dose
  this->Internal->DoseEngine->CalculateDose(
    beamNode, 
    plmTgt, 
    isocenter, 
    src,
    RxDose);

  // Create the MRML node for the volume
  vtkSmartPointer<vtkMRMLScalarVolumeNode> rangeCompensatorVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  itk::Image<float, 3>::Pointer rcVolumeItk = this->Internal->DoseEngine->GetRangeCompensatorVolume();

  // Convert range compensator image to vtk
  vtkSmartPointer<vtkImageData> rangeCompensatorImageData = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(rcVolumeItk, rangeCompensatorImageData, VTK_FLOAT);

  rangeCompensatorVolumeNode->SetAndObserveImageData(rangeCompensatorImageData);
  rangeCompensatorVolumeNode->SetSpacing(
    rcVolumeItk->GetSpacing()[0],
    rcVolumeItk->GetSpacing()[1],
    rcVolumeItk->GetSpacing()[2] );
  rangeCompensatorVolumeNode->SetOrigin(
    rcVolumeItk->GetOrigin()[0],
    rcVolumeItk->GetOrigin()[1],
    rcVolumeItk->GetOrigin()[2] );

  std::string rangeCompensatorNodeName = std::string(beamNode->GetName()) + "_RangeCompensator";
  rangeCompensatorVolumeNode->SetName(rangeCompensatorNodeName.c_str());
  this->GetMRMLScene()->AddNode(rangeCompensatorVolumeNode);
  std::string rangeCompensatorNodeRef = vtkMRMLRTPlanNode::AssembleRangeCompensatorVolumeReference(beamNode);
  this->RTPlanNode->SetNodeReferenceID(rangeCompensatorNodeRef.c_str(), rangeCompensatorVolumeNode->GetID());
  if (rtPlanShNode)
  {
    vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      this->GetMRMLScene(), rtPlanShNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(), 
      rangeCompensatorNodeName.c_str(), rangeCompensatorVolumeNode.GetPointer() );
  }

  // Create the MRML node for the volume
  vtkSmartPointer<vtkMRMLScalarVolumeNode> apertureVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  itk::Image<unsigned char, 3>::Pointer apertureVolumeItk = this->Internal->DoseEngine->GetApertureVolume();

  // Convert aperture image to vtk
  vtkSmartPointer<vtkImageData> apertureImageData = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<unsigned char>(apertureVolumeItk, apertureImageData, VTK_UNSIGNED_CHAR);

  apertureVolumeNode->SetAndObserveImageData(apertureImageData);
  apertureVolumeNode->SetSpacing(
    apertureVolumeItk->GetSpacing()[0],
    apertureVolumeItk->GetSpacing()[1],
    apertureVolumeItk->GetSpacing()[2] );
  apertureVolumeNode->SetOrigin(
    apertureVolumeItk->GetOrigin()[0],
    apertureVolumeItk->GetOrigin()[1],
    apertureVolumeItk->GetOrigin()[2] );

  std::string apertureNodeName = std::string(beamNode->GetName()) + "_Aperture";
  apertureVolumeNode->SetName(apertureNodeName.c_str());
  this->GetMRMLScene()->AddNode(apertureVolumeNode);
  std::string apertureNodeRef = vtkMRMLRTPlanNode::AssembleApertureVolumeReference(beamNode);
  this->RTPlanNode->SetNodeReferenceID(apertureNodeRef.c_str(), apertureVolumeNode->GetID());
  if (rtPlanShNode)
  {
    vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      this->GetMRMLScene(), rtPlanShNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(), 
      apertureNodeName.c_str(), apertureVolumeNode.GetPointer() );
  }

  // Create the MRML node for the volume
  vtkSmartPointer<vtkMRMLScalarVolumeNode> protonDoseVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  // Convert dose image to vtk
  vtkSmartPointer<vtkImageData> protonDoseImageData = vtkSmartPointer<vtkImageData>::New();
  itk::Image<float, 3>::Pointer doseVolumeItk = this->Internal->DoseEngine->GetComputedDose();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(doseVolumeItk, protonDoseImageData, VTK_FLOAT);

  protonDoseVolumeNode->SetAndObserveImageData(protonDoseImageData);
  protonDoseVolumeNode->CopyOrientation(referenceVolumeNode);

  std::string protonDoseNodeName = std::string(beamNode->GetName()) + "_ProtonDose";
  protonDoseVolumeNode->SetName(protonDoseNodeName.c_str());
  this->GetMRMLScene()->AddNode(protonDoseVolumeNode);
  std::string protonDoseNodeRef = vtkMRMLRTPlanNode::AssembleProtonDoseVolumeReference(beamNode);
  this->RTPlanNode->SetNodeReferenceID(protonDoseNodeRef.c_str(), protonDoseVolumeNode->GetID());
  if (rtPlanShNode)
  {
    vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      this->GetMRMLScene(), rtPlanShNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(), 
      protonDoseNodeName.c_str(), protonDoseVolumeNode.GetPointer() );
  }

  protonDoseVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  protonDoseVolumeNode->CreateDefaultDisplayNodes(); // Make sure display node is present
  if (protonDoseVolumeNode->GetDisplayNode())
  {
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(protonDoseVolumeNode->GetVolumeDisplayNode());
    doseScalarVolumeDisplayNode->SetAutoWindowLevel(0);
    doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, 16.0);

    // Set colormap to rainbow
    doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    doseScalarVolumeDisplayNode->SetLowerThreshold(1.0);
    doseScalarVolumeDisplayNode->ApplyThresholdOn();
  }
  else
  {
    vtkWarningMacro("ComputeDose: Display node is not available for dose volume node. The default color table will be used.");
  }

  return "";
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeWED()
{
  //TODO: Needs re-implementation
  vtkErrorMacro("ComputeWED: Not implemented!");
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetMatlabDoseCalculationModuleLogic(vtkSlicerCLIModuleLogic* logic)
{
  this->Internal->MatlabDoseCalculationModuleLogic = logic;
}

//----------------------------------------------------------------------------
vtkSlicerCLIModuleLogic* vtkSlicerExternalBeamPlanningModuleLogic::GetMatlabDoseCalculationModuleLogic()
{
  return this->Internal->MatlabDoseCalculationModuleLogic;
}

//---------------------------------------------------------------------------
std::string vtkSlicerExternalBeamPlanningModuleLogic::ComputeDoseByMatlab(vtkMRMLRTBeamNode* beamNode)
{
  if ( !this->GetMRMLScene() || !this->RTPlanNode )
  {
    std::string errorMessage("Invalid MRML scene or RT plan node");
    vtkErrorMacro("ComputeDoseByMatlab: " << errorMessage);
    return errorMessage;
  }

#if defined (commentout)
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->RTPlanNode->GetReferenceVolumeNode();
  vtkMRMLScalarVolumeNode* outputDoseVolume = this->RTPlanNode->GetDoseVolumeNode();
  // Make sure inputs are initialized
  if (!referenceVolumeNode || !beamNode)
  {
    vtkErrorMacro("ComputeDoseByMatlab: Inputs are not initialized!");
    return;
  }

  if (this->Internal->MatlabDoseCalculationModuleLogic == 0)
  {
    std::cerr << "ComputeDoseByMatlab: ERROR: logic is not set!";
    return;
  }

  vtkSmartPointer<vtkMRMLCommandLineModuleNode> cmdNode = 
    this->Internal->MatlabDoseCalculationModuleLogic->CreateNodeInScene();;
  assert(cmdNode.GetPointer() != 0);

  cmdNode->SetParameterAsString("referencevolume", referenceVolumeNode->GetID());
  cmdNode->SetParameterAsString("outputdosevolume", outputDoseVolume->GetID());

  this->Internal->MatlabDoseCalculationModuleLogic->ApplyAndWait(cmdNode);

  this->GetMRMLScene()->RemoveNode(cmdNode);
#endif

  return "Matlab dose engine unavailable";
}

//---------------------------------------------------------------------------
std::string vtkSlicerExternalBeamPlanningModuleLogic::InitializeAccumulatedDose()
{
  if ( !this->GetMRMLScene() || !this->RTPlanNode )
  {
    std::string errorMessage("Invalid MRML scene or RT plan node");
    vtkErrorMacro("InitializeAccumulatedDose: " << errorMessage);
    return errorMessage;
  }

  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->RTPlanNode->GetReferenceVolumeNode();
  Plm_image::Pointer plmRef = PlmCommon::ConvertVolumeNodeToPlmImage(referenceVolumeNode);
  this->Internal->DoseEngine->InitializeAccumulatedDose(plmRef);

  return "";
}

//---------------------------------------------------------------------------
std::string vtkSlicerExternalBeamPlanningModuleLogic::FinalizeAccumulatedDose()
{
  if (!this->GetMRMLScene() || !this->RTPlanNode)
  {
    std::string errorMessage("Invalid MRML scene or RT plan node");
    vtkErrorMacro("FinalizeAccumulatedDose: " << errorMessage);
    return errorMessage;
  }

  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->RTPlanNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    std::string errorMessage("Unable to access reference volume");
    vtkErrorMacro("FinalizeAccumulatedDose: " << errorMessage);
    return errorMessage;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = this->RTPlanNode->GetDoseVolumeNode();
  if (!doseVolumeNode)
  {
    std::string errorMessage("Unable to access output dose volume");
    vtkErrorMacro("FinalizeAccumulatedDose: " << errorMessage);
    return errorMessage;
  }

  // Convert dose image to vtk
  vtkSmartPointer<vtkImageData> doseImageData = vtkSmartPointer<vtkImageData>::New();
  itk::Image<float, 3>::Pointer accumulateVolumeItk = this->Internal->DoseEngine->GetAccumulatedDose();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(accumulateVolumeItk, doseImageData, VTK_FLOAT);

  doseVolumeNode->SetAndObserveImageData(doseImageData);
  doseVolumeNode->CopyOrientation(referenceVolumeNode);

  this->RTPlanNode->SetNodeReferenceID(vtkMRMLRTPlanNode::OUTPUT_TOTAL_DOSE_VOLUME_REFERENCE_ROLE, doseVolumeNode->GetID());
  doseVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Add total dose volume to subject hierarchy under the study of the reference volume
  vtkMRMLSubjectHierarchyNode* referenceVolumeSHNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(referenceVolumeNode);
  if (referenceVolumeSHNode)
  {
    vtkMRMLSubjectHierarchyNode* studySHNode = referenceVolumeSHNode->GetAncestorAtLevel(
      vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    if (studySHNode)
    {
      vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
        this->GetMRMLScene(), studySHNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(), 
        NULL, doseVolumeNode );
    }
  }

  this->Internal->DoseEngine->CleanUp();
  double totalRx = this->Internal->DoseEngine->GetTotalRx();

  doseVolumeNode->CreateDefaultDisplayNodes(); // Make sure display node is present
  if (doseVolumeNode->GetVolumeDisplayNode())
  {
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(doseVolumeNode->GetDisplayNode());
    doseScalarVolumeDisplayNode->SetAutoWindowLevel(0);
    doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, totalRx);

    //TODO: Use dose color table
    // Set colormap to rainbow
    doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    doseScalarVolumeDisplayNode->SetLowerThreshold (0.05 * totalRx);
    doseScalarVolumeDisplayNode->ApplyThresholdOn ();
  }
  else
  {
    vtkWarningMacro("ComputeDose: Display node is not available for gamma volume node. The default color table will be used.");
  }

  // Select as active volume
  if (this->GetApplicationLogic() && this->GetApplicationLogic()->GetSelectionNode())
  {
    this->GetApplicationLogic()->GetSelectionNode()->SetReferenceSecondaryVolumeID(doseVolumeNode->GetID());
    this->GetApplicationLogic()->PropagateVolumeSelection(0);
    //TODO: Set opacity too (it's 0 by default so the volume is not visible)
  }

  return "";
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RemoveDoseNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene();

  std::vector<vtkMRMLRTBeamNode*> beams;
  this->RTPlanNode->GetRTBeamNodes(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator it=beams.begin(); it!=beams.end(); ++it)
  {
    vtkMRMLRTBeamNode* beamNode = (*it);
    if (beamNode)
    {
      std::string apertureNodeRef = vtkMRMLRTPlanNode::AssembleApertureVolumeReference(beamNode);
      vtkMRMLNode* apertureVolumeNode = this->RTPlanNode->GetNodeReference(apertureNodeRef.c_str());
      if (apertureVolumeNode)
      {
        scene->RemoveNode(apertureVolumeNode);
      }

      std::string rangeCompensatorNodeRef = vtkMRMLRTPlanNode::AssembleRangeCompensatorVolumeReference(beamNode);
      vtkMRMLNode* rangeCompensatorVolumeNode = this->RTPlanNode->GetNodeReference(rangeCompensatorNodeRef.c_str());
      if (rangeCompensatorVolumeNode)
      {
        scene->RemoveNode(rangeCompensatorVolumeNode);
      }

      std::string protonDoseNodeRef = vtkMRMLRTPlanNode::AssembleProtonDoseVolumeReference(beamNode);
      vtkMRMLNode* protonDoseVolumeNode = this->RTPlanNode->GetNodeReference(protonDoseNodeRef.c_str());
      if (protonDoseVolumeNode)
      {
        scene->RemoveNode(protonDoseVolumeNode);
      }
    }
  }

  vtkMRMLNode* totalProtonDoseVolumeNode = this->RTPlanNode->GetNodeReference(vtkMRMLRTPlanNode::OUTPUT_TOTAL_DOSE_VOLUME_REFERENCE_ROLE);
  if (totalProtonDoseVolumeNode)
  {
    scene->RemoveNode(totalProtonDoseVolumeNode);
  }
}
