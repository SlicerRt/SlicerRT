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
#include "vtkMRMLRTProtonBeamNode.h"
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLDoseAccumulationNode.h"
#include "vtkSlicerDoseAccumulationModuleLogic.h"
#include "vtkSlicerPlastimatchProtonDoseEngine.h"

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
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLSliceCompositeNode.h>

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
#include <vtkVolumeProperty.h>
#include <vtkVolumeRayCastMapper.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkVolumeRayCastCompositeFunction.h>
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
vtkCxxSetObjectMacro(vtkSlicerExternalBeamPlanningModuleLogic, BeamsLogic, vtkSlicerBeamsModuleLogic);

//----------------------------------------------------------------------------
class vtkSlicerExternalBeamPlanningModuleLogic::vtkInternal
{
public:
  vtkInternal();

  vtkSlicerCLIModuleLogic* MatlabDoseCalculationModuleLogic;
  vtkSlicerPlastimatchProtonDoseEngine* DoseEngine;
};

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkInternal::vtkInternal()
{
  this->MatlabDoseCalculationModuleLogic = 0;
  this->DoseEngine = vtkSlicerPlastimatchProtonDoseEngine::New();
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerExternalBeamPlanningModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkSlicerExternalBeamPlanningModuleLogic()
{
  this->DRRImageSize[0] = 256;
  this->DRRImageSize[1] = 256;

  this->BeamsLogic = NULL;

  this->Internal = new vtkInternal;
}

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::~vtkSlicerExternalBeamPlanningModuleLogic()
{
  this->SetBeamsLogic(NULL);

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

//-----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  //TODO: If nothing to register then remove function
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
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
    // Observe plan events
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTPlanNode::IsocenterModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(node, events);

    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndImport()
{
  // Observe beam events of all beam nodes
  this->GetMRMLScene()->InitTraversal();
  vtkMRMLNode *node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLRTPlanNode");
  while (node != NULL)
  {
    // Observe plan events
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTPlanNode::IsocenterModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(node, events);
    node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLRTPlanNode");
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
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLNodesEvents(caller, event, callData);

  vtkMRMLScene* mrmlScene = this->GetMRMLScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("ProcessMRMLNodesEvents: Invalid MRML scene!");
    return;
  }
  if (mrmlScene->IsBatchProcessing())
  {
    return;
  }

  if (caller->IsA("vtkMRMLRTPlanNode"))
  {
    vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(caller);

    if (event == vtkMRMLRTPlanNode::IsocenterModifiedEvent && this->BeamsLogic)
    {
      // Update transform for child beams
      std::vector<vtkMRMLRTBeamNode*> beams;
      planNode->GetBeams(beams);
      for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
      {
        vtkMRMLRTBeamNode* beamNode = (*beamIt);
        this->BeamsLogic->UpdateBeamTransform(beamNode);
      }
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkSlicerExternalBeamPlanningModuleLogic::CopyAndAddBeamToPlan(vtkMRMLRTBeamNode* copiedBeamNode, vtkMRMLRTPlanNode* planNode)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("CopyAndAddBeamToPlan: Invalid MRML scene!");
    return NULL;
  }
  if (!copiedBeamNode || !planNode)
  {
    vtkErrorMacro("CopyAndAddBeamToPlan: Invalid copied beam node or plan node!");
    return NULL;
  }

  // Create beam node of the same class as the template
  vtkSmartPointer<vtkMRMLRTBeamNode> beamNode;
  beamNode.TakeReference((vtkMRMLRTBeamNode*)this->GetMRMLScene()->CreateNodeByClass(copiedBeamNode->GetClassName()));
  if (!beamNode.GetPointer())
  {
    vtkErrorMacro("CopyAndAddBeamToPlan: Could not clone beam node");
    return NULL;
  }

  // Copy properties from template
  beamNode->CopyWithScene(copiedBeamNode); //TODO: Does not copy everything, Copy method not fully implemented!
  this->GetMRMLScene()->AddNode(beamNode);

  // Change name of new beam to default
  std::string newBeamName = planNode->GenerateNewBeamName();
  beamNode->SetName(newBeamName.c_str());

  // Create default model
  beamNode->CreateDefaultBeamModel();

  // Add beam to plan
  planNode->AddBeam(beamNode);

  return beamNode;
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateDRR(vtkMRMLRTPlanNode* planNode, char* beamName)
{
  if ( !this->GetMRMLScene() || !planNode )
  {
    vtkErrorMacro("RemoveBeam: Invalid MRML scene or RT plan node!");
    return;
  }

#if defined (commentout)
  vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    vtkErrorMacro("UpdateDRR: Failed to access reference volume node!");
    return;
  }

  // Get beam node by name
  vtkMRMLRTBeamNode* beamNode = planNode->GetBeamByName(beamName);
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
  vtkSmartPointer<vtkSegmentation> selectedSegmentation = planNode->GetTargetSegmentationNode()->GetSegmentation();
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
std::string vtkSlicerExternalBeamPlanningModuleLogic::CalculateDose(vtkMRMLRTBeamNode* beamNode)
{
  return this->Internal->DoseEngine->CalculateDose(beamNode);
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
std::string vtkSlicerExternalBeamPlanningModuleLogic::ComputeDoseByMatlab(vtkMRMLRTPlanNode* planNode, vtkMRMLRTBeamNode* beamNode)
{
  if ( !this->GetMRMLScene() || !planNode )
  {
    std::string errorMessage("Invalid MRML scene or RT plan node");
    vtkErrorMacro("ComputeDoseByMatlab: " << errorMessage);
    return errorMessage;
  }

#if defined (commentout)
  vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();
  vtkMRMLScalarVolumeNode* outputDoseVolume = planNode->GetOutputTotalDoseVolumeNode();
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
std::string vtkSlicerExternalBeamPlanningModuleLogic::CreateAccumulatedDose(vtkMRMLRTPlanNode* planNode)
{
  if (!this->GetMRMLScene() || !planNode)
  {
    std::string errorMessage("Invalid MRML scene or RT plan node");
    vtkErrorMacro("CreateAccumulatedDose: " << errorMessage);
    return errorMessage;
  }

  vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    std::string errorMessage("Unable to access reference volume");
    vtkErrorMacro("CreateAccumulatedDose: " << errorMessage);
    return errorMessage;
  }

  vtkMRMLScalarVolumeNode* totalDoseVolumeNode = planNode->GetOutputTotalDoseVolumeNode();
  if (!totalDoseVolumeNode)
  {
    std::string errorMessage("Unable to access output dose volume");
    vtkErrorMacro("CreateAccumulatedDose: " << errorMessage);
    return errorMessage;
  }

  // Create parameter node for dose accumulation
  vtkSmartPointer<vtkMRMLDoseAccumulationNode> doseAccumulationNode = vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New();
  this->GetMRMLScene()->AddNode(doseAccumulationNode);
  std::string doseAccumulationNodeName = std::string("DoseAccumulation_") + planNode->GetName();
  doseAccumulationNodeName = this->GetMRMLScene()->GenerateUniqueName(doseAccumulationNodeName);
  doseAccumulationNode->SetName(doseAccumulationNodeName.c_str());
  doseAccumulationNode->SetAndObserveAccumulatedDoseVolumeNode(totalDoseVolumeNode);
  doseAccumulationNode->SetAndObserveReferenceDoseVolumeNode(referenceVolumeNode); //TODO: CT seems to be the reference based on old code but dose accumulation code suggests it should be a dose

  // Collect per-beam dose volumes from beams under the plan
  std::vector<vtkMRMLRTBeamNode*> beams;
  planNode->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* beamNode = (*beamIt);
    if (!beamNode)
    {
      std::string errorMessage("Beam not found");
      vtkErrorMacro("CreateAccumulatedDose: " << errorMessage);
      continue;
    }

    // Get calculation result dose volume from beam
    vtkMRMLScalarVolumeNode* perBeamDoseVolume = this->Internal->DoseEngine->GetResultDoseForBeam(beamNode);
    if (!perBeamDoseVolume)
    {
      std::string errorMessage = std::string("No calculated dose found for beam ") + beamNode->GetName();
      vtkErrorMacro("CreateAccumulatedDose: " << errorMessage);
      continue;
    }

    // Add dose volume to dose accumulation
    doseAccumulationNode->AddSelectedInputVolumeNode(perBeamDoseVolume, beamNode->GetBeamWeight());
  }

  // Accumulate dose
  vtkSmartPointer<vtkSlicerDoseAccumulationModuleLogic> doseAccumulationLogic = vtkSmartPointer<vtkSlicerDoseAccumulationModuleLogic>::New();
  doseAccumulationLogic->SetMRMLScene(this->GetMRMLScene());
  const char* errorMessage = doseAccumulationLogic->AccumulateDoseVolumes(doseAccumulationNode);
  if (errorMessage)
  {
    return errorMessage;
  }

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
        NULL, totalDoseVolumeNode );
    }
  }

  totalDoseVolumeNode->CreateDefaultDisplayNodes(); // Make sure display node is present
  if (totalDoseVolumeNode->GetVolumeDisplayNode())
  {
    // Set dose color table
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(totalDoseVolumeNode->GetDisplayNode());
    vtkMRMLColorTableNode* defaultDoseColorTable = vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(this->GetMRMLScene());
    if (defaultDoseColorTable)
    {
      doseScalarVolumeDisplayNode->SetAndObserveColorNodeID(defaultDoseColorTable->GetID());
    }
    else
    {
      doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
      vtkErrorMacro("CalculateDose: Failed to get default dose color table!");
    }

    // Set window level based on prescription dose
    double rxDose = planNode->GetRxDose();
    doseScalarVolumeDisplayNode->AutoWindowLevelOff();
    doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, rxDose);

    // Set threshold to hide very low dose values
    doseScalarVolumeDisplayNode->SetLowerThreshold(0.05 * rxDose);
    doseScalarVolumeDisplayNode->ApplyThresholdOn();
  }
  else
  {
    vtkWarningMacro("CalculateDose: Display node is not available for calculated dose volume node. The default color table will be used.");
  }

  // Show total dose in foreground
  if (this->GetApplicationLogic() && this->GetApplicationLogic()->GetSelectionNode())
  {
    // Make sure reference volume is shown in background
    this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(referenceVolumeNode->GetID());
    // Select as foreground volume
    this->GetApplicationLogic()->GetSelectionNode()->SetReferenceSecondaryVolumeID(totalDoseVolumeNode->GetID());
    this->GetApplicationLogic()->PropagateVolumeSelection(0);

    // Set opacity so that volume is visible
    vtkMRMLSliceCompositeNode* compositeNode = NULL;
    int numberOfCompositeNodes = this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");
    for (int i=0; i<numberOfCompositeNodes; i++)
    {
      compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast ( this->GetMRMLScene()->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
      if (compositeNode && compositeNode->GetForegroundOpacity() == 0.0)
      {
        compositeNode->SetForegroundOpacity(0.5);
      }
    }
  } 

  return "";
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RemoveIntermediateResults(vtkMRMLRTPlanNode* planNode)
{
  std::vector<vtkMRMLRTBeamNode*> beams;
  planNode->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator it=beams.begin(); it!=beams.end(); ++it)
  {
    vtkMRMLRTBeamNode* currentBeam = (*it);

    // Remove intermediate results other than the per-beam dose volume
    this->Internal->DoseEngine->RemoveIntermediateResults(currentBeam);

    // Remove per-beam dose volume
    vtkMRMLScalarVolumeNode* currentDose = this->Internal->DoseEngine->GetResultDoseForBeam(currentBeam);
    currentBeam->GetScene()->RemoveNode(currentDose);
  }
}
