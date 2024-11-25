/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

  The collision detection module was partly supported by Conselleria de
  Educación, Investigación, Cultura y Deporte (Generalitat Valenciana), Spain
  under grant number CDEIGENT/2019/011.

==============================================================================*/

// RoomsEyeView includes
#include "vtkSlicerRoomsEyeViewModuleLogic.h"
#include "vtkMRMLRoomsEyeViewNode.h"

// SlicerRT includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkSlicerBeamsModuleLogic.h"

// MRML includes
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLScene.h>
#include "vtkMRMLSubjectHierarchyNode.h"
#include <vtkMRMLViewNode.h>

// Slicer includes
#include <vtkSlicerModelsLogic.h>
#include <vtkSlicerSegmentationsModuleLogic.h>

// vtkSegmentationCore includes
#include <vtkSegmentationConverter.h>

// VTK includes
#include <vtkAppendPolyData.h>
#include "vtkCollisionDetectionFilter.h"
#include <vtkGeneralTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataReader.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVector.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// RapidJSON includes
#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/filereadstream.h"

// Constants
const char* vtkSlicerRoomsEyeViewModuleLogic::ORIENTATION_MARKER_MODEL_NODE_NAME = "RoomsEyeViewOrientationMarker";
const char* vtkSlicerRoomsEyeViewModuleLogic::TREATMENT_MACHINE_DESCRIPTOR_FILE_PATH_ATTRIBUTE_NAME = "TreatmentMachineDescriptorFilePath";
unsigned int vtkSlicerRoomsEyeViewModuleLogic::MAX_TRIANGLE_NUMBER_PRODUCT_FOR_COLLISIONS = 1e10;

static rapidjson::Value JSON_EMPTY_VALUE;


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerRoomsEyeViewModuleLogic);

//---------------------------------------------------------------------------
class vtkSlicerRoomsEyeViewModuleLogic::vtkInternal
{
public:
  vtkInternal(vtkSlicerRoomsEyeViewModuleLogic* external);
  ~vtkInternal();

  vtkSlicerRoomsEyeViewModuleLogic* External; 
  rapidjson::Document* CurrentTreatmentMachineDescription{nullptr};

  /// Utility function to get element for treatment machine part
  /// \return Json object if found, otherwise null Json object
  rapidjson::Value& GetTreatmentMachinePart(TreatmentMachinePartType partType);
  rapidjson::Value& GetTreatmentMachinePart(std::string partTypeStr);

  std::string GetTreatmentMachinePartFullFilePath(vtkMRMLRoomsEyeViewNode* parameterNode, std::string partPath);
  std::string GetTreatmentMachineFileNameWithoutExtension(vtkMRMLRoomsEyeViewNode* parameterNode);
  std::string GetTreatmentMachinePartModelName(vtkMRMLRoomsEyeViewNode* parameterNode, TreatmentMachinePartType partType);
  vtkMRMLModelNode* GetTreatmentMachinePartModelNode(vtkMRMLRoomsEyeViewNode* parameterNode, TreatmentMachinePartType partType);
  vtkMRMLModelNode* EnsureTreatmentMachinePartModelNode(vtkMRMLRoomsEyeViewNode* parameterNode, TreatmentMachinePartType partType, bool optional=false);
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::vtkInternal::vtkInternal(vtkSlicerRoomsEyeViewModuleLogic* external)
{
  this->External = external;
  this->CurrentTreatmentMachineDescription = new rapidjson::Document;
}

//---------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::vtkInternal::~vtkInternal()
{
  delete this->CurrentTreatmentMachineDescription;
  this->CurrentTreatmentMachineDescription = nullptr;
}

//---------------------------------------------------------------------------
rapidjson::Value& vtkSlicerRoomsEyeViewModuleLogic::vtkInternal::GetTreatmentMachinePart(TreatmentMachinePartType type)
{
  if (type >= TreatmentMachinePartType::LastPartType)
  {
    vtkErrorWithObjectMacro(this->External, "GetTreatmentMachinePart: Invalid part type given " << type);
    return JSON_EMPTY_VALUE;
  }
  std::string typeStr = this->External->GetTreatmentMachinePartTypeAsString(type);
  return this->GetTreatmentMachinePart(typeStr);
}

//---------------------------------------------------------------------------
rapidjson::Value& vtkSlicerRoomsEyeViewModuleLogic::vtkInternal::GetTreatmentMachinePart(std::string typeStr)
{
  if (this->CurrentTreatmentMachineDescription->IsNull())
  {
    vtkErrorWithObjectMacro(this->External, "GetTreatmentMachinePart: No treatment machine descriptor file loaded");
    return JSON_EMPTY_VALUE;
  }
  rapidjson::Value::MemberIterator partsIt = this->CurrentTreatmentMachineDescription->FindMember("Part");
  if (partsIt == this->CurrentTreatmentMachineDescription->MemberEnd() || !partsIt->value.IsArray())
  {
    vtkErrorWithObjectMacro(this->External, "GetTreatmentMachinePart: Failed to find parts array in treatment machine description");
    return JSON_EMPTY_VALUE;
  }
  rapidjson::Value& partsArray = partsIt->value;

  // Traverse parts and try to find the element with the given part type
  for (rapidjson::SizeType index=0; index < partsArray.Size(); ++index)
  {
    rapidjson::Value& currentObject = partsArray[index];
    if (currentObject.IsObject())
    {
      rapidjson::Value& currentType = currentObject["Type"];
      if (currentType.IsString() && !typeStr.compare(currentType.GetString()))
      {
        return currentObject;
      }
    }
  }

  // Not found
  return JSON_EMPTY_VALUE;
}

//---------------------------------------------------------------------------
std::string vtkSlicerRoomsEyeViewModuleLogic::vtkInternal::GetTreatmentMachinePartFullFilePath(
  vtkMRMLRoomsEyeViewNode* parameterNode, std::string partPath)
{
  if (!parameterNode)
  {
    vtkErrorWithObjectMacro(this->External, "GetTreatmentMachinePartFullFilePath: Invalid parameter node given");
    return "";
  }

  if (vtksys::SystemTools::FileIsFullPath(partPath))
  {
    // Simply return the path if it is absolute
    return partPath;
  }

  std::string descriptorFileDir = vtksys::SystemTools::GetFilenamePath(parameterNode->GetTreatmentMachineDescriptorFilePath());
  return descriptorFileDir + "/" + partPath;
}

//---------------------------------------------------------------------------
std::string vtkSlicerRoomsEyeViewModuleLogic::vtkInternal::GetTreatmentMachineFileNameWithoutExtension(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorWithObjectMacro(this->External, "GetTreatmentMachineFileNameWithoutExtension: Invalid parameter node given");
    return "";
  }
  if (!parameterNode->GetTreatmentMachineDescriptorFilePath() || strlen(parameterNode->GetTreatmentMachineDescriptorFilePath()) == 0)
  {
    vtkErrorWithObjectMacro(this->External, "GetTreatmentMachineFileNameWithoutExtension: Empty treatment machine descriptor file path");
    return "";
  }

  std::string fileName = vtksys::SystemTools::GetFilenameName(parameterNode->GetTreatmentMachineDescriptorFilePath());
  std::string extension = vtksys::SystemTools::GetFilenameExtension(parameterNode->GetTreatmentMachineDescriptorFilePath());
  return fileName.substr(0, fileName.length() - extension.length());
}

//---------------------------------------------------------------------------
std::string vtkSlicerRoomsEyeViewModuleLogic::vtkInternal::GetTreatmentMachinePartModelName(
  vtkMRMLRoomsEyeViewNode* parameterNode, TreatmentMachinePartType partType)
{
  if (!parameterNode)
  {
    vtkErrorWithObjectMacro(this->External, "GetTreatmentMachinePartModelName: Invalid parameter node given");
    return "";
  }
  std::string machineType = this->GetTreatmentMachineFileNameWithoutExtension(parameterNode);
  return machineType + "_" + this->External->GetTreatmentMachinePartTypeAsString(partType);
}

//---------------------------------------------------------------------------
vtkMRMLModelNode* vtkSlicerRoomsEyeViewModuleLogic::vtkInternal::GetTreatmentMachinePartModelNode(
  vtkMRMLRoomsEyeViewNode* parameterNode, TreatmentMachinePartType partType)
{
  if (!parameterNode)
  {
    vtkErrorWithObjectMacro(this->External, "GetTreatmentMachinePartModelName: Invalid parameter node given");
    return nullptr;
  }
  std::string partName = this->GetTreatmentMachinePartModelName(parameterNode, partType);
  return vtkMRMLModelNode::SafeDownCast(this->External->GetMRMLScene()->GetFirstNodeByName(partName.c_str()));
}

//---------------------------------------------------------------------------
vtkMRMLModelNode* vtkSlicerRoomsEyeViewModuleLogic::vtkInternal::EnsureTreatmentMachinePartModelNode(
  vtkMRMLRoomsEyeViewNode* parameterNode, TreatmentMachinePartType partType, bool optional/*=false*/)
{
  vtkMRMLScene* scene = this->External->GetMRMLScene();
  if (!scene || !parameterNode)
  {
    vtkErrorWithObjectMacro(this->External, "GetTreatmentMachinePartModelName: Invalid scene or parameter node");
    return nullptr;
  }
  vtkMRMLSubjectHierarchyNode* shNode = scene->GetSubjectHierarchyNode();
  if (!shNode)
  {
    vtkErrorWithObjectMacro(this->External, "LoadTreatmentMachine: Failed to access subject hierarchy node");
    return nullptr;
  }

  // Get root SH item
  std::string machineType = this->GetTreatmentMachineFileNameWithoutExtension(parameterNode);
  std::string rootFolderName = machineType + std::string("_Components");
  vtkIdType rootFolderItem = shNode->GetItemChildWithName(shNode->GetSceneItemID(), rootFolderName);
  if (!rootFolderItem)
  {
    // Create subject hierarchy folder so that the treatment machine can be shown/hidden easily
    rootFolderItem = shNode->CreateFolderItem(shNode->GetSceneItemID(), rootFolderName);
  }

  std::string partName = this->GetTreatmentMachinePartModelName(parameterNode, partType);
  vtkMRMLModelNode* partModelNode = this->GetTreatmentMachinePartModelNode(parameterNode, partType);
  if (!partModelNode)
  {
    // Skip model if state is disabled and part is optional
    std::string state = this->External->GetStateForPartType(this->External->GetTreatmentMachinePartTypeAsString(partType));
    if (state == "Disabled")
    {
      if (optional)
      {
        return nullptr;
      }
      else
      {
        vtkWarningWithObjectMacro(this->External, "LoadTreatmentMachine: State for part "
          << partName << " is set to Disabled but the part is mandatory. Loading anyway.");
      }
    }     
    // Get model file path
    std::string partModelFilePath = this->External->GetFilePathForPartType(
      this->External->GetTreatmentMachinePartTypeAsString(partType));
    if (partModelFilePath == "")
    {
      if (!optional)
      {
        vtkErrorWithObjectMacro(this->External, "LoadTreatmentMachine: Failed get file path for part "
          << partName << ". This mandatory part may be missing from the descriptor file");
      }
      return nullptr;
    }
    // Load model from file
    partModelFilePath = this->GetTreatmentMachinePartFullFilePath(parameterNode, partModelFilePath);
    if (vtksys::SystemTools::FileExists(partModelFilePath))
    {
      // Create a models logic for convenient loading of components
      vtkNew<vtkSlicerModelsLogic> modelsLogic;
      modelsLogic->SetMRMLScene(scene);
      partModelNode = modelsLogic->AddModel(partModelFilePath.c_str());
      partModelNode->SetName(partName.c_str());
      vtkIdType partItemID = shNode->GetItemByDataNode(partModelNode);
      shNode->SetItemParent(partItemID, rootFolderItem);
    }
    else if (!optional)
    {
      vtkErrorWithObjectMacro(this->External, "LoadTreatmentMachine: Failed to load " << partName << " model from file " << partModelFilePath);
      return nullptr;
    }
  }
  return partModelNode;
}

//---------------------------------------------------------------------------
// vtkSlicerRoomsEyeViewModuleLogic methods

//----------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::vtkSlicerRoomsEyeViewModuleLogic()
  : GantryPatientCollisionDetection(nullptr)
  , GantryTableTopCollisionDetection(nullptr)
  , GantryPatientSupportCollisionDetection(nullptr)
  , CollimatorPatientCollisionDetection(nullptr)
  , CollimatorTableTopCollisionDetection(nullptr)
{
  this->Internal = new vtkInternal(this); 

  this->IECLogic = vtkIECTransformLogic::New();

  this->GantryPatientCollisionDetection = vtkCollisionDetectionFilter::New();
  this->GantryPatientCollisionDetection->SetCollisionModeToFirstContact();
  this->GantryTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
  this->GantryTableTopCollisionDetection->SetCollisionModeToFirstContact();
  this->GantryPatientSupportCollisionDetection = vtkCollisionDetectionFilter::New();
  this->GantryPatientSupportCollisionDetection->SetCollisionModeToFirstContact();
  this->CollimatorPatientCollisionDetection = vtkCollisionDetectionFilter::New();
  this->CollimatorPatientCollisionDetection->SetCollisionModeToFirstContact();
  this->CollimatorTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
  this->CollimatorTableTopCollisionDetection->SetCollisionModeToFirstContact();
}

//----------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::~vtkSlicerRoomsEyeViewModuleLogic()
{
  if (this->IECLogic)
  {
    this->IECLogic->Delete();
    this->IECLogic = nullptr;
  }

  if (this->GantryPatientCollisionDetection)
  {
    this->GantryPatientCollisionDetection->Delete();
    this->GantryPatientCollisionDetection = nullptr;
  }
  if (this->GantryTableTopCollisionDetection)
  {
    this->GantryTableTopCollisionDetection->Delete();
    this->GantryTableTopCollisionDetection = nullptr;
  }
  if (this->GantryPatientSupportCollisionDetection)
  {
    this->GantryPatientSupportCollisionDetection->Delete();
    this->GantryPatientSupportCollisionDetection = nullptr;
  }
  if (this->CollimatorPatientCollisionDetection)
  {
    this->CollimatorPatientCollisionDetection->Delete();
    this->CollimatorPatientCollisionDetection = nullptr;
  }
  if (this->CollimatorTableTopCollisionDetection)
  {
    this->CollimatorTableTopCollisionDetection->Delete();
    this->CollimatorTableTopCollisionDetection = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // Transforms
  os << indent << "Transforms:" << std::endl;
  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  for (auto& transformPair : IECLogic->vtkIECTransformLogic::GetIECTransforms())
  {
    std::string transformName = IECLogic->GetTransformNameBetween(transformPair.first, transformPair.second);
    vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      this->GetMRMLScene()->GetFirstNodeByName(transformName.c_str()));

    os << indent.GetNextIndent() << transformName << std::endl;
    if (transformNode == nullptr)
    {
      os << indent.GetNextIndent() << indent.GetNextIndent() << "Not available" << std::endl;
      continue;
    }
    transformNode->GetMatrixTransformToParent(matrix);
    for (int i = 0; i < 4; i++)
    {
      os << indent.GetNextIndent() << indent.GetNextIndent();
      for (int j = 0; j < 4; j++)
      {
        os << matrix->GetElement(i, j) << " ";
      }
      os << std::endl;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene");
    return;
  }
  if (!scene->IsNodeClassRegistered("vtkMRMLRoomsEyeViewNode"))
  {
    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRoomsEyeViewNode>::New());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  this->Superclass::SetMRMLSceneInternal(newScene);
}

//-----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkSlicerRoomsEyeViewModuleLogic::GetTransformNodeBetween(
  vtkIECTransformLogic::CoordinateSystemIdentifier fromFrame, vtkIECTransformLogic::CoordinateSystemIdentifier toFrame)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("GetTransformNodeBetween: Invalid MRML scene");
    return nullptr;
  }

  return vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(this->IECLogic->GetTransformNameBetween(fromFrame, toFrame).c_str()));
}

//---------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic* vtkSlicerRoomsEyeViewModuleLogic::GetBeamsLogic()
{
  vtkSlicerBeamsModuleLogic* beamsLogic = vtkSlicerBeamsModuleLogic::SafeDownCast(this->GetModuleLogic("Beams"));
  if (beamsLogic)
  {
    return beamsLogic;
  }

  if (this->BeamsLogic != nullptr)
  {
    return this->BeamsLogic;
  }

  vtkErrorMacro("GetBeamsLogic: Beams logic cannot be accessed from the application and is not set externally either");
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::BuildRoomsEyeViewTransformHierarchy()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Invalid MRML scene");
    return;
  }
  vtkSlicerBeamsModuleLogic* beamsLogic = this->GetBeamsLogic();
  if (!beamsLogic)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Beams logic cannot be accessed");
    return;
  }

  // Build IEC hierarchy in MRML

  // Create transform nodes if they do not exist
  for (auto& transformPair : this->IECLogic->GetIECTransforms())
  {
    std::string transformNodeName = this->IECLogic->GetTransformNameBetween(transformPair.first, transformPair.second);
    if (!this->GetMRMLScene()->GetFirstNodeByName(transformNodeName.c_str()))
    {
      vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
      transformNode->SetName(transformNodeName.c_str());
      transformNode->SetHideFromEditors(1);
      std::string singletonTag = std::string("IEC_") + transformNodeName;
      transformNode->SetSingletonTag(singletonTag.c_str());
      this->GetMRMLScene()->AddNode(transformNode);
    }
  }

  // Organize transform nodes into hierarchy based on IEC Standard 61217 (duplicate IEC logic in MRML).
  // Set and observe individual transforms from IEC logic to the MRML nodes.
  vtkMRMLLinearTransformNode* fixedReferenceToRASTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::FixedReference, vtkIECTransformLogic::RAS);
  if (fixedReferenceToRASTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Fixed reference to RAS transform");
    return;
  }
  fixedReferenceToRASTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::FixedReference, vtkIECTransformLogic::RAS));

  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::Gantry, vtkIECTransformLogic::FixedReference);
  if (gantryToFixedReferenceTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Gantry to Fixed reference transform");
    return;
  }
  gantryToFixedReferenceTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::Gantry, vtkIECTransformLogic::FixedReference));
  gantryToFixedReferenceTransformNode->SetAndObserveTransformNodeID(fixedReferenceToRASTransformNode->GetID());


  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::Collimator, vtkIECTransformLogic::Gantry);
  if (collimatorToGantryTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Collimator to Gantry transform");
    return;
  }
  collimatorToGantryTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::Collimator, vtkIECTransformLogic::Gantry));
  collimatorToGantryTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());


  vtkMRMLLinearTransformNode* wedgeFilterToCollimatorTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::WedgeFilter, vtkIECTransformLogic::Collimator);
  if (wedgeFilterToCollimatorTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Wedge filter to Collimator transform");
    return;
  }
  wedgeFilterToCollimatorTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::WedgeFilter, vtkIECTransformLogic::Collimator));
  wedgeFilterToCollimatorTransformNode->SetAndObserveTransformNodeID(collimatorToGantryTransformNode->GetID());


  vtkMRMLLinearTransformNode* leftImagingPanelToGantryTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::LeftImagingPanel, vtkIECTransformLogic::Gantry);
  if (leftImagingPanelToGantryTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Left imaging panel to Gantry transform");
    return;
  }
  leftImagingPanelToGantryTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::LeftImagingPanel, vtkIECTransformLogic::Gantry));
  leftImagingPanelToGantryTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());


  vtkMRMLLinearTransformNode* rightImagingPanelToGantryTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::RightImagingPanel, vtkIECTransformLogic::Gantry);
  if (rightImagingPanelToGantryTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Right imaging panel to Gantry transform");
    return;
  }
  rightImagingPanelToGantryTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::RightImagingPanel, vtkIECTransformLogic::Gantry));
  rightImagingPanelToGantryTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());


  vtkMRMLLinearTransformNode* flatPanelToGantryTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::FlatPanel, vtkIECTransformLogic::Gantry);
  if (flatPanelToGantryTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Flat panel to Gantry transform");
    return;
  }
  flatPanelToGantryTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::FlatPanel, vtkIECTransformLogic::Gantry));
  flatPanelToGantryTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());


  vtkMRMLLinearTransformNode* patientSupportRotationToFixedReferenceTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::PatientSupportRotation, vtkIECTransformLogic::FixedReference);
  if (patientSupportRotationToFixedReferenceTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Patient support rotation to Fixed reference transform");
    return;
  }
  patientSupportRotationToFixedReferenceTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::PatientSupportRotation, vtkIECTransformLogic::FixedReference));
  patientSupportRotationToFixedReferenceTransformNode->SetAndObserveTransformNodeID(fixedReferenceToRASTransformNode->GetID());


  vtkMRMLLinearTransformNode* patientSupportToPatientSupportRotationTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::PatientSupport, vtkIECTransformLogic::PatientSupportRotation);
  if (patientSupportToPatientSupportRotationTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Patient support to Patient support rotation transform");
    return;
  }
  patientSupportToPatientSupportRotationTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::PatientSupport, vtkIECTransformLogic::PatientSupportRotation));
  patientSupportToPatientSupportRotationTransformNode->SetAndObserveTransformNodeID(patientSupportRotationToFixedReferenceTransformNode->GetID());


  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportRotationTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::TableTopEccentricRotation, vtkIECTransformLogic::PatientSupportRotation);
  if (tableTopEccentricRotationToPatientSupportRotationTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Table top eccentric rotation to Patient support rotation transform");
    return;
  }
  tableTopEccentricRotationToPatientSupportRotationTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::TableTopEccentricRotation, vtkIECTransformLogic::PatientSupportRotation));
  tableTopEccentricRotationToPatientSupportRotationTransformNode->SetAndObserveTransformNodeID(patientSupportRotationToFixedReferenceTransformNode->GetID());


  vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::TableTop, vtkIECTransformLogic::TableTopEccentricRotation);
  if (tableTopToTableTopEccentricRotationTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Table top to Table top eccentric rotation transform");
    return;
  }
  tableTopToTableTopEccentricRotationTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::TableTop, vtkIECTransformLogic::TableTopEccentricRotation));
  tableTopToTableTopEccentricRotationTransformNode->SetAndObserveTransformNodeID(tableTopEccentricRotationToPatientSupportRotationTransformNode->GetID());


  vtkMRMLLinearTransformNode* patientToTableTopTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::Patient, vtkIECTransformLogic::TableTop);
  if (patientToTableTopTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access Patient to Table top transform");
    return;
  }
  patientToTableTopTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::Patient, vtkIECTransformLogic::TableTop));
  patientToTableTopTransformNode->SetAndObserveTransformNodeID(tableTopToTableTopEccentricRotationTransformNode->GetID());


  vtkMRMLLinearTransformNode* rasToPatientTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::RAS, vtkIECTransformLogic::Patient);
  if (rasToPatientTransformNode == nullptr)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access RAS to Patient transform");
    return;
  }
  rasToPatientTransformNode->SetAndObserveTransformToParent(this->IECLogic->GetElementaryTransformBetween(vtkIECTransformLogic::RAS, vtkIECTransformLogic::Patient));
  rasToPatientTransformNode->SetAndObserveTransformNodeID(patientToTableTopTransformNode->GetID());


  // Make sure the fixed reference to RAS is correct
  beamsLogic->UpdateRASRelatedTransforms(this->IECLogic);
}

//----------------------------------------------------------------------------
std::vector<vtkSlicerRoomsEyeViewModuleLogic::TreatmentMachinePartType>
vtkSlicerRoomsEyeViewModuleLogic::LoadTreatmentMachine(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("LoadTreatmentMachine: Invalid scene");
    return std::vector<TreatmentMachinePartType>();
  }
  vtkMRMLSubjectHierarchyNode* shNode = scene->GetSubjectHierarchyNode();
  if (!shNode)
  {
    vtkErrorMacro("LoadTreatmentMachine: Failed to access subject hierarchy node");
    return std::vector<TreatmentMachinePartType>();
  }
  if (!parameterNode || !parameterNode->GetTreatmentMachineDescriptorFilePath())
  {
    vtkErrorMacro("LoadTreatmentMachine: Invalid parameter node");
    return std::vector<TreatmentMachinePartType>();
  }

  // Make sure the transform hierarchy is in place
  this->BuildRoomsEyeViewTransformHierarchy();

  std::string moduleShareDirectory = this->GetModuleShareDirectory();
  std::string descriptorFilePath(parameterNode->GetTreatmentMachineDescriptorFilePath());
  std::string machineType = this->Internal->GetTreatmentMachineFileNameWithoutExtension(parameterNode);

  // Load treatment machine JSON descriptor file
  FILE *fp = fopen(descriptorFilePath.c_str(), "r");
  if (!fp)
    {
      vtkErrorMacro("LoadTreatmentMachine: Failed to load treatment machine descriptor file '" << descriptorFilePath << "'");
      return std::vector<TreatmentMachinePartType>();
    }
  char buffer[4096];
  rapidjson::FileReadStream fs(fp, buffer, sizeof(buffer));
  if (this->Internal->CurrentTreatmentMachineDescription->ParseStream(fs).HasParseError())
    {
      vtkErrorMacro("LoadTreatmentMachine: Failed to load treatment machine descriptor file '" << descriptorFilePath << "'");
      fclose(fp);
      return std::vector<TreatmentMachinePartType>();
    }
  fclose(fp);

  // Create subject hierarchy folder so that the treatment machine can be shown/hidden easily
  std::string subjectHierarchyFolderName = machineType + std::string("_Components");
  vtkIdType rootFolderItem = shNode->CreateFolderItem(shNode->GetSceneItemID(), subjectHierarchyFolderName);
  shNode->SetItemAttribute(rootFolderItem, TREATMENT_MACHINE_DESCRIPTOR_FILE_PATH_ATTRIBUTE_NAME, descriptorFilePath);

  // Load treatment machine models

  // Collimator - mandatory
  this->Internal->EnsureTreatmentMachinePartModelNode(parameterNode, Collimator);
  // Gantry - mandatory
  this->Internal->EnsureTreatmentMachinePartModelNode(parameterNode, Gantry);
  // Patient support - mandatory
  this->Internal->EnsureTreatmentMachinePartModelNode(parameterNode, PatientSupport);
  // Table top - mandatory
  this->Internal->EnsureTreatmentMachinePartModelNode(parameterNode, TableTop);
  // Linac body - optional
  this->Internal->EnsureTreatmentMachinePartModelNode(parameterNode, Body, true);
  // Imaging panel left - optional
  this->Internal->EnsureTreatmentMachinePartModelNode(parameterNode, ImagingPanelLeft, true);
  // Imaging panel right - optional
  this->Internal->EnsureTreatmentMachinePartModelNode(parameterNode, ImagingPanelRight, true);
  // Flat panel - optional
  this->Internal->EnsureTreatmentMachinePartModelNode(parameterNode, FlatPanel, true);

  // Setup treatment machine model display and transforms
  return this->SetupTreatmentMachineModels(parameterNode);
}

//----------------------------------------------------------------------------
std::vector<vtkSlicerRoomsEyeViewModuleLogic::TreatmentMachinePartType>
vtkSlicerRoomsEyeViewModuleLogic::SetupTreatmentMachineModels(vtkMRMLRoomsEyeViewNode* parameterNode, bool forceEnableCollisionDetection/*=false*/)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Invalid scene");
    return std::vector<TreatmentMachinePartType>();
  }

  std::vector<TreatmentMachinePartType> loadedParts;
  std::map<TreatmentMachinePartType, unsigned int> loadedPartsNumTriangles;
  for (int partIdx=0; partIdx<LastPartType; ++partIdx)
  {
    std::string partType = this->GetTreatmentMachinePartTypeAsString((TreatmentMachinePartType)partIdx);
    vtkMRMLModelNode* partModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, (TreatmentMachinePartType)partIdx);
    if (!partModel || !partModel->GetPolyData())
    {
      switch (partIdx)
      {
        case Collimator: case Gantry: case PatientSupport: case TableTop:
          vtkErrorMacro("SetupTreatmentMachineModels: Unable to access " << partType << " model");
      }
      continue;
    }

    loadedParts.push_back((TreatmentMachinePartType)partIdx);
    loadedPartsNumTriangles[(TreatmentMachinePartType)partIdx] = partModel->GetPolyData()->GetNumberOfCells();

    // Set color
    vtkVector3d partColor(this->GetColorForPartType(partType));
    partModel->CreateDefaultDisplayNodes();
    partModel->GetDisplayNode()->SetColor((double)partColor[0] / 255.0, (double)partColor[1] / 255.0, (double)partColor[2] / 255.0);

    // Apply file to RAS transform matrix
    vtkNew<vtkMatrix4x4> fileToRASTransformMatrix;
    if (this->GetFileToRASTransformMatrixForPartType(partType, fileToRASTransformMatrix))
    {
      vtkNew<vtkTransform> fileToRASTransform;
      fileToRASTransform->SetMatrix(fileToRASTransformMatrix);
      vtkNew<vtkTransformPolyDataFilter> transformPolyDataFilter;
      transformPolyDataFilter->SetInputConnection(partModel->GetPolyDataConnection());
      transformPolyDataFilter->SetTransform(fileToRASTransform);
      transformPolyDataFilter->Update();
      vtkNew<vtkPolyData> partPolyDataRAS;
      partPolyDataRAS->DeepCopy(transformPolyDataFilter->GetOutput());
      partModel->SetAndObservePolyData(partPolyDataRAS);
    }
    else
    {
      vtkErrorMacro("SetupTreatmentMachineModels: Failed to set file to RAS matrix for treatment machine part " << partType);
    }

    // Setup transforms and collision detection
    if (partIdx == Collimator)
    {
      vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
        this->GetTransformNodeBetween(vtkIECTransformLogic::Collimator, vtkIECTransformLogic::Gantry);
      partModel->SetAndObserveTransformNodeID(collimatorToGantryTransformNode->GetID());
      this->CollimatorTableTopCollisionDetection->SetInputData(0, partModel->GetPolyData());
      // Patient model is set when calculating collisions, as it can be changed dynamically
      this->CollimatorPatientCollisionDetection->SetInputData(0, partModel->GetPolyData());
    }
    else if (partIdx == Gantry)
    {
      vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode =
        this->GetTransformNodeBetween(vtkIECTransformLogic::Gantry, vtkIECTransformLogic::FixedReference);
      partModel->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());
      this->GantryTableTopCollisionDetection->SetInputData(0, partModel->GetPolyData());
      this->GantryPatientSupportCollisionDetection->SetInputData(0, partModel->GetPolyData());
      // Patient model is set when calculating collisions, as it can be changed dynamically
      this->GantryPatientCollisionDetection->SetInputData(0, partModel->GetPolyData());
    }
    else if (partIdx == PatientSupport)
    {
      vtkMRMLLinearTransformNode* patientSupportToPatientSupportRotationTransformNode =
        this->GetTransformNodeBetween(vtkIECTransformLogic::PatientSupport, vtkIECTransformLogic::PatientSupportRotation);
      partModel->SetAndObserveTransformNodeID(patientSupportToPatientSupportRotationTransformNode->GetID());
      this->GantryPatientSupportCollisionDetection->SetInputData(1, partModel->GetPolyData());
    }
    else if (partIdx == TableTop)
    {
      vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode =
        this->GetTransformNodeBetween(vtkIECTransformLogic::TableTop, vtkIECTransformLogic::TableTopEccentricRotation);
      partModel->SetAndObserveTransformNodeID(tableTopToTableTopEccentricRotationTransformNode->GetID());
      this->GantryTableTopCollisionDetection->SetInputData(1, partModel->GetPolyData());
      this->CollimatorTableTopCollisionDetection->SetInputData(1, partModel->GetPolyData());
    }
    else if (partIdx == Body)
    {
      vtkMRMLLinearTransformNode* fixedReferenceToRasTransformNode =
        this->GetTransformNodeBetween(vtkIECTransformLogic::FixedReference, vtkIECTransformLogic::RAS);
      partModel->SetAndObserveTransformNodeID(fixedReferenceToRasTransformNode->GetID());
    }
    else if (partIdx == ImagingPanelLeft)
    {
      vtkMRMLLinearTransformNode* leftImagingPanelToGantryTransformNode =
        this->GetTransformNodeBetween(vtkIECTransformLogic::LeftImagingPanel, vtkIECTransformLogic::Gantry);
      partModel->SetAndObserveTransformNodeID(leftImagingPanelToGantryTransformNode->GetID());
    }
    else if (partIdx == ImagingPanelRight)
    {
      vtkMRMLLinearTransformNode* rightImagingPanelToGantryTransformNode =
        this->GetTransformNodeBetween(vtkIECTransformLogic::RightImagingPanel, vtkIECTransformLogic::Gantry);
      partModel->SetAndObserveTransformNodeID(rightImagingPanelToGantryTransformNode->GetID());
    }
    else if (partIdx == FlatPanel)
    {
      vtkMRMLLinearTransformNode* flatPanelToGantryTransformNode =
        this->GetTransformNodeBetween(vtkIECTransformLogic::FlatPanel, vtkIECTransformLogic::Gantry);
      partModel->SetAndObserveTransformNodeID(flatPanelToGantryTransformNode->GetID());
    }
    //TODO: ApplicatorHolder, ElectronApplicator?
  }

  // Disable collision detection if product of number of triangles of the two models is above threshold
  if (loadedPartsNumTriangles[Gantry] * loadedPartsNumTriangles[TableTop] > MAX_TRIANGLE_NUMBER_PRODUCT_FOR_COLLISIONS && !forceEnableCollisionDetection)
  {
    vtkWarningMacro("Too many combined triangles (product = " << loadedPartsNumTriangles[Gantry] * loadedPartsNumTriangles[TableTop]
      << ") detected between gantry and table top. Collision detection may take a very long time.");
    this->GantryTableTopCollisionDetection->SetInputData(0, nullptr);
    this->GantryTableTopCollisionDetection->SetInputData(1, nullptr);
  }
  if (loadedPartsNumTriangles[Gantry] * loadedPartsNumTriangles[PatientSupport] > MAX_TRIANGLE_NUMBER_PRODUCT_FOR_COLLISIONS && !forceEnableCollisionDetection)
  {
    vtkWarningMacro("Too many combined triangles (product = " << loadedPartsNumTriangles[Gantry] * loadedPartsNumTriangles[PatientSupport]
      << ") detected between gantry and patient support. Collision detection may take a very long time.");
    this->GantryPatientSupportCollisionDetection->SetInputData(0, nullptr);
    this->GantryPatientSupportCollisionDetection->SetInputData(1, nullptr);
  }
  if (loadedPartsNumTriangles[Collimator] * loadedPartsNumTriangles[TableTop] > MAX_TRIANGLE_NUMBER_PRODUCT_FOR_COLLISIONS && !forceEnableCollisionDetection)
  {
    vtkWarningMacro("Too many combined triangles (product = " << loadedPartsNumTriangles[Collimator] * loadedPartsNumTriangles[TableTop]
      << ") detected between collimator and table top. Collision detection may take a very long time.");
    this->CollimatorTableTopCollisionDetection->SetInputData(0, nullptr);
    this->CollimatorTableTopCollisionDetection->SetInputData(1, nullptr);
  }

  // Set identity transform for patient (parent transform is taken into account when getting poly data from segmentation)
  vtkNew<vtkTransform> identityTransform;
  identityTransform->Identity();
  this->GantryPatientCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(identityTransform));
  this->CollimatorPatientCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(identityTransform));

  return loadedParts;
}

//-----------------------------------------------------------------------------
vtkMRMLModelNode* vtkSlicerRoomsEyeViewModuleLogic::UpdateTreatmentOrientationMarker(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("UpdateTreatmentOrientationMarker: Invalid scene");
    return nullptr;
  }

  std::string machineType = this->Internal->GetTreatmentMachineFileNameWithoutExtension(parameterNode);

  vtkNew<vtkAppendPolyData> appendFilter;

  vtkMRMLModelNode* collimatorModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, Collimator);
  vtkMRMLModelNode* gantryModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, Gantry);
  vtkMRMLModelNode* patientSupportModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, PatientSupport);
  vtkMRMLModelNode* tableTopModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, TableTop);
  if ( !gantryModel || !gantryModel->GetPolyData() || !collimatorModel || !collimatorModel->GetPolyData()
    || !patientSupportModel || !patientSupportModel->GetPolyData() || !tableTopModel || !tableTopModel->GetPolyData() )
  {
    // Orientation marker cannot be assembled if poly data is missing from the mandatory model nodes.
    vtkErrorMacro("UpdateTreatmentOrientationMarker: Failed to access at least one mandatory treatment machine part.");
    return nullptr;
  }

  //
  // Mandatory models

  // Gantry
  vtkNew<vtkPolyData> gantryModelPolyData;
  gantryModelPolyData->DeepCopy(gantryModel->GetPolyData());

  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::Gantry, vtkIECTransformLogic::FixedReference);
  vtkNew<vtkTransformFilter> gantryTransformFilter;
  gantryTransformFilter->SetInputData(gantryModelPolyData);
  vtkNew<vtkGeneralTransform> gantryToFixedReferenceTransform;
  gantryToFixedReferenceTransformNode->GetTransformFromWorld(gantryToFixedReferenceTransform);
  gantryToFixedReferenceTransform->Inverse();
  gantryTransformFilter->SetTransform(gantryToFixedReferenceTransform);
  gantryTransformFilter->Update();

  appendFilter->AddInputData(vtkPolyData::SafeDownCast(gantryTransformFilter->GetOutput()));

  // Collimator
  vtkNew<vtkPolyData> collimatorModelPolyData;
  collimatorModelPolyData->DeepCopy(collimatorModel->GetPolyData());

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::Collimator, vtkIECTransformLogic::Gantry);
  vtkNew<vtkTransformFilter> collimatorTransformFilter;
  collimatorTransformFilter->SetInputData(collimatorModelPolyData);
  vtkNew<vtkGeneralTransform> collimatorToGantryTransform;
  collimatorToGantryTransformNode->GetTransformFromWorld(collimatorToGantryTransform);
  collimatorToGantryTransform->Inverse();
  collimatorTransformFilter->SetTransform(collimatorToGantryTransform);
  collimatorTransformFilter->Update();

  appendFilter->AddInputData(vtkPolyData::SafeDownCast(collimatorTransformFilter->GetOutput()));

  // Patient support
  vtkNew<vtkPolyData> patientSupportModelPolyData;
  patientSupportModelPolyData->DeepCopy(patientSupportModel->GetPolyData());

  vtkMRMLLinearTransformNode* patientSupportToPatientSupportRotationTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::PatientSupport, vtkIECTransformLogic::PatientSupportRotation);
  vtkNew<vtkTransformFilter> patientSupportTransformFilter;
  patientSupportTransformFilter->SetInputData(patientSupportModelPolyData);
  vtkNew<vtkGeneralTransform> patientSupportToPatientSupportRotationTransform;
  patientSupportToPatientSupportRotationTransformNode->GetTransformFromWorld(patientSupportToPatientSupportRotationTransform);
  patientSupportToPatientSupportRotationTransform->Inverse();
  patientSupportTransformFilter->SetTransform(patientSupportToPatientSupportRotationTransform);
  patientSupportTransformFilter->Update();

  appendFilter->AddInputData(vtkPolyData::SafeDownCast(patientSupportTransformFilter->GetOutput()));

  // Table top
  vtkNew<vtkPolyData> tableTopModelPolyData;
  tableTopModelPolyData->DeepCopy(tableTopModel->GetPolyData());

  vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::TableTop, vtkIECTransformLogic::TableTopEccentricRotation);
  vtkNew<vtkTransformFilter> tableTopTransformFilter;
  tableTopTransformFilter->SetInputData(tableTopModelPolyData);
  vtkNew<vtkGeneralTransform> tableTopModelTransform;
  tableTopToTableTopEccentricRotationTransformNode->GetTransformFromWorld(tableTopModelTransform);
  tableTopModelTransform->Inverse();
  tableTopTransformFilter->SetTransform(tableTopModelTransform);
  tableTopTransformFilter->Update();

  appendFilter->AddInputData(vtkPolyData::SafeDownCast(tableTopTransformFilter->GetOutput()));

  // Optional models
  vtkMRMLModelNode* imagingPanelLeftModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, ImagingPanelLeft);
  if (imagingPanelLeftModel)
  {
    vtkNew<vtkPolyData> leftImagingPanelModelPolyData;
    leftImagingPanelModelPolyData->DeepCopy(imagingPanelLeftModel->GetPolyData());

    vtkMRMLLinearTransformNode* leftImagingPanelToGantryTransformNode =
      this->GetTransformNodeBetween(vtkIECTransformLogic::LeftImagingPanel, vtkIECTransformLogic::Gantry);
    vtkNew<vtkTransformFilter> leftImagingPanelTransformFilter;
    leftImagingPanelTransformFilter->SetInputData(leftImagingPanelModelPolyData);
    vtkNew<vtkGeneralTransform> leftImagingPanelToGantryTransform;
    leftImagingPanelToGantryTransformNode->GetTransformFromWorld(leftImagingPanelToGantryTransform);
    leftImagingPanelToGantryTransform->Inverse();
    leftImagingPanelTransformFilter->SetTransform(leftImagingPanelToGantryTransform);
    leftImagingPanelTransformFilter->Update();

    appendFilter->AddInputData(vtkPolyData::SafeDownCast(leftImagingPanelTransformFilter->GetOutput()));
  }

  vtkMRMLModelNode* imagingPanelRightModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, ImagingPanelRight);
  if (imagingPanelRightModel)
  {
    vtkNew<vtkPolyData> rightImagingPanelModelPolyData;
    rightImagingPanelModelPolyData->DeepCopy(imagingPanelRightModel->GetPolyData());

    vtkMRMLLinearTransformNode* rightImagingPanelToGantryTransformNode =
      this->GetTransformNodeBetween(vtkIECTransformLogic::RightImagingPanel, vtkIECTransformLogic::Gantry);
    vtkNew<vtkTransformFilter> rightImagingPanelTransformFilter;
    rightImagingPanelTransformFilter->SetInputData(rightImagingPanelModelPolyData);
    vtkNew<vtkGeneralTransform> rightImagingPanelToGantryTransform;
    rightImagingPanelToGantryTransformNode->GetTransformFromWorld(rightImagingPanelToGantryTransform);
    rightImagingPanelToGantryTransform->Inverse();
    rightImagingPanelTransformFilter->SetTransform(rightImagingPanelToGantryTransform);
    rightImagingPanelTransformFilter->Update();

    appendFilter->AddInputData(vtkPolyData::SafeDownCast(rightImagingPanelTransformFilter->GetOutput()));
  }

  vtkMRMLModelNode* flatPanelModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, FlatPanel);
  if (flatPanelModel)
  {
    vtkNew<vtkPolyData> flatPanelModelPolyData;
    flatPanelModelPolyData->DeepCopy(flatPanelModel->GetPolyData());

    vtkMRMLLinearTransformNode* flatPanelToGantryTransformNode =
      this->GetTransformNodeBetween(vtkIECTransformLogic::FlatPanel, vtkIECTransformLogic::Gantry);
    vtkNew<vtkTransformFilter> flatPanelTransformFilter;
    flatPanelTransformFilter->SetInputData(flatPanelModelPolyData);
    vtkNew<vtkGeneralTransform> flatPanelToGantryTransform;
    flatPanelToGantryTransformNode->GetTransformFromWorld(flatPanelToGantryTransform);
    flatPanelToGantryTransform->Inverse();
    flatPanelTransformFilter->SetTransform(flatPanelToGantryTransform);
    flatPanelTransformFilter->Update();

    appendFilter->AddInputData(vtkPolyData::SafeDownCast(flatPanelTransformFilter->GetOutput()));
  }

  vtkNew<vtkPolyData> orientationMarkerPolyData;
  appendFilter->Update();
  orientationMarkerPolyData->DeepCopy(appendFilter->GetOutput());

  // Get or create orientation marker model node
  vtkSmartPointer<vtkMRMLModelNode> orientationMarkerModel =
    vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(ORIENTATION_MARKER_MODEL_NODE_NAME));
  if (!orientationMarkerModel)
  {
    orientationMarkerModel = vtkSmartPointer<vtkMRMLModelNode>::New();
    orientationMarkerModel->SetName(ORIENTATION_MARKER_MODEL_NODE_NAME);
    this->GetMRMLScene()->AddNode(orientationMarkerModel);
  }
  orientationMarkerModel->SetAndObservePolyData(orientationMarkerPolyData);

  return orientationMarkerModel.GetPointer();
}

//----------------------------------------------------------------------------
bool vtkSlicerRoomsEyeViewModuleLogic::GetPatientBodyPolyData(vtkMRMLRoomsEyeViewNode* parameterNode, vtkPolyData* patientBodyPolyData)
{
  if (!parameterNode)
  {
    vtkErrorMacro("GetPatientBodyPolyData: Invalid parameter set node");
    return false;
  }
  if (!patientBodyPolyData)
  {
    vtkErrorMacro("GetPatientBodyPolyData: Invalid output poly data");
    return false;
  }

  // Get patient body segmentation
  vtkMRMLSegmentationNode* segmentationNode = parameterNode->GetPatientBodySegmentationNode();
  if (!segmentationNode || !parameterNode->GetPatientBodySegmentID())
  {
    return false;
  }

  // Get closed surface representation for patient body
  return vtkSlicerSegmentationsModuleLogic::GetSegmentRepresentation(
    segmentationNode, parameterNode->GetPatientBodySegmentID(),
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName(),
    patientBodyPolyData );
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateGantryToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateGantryToFixedReferenceTransform: Invalid parameter set node");
    return;
  }

  this->IECLogic->UpdateGantryToFixedReferenceTransform(parameterNode->GetGantryRotationAngle());
  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::Gantry, vtkIECTransformLogic::FixedReference);
  gantryToFixedReferenceTransformNode->Modified(); // Modified call is needed because it does not update display in app
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateCollimatorToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateCollimatorToGantryTransform: Invalid parameter set node");
    return;
  }

  this->IECLogic->UpdateCollimatorToGantryTransform(parameterNode->GetCollimatorRotationAngle());
  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::Collimator, vtkIECTransformLogic::Gantry);
  collimatorToGantryTransformNode->Modified(); // Modified call is needed because it does not update display in app
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateLeftImagingPanelToGantryTransform: Invalid parameter set node");
    return;
  }

  std::string machineType = this->Internal->GetTreatmentMachineFileNameWithoutExtension(parameterNode);

  vtkMRMLModelNode* imagingPanelLeftModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, ImagingPanelLeft);
  if (!imagingPanelLeftModel)
  {
    vtkDebugMacro("UpdateLeftImagingPanelToGantryTransform: Optional imaging panel left model not found");
    return;
  }

  // Translation to origin for in-place rotation
  vtkPolyData* leftImagingPanelModelPolyData = imagingPanelLeftModel->GetPolyData();

  double leftImagingPanelModelBounds[6] = { 0, 0, 0, 0, 0, 0 };
  leftImagingPanelModelPolyData->GetBounds(leftImagingPanelModelBounds);

  double leftImagingPanelTranslationFromOrigin[3] = { 0, 0, 0 };
  leftImagingPanelTranslationFromOrigin[0] = leftImagingPanelModelBounds[1];
  leftImagingPanelTranslationFromOrigin[1] = (leftImagingPanelModelBounds[2] + leftImagingPanelModelBounds[3]) / 2;
  leftImagingPanelTranslationFromOrigin[2] = (leftImagingPanelModelBounds[4] + leftImagingPanelModelBounds[5]) / 2;

  double leftImagingPanelTranslationToOrigin[3] = { 0, 0, 0 };
  for (int i = 0; i < 3; i++)
  {
    leftImagingPanelTranslationToOrigin[i] = (-1.0) * leftImagingPanelTranslationFromOrigin[i];
  }

  vtkNew<vtkTransform> leftImagingPanelToRasTransform;
  leftImagingPanelToRasTransform->Translate(leftImagingPanelTranslationToOrigin);

  // Rotation
  vtkNew<vtkTransform> rasToRotatedRasTransform;
  double panelMovement = parameterNode->GetImagingPanelMovement();
  if (panelMovement > 0)
  {
    rasToRotatedRasTransform->RotateZ(68.5);
  }
  else
  {
    rasToRotatedRasTransform->RotateZ(panelMovement + 68.5);
  }

  // Translation back from origin after in-place rotation
  vtkNew<vtkTransform> rotatedRasToRotatedLeftImagingPanelTransform;
  rotatedRasToRotatedLeftImagingPanelTransform->Translate(leftImagingPanelTranslationFromOrigin);

  // Translation
  vtkNew<vtkTransform> rotatedLeftImagingPanelToGantryTransform;
  if (panelMovement > 0)
  {
    double translationArray[3] = { 0, -(panelMovement), 0 };
    rotatedLeftImagingPanelToGantryTransform->Translate(translationArray);
  }

  // Assemble transform and update node
  vtkMRMLLinearTransformNode* leftImagingPanelToGantryTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::LeftImagingPanel, vtkIECTransformLogic::Gantry);
  vtkNew<vtkTransform> leftImagingPanelToGantryTransform;
  leftImagingPanelToGantryTransform->PostMultiply();
  leftImagingPanelToGantryTransform->Concatenate(leftImagingPanelToRasTransform);
  leftImagingPanelToGantryTransform->Concatenate(rasToRotatedRasTransform);
  leftImagingPanelToGantryTransform->Concatenate(rotatedRasToRotatedLeftImagingPanelTransform);
  leftImagingPanelToGantryTransform->Concatenate(rotatedLeftImagingPanelToGantryTransform);
  leftImagingPanelToGantryTransformNode->SetAndObserveTransformToParent(leftImagingPanelToGantryTransform);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateRightImagingPanelToGantryTransform: Invalid parameter set node");
    return;
  }

  std::string machineType = this->Internal->GetTreatmentMachineFileNameWithoutExtension(parameterNode);

  vtkMRMLModelNode* imagingPanelRightModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, ImagingPanelRight);
  if (!imagingPanelRightModel)
  {
    vtkDebugMacro("UpdateRightImagingPanelToGantryTransform: Optional imaging panel right model not found");
    return;
  }
  vtkPolyData* rightImagingPanelModelPolyData = imagingPanelRightModel->GetPolyData();
  double rightImagingPanelModelBounds[6] = { 0, 0, 0, 0, 0, 0 };
  rightImagingPanelModelPolyData->GetBounds(rightImagingPanelModelBounds);

  // Translation to origin for in-place rotation
  double rightImagingPanelTranslationFromOrigin[3] = { 0, 0, 0 };
  rightImagingPanelTranslationFromOrigin[0] = rightImagingPanelModelBounds[0];
  rightImagingPanelTranslationFromOrigin[1] = (rightImagingPanelModelBounds[2] + rightImagingPanelModelBounds[3]) / 2;
  rightImagingPanelTranslationFromOrigin[2] = (rightImagingPanelModelBounds[4] + rightImagingPanelModelBounds[5]) / 2;

  double rightImagingPanelTranslationToOrigin[3] = { 0, 0, 0 };
  for (int i = 0; i < 3; i++)
  {
    rightImagingPanelTranslationToOrigin[i] = (-1.0) * rightImagingPanelTranslationFromOrigin[i];
  }

  vtkNew<vtkTransform> rightImagingPanelToRasTransform;
  rightImagingPanelToRasTransform->Translate(rightImagingPanelTranslationToOrigin);

  // Rotation
  vtkNew<vtkTransform> rasToRotatedRasTransform;
  double panelMovement = parameterNode->GetImagingPanelMovement();
  if (panelMovement > 0)
  {
    rasToRotatedRasTransform->RotateZ(-(68.5));
  }
  else
  {
    rasToRotatedRasTransform->RotateZ(-(68.5 + panelMovement));
  }

  // Translation back from origin after in-place rotation
  vtkNew<vtkTransform> rotatedRasToRotatedRightImagingPanelTransform;
  rotatedRasToRotatedRightImagingPanelTransform->Translate(rightImagingPanelTranslationFromOrigin);

  // Translation
  vtkNew<vtkTransform> rotatedRightImagingPanelToGantryTransform;
  if (panelMovement > 0)
  {
    double translationArray[3] = { 0, -(panelMovement), 0 };
    rotatedRightImagingPanelToGantryTransform->Translate(translationArray);
  }

  // Assemble transform and update node
  vtkMRMLLinearTransformNode* rightImagingPanelToGantryTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::RightImagingPanel, vtkIECTransformLogic::Gantry);
  vtkNew<vtkTransform> rightImagingPanelToGantryTransform;
  rightImagingPanelToGantryTransform->PostMultiply();
  rightImagingPanelToGantryTransform->Concatenate(rightImagingPanelToRasTransform);
  rightImagingPanelToGantryTransform->Concatenate(rasToRotatedRasTransform);
  rightImagingPanelToGantryTransform->Concatenate(rotatedRasToRotatedRightImagingPanelTransform);
  rightImagingPanelToGantryTransform->Concatenate(rotatedRightImagingPanelToGantryTransform);
  rightImagingPanelToGantryTransformNode->SetAndObserveTransformToParent(rightImagingPanelToGantryTransform);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateImagingPanelMovementTransforms(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateImagingPanelMovementTransforms: Invalid parameter set node");
    return;
  }

  this->UpdateLeftImagingPanelToGantryTransform(parameterNode);
  this->UpdateRightImagingPanelToGantryTransform(parameterNode);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportRotationToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdatePatientSupportRotationToFixedReferenceTransform: Invalid parameter set node");
    return;
  }

  this->IECLogic->UpdatePatientSupportRotationToFixedReferenceTransform(parameterNode->GetPatientSupportRotationAngle());
  vtkMRMLLinearTransformNode* patientSupportRotationToFixedReferenceTransformNode = this->GetTransformNodeBetween(vtkIECTransformLogic::PatientSupportRotation, vtkIECTransformLogic::FixedReference);
  patientSupportRotationToFixedReferenceTransformNode->Modified(); // Modified call is needed because it does not update display in app
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportToPatientSupportRotationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdatePatientSupportToPatientSupportRotationTransform: Invalid parameter set node");
    return;
  }

  std::string machineType = this->Internal->GetTreatmentMachineFileNameWithoutExtension(parameterNode);

  vtkMRMLModelNode* patientSupportModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, PatientSupport);
  vtkMRMLModelNode* tableTopModel = this->Internal->GetTreatmentMachinePartModelNode(parameterNode, TableTop);
  if (!patientSupportModel || !patientSupportModel->GetPolyData() || !tableTopModel || !tableTopModel->GetPolyData())
  {
    vtkErrorMacro("UpdatePatientSupportToPatientSupportRotationTransform: Failed to access treatment machine part models");
    return;
  }

  // Get bounds of parts involved in computation
  double patientSupportModelBounds[6] = { 0, 0, 0, 0, 0, 0 };
  patientSupportModel->GetPolyData()->GetBounds(patientSupportModelBounds);
  double tableTopModelBounds[6] = { 0, 0, 0, 0, 0, 0 };
  tableTopModel->GetPolyData()->GetBounds(tableTopModelBounds);

  // Translation to origin for in-place vertical scaling
  vtkNew<vtkTransform> patientSupportScalingTransform;
  patientSupportScalingTransform->PostMultiply();
  double patientSupportTranslationToOrigin[3] = { 0, 0, (-1.0) * patientSupportModelBounds[4]};
  patientSupportScalingTransform->Translate(patientSupportTranslationToOrigin);

  // Apply patient support vertical scale
  double tableTopDisplacement = parameterNode->GetVerticalTableTopDisplacement();
  double newPatientSupportHeight = /*tableTopModelBounds[4] + */patientSupportModelBounds[5] + tableTopDisplacement - patientSupportModelBounds[4];
  double originalPatientSupportHeight = patientSupportModelBounds[5] - patientSupportModelBounds[4];
  patientSupportScalingTransform->Scale(1.0, 1.0, newPatientSupportHeight / originalPatientSupportHeight);

  // Translate back so that the patient support base is at the same height
  double patientSupportTranslationFromOrigin[3] = { 0, 0, patientSupportModelBounds[4]};
  patientSupportScalingTransform->Translate(patientSupportTranslationFromOrigin);

  vtkMRMLLinearTransformNode* patientSupportToPatientSupportRotationTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::PatientSupport, vtkIECTransformLogic::PatientSupportRotation);
  patientSupportToPatientSupportRotationTransformNode->SetAndObserveTransformToParent(patientSupportScalingTransform);
  patientSupportToPatientSupportRotationTransformNode->Modified(); // Modified call is needed because it does not update display in app
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateTableTopEccentricRotationToPatientSupportRotationTransform(vtkMRMLRoomsEyeViewNode* vtkNotUsed(parameterNode))
{
  vtkErrorMacro("UpdateTableTopEccentricRotationToPatientSupportRotationTransform: Not implemented, as table top eccentric rotation is not yet supported");
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateTableTopToTableTopEccentricRotationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateTableTopToTableTopEccentricRotationTransform: Invalid parameter set node");
    return;
  }

  vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::TableTop, vtkIECTransformLogic::TableTopEccentricRotation);
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopToTableTopEccentricRotationTransformNode->GetTransformToParent() );

  double translationArray[3] =
    { parameterNode->GetLateralTableTopDisplacement(), parameterNode->GetLongitudinalTableTopDisplacement(), parameterNode->GetVerticalTableTopDisplacement() };

  vtkNew<vtkMatrix4x4> tableTopEccentricRotationToPatientSupportMatrix;
  tableTopEccentricRotationToPatientSupportMatrix->SetElement(0,3, translationArray[0]);
  tableTopEccentricRotationToPatientSupportMatrix->SetElement(1,3, translationArray[1]);
  tableTopEccentricRotationToPatientSupportMatrix->SetElement(2,3, translationArray[2]);
  tableTopEccentricRotationToPatientSupportTransform->SetMatrix(tableTopEccentricRotationToPatientSupportMatrix);
  tableTopEccentricRotationToPatientSupportTransform->Modified(); // Modified call is needed because it does not update display in app
}

//-----------------------------------------------------------------------------
std::string vtkSlicerRoomsEyeViewModuleLogic::CheckForCollisions(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("CheckForCollisions: Invalid parameter set node");
    return "Invalid parameters";
  }
  if (!parameterNode->GetCollisionDetectionEnabled())
  {
    return "";
  }

  std::string statusString = "";

  // Get transforms used in the collision detection filters
  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::Gantry, vtkIECTransformLogic::FixedReference);
  vtkMRMLLinearTransformNode* patientSupportToPatientSupportRotationTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::PatientSupport, vtkIECTransformLogic::PatientSupportRotation);
  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::Collimator, vtkIECTransformLogic::Gantry);
  vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode =
    this->GetTransformNodeBetween(vtkIECTransformLogic::TableTop, vtkIECTransformLogic::TableTopEccentricRotation);

  if ( !gantryToFixedReferenceTransformNode || !patientSupportToPatientSupportRotationTransformNode
    || !collimatorToGantryTransformNode || !tableTopToTableTopEccentricRotationTransformNode )
  {
    statusString = "Failed to access IEC transforms";
    vtkErrorMacro("CheckForCollisions: " + statusString);
    return statusString;
  }

  // Get transforms to world, make sure they are linear
  vtkNew<vtkGeneralTransform> gantryToRasGeneralTransform;
  gantryToFixedReferenceTransformNode->GetTransformToWorld(gantryToRasGeneralTransform);
  vtkNew<vtkTransform> gantryToRasTransform;

  vtkNew<vtkGeneralTransform> patientSupportToRasGeneralTransform;
  patientSupportToPatientSupportRotationTransformNode->GetTransformToWorld(patientSupportToRasGeneralTransform);
  vtkNew<vtkTransform> patientSupportToRasTransform;

  vtkNew<vtkGeneralTransform> collimatorToRasGeneralTransform;
  collimatorToGantryTransformNode->GetTransformToWorld(collimatorToRasGeneralTransform);
  vtkNew<vtkTransform> collimatorToRasTransform ;

  vtkNew<vtkGeneralTransform> tableTopToRasGeneralTransform;
  tableTopToTableTopEccentricRotationTransformNode->GetTransformToWorld(tableTopToRasGeneralTransform);
  vtkNew<vtkTransform> tableTopToRasTransform;

  if ( !vtkMRMLTransformNode::IsGeneralTransformLinear(gantryToRasGeneralTransform, gantryToRasTransform)
    || !vtkMRMLTransformNode::IsGeneralTransformLinear(patientSupportToRasGeneralTransform, patientSupportToRasTransform)
    || !vtkMRMLTransformNode::IsGeneralTransformLinear(collimatorToRasGeneralTransform, collimatorToRasTransform)
    || !vtkMRMLTransformNode::IsGeneralTransformLinear(tableTopToRasGeneralTransform, tableTopToRasTransform) )
  {
    statusString = "Non-linear transform detected";
    vtkErrorMacro("CheckForCollisions: " + statusString);
    return statusString;
  }

  // Get states of the treatment machine parts involved
  std::string collimatorState = this->GetStateForPartType(this->GetTreatmentMachinePartTypeAsString(Collimator));
  std::string gantryState = this->GetStateForPartType(this->GetTreatmentMachinePartTypeAsString(Gantry));
  std::string patientSupportState = this->GetStateForPartType(this->GetTreatmentMachinePartTypeAsString(PatientSupport));
  std::string tableTopState = this->GetStateForPartType(this->GetTreatmentMachinePartTypeAsString(TableTop));

  // If number of contacts between pieces of treatment room is greater than 0, the collision between which pieces
  // will be set to the output string and returned by the function.
  if (gantryState == "Active" && tableTopState == "Active" && this->GantryTableTopCollisionDetection->GetInputData(0))
  {
    this->GantryTableTopCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(gantryToRasTransform));
    this->GantryTableTopCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(tableTopToRasTransform));
    this->GantryTableTopCollisionDetection->Update();
    if (this->GantryTableTopCollisionDetection->GetNumberOfContacts() > 0)
    {
      statusString = statusString + "Collision between gantry and table top\n";
    }
  }

  if (gantryState == "Active" && patientSupportState == "Active" && this->GantryTableTopCollisionDetection->GetInputData(0))
  {
    this->GantryPatientSupportCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(gantryToRasTransform));
    this->GantryPatientSupportCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(patientSupportToRasTransform));
    this->GantryPatientSupportCollisionDetection->Update();
    if (this->GantryPatientSupportCollisionDetection->GetNumberOfContacts() > 0)
    {
      statusString = statusString + "Collision between gantry and patient support\n";
    }
  }

  if (collimatorState == "Active" && tableTopState == "Active" && this->CollimatorTableTopCollisionDetection->GetInputData(0))
  {
    this->CollimatorTableTopCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(collimatorToRasTransform));
    this->CollimatorTableTopCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(tableTopToRasTransform));
    this->CollimatorTableTopCollisionDetection->Update();
    if (this->CollimatorTableTopCollisionDetection->GetNumberOfContacts() > 0)
    {
      statusString = statusString + "Collision between collimator and table top\n";
    }
  }

  // Get patient body poly data
  vtkNew<vtkPolyData> patientBodyPolyData;
  if (this->GetPatientBodyPolyData(parameterNode, patientBodyPolyData))
  {
    if (gantryState == "Active")
    {
      this->GantryPatientCollisionDetection->SetInputData(1, patientBodyPolyData);
      this->GantryPatientCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(gantryToRasTransform));
      this->GantryPatientCollisionDetection->Update();
      if (this->GantryPatientCollisionDetection->GetNumberOfContacts() > 0)
      {
        statusString = statusString + "Collision between gantry and patient\n";
      }
    }

    if (collimatorState == "Active")
    {
      this->CollimatorPatientCollisionDetection->SetInputData(1, patientBodyPolyData);
      this->CollimatorPatientCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(collimatorToRasTransform));
      this->CollimatorPatientCollisionDetection->Update();
      if (this->CollimatorPatientCollisionDetection->GetNumberOfContacts() > 0)
      {
        statusString = statusString + "Collision between collimator and patient\n";
      }
    }
  }

  return statusString;
}

//---------------------------------------------------------------------------
const char* vtkSlicerRoomsEyeViewModuleLogic::GetTreatmentMachinePartTypeAsString(TreatmentMachinePartType type)
{
  switch (type)
  {
    case Collimator: return "Collimator";
    case Gantry: return "Gantry";
    case PatientSupport: return "PatientSupport";
    case TableTop: return "TableTop";
    case Body: return "Body";
    case ImagingPanelLeft: return "ImagingPanelLeft";
    case ImagingPanelRight: return "ImagingPanelRight";
    case FlatPanel: return "FlatPanel";
    case ApplicatorHolder: return "ApplicatorHolder";
    case ElectronApplicator: return "ElectronApplicator";
    default:
      // invalid type
      return "";
  }
}

//---------------------------------------------------------------------------
std::string vtkSlicerRoomsEyeViewModuleLogic::GetNameForPartType(std::string partType)
{
  rapidjson::Value& partObject = this->Internal->GetTreatmentMachinePart(partType);
  if (partObject.IsNull())
  {
    // The part may not have been included in the description
    return "";
  }

  rapidjson::Value& name = partObject["Name"];
  if (!name.IsString())
  {
    vtkErrorMacro("GetNameForPartType: Invalid treatment machine part name for part " << partType);
    return "";
  }

  return name.GetString();
}

//---------------------------------------------------------------------------
std::string vtkSlicerRoomsEyeViewModuleLogic::GetFilePathForPartType(std::string partType)
{
  rapidjson::Value& partObject = this->Internal->GetTreatmentMachinePart(partType);
  if (partObject.IsNull())
  {
    // The part may not have been included in the description
    return "";
  }

  rapidjson::Value& filePath = partObject["FilePath"];
  if (!filePath.IsString())
  {
    vtkErrorMacro("GetFilePathForPartType: Invalid treatment machine part file path for part " << partType);
    return "";
  }

  return filePath.GetString();
}

//---------------------------------------------------------------------------
bool vtkSlicerRoomsEyeViewModuleLogic::GetFileToRASTransformMatrixForPartType(std::string partType, vtkMatrix4x4* fileToPartTransformMatrix)
{
  if (!fileToPartTransformMatrix)
  {
    vtkErrorMacro("GetFileToRASTransformMatrixForPartType: Invalid treatment machine file to RAS matrix for part " << partType);
    return false;
  }

  fileToPartTransformMatrix->Identity();

  rapidjson::Value& partObject = this->Internal->GetTreatmentMachinePart(partType);
  if (partObject.IsNull())
  {
    // The part may not have been included in the description
    return false;
  }

  rapidjson::Value& columnsArray = partObject["FileToRASTransformMatrix"];
  if (!columnsArray.IsArray() || columnsArray.Size() != 4)
  {
    vtkErrorMacro("GetFileToRASTransformMatrixForPartType: Invalid treatment machine file to RAS matrix for part " << partType);
    return false;
  }

  for (int i=0; i<columnsArray.Size(); ++i)
  {
    if (!columnsArray[i].IsArray() || columnsArray[i].Size() != 4)
    {
      vtkErrorMacro("GetFileToRASTransformMatrixForPartType: Invalid treatment machine file to RAS matrix for part " << partType
        << " (problem in row " << i << ")");
      return false;
    }
    for (int j=0; j<4; ++j)
    {
      fileToPartTransformMatrix->SetElement(i, j, columnsArray[i][j].GetDouble());
    }
  }

  return true;
}

//---------------------------------------------------------------------------
vtkVector3d vtkSlicerRoomsEyeViewModuleLogic::GetColorForPartType(std::string partType)
{
  rapidjson::Value& partObject = this->Internal->GetTreatmentMachinePart(partType);
  if (partObject.IsNull())
  {
    // The part may not have been included in the description
    return vtkVector3d(255, 255, 255);
  }

  rapidjson::Value& colorArray = partObject["Color"];
  if (!colorArray.IsArray() || colorArray.Size() != 3 || !colorArray[0].IsInt())
  {
    vtkErrorMacro("GetFilePathForPartType: Invalid treatment machine color for part " << partType);
    return vtkVector3d(255, 255, 255);
  }

  return vtkVector3d( (unsigned char)colorArray[0].GetInt(),
                      (unsigned char)colorArray[1].GetInt(),
                      (unsigned char)colorArray[2].GetInt() );
}

//---------------------------------------------------------------------------
std::string vtkSlicerRoomsEyeViewModuleLogic::GetStateForPartType(std::string partType)
{
  rapidjson::Value& partObject = this->Internal->GetTreatmentMachinePart(partType);
  if (partObject.IsNull())
  {
    // The part may not have been included in the description
    return "";
  }

  rapidjson::Value& state = partObject["State"];
  if (!state.IsString())
  {
    vtkErrorMacro("GetStateForPartType: Invalid treatment machine state value type for part " << partType);
    return "";
  }

  std::string stateStr(state.GetString());
  if (state != "Disabled" && state != "Active" && state != "Passive")
  {
    vtkErrorMacro("GetStateForPartType: Invalid treatment machine state for part " << partType
      << ". Valid states are Disabled, Active, or Passive.");
    return "";
  }

  return stateStr;
}
