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

// Beams includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkSlicerIECTransformLogic.h"

// MRML includes
//#include <vtkMRMLMarkupsFiducialNode.h> //TODO: Includes commented out due to obsolete methods, see below
//#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>
//#include <vtkMRMLScalarVolumeDisplayNode.h>
//#include <vtkMRMLDoubleArrayNode.h>
//#include <vtkMRMLSliceLogic.h>
//#include <vtkMRMLSliceNode.h>
//#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// Slicer includes
#include <vtkSlicerCLIModuleLogic.h>
#include <vtkSlicerSubjectHierarchyModuleLogic.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
//#include <vtkConeSource.h>
//#include <vtkPoints.h>
//#include <vtkCellArray.h>
//#include <vtkDoubleArray.h>
//#include <vtkPolyData.h>
//#include <vtkObjectFactory.h>
//#include <vtkRenderer.h>
//#include <vtkRenderWindow.h>
//#include <vtkCamera.h>
//#include <vtkColorTransferFunction.h>
//#include <vtkImageData.h>
//#include <vtkImageCast.h>
//#include <vtkPiecewiseFunction.h>
//#include <vtkProperty.h>
//#include <vtkActor.h>
//#include <vtkVolumeProperty.h>
//#include <vtkVolumeRayCastMapper.h>
//#include <vtkGPUVolumeRayCastMapper.h>
//#include <vtkVolumeRayCastCompositeFunction.h>
//#include <vtkVolumeTextureMapper2D.h>
//#include <vtkWindowToImageFilter.h>
//#include <vtkImageShiftScale.h>
//#include <vtkImageExtractComponents.h>
//#include <vtkTransform.h>
//#include <vtkPolyDataMapper.h>
//#include <vtkImageGradientMagnitude.h>
//#include <vtkImageMathematics.h>

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkSlicerExternalBeamPlanningModuleLogic, BeamsLogic, vtkSlicerBeamsModuleLogic);

//----------------------------------------------------------------------------
class vtkSlicerExternalBeamPlanningModuleLogic::vtkInternal
{
public:
  vtkInternal();

  //TODO: Add Matlab dose engine plugin infrastructure
  vtkSlicerCLIModuleLogic* MatlabDoseCalculationModuleLogic;
};

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkInternal::vtkInternal()
{
  this->MatlabDoseCalculationModuleLogic = nullptr;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerExternalBeamPlanningModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkSlicerExternalBeamPlanningModuleLogic()
{
  this->DRRImageSize[0] = 256;
  this->DRRImageSize[1] = 256;

  this->BeamsLogic = nullptr;

  this->Internal = new vtkInternal;
}

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::~vtkSlicerExternalBeamPlanningModuleLogic()
{
  this->SetBeamsLogic(nullptr);

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
    vtkErrorMacro("RegisterNodes: Invalid MRML scene");
    return;
  }

  //TODO: Remove if still unused
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
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node");
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
  if (node->IsA("vtkMRMLRTBeamNode"))
  {
    // Observe beam events
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTBeamNode::CloningRequested);
    vtkObserveMRMLNodeEventsMacro(node, events);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndImport()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndImport: Invalid MRML scene");
    return;
  }

  // Observe plan events of all plan nodes
  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLRTPlanNode", nodes);

  for (auto nodeIt=nodes.begin(); nodeIt!=nodes.end(); ++nodeIt)
  {
    // Observe plan events
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTPlanNode::IsocenterModifiedEvent);
    vtkObserveMRMLNodeEventsMacro((*nodeIt), events);
  }

  // Observe beam events of all beam nodes
  nodes.clear();
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLRTBeamNode", nodes);

  for (auto nodeIt=nodes.begin(); nodeIt!=nodes.end(); ++nodeIt)
  {
    // Observe beam events
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTBeamNode::CloningRequested);
    vtkObserveMRMLNodeEventsMacro((*nodeIt), events);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or input node");
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
    vtkErrorMacro("ProcessMRMLNodesEvents: Invalid MRML scene");
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
      for (auto beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
      {
        // Calculate transform from beam parameters and isocenter from plan
        vtkMRMLRTBeamNode* beamNode = (*beamIt);
        beamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);
      }
    }
  }
  else if (caller->IsA("vtkMRMLRTBeamNode"))
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(caller);

    if (event == vtkMRMLRTBeamNode::CloningRequested)
    {
      this->CloneBeamInPlan(beamNode);
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkSlicerExternalBeamPlanningModuleLogic::CloneBeamInPlan(vtkMRMLRTBeamNode* copiedBeamNode, vtkMRMLRTPlanNode* planNode/*=nullptr*/)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("CloneBeamInPlan: Invalid MRML scene");
    return nullptr;
  }
  if (!copiedBeamNode)
  {
    vtkErrorMacro("CloneBeamInPlan: Invalid copied beam node");
    return nullptr;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("CloneBeamInPlan: Failed to access subject hierarchy node");
    return nullptr;
  }

  // Determine name of beam clone
  std::string newBeamName = (planNode ? planNode->GenerateNewBeamName() : (copiedBeamNode->GetParentPlanNode() ? copiedBeamNode->GetParentPlanNode()->GenerateNewBeamName() : "") );

  // Clone beam node via subject hierarchy
  vtkIdType beamShItemID = shNode->GetItemByDataNode(copiedBeamNode);
  vtkIdType beamCloneShItemID = vtkSlicerSubjectHierarchyModuleLogic::CloneSubjectHierarchyItem(
    shNode, beamShItemID, (newBeamName.empty() ? nullptr : newBeamName.c_str()) );
  vtkMRMLRTBeamNode* beamCloneNode = vtkMRMLRTBeamNode::SafeDownCast(shNode->GetItemDataNode(beamCloneShItemID));
  if (!beamCloneNode)
  {
    vtkErrorMacro("CloneBeamInPlan: Failed to copy beam node " << copiedBeamNode->GetName());
    return nullptr;
  }

  // Add beam clone to given plan if specified and different than default
  if (planNode && planNode != beamCloneNode->GetParentPlanNode())
  {
    beamCloneNode->GetParentPlanNode()->RemoveBeam(beamCloneNode);
    planNode->AddBeam(beamCloneNode);
  }

  return beamCloneNode;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
// Obsolete methods
//
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeWED()
{
  //TODO: Needs implementation
  vtkErrorMacro("ComputeWED: Not implemented");
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

  (void)(beamNode); // unused
#if defined (commentout)
  vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();
  vtkMRMLScalarVolumeNode* outputDoseVolume = planNode->GetOutputTotalDoseVolumeNode();
  // Make sure inputs are initialized
  if (!referenceVolumeNode || !beamNode)
  {
    vtkErrorMacro("ComputeDoseByMatlab: Inputs are not initialized");
    return;
  }

  if (this->Internal->MatlabDoseCalculationModuleLogic == nullptr)
  {
    vtkErrorMacro("ComputeDoseByMatlab: ERROR: logic is not set");
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
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateDRR(vtkMRMLRTPlanNode* planNode, char* beamName)
{
  if ( !this->GetMRMLScene() || !planNode )
  {
    vtkErrorMacro("RemoveBeam: Invalid MRML scene or RT plan node");
    return;
  }

  (void)(beamName); // unused
#if defined (commentout)
  vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    vtkErrorMacro("UpdateDRR: Failed to access reference volume node");
    return;
  }

  // Get beam node by name
  vtkMRMLRTBeamNode* beamNode = planNode->GetBeamByName(beamName);
  if (!beamNode)
  {
    vtkErrorMacro("UpdateDRR: Unable to access beam node with name " << (beamName?beamName:"nullptr"));
    return;
  }

  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = beamNode->GetIsocenterFiducialNode();
  // vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = beamNode->GetMLCPositionDoubleArrayNode();
  if (!isocenterMarkupsNode) // || !MLCPositionDoubleArrayNode)
  {
    vtkErrorMacro("UpdateDRR: Inputs are not initialized");
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
    vtkErrorMacro("UpdateDRR: Invalid sliceLogic for DRR viewer");
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

  // Compute DVH and get result nodes
  vtkSmartPointer<vtkImageData> mergedImageData = vtkSmartPointer<vtkImageData>::New();
  unsigned int numberOfContours = 0;

  vtkSmartPointer<vtkSegmentation> selectedSegmentation = planNode->GetTargetSegmentationNode()->GetSegmentation();
  std::vector< std::string > segmentIDs;
  selectedSegmentation->GetSegmentIDs(segmentIDs);
  for (std::vector< std::string >::const_iterator segmentIdIt = segmentIDs.begin(); segmentIdIt != segmentIDs.end(); ++segmentIdIt)
  {
    vtkSegment* segment = selectedSegmentation->GetSegment(*segmentIdIt);

    numberOfContours++;
    // Create the translucent contour object in BEV 
    // Create the renderer, render window 
    vtkSmartPointer<vtkRenderer> renderer2 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renWin2 = vtkSmartPointer<vtkRenderWindow>::New();
    renWin2->SetOffScreenRendering(1);
    renWin2->AddRenderer(renderer2);

    // Now we'll look at it. //TODO: This comment is meaningless
    vtkSmartPointer<vtkPolyDataMapper> contourPolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkPolyData> polydata = vtkPolyData::SafeDownCast(segment->GetRepresentation(vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()));
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

  sliceNode->SetSliceToRAS(transformSlice->GetMatrix());
  sliceNode->UpdateMatrices();

  sliceLogic->FitSliceToAll();

#endif
}
