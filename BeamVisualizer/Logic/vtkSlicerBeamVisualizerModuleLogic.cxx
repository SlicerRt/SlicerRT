/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// BeamVisualizer includes
#include "vtkSlicerBeamVisualizerModuleLogic.h"
#include "vtkMRMLBeamVisualizerNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkConeSource.h>
#include <vtkTransformPolyDataFilter.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerBeamVisualizerModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerBeamVisualizerModuleLogic::vtkSlicerBeamVisualizerModuleLogic()
{
  this->BeamVisualizerNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerBeamVisualizerModuleLogic::~vtkSlicerBeamVisualizerModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->BeamVisualizerNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::SetAndObserveBeamVisualizerNode(vtkMRMLBeamVisualizerNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->BeamVisualizerNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLBeamVisualizerNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLAnnotationFiducialNode") || node->IsA("vtkMRMLBeamVisualizerNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLAnnotationFiducialNode") || node->IsA("vtkMRMLBeamVisualizerNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLBeamVisualizerNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLBeamVisualizerNode");
  if (node)
  {
    paramNode = vtkMRMLBeamVisualizerNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->BeamVisualizerNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::ComputeSourceFiducialPosition(std::string &errorMessage, vtkTransform* aIsocenterToSourceTransform/*=NULL*/)
{
  if (!this->BeamVisualizerNode || !this->GetMRMLScene())
  {
    return;
  }
  if ( !this->BeamVisualizerNode->GetIsocenterFiducialNodeId()
    || !strcmp(this->BeamVisualizerNode->GetIsocenterFiducialNodeId(), "") )
  {
    errorMessage = "Empty isocenter fiducial node ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }

  // Get isocenter fiducial node
  vtkMRMLAnnotationFiducialNode* isocenterNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamVisualizerNode->GetIsocenterFiducialNodeId()) );
  if (!isocenterNode)
  {
    errorMessage = "Unable to retrieve isocenter fiducial node according its ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }

  // Get isocenter coordinates
  if (isocenterNode->GetNumberOfControlPoints() != 1)
  {
    errorMessage = "Invalid isocenter fiducial control point count! It is supposed to be 1.";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  double* isocenterCoordinates = isocenterNode->GetControlPointCoordinates(0);

  // Extract beam-related parameters needed to compute source position
  double sourceAxisDistance = 0.0;
  const char* sourceAxisDistanceChars = isocenterNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str());
  if (sourceAxisDistanceChars != NULL)
  {
    std::stringstream ss;
    ss << sourceAxisDistanceChars;
    ss >> sourceAxisDistance;
  }

  double gantryAngle = 0.0;
  const char* gantryAngleChars = isocenterNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_GANTRY_ANGLE_ATTRIBUTE_NAME.c_str());
  if (gantryAngleChars != NULL)
  {
    std::stringstream ss;
    ss << gantryAngleChars;
    ss >> gantryAngle;
  }

  double couchAngle = 0.0;
  const char* couchAngleChars = isocenterNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_COUCH_ANGLE_ATTRIBUTE_NAME.c_str());
  if (couchAngleChars != NULL)
  {
    std::stringstream ss;
    ss << couchAngleChars;
    ss >> couchAngle;
  }

  // Get source fiducial node
  if ( !this->BeamVisualizerNode->GetSourceFiducialNodeId()
    || !strcmp(this->BeamVisualizerNode->GetSourceFiducialNodeId(), "") )
  {
    errorMessage = "Empty source fiducial node ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  if (!strcmp(this->BeamVisualizerNode->GetSourceFiducialNodeId(), this->BeamVisualizerNode->GetIsocenterFiducialNodeId()))
  {
    errorMessage = "Source and Isocenter fiducial nodes are set to be the same! They have to be different.";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  vtkMRMLAnnotationFiducialNode* sourceNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamVisualizerNode->GetSourceFiducialNodeId()) );
  if (!sourceNode)
  {
    errorMessage = "Unable to retrieve source fiducial node according its ID!";
    vtkErrorMacro(<<errorMessage);
    return;
  }

  // Compute isocenter to source transformation
  //TODO: It is assumed that the center of rotation for the couch is the isocenter. Is it true?
  vtkSmartPointer<vtkTransform> couchToSourceTransform = vtkSmartPointer<vtkTransform>::New();
  couchToSourceTransform->Identity();
  couchToSourceTransform->RotateWXYZ((-1.0)*couchAngle, 0.0, 1.0, 0.0);

  vtkSmartPointer<vtkTransform> gantryToCouchTransform = vtkSmartPointer<vtkTransform>::New();
  gantryToCouchTransform->Identity();
  gantryToCouchTransform->RotateWXYZ(gantryAngle, 0.0, 0.0, 1.0);

  vtkSmartPointer<vtkTransform> isocenterToGantryTransform = vtkSmartPointer<vtkTransform>::New();
  isocenterToGantryTransform->Identity();
  isocenterToGantryTransform->Translate(0.0, sourceAxisDistance, 0.0);

  // Psource = Tcollimator2source * Tgantry2collimator * Tisocenter2gantry * Pisocenter
  vtkSmartPointer<vtkTransform> isocenterToSourceTransform = vtkSmartPointer<vtkTransform>::New();
  isocenterToSourceTransform->Identity();
  isocenterToSourceTransform->PreMultiply();
  isocenterToSourceTransform->Concatenate(couchToSourceTransform);
  isocenterToSourceTransform->Concatenate(gantryToCouchTransform);
  isocenterToSourceTransform->Concatenate(isocenterToGantryTransform);

  // Get source position
  double* sourceCoordinates = isocenterToSourceTransform->TransformPoint(isocenterCoordinates);
  if (sourceNode->GetNumberOfControlPoints() == 0)
  {
    sourceNode->AddControlPoint(sourceCoordinates, 0, 1);
  }
  else
  {
    sourceNode->SetControlPoint(0, sourceCoordinates);
  }

  vtkDebugMacro("Source coordinates computed to be ("
    << sourceCoordinates[0] << ", " << sourceCoordinates[1] << ", " << sourceCoordinates[2] << ")");

  if (aIsocenterToSourceTransform)
  {
    aIsocenterToSourceTransform->Identity();
    aIsocenterToSourceTransform->DeepCopy(isocenterToSourceTransform);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::CreateBeamModel(std::string &errorMessage)
{
  if (!this->BeamVisualizerNode || !this->GetMRMLScene())
  {
    return;
  }

  if ( !this->BeamVisualizerNode->GetIsocenterFiducialNodeId()
    || !strcmp(this->BeamVisualizerNode->GetIsocenterFiducialNodeId(), "")
    || !this->BeamVisualizerNode->GetSourceFiducialNodeId()
    || !strcmp(this->BeamVisualizerNode->GetSourceFiducialNodeId(), "") )
  {
    errorMessage = "Insufficient input (isocenter and/or source fiducial is empty)!";
    vtkErrorMacro(<<errorMessage);
    return;
  }

  // Compute source position
  std::string errorMessageSource("");
  vtkSmartPointer<vtkTransform> isocenterToSourceTransform = vtkSmartPointer<vtkTransform>::New();
  this->ComputeSourceFiducialPosition(errorMessageSource, isocenterToSourceTransform);
  if (!errorMessageSource.empty())
  {
    errorMessage = "Failed to compute source position!";
    vtkErrorMacro(<<errorMessage);
    return;
  }

  // Get isocenter and source fiducial nodes
  vtkMRMLAnnotationFiducialNode* isocenterNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamVisualizerNode->GetIsocenterFiducialNodeId()) );
  if (!isocenterNode)
  {
    errorMessage = "Unable to retrieve isocenter fiducial node according its ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  // Get source fiducial node
  vtkMRMLAnnotationFiducialNode* sourceNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamVisualizerNode->GetSourceFiducialNodeId()) );
  if (!sourceNode)
  {
    errorMessage = "Unable to retrieve source fiducial node according its ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }

  // Extract beam-related parameters needed to compute source position
  double collimatorAngle = 0.0;
  const char* collimatorAngleChars = isocenterNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_COLLIMATOR_ANGLE_ATTRIBUTE_NAME.c_str());
  if (collimatorAngleChars != NULL)
  {
    std::stringstream ss;
    ss << collimatorAngleChars;
    ss >> collimatorAngle;
  }

  double jawPosition[2][2];
  jawPosition[0][0]=jawPosition[0][1]=jawPosition[1][0]=jawPosition[1][1]=0.0;
  const char* jawPositionChars = isocenterNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME.c_str());
  if (jawPositionChars != NULL)
  {
    std::stringstream ss;
    ss << jawPositionChars;
    ss >> jawPosition[0][0] >> jawPosition[0][1] >> jawPosition[1][0] >> jawPosition[1][1];
  }

  double sourceAxisDistance = 0.0;
  const char* sourceAxisDistanceChars = isocenterNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str());
  if (sourceAxisDistanceChars != NULL)
  {
    std::stringstream ss;
    ss << sourceAxisDistanceChars;
    ss >> sourceAxisDistance;
  }

  // Get beam model node
  if ( !this->BeamVisualizerNode->GetBeamModelNodeId()
    || !strcmp(this->BeamVisualizerNode->GetBeamModelNodeId(), "") )
  {
    errorMessage = "Empty beam model node ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  vtkMRMLModelNode* beamModelNode = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamVisualizerNode->GetBeamModelNodeId()) );
  if (!beamModelNode)
  {
    errorMessage = "Unable to retrieve beam model node according its ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  if (beamModelNode->IsA("vtkMRMLAnnotationFiducialNode"))
  {
    errorMessage = "Beam model node is not supposed to be an annotation fiducial node!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }

  // Get isocenter and source positions
  if (isocenterNode->GetNumberOfControlPoints() != 1)
  {
    errorMessage = "Invalid isocenter fiducial control point count! It is supposed to be 1.";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  double* isocenterCoordinates = isocenterNode->GetControlPointCoordinates(0);
  if (sourceNode->GetNumberOfControlPoints() != 1)
  {
    errorMessage = "Invalid source fiducial control point count! It is supposed to be 1.";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  double* sourceCoordinates = sourceNode->GetControlPointCoordinates(0);

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  // TODO: creat a real "pyramid" shape considering collimator angle instead of low-resolution cone
  vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
  //coneSource->SetCenter(
  //  (isocenterCoordinates[0]),
  //  (isocenterCoordinates[1]),
  //  (isocenterCoordinates[2]) );
  //coneSource->SetDirection(
  //  sourceCoordinates[0] - isocenterCoordinates[0],
  //  sourceCoordinates[1] - isocenterCoordinates[1],
  //  sourceCoordinates[2] - isocenterCoordinates[2] );
  double baseRadius = fabs(jawPosition[0][0] - jawPosition[0][1]) / 2.0;
  coneSource->SetRadius(baseRadius*2.0*sqrt(2.0));
  coneSource->SetHeight(sourceAxisDistance*2.0);
  coneSource->SetResolution(4);
  coneSource->Update();

  // Assemble transform that places the beam in the proper position and orientation
  vtkSmartPointer<vtkMatrix4x4> isocenterPositionToIsocenterTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  isocenterPositionToIsocenterTransformMatrix->DeepCopy(isocenterToSourceTransform->GetMatrix());
  isocenterPositionToIsocenterTransformMatrix->SetElement(0, 3, 0.0);
  isocenterPositionToIsocenterTransformMatrix->SetElement(1, 3, 0.0);
  isocenterPositionToIsocenterTransformMatrix->SetElement(2, 3, 0.0);
  vtkSmartPointer<vtkTransform> isocenterPositionToIsocenterTransform = vtkSmartPointer<vtkTransform>::New();
  isocenterPositionToIsocenterTransform->SetMatrix(isocenterPositionToIsocenterTransformMatrix);

  vtkSmartPointer<vtkTransform> coneSourceToIsocenterPositionTransform = vtkSmartPointer<vtkTransform>::New();
  coneSourceToIsocenterPositionTransform->Identity();
  coneSourceToIsocenterPositionTransform->Translate(isocenterCoordinates);

  //TODO: Scale pyramid to match jaws positions in Y direction

  // Rotate cone by 45 degrees to match the beam shape
  vtkSmartPointer<vtkTransform> tiltedConeSourceToConeSourceTransform = vtkSmartPointer<vtkTransform>::New();
  tiltedConeSourceToConeSourceTransform->Identity();
  tiltedConeSourceToConeSourceTransform->RotateZ(90.0);
  tiltedConeSourceToConeSourceTransform->RotateX(45.0);
  //tiltedConeSourceToConeSourceTransform->RotateY(collimatorAngle); //TODO: Uncomment and try with a suitable test data

  vtkSmartPointer<vtkTransform> tiltedConeSourceToIsocenterTransform = vtkSmartPointer<vtkTransform>::New();
  tiltedConeSourceToIsocenterTransform->Identity();
  tiltedConeSourceToIsocenterTransform->PreMultiply();
  tiltedConeSourceToIsocenterTransform->Concatenate(isocenterPositionToIsocenterTransform);
  tiltedConeSourceToIsocenterTransform->Concatenate(coneSourceToIsocenterPositionTransform);
  tiltedConeSourceToIsocenterTransform->Concatenate(tiltedConeSourceToConeSourceTransform);

  vtkSmartPointer<vtkTransformPolyDataFilter> coneTransformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  coneTransformFilter->AddInputConnection(coneSource->GetOutputPort());
  coneTransformFilter->SetTransform(tiltedConeSourceToIsocenterTransform);
  coneTransformFilter->Update();

  // Create display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
  displayNode->SliceIntersectionVisibilityOn();  
  displayNode->VisibilityOn(); 
  displayNode->SetColor(0.0, 1.0, 0.0);
  displayNode->SetOpacity(0.3);
  // Disable backface culling to make the back side of the contour visible as well
  displayNode->SetBackfaceCulling(0);

  beamModelNode->SetAndObservePolyData( coneTransformFilter->GetOutput() );
  //beamModelNode->SetAndObservePolyData( coneSource->GetOutput() );
  beamModelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  beamModelNode->SetHideFromEditors(0);
  beamModelNode->SetSelectable(1);
  beamModelNode->Modified();

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
}
