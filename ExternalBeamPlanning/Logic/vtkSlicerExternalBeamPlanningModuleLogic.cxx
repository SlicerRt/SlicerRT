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

// ExternalBeamPlanning includes
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"
#include "vtkMRMLExternalBeamPlanningNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkSlicerDoseCalculationEngine.h"
#include "vtkMRMLRTPlanHierarchyNode.h"

// SlicerRT includes
#include "PlmCommon.h"
#include "SlicerRtCommon.h"
#include "vtkRTBeamData.h"

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
  vtkSlicerDoseCalculationEngine* doseEngine;

  Plm_image::Pointer plmRef;
  float TotalRx;
};

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkInternal::vtkInternal()
{
  this->MatlabDoseCalculationModuleLogic = 0;
  this->doseEngine = vtkSlicerDoseCalculationEngine::New();
  this->TotalRx = 0.f;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerExternalBeamPlanningModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkSlicerExternalBeamPlanningModuleLogic()
{
  this->ExternalBeamPlanningNode = NULL;

  this->DRRImageSize[0] = 256;
  this->DRRImageSize[1] = 256;

  this->Internal = new vtkInternal;
}

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::~vtkSlicerExternalBeamPlanningModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, NULL);

  if (this->Internal->doseEngine)
  {
    this->Internal->doseEngine->Delete();
  }
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetAndObserveExternalBeamPlanningNode(vtkMRMLExternalBeamPlanningNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, node);
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

//-----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLExternalBeamPlanningNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTBeamNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanHierarchyNode>::New());
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

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLExternalBeamPlanningNode"))
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

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLExternalBeamPlanningNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLExternalBeamPlanningNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLExternalBeamPlanningNode");
  if (node)
  {
    paramNode = vtkMRMLExternalBeamPlanningNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateBeamTransform(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("UpdateBeamTransform: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();

  // Make sure inputs are initialized
  if (!rtPlanNode || !referenceVolumeNode)
  {
    vtkErrorMacro("UpdateBeamTransform: Inputs are not initialized!");
    return;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
  // Fill the table
  if (!beams) return;
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode && beamNode->BeamNameIs(beamname)) {
      break;
    }
  }

  // Make sure inputs are initialized
  if (!beamNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!");
    return;
  }
  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = beamNode->GetIsocenterFiducialNode();
  if (!isocenterMarkupsNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!");
    return;
  }

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(beamNode->GetGantryAngle());
  transform->RotateY(beamNode->GetCollimatorAngle());
  transform->RotateX(-90);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  isocenterMarkupsNode->GetNthFiducialPosition(0,isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();

  vtkMRMLLinearTransformNode *transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(beamModelNode->GetTransformNodeID()) );
  if (transformNode)
  {
    transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateBeamGeometryModel(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();

  // Make sure inputs are initialized
  if (!rtPlanNode || !referenceVolumeNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!");
    return;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
  if (!beams) 
  {
    return;
  }
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode && beamNode->BeamNameIs(beamname)) {
      break;
    }
  }

  // Make sure inputs are initialized
  if (!beamNode) {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!");
    return;
  }

  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = beamNode->GetIsocenterFiducialNode();
  if (!isocenterMarkupsNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!");
    return;
  }

  vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();
  vtkSmartPointer<vtkPolyData> beamModelPolyData = NULL;
  vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = beamNode->GetMLCPositionDoubleArrayNode();
  if (MLCPositionDoubleArrayNode)
  {
    beamModelPolyData = this->CreateBeamPolyData(
        beamNode->GetX1Jaw(),
        beamNode->GetX2Jaw(), 
        beamNode->GetY1Jaw(),
        beamNode->GetY2Jaw(),
        beamNode->GetSAD(),
        beamNode->GetMLCPositionDoubleArrayNode()->GetArray());
  }
  else
  {
    beamModelPolyData = this->CreateBeamPolyData(
        beamNode->GetX1Jaw(),
        beamNode->GetX2Jaw(), 
        beamNode->GetY1Jaw(),
        beamNode->GetY2Jaw(),
        beamNode->GetSAD());
  }

  beamModelNode->SetAndObservePolyData(beamModelPolyData);
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerExternalBeamPlanningModuleLogic::CreateBeamPolyData(
    double X1, double X2, double Y1, double Y2, double SAD)
{
  // Create beam model
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertPoint(0,0,0,SAD);
  points->InsertPoint(1,-2*Y2, -2*X2, -SAD );
  points->InsertPoint(2,-2*Y2,  2*X1, -SAD );
  points->InsertPoint(3, 2*Y1,  2*X1, -SAD );
  points->InsertPoint(4, 2*Y1, -2*X2, -SAD );

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(1);
  cellArray->InsertCellPoint(2);

  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(2);
  cellArray->InsertCellPoint(3);

  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(3);
  cellArray->InsertCellPoint(4);

  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(4);
  cellArray->InsertCellPoint(1);

  // Add the cap to the bottom
  cellArray->InsertNextCell(4);
  cellArray->InsertCellPoint(1);
  cellArray->InsertCellPoint(2);
  cellArray->InsertCellPoint(3);
  cellArray->InsertCellPoint(4);

  vtkSmartPointer<vtkPolyData> beamModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  beamModelPolyData->SetPoints(points);
  beamModelPolyData->SetPolys(cellArray);

  return beamModelPolyData;
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerExternalBeamPlanningModuleLogic::CreateBeamPolyData(
    double X1, double X2, double Y1, double Y2, double SAD, vtkDoubleArray* doubleArray)
{
  // First we extract the shape of the mlc
  int X2count = X2/10;
  int X1count = X1/10;
  int numLeavesVisible = X2count - (-X1count); // Calculate the number of leaves visible
  int numPointsEachSide = numLeavesVisible *2;

  double Y2LeavePosition[40];
  double Y1LeavePosition[40];

  // Calculate Y2 first
  for (int i = X2count; i >= -X1count; i--)
  {
    double leafPosition = doubleArray->GetComponent(-(i-20), 1);
    if (-leafPosition>-Y2)
    {
      Y2LeavePosition[-(i-20)] = leafPosition;
    }
    else
    {
      Y2LeavePosition[-(i-20)] = Y2;
    }
  }
  // Calculate Y1 next
  for (int i = X2count; i >= -X1count; i--)
  {
    double leafPosition = doubleArray->GetComponent(-(i-20), 0);
    if (leafPosition<Y1)
    {
      Y1LeavePosition[-(i-20)] = leafPosition;
    }
    else
    {
      Y1LeavePosition[-(i-20)] = Y1;
    }
  }

  // Create beam model
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertPoint(0,0,0,SAD);

  int count = 1;
  for (int i = X2count; i > -X1count; i--)
  {
    points->InsertPoint(count,-Y2LeavePosition[-(i-20)]*2, i*10*2, -SAD );
    count ++;
    points->InsertPoint(count,-Y2LeavePosition[-(i-20)]*2, (i-1)*10*2, -SAD );
    count ++;
  }

  for (int i = -X1count; i < X2count; i++)
  {
    points->InsertPoint(count,Y1LeavePosition[-(i-20)]*2, i*10*2, -SAD );
    count ++;
    points->InsertPoint(count,Y1LeavePosition[-(i-20)]*2, (i+1)*10*2, -SAD );
    count ++;
  }

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  for (int i = 1; i <numPointsEachSide; i++)
  {
    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(i);
    cellArray->InsertCellPoint(i+1);
  }
  // Add connection between Y2 and Y1
  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(numPointsEachSide);
  cellArray->InsertCellPoint(numPointsEachSide+1);
  for (int i = numPointsEachSide+1; i <2*numPointsEachSide; i++)
  {
    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(i);
    cellArray->InsertCellPoint(i+1);
  }

  // Add connection between Y2 and Y1
  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(2*numPointsEachSide);
  cellArray->InsertCellPoint(1);

  // Add the cap to the bottom
  cellArray->InsertNextCell(2*numPointsEachSide);
  for (int i = 1; i <= 2*numPointsEachSide; i++)
  {
    cellArray->InsertCellPoint(i);
  }

  vtkSmartPointer<vtkPolyData> beamModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  beamModelPolyData->SetPoints(points);
  beamModelPolyData->SetPolys(cellArray);

  return beamModelPolyData;
}

//---------------------------------------------------------------------------
vtkMRMLRTPlanNode* vtkSlicerExternalBeamPlanningModuleLogic::GetRTPlanNode()
{
  return this->ExternalBeamPlanningNode->GetRtPlanNode();
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkSlicerExternalBeamPlanningModuleLogic::AddBeam()
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("AddBeam: Invalid MRML scene or parameter set node!");
    return 0;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->GetRTPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();
  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = this->ExternalBeamPlanningNode->GetIsocenterFiducialNode();
  
  // Make sure inputs are initialized
  if (!rtPlanNode)
  {
    vtkErrorMacro("AddBeam: No RT Plan Node specified");
    return 0;
  }

  if (!isocenterMarkupsNode)
  {
    vtkErrorMacro("AddBeam: Isocenter not specified");
    return 0;
  }

  if (!referenceVolumeNode)
  {
    vtkErrorMacro("AddBeam: Reference volume not specified");
    return 0;
  }

  // Get RT plan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  vtkSmartPointer<vtkPolyData> beamModelPolyData;
  vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = this->ExternalBeamPlanningNode->GetMLCPositionDoubleArrayNode();
  if (MLCPositionDoubleArrayNode) 
  {
    beamModelPolyData = this->CreateBeamPolyData(
      this->ExternalBeamPlanningNode->GetX1Jaw(),
      this->ExternalBeamPlanningNode->GetX2Jaw(), 
      this->ExternalBeamPlanningNode->GetY1Jaw(),
      this->ExternalBeamPlanningNode->GetY2Jaw(),
      this->ExternalBeamPlanningNode->GetSAD(),
      MLCPositionDoubleArrayNode->GetArray());
  }
  else
  {
    beamModelPolyData = this->CreateBeamPolyData(
      this->ExternalBeamPlanningNode->GetX1Jaw(),
      this->ExternalBeamPlanningNode->GetX2Jaw(), 
      this->ExternalBeamPlanningNode->GetY1Jaw(),
      this->ExternalBeamPlanningNode->GetY2Jaw(),
      this->ExternalBeamPlanningNode->GetSAD());
  }

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(this->ExternalBeamPlanningNode->GetGantryAngle());
  transform->RotateY(this->ExternalBeamPlanningNode->GetCollimatorAngle());
  transform->RotateX(-90);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  isocenterMarkupsNode->GetNthFiducialPosition(0,isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  transformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->AddNode(transformNode));
  transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  
  std::string rtBeamNodeName;
  std::string rtBeamModelNodeName;
  rtBeamNodeName = std::string("RTBeam");
  rtBeamNodeName = this->GetMRMLScene()->GenerateUniqueName(rtBeamNodeName);
  rtBeamModelNodeName = rtBeamNodeName + "_SurfaceModel";

  // Create rtbeam model node
  vtkSmartPointer<vtkMRMLModelNode> RTBeamModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  RTBeamModelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelNode));
  RTBeamModelNode->SetName(rtBeamModelNodeName.c_str());
  RTBeamModelNode->SetAndObservePolyData(beamModelPolyData);
  RTBeamModelNode->SetAndObserveTransformNodeID(transformNode->GetID());
  RTBeamModelNode->HideFromEditorsOff();

  // Create model node for beam
  vtkSmartPointer<vtkMRMLModelDisplayNode> RTBeamModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  RTBeamModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelDisplayNode));
  RTBeamModelDisplayNode->SetColor(0.0, 1.0, 0.0);
  RTBeamModelDisplayNode->SetOpacity(0.3);

  // Disable backface culling to make the back side of the contour visible as well
  RTBeamModelDisplayNode->SetBackfaceCulling(0);
  RTBeamModelDisplayNode->HideFromEditorsOff();
  RTBeamModelDisplayNode->VisibilityOn(); 
  RTBeamModelNode->SetAndObserveDisplayNodeID(RTBeamModelDisplayNode->GetID());
  RTBeamModelDisplayNode->SliceIntersectionVisibilityOn();  

  // Create rtbeam node
  vtkSmartPointer<vtkMRMLRTBeamNode> RTBeamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  RTBeamNode = vtkMRMLRTBeamNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamNode));
  vtkRTBeamData *beamData = RTBeamNode->GetBeamData();
  beamData->SetBeamName(rtBeamNodeName.c_str());
  RTBeamNode->SetAndObserveBeamModelNodeId(RTBeamModelNode->GetID());
  RTBeamNode->HideFromEditorsOff();

  // Put the RTBeam node in the hierarchy
  rtPlanNode->AddRTBeamNode(RTBeamNode);

  // TODO GCS FIX --- this will be enveloped into the BeamData::Copy() function
  //RTBeamNode->SetBeamName(this->ExternalBeamPlanningNode->GetBeamName());
  this->ExternalBeamPlanningNode->SetBeamName(beamData->GetBeamName());

  /* Copy all appropriate settings from EBP node into beam node */
  RTBeamNode->SetX1Jaw(this->ExternalBeamPlanningNode->GetX1Jaw());
  RTBeamNode->SetX2Jaw(this->ExternalBeamPlanningNode->GetX2Jaw());
  RTBeamNode->SetY1Jaw(this->ExternalBeamPlanningNode->GetY1Jaw());
  RTBeamNode->SetY2Jaw(this->ExternalBeamPlanningNode->GetY2Jaw());
  RTBeamNode->SetAndObserveMLCPositionDoubleArrayNode(this->ExternalBeamPlanningNode->GetMLCPositionDoubleArrayNode());

  RTBeamNode->SetGantryAngle(this->ExternalBeamPlanningNode->GetGantryAngle());
  RTBeamNode->SetCollimatorAngle(this->ExternalBeamPlanningNode->GetCollimatorAngle());
  RTBeamNode->SetCouchAngle(this->ExternalBeamPlanningNode->GetCouchAngle());

  RTBeamNode->SetBeamFlavor(this->ExternalBeamPlanningNode->GetBeamFlavor());
  RTBeamNode->SetEnergyResolution(this->ExternalBeamPlanningNode->GetEnergyResolution());

  RTBeamNode->SetSAD(this->ExternalBeamPlanningNode->GetSAD());
  RTBeamNode->SetSmearing(this->ExternalBeamPlanningNode->GetSmearing());
  RTBeamNode->SetProximalMargin(this->ExternalBeamPlanningNode->GetProximalMargin());
  RTBeamNode->SetDistalMargin(this->ExternalBeamPlanningNode->GetDistalMargin());

  RTBeamNode->SetApertureSpacingAtIso(this->ExternalBeamPlanningNode->GetApertureSpacingAtIso());

  RTBeamNode->SetAndObserveIsocenterFiducialNode(this->ExternalBeamPlanningNode->GetIsocenterFiducialNode());
  RTBeamNode->SetAndObserveTargetSegmentationNode(this->ExternalBeamPlanningNode->GetTargetSegmentationNode());

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
  this->Modified();

  return RTBeamNode;
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RemoveBeam(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("RemoveBeam: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  // Make sure inputs are initialized
  if (!rtPlanNode)
  {
    vtkErrorMacro("RemoveBeam: Inputs are not initialized!");
    return;
  }

  // Get RT plan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  // Find RT beam node in RT plan hierarchy node
  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);

  beams->InitTraversal();
  if (beams->GetNumberOfItems() < 1)
  {
    vtkWarningMacro("RemoveBeam: Selected RTPlan node has no children nodes!");
    return;
  }

  // Fill the table
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode && beamNode->BeamNameIs(beamname)) {
      vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();
      vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelDisplayNode = beamModelNode->GetModelDisplayNode();
      rtPlanNode->RemoveRTBeamNode(beamNode);
      this->GetMRMLScene()->RemoveNode(beamModelNode);
      this->GetMRMLScene()->RemoveNode(beamModelDisplayNode);
      break;
    }
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateDRR(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("RemoveBeam: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();

  // Make sure inputs are initialized
  if (!rtPlanNode || !referenceVolumeNode)
  {
    vtkErrorMacro("UpdateDRR: Inputs are not initialized!");
    return;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
  // Fill the table
  if (!beams) return;
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode && beamNode->BeamNameIs(beamname)) {
      break;
    }
  }

  // Make sure inputs are initialized
  if (!beamNode)
  {
    vtkErrorMacro("UpdateDRR: Inputs are not initialized!");
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
#if (VTK_MAJOR_VERSION <= 5)
  cast->SetInput(referenceVolumeNode->GetImageData());
#else
  cast->SetInputData(referenceVolumeNode->GetImageData());
#endif
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
#if (VTK_MAJOR_VERSION <= 5)
  mapper->SetInput( cast->GetOutput() );
#else
  mapper->SetInputData( cast->GetOutput() );
#endif
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
  transform->RotateY(this->ExternalBeamPlanningNode->GetGantryAngle());
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
  vtkSmartPointer<vtkMRMLScalarVolumeNode> DRRImageNode = beamNode->GetDRRVolumeNode();
  if (!DRRImageNode)
  {
    DRRImageNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    DRRImageNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->AddNode(DRRImageNode));
    std::string DRRImageNodeName = std::string(beamname) + std::string("_DRRImage");
    DRRImageNodeName = this->GetMRMLScene()->GenerateUniqueName(DRRImageNodeName);
    DRRImageNode->SetName(DRRImageNodeName.c_str());
    beamNode->SetAndObserveDRRVolumeNode(DRRImageNode);
  }
  //DRRImageNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  DRRImageNode->SetAndObserveImageData(extract->GetOutput());
  DRRImageNode->SetOrigin(-128,-128,0);
  DRRImageNode->SetSelectable(1);

  vtkSmartPointer<vtkMRMLSliceLogic> sliceLogic = this->GetApplicationLogic()->GetSliceLogicByLayoutName("Slice4");
  if (!sliceLogic)
  {
    vtkErrorMacro("UpdateDRR: Invalid sliceLogic for DRR viewer!");
    return;
  }

  // Transforms to place the DRR image aligned with the beam geometry
  vtkSmartPointer<vtkTransform> transformDRR = vtkSmartPointer<vtkTransform>::New();
  transformDRR->Identity();
  transformDRR->RotateZ(this->ExternalBeamPlanningNode->GetGantryAngle());
  transformDRR->RotateX(-90);

  vtkSmartPointer<vtkTransform> transformDRR2 = vtkSmartPointer<vtkTransform>::New();
  transformDRR2->Identity();
  transformDRR2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transformDRR->PostMultiply();
  transformDRR->Concatenate(transformDRR2->GetMatrix());

  vtkSmartPointer<vtkMRMLLinearTransformNode> DRRImageTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(DRRImageNode->GetTransformNodeID()));
  if (!DRRImageTransformNode)
  {
    DRRImageTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    DRRImageTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->AddNode(DRRImageTransformNode));
    std::string DRRImageTransformNodeName = std::string(beamname) + std::string("_DRRImage_Transform");
    DRRImageTransformNodeName = this->GetMRMLScene()->GenerateUniqueName(DRRImageTransformNodeName);
    DRRImageTransformNode->SetName(DRRImageTransformNodeName.c_str());
    DRRImageNode->SetAndObserveTransformNodeID(DRRImageTransformNode->GetID());
  }
  DRRImageTransformNode->SetAndObserveMatrixTransformToParent(transformDRR->GetMatrix());

  // 
  vtkSmartPointer<vtkMRMLSliceNode> sliceNode = sliceLogic->GetSliceNode();
  vtkSmartPointer<vtkMRMLSliceCompositeNode> compositeSliceNode = sliceLogic->GetSliceCompositeNode();
  compositeSliceNode->SetBackgroundVolumeID(DRRImageNode->GetID());

  //TODO: convert to segmentations
  // Get list of segments from the segmentation node
  vtkSmartPointer<vtkSegmentation> selectedSegmentation = this->ExternalBeamPlanningNode->GetTargetSegmentationNode()->GetSegmentation();
  vtkSegmentation::SegmentMap segmentMap = selectedSegmentation->GetSegments();

  // Compute DVH and get result nodes
  vtkSmartPointer<vtkImageData> mergedImageData = vtkSmartPointer<vtkImageData>::New();
  unsigned int numberOfContours = 0;
  
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
  {
    numberOfContours++;
    //if (contourNode)
    //{
    // Create the translucent contour object in BEV 
    // Create the renderer, render window 
    vtkSmartPointer<vtkRenderer> renderer2 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renWin2 = vtkSmartPointer<vtkRenderWindow>::New();
    renWin2->SetOffScreenRendering(1);
    renWin2->AddRenderer(renderer2);

    // Now we'll look at it.
    vtkSmartPointer<vtkPolyDataMapper> contourPolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkPolyData> polydata = vtkPolyData::SafeDownCast(segmentIt->second->GetRepresentation(vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()));
#if (VTK_MAJOR_VERSION <= 5)
    contourPolyDataMapper->SetInput(polydata);
#else
    contourPolyDataMapper->SetInputData(polydata);
#endif
    //contourPolyDataMapper->SetScalarRange(0,255);
    vtkSmartPointer<vtkActor> contourPolyDataActor = vtkSmartPointer<vtkActor>::New();
    contourPolyDataActor->SetMapper(contourPolyDataMapper);
    contourPolyDataActor->GetProperty()->LightingOff();
    contourPolyDataActor->GetProperty()->SetColor(1.0,1.0,1.0);

    vtkSmartPointer<vtkTransform> transformContourBEV = vtkSmartPointer<vtkTransform>::New();
    transformContourBEV->Identity();
    transformContourBEV->RotateY(this->ExternalBeamPlanningNode->GetGantryAngle());
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
#if (VTK_MAJOR_VERSION <= 5)
      addFilter->SetInput1(mergedImageData);
      addFilter->SetInput2(magnitude->GetOutput());
#else
      addFilter->SetInput1Data(mergedImageData);
      addFilter->SetInput2Data(magnitude->GetOutput());
#endif
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
    std::string contourBEVImageNodeName = std::string(beamname) + std::string("_ContourBEVImage");
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

  contourBEVImageNode->SetAndObserveTransformNodeID(DRRImageTransformNode->GetID());

  compositeSliceNode->SetForegroundVolumeID(contourBEVImageNode->GetID());
  compositeSliceNode->SetForegroundOpacity(0.3);

  // Transform ???
  vtkSmartPointer<vtkTransform> transformSlice = vtkSmartPointer<vtkTransform>::New();
  transformSlice->Identity();
  transformSlice->RotateZ(this->ExternalBeamPlanningNode->GetGantryAngle());
  transformSlice->RotateX(-90);
  transformSlice->Update();

  sliceNode->SetOrientationToReformat();
  sliceNode->SetSliceToRAS(transformSlice->GetMatrix());
  sliceNode->UpdateMatrices();

  sliceLogic->FitSliceToAll();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeDose(vtkMRMLRTBeamNode* beamNode)
{
  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  if (!rtPlanNode )
  {
    vtkErrorMacro("ComputeDose: Inputs are not initialized!");
    return;
  }

  if(rtPlanNode->GetRTPlanDoseEngine() == vtkMRMLRTPlanNode::Plastimatch)
  {
    this->ComputeDoseByPlastimatch(beamNode);
  }
  else if (rtPlanNode->GetRTPlanDoseEngine() == vtkMRMLRTPlanNode::Matlab)
  {
    this->ComputeDoseByMatlab(beamNode);
  }
  else
  {
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeDoseByPlastimatch(vtkMRMLRTBeamNode* beamNode)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("ComputeDoseByPlastimatch: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();

  // Make sure inputs are initialized
  if (!rtPlanNode || !referenceVolumeNode)
  {
    vtkErrorMacro("ComputeDoseByPlastimatch: Inputs are not initialized!");
    return;
  }

  // Make sure inputs are initialized
  if (!beamNode)
  {
    vtkErrorMacro("ComputeDoseByPlastimatch: Inputs are not initialized!");
    return;
  }

  printf ("1\n");
  vtkMRMLSegmentationNode* targetSegmentationNode = beamNode->GetTargetSegmentationNode();
  if (!beamNode)
  {
    vtkErrorMacro("ComputeDoseByPlastimatch: Didn't get target contour node");
    return;
  }

  printf ("2\n");
  vtkSegmentation *segmentation = targetSegmentationNode->GetSegmentation();
  if (!segmentation)
  {
    vtkErrorMacro("ComputeDoseByPlastimatch: Failed to get segmentation");
    return;
  }

  printf ("3\n");
  vtkSegment *segment = segmentation->GetSegment(beamNode->GetTargetSegmentID());
  if (!segment) 
  {
    vtkErrorMacro("ComputeDoseByPlastimatch: Failed to get segment");
    return;
  }

  printf ("4\n");
  
  //segmentationNode->GetImageData ();
  vtkSmartPointer<vtkOrientedImageData> targetLabelmap;
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
      vtkErrorMacro("ComputeDoseByPlastimatch: " << errorMessage);
      return;
    }
  }

  // Apply parent transformation nodes if necessary
  if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(targetSegmentationNode, targetLabelmap))
  {
    std::string errorMessage("Failed to apply parent transformation to compare segmentation.\nMaybe it has a deformable transformation parent, which is not allowed");
    vtkErrorMacro("ComputeDoseByPlastimatch: " << errorMessage);
    return;
  }

  // Convert inputs to ITK images
  Plm_image::Pointer plmTgt 
    = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(targetLabelmap);
  if (!plmTgt)
  {
    std::string errorMessage("Failed to convert reference segment labelmap into Plm_image");
    vtkErrorMacro("ComputeDoseByPlastimatch: " << errorMessage);
    return;
  }

  // Reference code for setting the geometry of the segmentation rasterization
  // in case the default one (from DICOM) is not desired
#if defined (commentout)
  vtkSmartPointer<vtkMatrix4x4> doseIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  doseVolumeNode->GetIJKToRASMatrix(doseIjkToRasMatrix);
  std::string doseGeometryString = vtkSegmentationConverter::SerializeImageGeometry(doseIjkToRasMatrix, doseVolumeNode->GetImageData());
  segmentationCopy->SetConversionParameter( vtkSegmentationConverter::GetReferenceImageGeometryParameterName(),
    doseGeometryString );
#endif
  
  
//  vtkOrientedImageData* targetLabelmap = vtkOrientedImageData::SafeDownCast(targetSegmentationNode->GetSegmentation()->GetSegmentRepresentation(beamNode->GetTargetSegmentID(), vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()));
//  printf ("8\n");
//  vtkSmartPointer<vtkMRMLScalarVolumeNode> targetLabelmapNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
//  printf ("9\n");
//  targetLabelmapNode->SetAndObserveImageData(targetLabelmap);

  printf ("10a\n");
//  Plm_image::Pointer plmTgt = PlmCommon::ConvertVolumeNodeToPlmImage(
//    targetLabelmapNode);
  printf ("10b\n");
  plmTgt->print ();

  printf ("11\n");
  double src[3];
  double isocenter[3] = { 0, 0, 0 };
  beamNode->GetIsocenterFiducialNode()->GetNthFiducialPosition(0,isocenter);
  isocenter[0] = -isocenter[0];
  isocenter[1] = -isocenter[1];

  /* Adjust src according to gantry angle */
  printf ("12\n");
  double ga_radians = 
    beamNode->GetGantryAngle() * M_PI / 180.;
  double src_dist = beamNode->GetSAD();
  src[0] = isocenter[0] + src_dist * sin(ga_radians);
  src[1] = isocenter[1] - src_dist * cos(ga_radians);
  src[2] = isocenter[2];

  printf ("13\n");
  double proximalMargin = this->ExternalBeamPlanningNode->GetProximalMargin();
  double distalMargin = this->ExternalBeamPlanningNode->GetDistalMargin();

  printf ("14\n");
  double beamRx = this->GetExternalBeamPlanningNode()->GetRxDose() * beamNode->GetBeamWeight();

  printf ("Gonna try to doseEngine->CalculateDose()\n");
  this->Internal->doseEngine->CalculateDose (
    beamNode, 
    plmTgt, 
    isocenter, 
    src, 
    proximalMargin, 
    distalMargin, 
    beamRx);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> rcVolumeNode =
    vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  itk::Image<float, 3>::Pointer rcVolumeItk = this->Internal->doseEngine->GetRangeCompensatorVolume();

  /* Convert range compensator image to vtk */
  vtkSmartPointer<vtkImageData> rcVolume = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(rcVolumeItk, rcVolume, VTK_FLOAT);

  rcVolumeNode->SetAndObserveImageData (rcVolume);
  rcVolumeNode->SetSpacing (
    rcVolumeItk->GetSpacing()[0],
    rcVolumeItk->GetSpacing()[1],
    rcVolumeItk->GetSpacing()[2]);
  rcVolumeNode->SetOrigin (
    rcVolumeItk->GetOrigin()[0],
    rcVolumeItk->GetOrigin()[1],
    rcVolumeItk->GetOrigin()[2]);

  std::string nodeName = "range_compensator_" + std::string(beamNode->GetName());

  rcVolumeNode->SetName(nodeName.c_str());

  rcVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(rcVolumeNode);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> apertureVolumeNode =
    vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  itk::Image<unsigned char, 3>::Pointer apertureVolumeItk = this->Internal->doseEngine->GetApertureVolume();

  /* Convert aperture image to vtk */
  vtkSmartPointer<vtkImageData> apertureVolume = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<unsigned char>(apertureVolumeItk, apertureVolume, VTK_UNSIGNED_CHAR);

  apertureVolumeNode->SetAndObserveImageData (apertureVolume);
  apertureVolumeNode->SetSpacing (
    apertureVolumeItk->GetSpacing()[0],
    apertureVolumeItk->GetSpacing()[1],
    apertureVolumeItk->GetSpacing()[2]);
  apertureVolumeNode->SetOrigin (
    apertureVolumeItk->GetOrigin()[0],
    apertureVolumeItk->GetOrigin()[1],
    apertureVolumeItk->GetOrigin()[2]);

  nodeName = "aperture_" + std::string(beamNode->GetName());

  apertureVolumeNode->SetName(nodeName.c_str());

  apertureVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(apertureVolumeNode);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> doseVolumeNode =
    vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  /* Convert dose image to vtk */
  vtkSmartPointer<vtkImageData> doseVolume = vtkSmartPointer<vtkImageData>::New();
  itk::Image<float, 3>::Pointer doseVolumeItk = this->Internal->doseEngine->GetComputedDose();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(doseVolumeItk, doseVolume, VTK_FLOAT);

  doseVolumeNode->SetAndObserveImageData (doseVolume);
  doseVolumeNode->CopyOrientation (referenceVolumeNode);

  nodeName = "proton_dose_" + std::string(beamNode->GetName());

  doseVolumeNode->SetName(nodeName.c_str());

  doseVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(doseVolumeNode); // to be removed if we want to show only the last image

  /* Testing .. */
  doseVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  /* More testing .. */
  if (doseVolumeNode->GetVolumeDisplayNode() == NULL)
  {
    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    displayNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(displayNode);
    doseVolumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  }
  if (doseVolumeNode->GetVolumeDisplayNode())
  {
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(doseVolumeNode->GetVolumeDisplayNode());
    doseScalarVolumeDisplayNode->SetAutoWindowLevel(0);
    doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, 16.0);

    /* Set colormap to rainbow */
    doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    doseScalarVolumeDisplayNode->SetLowerThreshold (1.0);
    doseScalarVolumeDisplayNode->ApplyThresholdOn ();
  }
  else
  {
    vtkWarningMacro("ComputeDose: Display node is not available for gamma volume node. The default color table will be used.");
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeWED()
{
  /* Needs re-implmentation */
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
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeDoseByMatlab(vtkMRMLRTBeamNode* beamNode)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("ComputeDose: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();
  vtkMRMLScalarVolumeNode* outputDoseVolume = rtPlanNode->GetRTPlanDoseVolumeNode();
  // Make sure inputs are initialized
  if (!rtPlanNode || !referenceVolumeNode || !beamNode)
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
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::InitializeAccumulatedDose()
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("InitializeAccumulatedDose: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();
  Plm_image::Pointer plmRef = PlmCommon::ConvertVolumeNodeToPlmImage(referenceVolumeNode);

  this->Internal->doseEngine->InitializeAccumulatedDose(plmRef);
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RegisterAccumulatedDose()
{
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> doseVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  /* Convert dose image to vtk */
  vtkSmartPointer<vtkImageData> doseVolume = vtkSmartPointer<vtkImageData>::New();
  itk::Image<float, 3>::Pointer accumulateVolumeItk = this->Internal->doseEngine->GetAccumulatedDose();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(accumulateVolumeItk, doseVolume, VTK_FLOAT);

  doseVolumeNode->SetAndObserveImageData(doseVolume);
  doseVolumeNode->CopyOrientation(referenceVolumeNode);

  std::string nodeName = "total_proton_dose";
  doseVolumeNode->SetName(nodeName.c_str());

  doseVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(doseVolumeNode); // to be removed if we want to show only the last image

  this->Internal->doseEngine->FinalizeAccumulatedDose();
  double totalRx = this->Internal->doseEngine->GetTotalRx();

#if (TODO)
  /* This requires an existing study node */
  vtkMRMLSubjectHierarchyNode* doseSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode( this->GetMRMLScene(), studyNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(), doseVolumeNodeName, doseVolumeNode );
  /* Look in vtkMRMLSubjectHierarchyNode for utility functions, such as 
     how to find the study node, "getAssociatedHeirarchyNode", 
     if there is an associated heirarchy node, 
     the parent node should be the study,
     navigate the tree to find what we want.

     vtkMRMLSubjectHierarchyNode
     vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode
     GetParentNode()

     If no existing study node, maybe better to create one.
     It needs a UID, like this:

     subjectHierarchySeriesNode->AddUID(vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(),
     rtReader->GetSeriesInstanceUid());
  */
#endif

  /* Testing .. */
  doseVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  /* More testing .. */
  if (doseVolumeNode->GetVolumeDisplayNode() == NULL)
  {
    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    displayNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(displayNode);
    doseVolumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  }
  if (doseVolumeNode->GetVolumeDisplayNode())
  {
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(doseVolumeNode->GetVolumeDisplayNode());
    doseScalarVolumeDisplayNode->SetAutoWindowLevel(0);
    doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, totalRx);

    /* Set colormap to rainbow */
    doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    doseScalarVolumeDisplayNode->SetLowerThreshold (0.05 * totalRx);
    doseScalarVolumeDisplayNode->ApplyThresholdOn ();
  }
  else
  {
    vtkWarningMacro("ComputeDose: Display node is not available for gamma volume node. The default color table will be used.");
  }

  // Select as active volume
  if (this->GetApplicationLogic()!=NULL)
  {
    if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceSecondaryVolumeID(doseVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection(0);
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RemoveDoseNodes()
{
  vtkMRMLScene *scene = this->GetMRMLScene();

  std::vector<vtkMRMLNode *> nodes;
  scene->GetNodesByClass("vtkMRMLScalarVolumeNode", nodes);

  std::vector<vtkMRMLNode *>::iterator it;
  for (it = nodes.begin(); it != nodes.end(); it++) {
    vtkMRMLScalarVolumeNode *node = vtkMRMLScalarVolumeNode::SafeDownCast(*it);
    if (string_starts_with (node->GetName(), "aperture_")) {
      scene->RemoveNode (node);
    }
    else if (string_starts_with (node->GetName(), "range_compensator_")) {
      scene->RemoveNode (node);
    }
    else if (string_starts_with (node->GetName(), "proton_dose_")) {
      scene->RemoveNode (node);
    }
    else if (string_starts_with (node->GetName(), "total_proton_dose")) {
      scene->RemoveNode (node);
    }
  }
}
