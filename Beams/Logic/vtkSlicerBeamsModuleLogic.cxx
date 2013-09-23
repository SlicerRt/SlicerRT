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

// Beams includes
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkMRMLBeamsNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLAnnotationPointDisplayNode.h>
#include <vtkMRMLAnnotationTextDisplayNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLHierarchyNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkConeSource.h>
#include <vtkTransformPolyDataFilter.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerBeamsModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic::vtkSlicerBeamsModuleLogic()
{
  this->BeamsNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic::~vtkSlicerBeamsModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->BeamsNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::SetAndObserveBeamsNode(vtkMRMLBeamsNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->BeamsNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerBeamsModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLBeamsNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLAnnotationFiducialNode") || node->IsA("vtkMRMLBeamsNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLAnnotationFiducialNode") || node->IsA("vtkMRMLBeamsNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLBeamsNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLBeamsNode");
  if (node)
  {
    paramNode = vtkMRMLBeamsNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->BeamsNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::ComputeSourceFiducialPosition(std::string &errorMessage, vtkTransform* aSourceToIsocenterTransform/*=NULL*/)
{
  if (!this->BeamsNode || !this->GetMRMLScene())
  {
    return;
  }
  if ( !this->BeamsNode->GetIsocenterFiducialNodeId()
    || !strcmp(this->BeamsNode->GetIsocenterFiducialNodeId(), "") )
  {
    errorMessage = "Empty isocenter fiducial node ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }

  // Get isocenter fiducial node
  vtkMRMLAnnotationFiducialNode* isocenterNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamsNode->GetIsocenterFiducialNodeId()) );
  if (!isocenterNode)
  {
    errorMessage = "Unable to retrieve isocenter fiducial node according its ID!";
    vtkErrorMacro("ComputeSourceFiducialPosition: " << errorMessage); 
    return;
  }

  // Get isocenter coordinates
  if (isocenterNode->GetNumberOfControlPoints() != 1)
  {
    errorMessage = "Invalid isocenter fiducial control point count! It is supposed to be 1.";
    vtkErrorMacro("ComputeSourceFiducialPosition: " << errorMessage); 
    return;
  }
  double* isocenterCoordinates = isocenterNode->GetControlPointCoordinates(0);

  // Get patient hierarchy node for the isocenter
  vtkMRMLHierarchyNode* isocenterPatientHierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(isocenterNode);
  if (!isocenterPatientHierarchyNode)
  {
    errorMessage = "Unable to retrieve isocenter patient hierarchy node!";
    vtkErrorMacro("ComputeSourceFiducialPosition: " << errorMessage); 
    return;
  }

  // Extract beam-related parameters needed to compute source position
  double sourceAxisDistance = 0.0;
  const char* sourceAxisDistanceChars = isocenterPatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str());
  if (sourceAxisDistanceChars != NULL)
  {
    std::stringstream ss;
    ss << sourceAxisDistanceChars;
    ss >> sourceAxisDistance;
  }

  double gantryAngle = 0.0;
  const char* gantryAngleChars = isocenterPatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME.c_str());
  if (gantryAngleChars != NULL)
  {
    std::stringstream ss;
    ss << gantryAngleChars;
    ss >> gantryAngle;
  }

  double couchAngle = 0.0;
  const char* couchAngleChars = isocenterPatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME.c_str());
  if (couchAngleChars != NULL)
  {
    std::stringstream ss;
    ss << couchAngleChars;
    ss >> couchAngle;
  }

  // Get source fiducial node
  if (SlicerRtCommon::IsStringNullOrEmpty(this->BeamsNode->GetSourceFiducialNodeId()))
  {
    errorMessage = "Empty source fiducial node ID!";
    vtkErrorMacro("ComputeSourceFiducialPosition: " << errorMessage); 
    return;
  }
  if (!strcmp(this->BeamsNode->GetSourceFiducialNodeId(), this->BeamsNode->GetIsocenterFiducialNodeId()))
  {
    errorMessage = "Source and Isocenter fiducial nodes are set to be the same! They have to be different.";
    vtkErrorMacro("ComputeSourceFiducialPosition: " << errorMessage); 
    return;
  }
  vtkMRMLAnnotationFiducialNode* sourceNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamsNode->GetSourceFiducialNodeId()) );
  if (!sourceNode)
  {
    errorMessage = "Unable to retrieve source fiducial node according its ID!";
    vtkErrorMacro("ComputeSourceFiducialPosition: " << errorMessage);
    return;
  }

  sourceNode->SetLocked(1);
  sourceNode->CreateAnnotationTextDisplayNode();
  sourceNode->CreateAnnotationPointDisplayNode();
  sourceNode->GetAnnotationPointDisplayNode()->SetGlyphType(vtkMRMLAnnotationPointDisplayNode::Sphere3D);
  sourceNode->GetAnnotationPointDisplayNode()->SetColor(
    isocenterNode->GetAnnotationPointDisplayNode()->GetColor() );
  sourceNode->GetAnnotationTextDisplayNode()->SetColor(
    isocenterNode->GetAnnotationTextDisplayNode()->GetColor() );

  // Create connection from isocenter patient hierarchy node to the output source fiducial node
  isocenterPatientHierarchyNode->SetAttribute(SlicerRtCommon::BEAMS_SOURCE_FIDUCIAL_NODE_ID_ATTRIBUTE_NAME.c_str(), sourceNode->GetID());

  // Compute isocenter to source transformation
  //TODO: It is assumed that the center of rotation for the couch is the isocenter. Is it true?
  vtkSmartPointer<vtkTransform> couchToIsocenterTransform = vtkSmartPointer<vtkTransform>::New();
  couchToIsocenterTransform->Identity();
  couchToIsocenterTransform->RotateWXYZ((-1.0)*couchAngle, 0.0, 1.0, 0.0);

  vtkSmartPointer<vtkTransform> gantryToCouchTransform = vtkSmartPointer<vtkTransform>::New();
  gantryToCouchTransform->Identity();
  gantryToCouchTransform->RotateWXYZ(gantryAngle, 0.0, 0.0, 1.0);

  vtkSmartPointer<vtkTransform> sourceToGantryTransform = vtkSmartPointer<vtkTransform>::New();
  sourceToGantryTransform->Identity();
  sourceToGantryTransform->Translate(0.0, sourceAxisDistance, 0.0);

  // Pisocenter = Tcouch2isocenter * Tgantry2couch * Tsource2gantry * Psource
  vtkSmartPointer<vtkTransform> sourceToIsocenterTransform = vtkSmartPointer<vtkTransform>::New();
  sourceToIsocenterTransform->Identity();
  sourceToIsocenterTransform->PreMultiply();
  sourceToIsocenterTransform->Concatenate(couchToIsocenterTransform);
  sourceToIsocenterTransform->Concatenate(gantryToCouchTransform);
  sourceToIsocenterTransform->Concatenate(sourceToGantryTransform);

  // Get source position
  double* sourceCoordinates = sourceToIsocenterTransform->TransformPoint(isocenterCoordinates);
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

  if (aSourceToIsocenterTransform)
  {
    aSourceToIsocenterTransform->Identity();
    aSourceToIsocenterTransform->DeepCopy(sourceToIsocenterTransform);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::CreateBeamModel(std::string &errorMessage)
{
  if (!this->BeamsNode || !this->GetMRMLScene())
  {
    return;
  }

  if ( SlicerRtCommon::IsStringNullOrEmpty(this->BeamsNode->GetIsocenterFiducialNodeId())
    || SlicerRtCommon::IsStringNullOrEmpty(this->BeamsNode->GetSourceFiducialNodeId()) )
  {
    errorMessage = "Insufficient input (isocenter and/or source fiducial is empty)!";
    vtkErrorMacro(<<errorMessage);
    return;
  }

  // Compute source position
  std::string errorMessageSource("");
  vtkSmartPointer<vtkTransform> sourceToIsocenterTransform = vtkSmartPointer<vtkTransform>::New();
  this->ComputeSourceFiducialPosition(errorMessageSource, sourceToIsocenterTransform);
  if (!errorMessageSource.empty())
  {
    errorMessage = "Failed to compute source position:\n" + errorMessageSource;
    vtkErrorMacro(<<errorMessage);
    return;
  }

  // Get isocenter and source fiducial nodes
  vtkMRMLAnnotationFiducialNode* isocenterNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamsNode->GetIsocenterFiducialNodeId()) );
  if (!isocenterNode)
  {
    errorMessage = "Unable to retrieve isocenter fiducial node according its ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  // Get source fiducial node
  vtkMRMLAnnotationFiducialNode* sourceNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamsNode->GetSourceFiducialNodeId()) );
  if (!sourceNode)
  {
    errorMessage = "Unable to retrieve source fiducial node according its ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }

  // Get patient hierarchy node for the isocenter
  vtkMRMLHierarchyNode* isocenterPatientHierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(isocenterNode);
  if (!isocenterPatientHierarchyNode)
  {
    errorMessage = "Unable to retrieve isocenter patient hierarchy node!";
    vtkErrorMacro("ComputeSourceFiducialPosition: " << errorMessage); 
    return;
  }

  // Extract beam-related parameters needed to compute source position
  double collimatorAngle = 0.0;
  const char* collimatorAngleChars = isocenterPatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_COLLIMATOR_ANGLE_ATTRIBUTE_NAME.c_str());
  if (collimatorAngleChars != NULL)
  {
    std::stringstream ss;
    ss << collimatorAngleChars;
    ss >> collimatorAngle;
  }

  double jawPosition[2][2] = {{0.0, 0.0},{0.0, 0.0}};
  const char* jawPositionChars = isocenterPatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME.c_str());
  if (jawPositionChars != NULL)
  {
    std::stringstream ss;
    ss << jawPositionChars;
    ss >> jawPosition[0][0] >> jawPosition[0][1] >> jawPosition[1][0] >> jawPosition[1][1];
  }

  double sourceAxisDistance = 0.0;
  const char* sourceAxisDistanceChars = isocenterPatientHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str());
  if (sourceAxisDistanceChars != NULL)
  {
    std::stringstream ss;
    ss << sourceAxisDistanceChars;
    ss >> sourceAxisDistance;
  }

  // Get beam model node
  if ( !this->BeamsNode->GetBeamModelNodeId()
    || !strcmp(this->BeamsNode->GetBeamModelNodeId(), "") )
  {
    errorMessage = "Empty beam model node ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }
  vtkMRMLModelNode* beamModelNode = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamsNode->GetBeamModelNodeId()) );
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

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
  double baseRadius = fabs(jawPosition[0][0] - jawPosition[0][1]) / 2.0;
  coneSource->SetRadius(baseRadius*2.0*sqrt(2.0));
  coneSource->SetHeight(sourceAxisDistance*2.0);
  coneSource->SetResolution(4);
  coneSource->Update();

  // Assemble transform that places the beam in the proper position and orientation
  vtkSmartPointer<vtkMatrix4x4> isocenterPositionToOrientedIsocenterTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  isocenterPositionToOrientedIsocenterTransformMatrix->DeepCopy(sourceToIsocenterTransform->GetMatrix());
  isocenterPositionToOrientedIsocenterTransformMatrix->SetElement(0, 3, 0.0);
  isocenterPositionToOrientedIsocenterTransformMatrix->SetElement(1, 3, 0.0);
  isocenterPositionToOrientedIsocenterTransformMatrix->SetElement(2, 3, 0.0);
  vtkSmartPointer<vtkTransform> isocenterPositionToIsocenterTransform = vtkSmartPointer<vtkTransform>::New();
  isocenterPositionToIsocenterTransform->SetMatrix(isocenterPositionToOrientedIsocenterTransformMatrix);

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
  displayNode->SetOpacity(this->BeamsNode->GetBeamModelOpacity());
  // Disable backface culling to make the back side of the contour visible as well
  displayNode->SetBackfaceCulling(0);

  beamModelNode->SetAndObservePolyData( coneTransformFilter->GetOutput() );
  beamModelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  beamModelNode->SetHideFromEditors(0);
  beamModelNode->SetSelectable(1);
  beamModelNode->Modified();

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
}
