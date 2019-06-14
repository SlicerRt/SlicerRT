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

==============================================================================*/

// RoomsEyeView includes
#include "vtkSlicerRoomsEyeViewModuleLogic.h"
#include "vtkMRMLRoomsEyeViewNode.h"
#include "vtkSlicerIECTransformLogic.h"

// SlicerRT includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkCollisionDetectionFilter.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLViewNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelDisplayNode.h>

// Slicer includes
#include <vtkSlicerModelsLogic.h>
#include <vtkSlicerSegmentationsModuleLogic.h>

// vtkSegmentationCore includes
#include <vtkSegmentationConverter.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>
#include <vtkAppendPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtksys/SystemTools.hxx>
#include <vtkTransformPolyDataFilter.h>
#include <vtkGeneralTransform.h>
#include <vtkTransformFilter.h>

//----------------------------------------------------------------------------
// Treatment machine component names
const char* vtkSlicerRoomsEyeViewModuleLogic::COLLIMATOR_MODEL_NAME = "Collimator";
const char* vtkSlicerRoomsEyeViewModuleLogic::GANTRY_MODEL_NAME = "Gantry";
const char* vtkSlicerRoomsEyeViewModuleLogic::PATIENTSUPPORT_MODEL_NAME = "PatientSupport";
const char* vtkSlicerRoomsEyeViewModuleLogic::TABLETOP_MODEL_NAME = "TableTop";

const char* vtkSlicerRoomsEyeViewModuleLogic::LINACBODY_MODEL_NAME = "LinacBody";
const char* vtkSlicerRoomsEyeViewModuleLogic::IMAGINGPANELLEFT_MODEL_NAME = "ImagingPanelLeft";
const char* vtkSlicerRoomsEyeViewModuleLogic::IMAGINGPANELRIGHT_MODEL_NAME = "ImagingPanelRight";
const char* vtkSlicerRoomsEyeViewModuleLogic::FLATPANEL_MODEL_NAME = "FlatPanel";

const char* vtkSlicerRoomsEyeViewModuleLogic::APPLICATORHOLDER_MODEL_NAME = "ApplicatorHolder";
const char* vtkSlicerRoomsEyeViewModuleLogic::ELECTRONAPPLICATOR_MODEL_NAME = "ElectronApplicator";

const char* vtkSlicerRoomsEyeViewModuleLogic::ORIENTATION_MARKER_MODEL_NODE_NAME = "RoomsEyeViewOrientationMarker";

// Transform names
//TODO: Add this dynamically to the IEC transform map
static const char* ADDITIONALCOLLIMATORMOUNTEDDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME = "AdditionalCollimatorDevicesToCollimatorTransform";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerRoomsEyeViewModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::vtkSlicerRoomsEyeViewModuleLogic()
  : GantryPatientCollisionDetection(nullptr)
  , GantryTableTopCollisionDetection(nullptr)
  , GantryPatientSupportCollisionDetection(nullptr)
  , CollimatorPatientCollisionDetection(nullptr)
  , CollimatorTableTopCollisionDetection(nullptr)
  , AdditionalModelsTableTopCollisionDetection(nullptr)
  , AdditionalModelsPatientSupportCollisionDetection(nullptr)
{
  this->IECLogic = vtkSlicerIECTransformLogic::New();

  this->GantryPatientCollisionDetection = vtkCollisionDetectionFilter::New();
  this->GantryTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
  this->GantryPatientSupportCollisionDetection = vtkCollisionDetectionFilter::New();
  this->CollimatorPatientCollisionDetection = vtkCollisionDetectionFilter::New();
  this->CollimatorTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
  this->AdditionalModelsTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
  this->AdditionalModelsPatientSupportCollisionDetection = vtkCollisionDetectionFilter::New();
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
  if (this->AdditionalModelsTableTopCollisionDetection)
  {
    this->AdditionalModelsTableTopCollisionDetection->Delete();
    this->AdditionalModelsTableTopCollisionDetection = nullptr;
  }
  if (this->AdditionalModelsPatientSupportCollisionDetection)
  {
    this->AdditionalModelsPatientSupportCollisionDetection->Delete();
    this->AdditionalModelsPatientSupportCollisionDetection = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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

  this->IECLogic->SetMRMLScene(newScene);
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

  // Build IEC hierarchy
  //TODO: Add the REV transform to the IEC transform map and use it for the GetTransform... functions
  this->IECLogic->BuildIECTransformHierarchy();

  // Create transform nodes if they do not exist
  vtkSmartPointer<vtkMRMLLinearTransformNode> additionalCollimatorDevicesToCollimatorTransformNode;
  if (!scene->GetFirstNodeByName(ADDITIONALCOLLIMATORMOUNTEDDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME))
  {
    additionalCollimatorDevicesToCollimatorTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    additionalCollimatorDevicesToCollimatorTransformNode->SetName(ADDITIONALCOLLIMATORMOUNTEDDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME);
    additionalCollimatorDevicesToCollimatorTransformNode->SetHideFromEditors(1);
    std::string singletonTag = std::string("IEC_") + ADDITIONALCOLLIMATORMOUNTEDDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME;
    additionalCollimatorDevicesToCollimatorTransformNode->SetSingletonTag(singletonTag.c_str());
    scene->AddNode(additionalCollimatorDevicesToCollimatorTransformNode);
  }
  else
  {
    additionalCollimatorDevicesToCollimatorTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(ADDITIONALCOLLIMATORMOUNTEDDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME));
  }

  // Get IEC transform nodes that are used below
  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry);
  if (!collimatorToGantryTransformNode)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access collimatorToGantryTransformNode");
    return;
  }

  // Organize transforms into hierarchy
  additionalCollimatorDevicesToCollimatorTransformNode->SetAndObserveTransformNodeID(collimatorToGantryTransformNode->GetID());
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LoadTreatmentMachineModels(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("LoadTreatmentMachineModels: Invalid scene");
    return;
  }
  if (!parameterNode || !parameterNode->GetTreatmentMachineType())
  {
    vtkErrorMacro("LoadTreatmentMachineModels: Invalid parameter node");
    return;
  }

  // Make sure the transform hierarchy is in place
  this->BuildRoomsEyeViewTransformHierarchy();

  std::string moduleShareDirectory = this->GetModuleShareDirectory();
  std::string machineType(parameterNode->GetTreatmentMachineType());
  std::string treatmentMachineModelsDirectory = moduleShareDirectory + "/" + machineType;

  // Create a models logic for convenient loading of components
  vtkNew<vtkSlicerModelsLogic> modelsLogic;
  modelsLogic->SetMRMLScene(scene);

  // Create model hierarchy so that the treatment machine can be shown/hidden easily
  std::string rootModelHierarchyNodeName = machineType + std::string("_Components");
  vtkSmartPointer<vtkMRMLModelHierarchyNode> rootModelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(
    scene->GetSingletonNode(rootModelHierarchyNodeName.c_str(), "vtkMRMLModelHierarchyNode") );
  if (!rootModelHierarchyNode)
  {
    rootModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
    scene->AddNode(rootModelHierarchyNode);
    rootModelHierarchyNode->SetName(rootModelHierarchyNodeName.c_str());
    rootModelHierarchyNode->SetSingletonTag(rootModelHierarchyNodeName.c_str());
  }
  if (!rootModelHierarchyNode->GetDisplayNode())
  {
    vtkSmartPointer<vtkMRMLModelDisplayNode> rootModelHierarchyDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
    scene->AddNode(rootModelHierarchyDisplayNode);
    rootModelHierarchyNode->SetAndObserveDisplayNodeID( rootModelHierarchyDisplayNode->GetID() );
  }

  //
  // Load treatment machine models

  // Collimator - mandatory
  std::string collimatorModelSingletonTag = machineType + "_" + COLLIMATOR_MODEL_NAME;
  vtkMRMLModelNode* collimatorModelNode = vtkMRMLModelNode::SafeDownCast(
    scene->GetSingletonNode(collimatorModelSingletonTag.c_str(), "vtkMRMLModelNode") );
  if (collimatorModelNode && !collimatorModelNode->GetPolyData())
  {
    // Remove node if contains empty polydata (e.g. after closing scene), so that it can be loaded again
    scene->RemoveNode(collimatorModelNode);
    collimatorModelNode = nullptr;
  }
  if (!collimatorModelNode)
  {
    std::string collimatorModelFilePath = treatmentMachineModelsDirectory + "/" + COLLIMATOR_MODEL_NAME + ".stl";
    if (vtksys::SystemTools::FileExists(collimatorModelFilePath))
    {
      collimatorModelNode = modelsLogic->AddModel(collimatorModelFilePath.c_str());
    }
    if (collimatorModelNode)
    {
      collimatorModelNode->SetSingletonTag(collimatorModelSingletonTag.c_str());
      vtkNew<vtkMRMLModelHierarchyNode> collimatorModelHierarchyNode;
      scene->AddNode(collimatorModelHierarchyNode);
      collimatorModelHierarchyNode->SetModelNodeID(collimatorModelNode->GetID());
      collimatorModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
      collimatorModelHierarchyNode->HideFromEditorsOn();
    }
    else
    {
      vtkErrorMacro("LoadTreatmentMachineModels: Failed to load collimator model");
    }
  }

  // Gantry - mandatory
  std::string gantryModelSingletonTag = machineType + "_" + GANTRY_MODEL_NAME;
  vtkMRMLModelNode* gantryModelNode = vtkMRMLModelNode::SafeDownCast(
    scene->GetSingletonNode(gantryModelSingletonTag.c_str(), "vtkMRMLModelNode") );
  if (gantryModelNode && !gantryModelNode->GetPolyData())
  {
    // Remove node if contains empty polydata (e.g. after closing scene), so that it can be loaded again
    scene->RemoveNode(gantryModelNode);
    gantryModelNode = nullptr;
  }
  if (!gantryModelNode)
  {
    std::string gantryModelFilePath = treatmentMachineModelsDirectory + "/" + GANTRY_MODEL_NAME + ".stl";
    if (vtksys::SystemTools::FileExists(gantryModelFilePath))
    {
      gantryModelNode = modelsLogic->AddModel(gantryModelFilePath.c_str());
    }
    if (gantryModelNode)
    {
      gantryModelNode->SetSingletonTag(gantryModelSingletonTag.c_str());
      vtkNew<vtkMRMLModelHierarchyNode> gantryModelHierarchyNode;
      scene->AddNode(gantryModelHierarchyNode);
      gantryModelHierarchyNode->SetModelNodeID(gantryModelNode->GetID());
      gantryModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
      gantryModelHierarchyNode->HideFromEditorsOn();
    }
    else
    {
      vtkErrorMacro("LoadTreatmentMachineModels: Failed to load gantry model");
    }
  }

  // Patient support - mandatory
  std::string patientSupportModelSingletonTag = machineType + "_" + PATIENTSUPPORT_MODEL_NAME;
  vtkMRMLModelNode* patientSupportModelNode = vtkMRMLModelNode::SafeDownCast(
    scene->GetSingletonNode(patientSupportModelSingletonTag.c_str(), "vtkMRMLModelNode") );
  if (patientSupportModelNode && !patientSupportModelNode->GetPolyData())
  {
    // Remove node if contains empty polydata (e.g. after closing scene), so that it can be loaded again
    scene->RemoveNode(patientSupportModelNode);
    patientSupportModelNode = nullptr;
  }
  if (!patientSupportModelNode)
  {
    std::string patientSupportModelFilePath = treatmentMachineModelsDirectory + "/" + PATIENTSUPPORT_MODEL_NAME + ".stl";
    if (vtksys::SystemTools::FileExists(patientSupportModelFilePath))
    {
      patientSupportModelNode = modelsLogic->AddModel(patientSupportModelFilePath.c_str());
    }
    if (patientSupportModelNode)
    {
      patientSupportModelNode->SetSingletonTag(patientSupportModelSingletonTag.c_str());
      vtkNew<vtkMRMLModelHierarchyNode> patientSupportModelHierarchyNode;
      scene->AddNode(patientSupportModelHierarchyNode);
      patientSupportModelHierarchyNode->SetModelNodeID(patientSupportModelNode->GetID());
      patientSupportModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
      patientSupportModelHierarchyNode->HideFromEditorsOn();
    }
    else
    {
      vtkErrorMacro("LoadTreatmentMachineModels: Failed to load patient support model");
    }
  }

  // Table top - mandatory
  std::string tableTopModelSingletonTag = machineType + "_" + TABLETOP_MODEL_NAME;
  vtkMRMLModelNode* tableTopModelNode = vtkMRMLModelNode::SafeDownCast(
    scene->GetSingletonNode(tableTopModelSingletonTag.c_str(), "vtkMRMLModelNode") );
  if (tableTopModelNode && !tableTopModelNode->GetPolyData())
  {
    // Remove node if contains empty polydata (e.g. after closing scene), so that it can be loaded again
    scene->RemoveNode(tableTopModelNode);
    tableTopModelNode = nullptr;
  }
  if (!tableTopModelNode)
  {
    std::string tableTopModelFilePath = treatmentMachineModelsDirectory + "/" + TABLETOP_MODEL_NAME + ".stl";
    if (vtksys::SystemTools::FileExists(tableTopModelFilePath))
    {
      tableTopModelNode = modelsLogic->AddModel(tableTopModelFilePath.c_str());
    }
    if (tableTopModelNode)
    {
      tableTopModelNode->SetSingletonTag(tableTopModelSingletonTag.c_str());
      vtkNew<vtkMRMLModelHierarchyNode> tableTopModelHierarchyNode;
      scene->AddNode(tableTopModelHierarchyNode);
      tableTopModelHierarchyNode->SetModelNodeID(tableTopModelNode->GetID());
      tableTopModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
      tableTopModelHierarchyNode->HideFromEditorsOn();
    }
    else
    {
      vtkErrorMacro("LoadTreatmentMachineModels: Failed to load table top model");
    }
  }

  // Linac body - optional
  std::string linacBodyModelSingletonTag = machineType + "_" + LINACBODY_MODEL_NAME;
  vtkMRMLModelNode* linacBodyModelNode = vtkMRMLModelNode::SafeDownCast(
    scene->GetSingletonNode(linacBodyModelSingletonTag.c_str(), "vtkMRMLModelNode") );
  if (linacBodyModelNode && !linacBodyModelNode->GetPolyData())
  {
    // Remove node if contains empty polydata (e.g. after closing scene), so that it can be loaded again
    scene->RemoveNode(linacBodyModelNode);
    linacBodyModelNode = nullptr;
  }
  if (!linacBodyModelNode)
  {
    std::string linacBodyModelFilePath = treatmentMachineModelsDirectory + "/" + LINACBODY_MODEL_NAME + ".stl";
    if (vtksys::SystemTools::FileExists(linacBodyModelFilePath))
    {
      linacBodyModelNode = modelsLogic->AddModel(linacBodyModelFilePath.c_str());
    }
    if (linacBodyModelNode)
    {
      linacBodyModelNode->SetSingletonTag(linacBodyModelSingletonTag.c_str());
      vtkNew<vtkMRMLModelHierarchyNode> linacBodyModelHierarchyNode;
      scene->AddNode(linacBodyModelHierarchyNode);
      linacBodyModelHierarchyNode->SetModelNodeID(linacBodyModelNode->GetID());
      linacBodyModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
      linacBodyModelHierarchyNode->HideFromEditorsOn();
    }
  }

  // Imaging panel left - optional
  std::string imagingPanelLeftModelSingletonTag = machineType + "_" + IMAGINGPANELLEFT_MODEL_NAME;
  vtkMRMLModelNode* imagingPanelLeftModelNode = vtkMRMLModelNode::SafeDownCast(
    scene->GetSingletonNode(imagingPanelLeftModelSingletonTag.c_str(), "vtkMRMLModelNode") );
  if (imagingPanelLeftModelNode && !imagingPanelLeftModelNode->GetPolyData())
  {
    // Remove node if contains empty polydata (e.g. after closing scene), so that it can be loaded again
    scene->RemoveNode(imagingPanelLeftModelNode);
    imagingPanelLeftModelNode = nullptr;
  }
  if (!imagingPanelLeftModelNode)
  {
    std::string imagingPanelLeftModelFilePath = treatmentMachineModelsDirectory + "/" + IMAGINGPANELLEFT_MODEL_NAME + ".stl";
    if (vtksys::SystemTools::FileExists(imagingPanelLeftModelFilePath))
    {
      imagingPanelLeftModelNode = modelsLogic->AddModel(imagingPanelLeftModelFilePath.c_str());
    }
    if (imagingPanelLeftModelNode)
    {
      imagingPanelLeftModelNode->SetSingletonTag(imagingPanelLeftModelSingletonTag.c_str());
      vtkNew<vtkMRMLModelHierarchyNode> imagingPanelLeftModelHierarchyNode;
      scene->AddNode(imagingPanelLeftModelHierarchyNode);
      imagingPanelLeftModelHierarchyNode->SetModelNodeID(imagingPanelLeftModelNode->GetID());
      imagingPanelLeftModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
      imagingPanelLeftModelHierarchyNode->HideFromEditorsOn();
    }
  }

  // Imaging panel right - optional
  std::string imagingPanelRightModelSingletonTag = machineType + "_" + IMAGINGPANELRIGHT_MODEL_NAME;
  vtkMRMLModelNode* imagingPanelRightModelNode = vtkMRMLModelNode::SafeDownCast(
    scene->GetSingletonNode(imagingPanelRightModelSingletonTag.c_str(), "vtkMRMLModelNode") );
  if (imagingPanelRightModelNode && !imagingPanelRightModelNode->GetPolyData())
  {
    // Remove node if contains empty polydata (e.g. after closing scene), so that it can be loaded again
    scene->RemoveNode(imagingPanelRightModelNode);
    imagingPanelRightModelNode = nullptr;
  }
  if (!imagingPanelRightModelNode)
  {
    std::string imagingPanelRightModelFilePath = treatmentMachineModelsDirectory + "/" + IMAGINGPANELRIGHT_MODEL_NAME + ".stl";
    if (vtksys::SystemTools::FileExists(imagingPanelRightModelFilePath))
    {
      imagingPanelRightModelNode = modelsLogic->AddModel(imagingPanelRightModelFilePath.c_str());
    }
    if (imagingPanelRightModelNode)
    {
      imagingPanelRightModelNode->SetSingletonTag(imagingPanelRightModelSingletonTag.c_str());
      vtkNew<vtkMRMLModelHierarchyNode> imagingPanelRightModelHierarchyNode;
      scene->AddNode(imagingPanelRightModelHierarchyNode);
      imagingPanelRightModelHierarchyNode->SetModelNodeID(imagingPanelRightModelNode->GetID());
      imagingPanelRightModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
      imagingPanelRightModelHierarchyNode->HideFromEditorsOn();
    }
  }

  // Flat panel - optional
  std::string flatPanelModelSingletonTag = machineType + "_" + FLATPANEL_MODEL_NAME;
  vtkMRMLModelNode* flatPanelModelNode = vtkMRMLModelNode::SafeDownCast(
    scene->GetSingletonNode(flatPanelModelSingletonTag.c_str(), "vtkMRMLModelNode") );
  if (flatPanelModelNode && !flatPanelModelNode->GetPolyData())
  {
    // Remove node if contains empty polydata (e.g. after closing scene), so that it can be loaded again
    scene->RemoveNode(flatPanelModelNode);
    flatPanelModelNode = nullptr;
  }
  if (!flatPanelModelNode)
  {
    std::string flatPanelModelFilePath = treatmentMachineModelsDirectory + "/" + FLATPANEL_MODEL_NAME + ".stl";
    if (vtksys::SystemTools::FileExists(flatPanelModelFilePath))
    {
      flatPanelModelNode = modelsLogic->AddModel(flatPanelModelFilePath.c_str());
    }
    if (flatPanelModelNode)
    {
      flatPanelModelNode->SetSingletonTag(flatPanelModelSingletonTag.c_str());
      vtkNew<vtkMRMLModelHierarchyNode> flatPanelModelHierarchyNode;
      scene->AddNode(flatPanelModelHierarchyNode);
      flatPanelModelHierarchyNode->SetModelNodeID(flatPanelModelNode->GetID());
      flatPanelModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
      flatPanelModelHierarchyNode->HideFromEditorsOn();
    }
  }

  if ( !collimatorModelNode || !collimatorModelNode->GetPolyData()
    || !gantryModelNode || !gantryModelNode->GetPolyData()
    || !patientSupportModelNode || !patientSupportModelNode->GetPolyData()
    || !tableTopModelNode || !tableTopModelNode->GetPolyData() )
  {
    vtkErrorMacro("LoadTreatmentMachineModels: Failed to load every mandatory treatment machine component");
    return;
  }

  // Setup treatment machine model display and transforms
  this->SetupTreatmentMachineModels();
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::SetupTreatmentMachineModels()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Invalid scene");
    return;
  }

  //TODO: Store treatment machine component color and other properties in JSON

  // Display all pieces of the treatment room and sets each piece a color to provide realistic representation

  // Gantry - mandatory
  vtkMRMLModelNode* gantryModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(GANTRY_MODEL_NAME) );
  if (!gantryModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access gantry model");
    return;
  }
  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Gantry, vtkSlicerIECTransformLogic::FixedReference);
  gantryModel->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());
  gantryModel->CreateDefaultDisplayNodes();
  gantryModel->GetDisplayNode()->SetColor(0.95, 0.95, 0.95);

  // Collimator - mandatory
  vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_MODEL_NAME) );
  if (!collimatorModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access collimator model");
    return;
  }
  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry);
  collimatorModel->SetAndObserveTransformNodeID(collimatorToGantryTransformNode->GetID());
  collimatorModel->CreateDefaultDisplayNodes();
  collimatorModel->GetDisplayNode()->SetColor(0.7, 0.7, 0.95);

  // Patient support - mandatory
  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME) );
  if (!patientSupportModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access patient support model");
    return;
  }
  vtkMRMLLinearTransformNode* patientSupportToPatientSupportRotationTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupport, vtkSlicerIECTransformLogic::PatientSupportRotation);
  patientSupportModel->SetAndObserveTransformNodeID(patientSupportToPatientSupportRotationTransformNode->GetID());
  patientSupportModel->CreateDefaultDisplayNodes();
  patientSupportModel->GetDisplayNode()->SetColor(0.85, 0.85, 0.85);

  // Table top - mandatory
  vtkMRMLModelNode* tableTopModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOP_MODEL_NAME) );
  if (!tableTopModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access table top model");
    return;
  }
  vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTop, vtkSlicerIECTransformLogic::TableTopEccentricRotation);
  tableTopModel->SetAndObserveTransformNodeID(tableTopToTableTopEccentricRotationTransformNode->GetID());
  tableTopModel->CreateDefaultDisplayNodes();
  tableTopModel->GetDisplayNode()->SetColor(0, 0, 0);

  // Linac body - optional
  vtkMRMLModelNode* linacBodyModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(LINACBODY_MODEL_NAME) );
  if (linacBodyModel)
  {
    vtkMRMLLinearTransformNode* fixedReferenceToRasTransformNode =
      this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::FixedReference, vtkSlicerIECTransformLogic::RAS);
    linacBodyModel->SetAndObserveTransformNodeID(fixedReferenceToRasTransformNode->GetID());
    linacBodyModel->CreateDefaultDisplayNodes();
    linacBodyModel->GetDisplayNode()->SetColor(0.9, 0.9, 0.9);
  }

  // Imaging panel left - optional
  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME) );
  if (leftImagingPanelModel)
  {
    vtkMRMLLinearTransformNode* leftImagingPanelToGantryTransformNode =
      this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::LeftImagingPanel, vtkSlicerIECTransformLogic::Gantry);
    leftImagingPanelModel->SetAndObserveTransformNodeID(leftImagingPanelToGantryTransformNode->GetID());
    leftImagingPanelModel->CreateDefaultDisplayNodes();
    leftImagingPanelModel->GetDisplayNode()->SetColor(0.95, 0.95, 0.95);
  }

  // Imaging panel right - optional
  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME) );
  if (rightImagingPanelModel)
  {
    vtkMRMLLinearTransformNode* rightImagingPanelToGantryTransformNode =
      this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::RightImagingPanel, vtkSlicerIECTransformLogic::Gantry);
    rightImagingPanelModel->SetAndObserveTransformNodeID(rightImagingPanelToGantryTransformNode->GetID());
    rightImagingPanelModel->CreateDefaultDisplayNodes();
    rightImagingPanelModel->GetDisplayNode()->SetColor(0.95, 0.95, 0.95);
  }

  // Flat panel - optional
  vtkMRMLModelNode* flatPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(FLATPANEL_MODEL_NAME) );
  if (flatPanelModel)
  {
    vtkMRMLLinearTransformNode* flatPanelToGantryTransformNode =
      this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::FlatPanel, vtkSlicerIECTransformLogic::Gantry);
    flatPanelModel->SetAndObserveTransformNodeID(flatPanelToGantryTransformNode->GetID());
    flatPanelModel->CreateDefaultDisplayNodes();
    flatPanelModel->GetDisplayNode()->SetColor(0.95, 0.95, 0.95);
  }

  //
  // Set up collision detection between components
  this->GantryTableTopCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());

  this->GantryPatientSupportCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryPatientSupportCollisionDetection->SetInput(1, patientSupportModel->GetPolyData());

  this->CollimatorTableTopCollisionDetection->SetInput(0, collimatorModel->GetPolyData());
  this->CollimatorTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());

  //TODO: Whole patient (segmentation, CT) will need to be transformed when the table top is transformed
  //vtkMRMLLinearTransformNode* patientModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
  //  this->GetMRMLScene()->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  //patientModel->SetAndObserveTransformNodeID(patientModelTransforms->GetID());

  // Patient model is set when calculating collisions, as it can be changed dynamically
  this->GantryPatientCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->CollimatorPatientCollisionDetection->SetInput(0, collimatorModel->GetPolyData());
  // Set identity transform for patient (parent transform is taken into account when getting poly data from segmentation)
  vtkNew<vtkTransform> identityTransform;
  identityTransform->Identity();
  this->GantryPatientCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(identityTransform));
  this->CollimatorPatientCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(identityTransform));
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LoadBasicCollimatorMountedDevices()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("LoadBasicCollimatorMountedDevices: Invalid scene");
    return;
  }

  std::string moduleShareDirectory = this->GetModuleShareDirectory();
  std::string additionalDevicesDirectory = moduleShareDirectory + "/" + "AdditionalTreatmentModels";

  // Create a models logic for convenient loading of components
  vtkNew<vtkSlicerModelsLogic> modelsLogic;
  modelsLogic->SetMRMLScene(this->GetMRMLScene());

  // Create model hierarchy so that the treatment machine can be shown/hidden easily
  vtkNew<vtkMRMLModelHierarchyNode> rootModelHierarchyNode;
  this->GetMRMLScene()->AddNode(rootModelHierarchyNode);
  rootModelHierarchyNode->SetName("Additional treatment machine devices");

  vtkNew<vtkMRMLModelDisplayNode> rootModelHierarchyDisplayNode;
  this->GetMRMLScene()->AddNode(rootModelHierarchyDisplayNode);
  rootModelHierarchyNode->SetAndObserveDisplayNodeID( rootModelHierarchyDisplayNode->GetID() );

  //
  // Load basic additional device models
  std::string applicatorHolderModelFilePath = additionalDevicesDirectory + "/" + APPLICATORHOLDER_MODEL_NAME + ".stl";
  vtkMRMLModelNode* applicatorHolderModelNode = modelsLogic->AddModel(applicatorHolderModelFilePath.c_str());
  vtkNew<vtkMRMLModelHierarchyNode> applicatorHolderModelHierarchyNode;
  this->GetMRMLScene()->AddNode(applicatorHolderModelHierarchyNode);
  applicatorHolderModelHierarchyNode->SetModelNodeID(applicatorHolderModelNode->GetID());
  applicatorHolderModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
  applicatorHolderModelHierarchyNode->HideFromEditorsOn();
  applicatorHolderModelHierarchyNode->SetSingletonTag("BasicCollimatorMountedDevices");

  std::string electronApplicatorModelFilePath = additionalDevicesDirectory + "/" + ELECTRONAPPLICATOR_MODEL_NAME + ".stl";
  vtkMRMLModelNode* electronApplicatorModelNode = modelsLogic->AddModel(electronApplicatorModelFilePath.c_str());
  vtkNew<vtkMRMLModelHierarchyNode> electronApplicatorModelHierarchyNode;
  this->GetMRMLScene()->AddNode(electronApplicatorModelHierarchyNode);
  electronApplicatorModelHierarchyNode->SetModelNodeID(electronApplicatorModelNode->GetID());
  electronApplicatorModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
  electronApplicatorModelHierarchyNode->HideFromEditorsOn();
  electronApplicatorModelHierarchyNode->SetSingletonTag("BasicCollimatorMountedDevices");

  // Setup basic additional device model display and transforms
  this->SetupBasicCollimatorMountedDeviceModels();
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::SetupBasicCollimatorMountedDeviceModels()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("SetupBasicCollimatorMountedDeviceModels: Invalid scene");
    return;
  }

  //TODO: Separate to a function and call it from LoadBasicCollimatorMountedDevices
  vtkMRMLModelNode* applicatorHolderModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(APPLICATORHOLDER_MODEL_NAME));
  if (!applicatorHolderModel)
  {
    vtkErrorMacro("SetupBasicCollimatorMountedDeviceModels: Unable to access applicator holder model");
    return;
  }
  vtkMRMLLinearTransformNode* applicatorHolderModelTransformNode = vtkMRMLLinearTransformNode::SafeDownCast( //TODO:
    this->GetMRMLScene()->GetFirstNodeByName(ADDITIONALCOLLIMATORMOUNTEDDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME));
  applicatorHolderModel->SetAndObserveTransformNodeID(applicatorHolderModelTransformNode->GetID());
  applicatorHolderModel->CreateDefaultDisplayNodes();
  applicatorHolderModel->GetDisplayNode()->VisibilityOff();

  vtkMRMLModelNode* electronApplicatorModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(ELECTRONAPPLICATOR_MODEL_NAME));
  if (!electronApplicatorModel)
  {
    vtkErrorMacro("SetupBasicCollimatorMountedDeviceModels: Unable to access electron applicator model");
    return;
  }
  vtkMRMLLinearTransformNode* electronApplicatorModelTransformNode = vtkMRMLLinearTransformNode::SafeDownCast( //TODO:
    this->GetMRMLScene()->GetFirstNodeByName(ADDITIONALCOLLIMATORMOUNTEDDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME));
  electronApplicatorModel->SetAndObserveTransformNodeID(electronApplicatorModelTransformNode->GetID());
  electronApplicatorModel->CreateDefaultDisplayNodes();
  electronApplicatorModel->GetDisplayNode()->VisibilityOff();

  //TODO: Additional device collision detection is broken, because the transforms of the additional devices are not
  // taken into account, so the appended model will contain randomly placed devices
  //TODO: Add additional models to patient collision detection
  //vtkSmartPointer<vtkAppendPolyData> additionalDeviceAppending = vtkSmartPointer<vtkAppendPolyData>::New();
  //vtkPolyData* inputs[] = { applicatorHolderModel->GetPolyData(), electronApplicatorModel->GetPolyData() };
  //vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();
  //vtkSmartPointer<vtkMRMLModelNode> outputModel = vtkSmartPointer<vtkMRMLModelNode>::New();
  //additionalDeviceAppending->ExecuteAppend(output, inputs, 2);
  //this->GetMRMLScene()->AddNode(outputModel);
  //outputModel->SetAndObservePolyData(output);

  //this->AdditionalModelsTableTopCollisionDetection->SetInput(0, outputModel->GetPolyData());
  //this->AdditionalModelsTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());
  //this->AdditionalModelsTableTopCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  //this->AdditionalModelsTableTopCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  //this->AdditionalModelsTableTopCollisionDetection->Update();

  //this->AdditionalModelsPatientSupportCollisionDetection->SetInput(0, outputModel->GetPolyData());
  //this->AdditionalModelsPatientSupportCollisionDetection->SetInput(1, patientSupportModel->GetPolyData());
  //this->AdditionalModelsPatientSupportCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  //this->AdditionalModelsPatientSupportCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  //this->AdditionalModelsPatientSupportCollisionDetection->Update();
}

//-----------------------------------------------------------------------------
vtkMRMLModelNode* vtkSlicerRoomsEyeViewModuleLogic::UpdateTreatmentOrientationMarker()
{
  vtkNew<vtkAppendPolyData> appendFilter;

  vtkMRMLModelNode* gantryModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(GANTRY_MODEL_NAME));
  vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_MODEL_NAME));
  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  vtkMRMLModelNode* tableTopModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(TABLETOP_MODEL_NAME));
  if ( !gantryModel->GetPolyData() || !collimatorModel->GetPolyData() || !patientSupportModel->GetPolyData() || !tableTopModel->GetPolyData() )
  {
    // Orientation marker cannot be assembled if poly data is missing from the mandatory model nodes.
    // This is possible and can be completely valid, for example after closing the scene (because the model nodes are singletons)
    return nullptr;
  }

  //
  // Mandatory models

  // Gantry
  vtkNew<vtkPolyData> gantryModelPolyData;
  gantryModelPolyData->DeepCopy(gantryModel->GetPolyData());

  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Gantry, vtkSlicerIECTransformLogic::FixedReference);
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
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry);
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
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupport, vtkSlicerIECTransformLogic::PatientSupportRotation);
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
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTop, vtkSlicerIECTransformLogic::TableTopEccentricRotation);
  vtkNew<vtkTransformFilter> tableTopTransformFilter;
  tableTopTransformFilter->SetInputData(tableTopModelPolyData);
  vtkNew<vtkGeneralTransform> tableTopModelTransform;
  tableTopToTableTopEccentricRotationTransformNode->GetTransformFromWorld(tableTopModelTransform);
  tableTopModelTransform->Inverse();
  tableTopTransformFilter->SetTransform(tableTopModelTransform);
  tableTopTransformFilter->Update();

  appendFilter->AddInputData(vtkPolyData::SafeDownCast(tableTopTransformFilter->GetOutput()));

  // Optional models
  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME));
  if (leftImagingPanelModel)
  {
    vtkNew<vtkPolyData> leftImagingPanelModelPolyData;
    leftImagingPanelModelPolyData->DeepCopy(leftImagingPanelModel->GetPolyData());

    vtkMRMLLinearTransformNode* leftImagingPanelToGantryTransformNode =
      this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::LeftImagingPanel, vtkSlicerIECTransformLogic::Gantry);
    vtkNew<vtkTransformFilter> leftImagingPanelTransformFilter;
    leftImagingPanelTransformFilter->SetInputData(leftImagingPanelModelPolyData);
    vtkNew<vtkGeneralTransform> leftImagingPanelToGantryTransform;
    leftImagingPanelToGantryTransformNode->GetTransformFromWorld(leftImagingPanelToGantryTransform);
    leftImagingPanelToGantryTransform->Inverse();
    leftImagingPanelTransformFilter->SetTransform(leftImagingPanelToGantryTransform);
    leftImagingPanelTransformFilter->Update();

    appendFilter->AddInputData(vtkPolyData::SafeDownCast(leftImagingPanelTransformFilter->GetOutput()));
  }
  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME));
  if (rightImagingPanelModel)
  {
    vtkNew<vtkPolyData> rightImagingPanelModelPolyData;
    rightImagingPanelModelPolyData->DeepCopy(rightImagingPanelModel->GetPolyData());

    vtkMRMLLinearTransformNode* rightImagingPanelToGantryTransformNode =
      this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::RightImagingPanel, vtkSlicerIECTransformLogic::Gantry);
    vtkNew<vtkTransformFilter> rightImagingPanelTransformFilter;
    rightImagingPanelTransformFilter->SetInputData(rightImagingPanelModelPolyData);
    vtkNew<vtkGeneralTransform> rightImagingPanelToGantryTransform;
    rightImagingPanelToGantryTransformNode->GetTransformFromWorld(rightImagingPanelToGantryTransform);
    rightImagingPanelToGantryTransform->Inverse();
    rightImagingPanelTransformFilter->SetTransform(rightImagingPanelToGantryTransform);
    rightImagingPanelTransformFilter->Update();

    appendFilter->AddInputData(vtkPolyData::SafeDownCast(rightImagingPanelTransformFilter->GetOutput()));
  }
  vtkMRMLModelNode* flatPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(FLATPANEL_MODEL_NAME));
  if (flatPanelModel)
  {
    vtkNew<vtkPolyData> flatPanelModelPolyData;
    flatPanelModelPolyData->DeepCopy(flatPanelModel->GetPolyData());

    vtkMRMLLinearTransformNode* flatPanelToGantryTransformNode =
      this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::FlatPanel, vtkSlicerIECTransformLogic::Gantry);
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
void vtkSlicerRoomsEyeViewModuleLogic::UpdateCollimatorToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateFixedReferenceIsocenterToCollimatorRotatedTransform: Invalid parameter set node");
    return;
  }

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry);

  vtkNew<vtkTransform> collimatorToGantryTransform;
  collimatorToGantryTransform->RotateZ(parameterNode->GetCollimatorRotationAngle());
  collimatorToGantryTransformNode->SetAndObserveTransformToParent(collimatorToGantryTransform);
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateGantryToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateGantryToFixedReferenceTransform: Invalid parameter set node");
    return;
  }

  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Gantry, vtkSlicerIECTransformLogic::FixedReference);

  vtkNew<vtkTransform> gantryToFixedReferenceTransform;
  gantryToFixedReferenceTransform->RotateY(parameterNode->GetGantryRotationAngle());
  gantryToFixedReferenceTransformNode->SetAndObserveTransformToParent(gantryToFixedReferenceTransform);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateLeftImagingPanelToGantryTransform: Invalid parameter set node");
    return;
  }
  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME));
  if (!leftImagingPanelModel)
  {
    vtkDebugMacro("UpdateLeftImagingPanelToGantryTransform: Optional imaging panel left model not found");
    return;
  }

  // Translation to origin for in-place rotation
  vtkPolyData* leftImagingPanelModelPolyData = leftImagingPanelModel->GetPolyData();

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
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::LeftImagingPanel, vtkSlicerIECTransformLogic::Gantry);
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
  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME));
  if (!rightImagingPanelModel)
  {
    vtkDebugMacro("UpdateRightImagingPanelToGantryTransform: Optional imaging panel right model not found");
    return;
  }
  vtkPolyData* rightImagingPanelModelPolyData = rightImagingPanelModel->GetPolyData();
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
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::RightImagingPanel, vtkSlicerIECTransformLogic::Gantry);
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

  vtkMRMLLinearTransformNode* patientSupportRotationToFixedReferenceTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupportRotation, vtkSlicerIECTransformLogic::FixedReference);

  double rotationAngle = parameterNode->GetPatientSupportRotationAngle();
  vtkNew<vtkTransform> patientSupportToRotatedPatientSupportTransform;
  patientSupportToRotatedPatientSupportTransform->RotateZ(rotationAngle);
  patientSupportRotationToFixedReferenceTransformNode->SetAndObserveTransformToParent(patientSupportToRotatedPatientSupportTransform);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportToPatientSupportRotationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdatePatientSupportToPatientSupportRotationTransform: Invalid parameter set node");
    return;
  }

  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  if (!patientSupportModel)
  {
    vtkErrorMacro("UpdatePatientSupportToPatientSupportRotationTransform: Invalid MRML model node");
  }
  vtkPolyData* patientSupportModelPolyData = patientSupportModel->GetPolyData();
  double patientSupportModelBounds[6] = { 0, 0, 0, 0, 0, 0 };
  patientSupportModelPolyData->GetBounds(patientSupportModelBounds);

  // Translation to origin for in-place vertical scaling
  vtkNew<vtkTransform> patientSupportRotationToRasTransform;
  double patientSupportTranslationToOrigin[3] = { 0, 0, (-1.0) * patientSupportModelBounds[4]}; //TODO: Subtract [1]?
  patientSupportRotationToRasTransform->Translate(patientSupportTranslationToOrigin);

  // Vertical scaling
  double tableTopDisplacement = parameterNode->GetVerticalTableTopDisplacement();
  double tableTopDisplacementScaling = 1.0;
  char* treatmentMachineType = parameterNode->GetTreatmentMachineType();
  if (treatmentMachineType && !strcmp(treatmentMachineType, "VarianTrueBeamSTx"))
  {
    tableTopDisplacementScaling = 0.525;
  }
  else if (treatmentMachineType && !strcmp(treatmentMachineType, "SiemensArtiste"))
  {
    tableTopDisplacementScaling = 0.095;
  }
  vtkNew<vtkTransform> rasToScaledRasTransform;
  rasToScaledRasTransform->Scale(1, 1,
    ( ( fabs(patientSupportModelBounds[5]) + tableTopDisplacement*tableTopDisplacementScaling)
      / fabs(patientSupportModelBounds[5]) ) ); //TODO: Subtract [2]?

  // Translation back from origin after in-place scaling
  vtkNew<vtkTransform> scaledRasToFixedReferenceTransform;
  double patientSupportTranslationFromOrigin[3] = { 0, 0, patientSupportModelBounds[4] }; //TODO: Subtract [1]?
  scaledRasToFixedReferenceTransform->Translate(patientSupportTranslationFromOrigin);

  // Assemble transform and update node
  vtkMRMLLinearTransformNode* patientSupportToPatientSupportRotationTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupport, vtkSlicerIECTransformLogic::PatientSupportRotation);
  vtkNew<vtkTransform> patientSupportToFixedReferenceTransform;
  patientSupportToFixedReferenceTransform->PostMultiply();
  patientSupportToFixedReferenceTransform->Concatenate(patientSupportRotationToRasTransform);
  patientSupportToFixedReferenceTransform->Concatenate(rasToScaledRasTransform);
  patientSupportToFixedReferenceTransform->Concatenate(scaledRasToFixedReferenceTransform);
  patientSupportToPatientSupportRotationTransformNode->SetAndObserveTransformToParent(patientSupportToFixedReferenceTransform);
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
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTop, vtkSlicerIECTransformLogic::TableTopEccentricRotation);
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopToTableTopEccentricRotationTransformNode->GetTransformToParent() );

  double translationArray[3] =
    { parameterNode->GetLateralTableTopDisplacement(), parameterNode->GetLongitudinalTableTopDisplacement(), parameterNode->GetVerticalTableTopDisplacement() };

  vtkNew<vtkMatrix4x4> tableTopEccentricRotationToPatientSupportMatrix;
  tableTopEccentricRotationToPatientSupportMatrix->SetElement(0,3, translationArray[0]);
  tableTopEccentricRotationToPatientSupportMatrix->SetElement(1,3, translationArray[1]);
  tableTopEccentricRotationToPatientSupportMatrix->SetElement(2,3, translationArray[2]);
  tableTopEccentricRotationToPatientSupportTransform->SetMatrix(tableTopEccentricRotationToPatientSupportMatrix);
  tableTopEccentricRotationToPatientSupportTransform->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateAdditionalCollimatorDevicesToCollimatorTransforms(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateAdditionalCollimatorDeviceToCollimatorTransforms: Invalid parameter set node");
    return;
  }

  vtkMRMLLinearTransformNode* additionalCollimatorDeviceToCollimatorTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(ADDITIONALCOLLIMATORMOUNTEDDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME) );
  vtkTransform* additionalCollimatorDeviceToCollimatorTransform = vtkTransform::SafeDownCast(
    additionalCollimatorDeviceToCollimatorTransformNode->GetTransformToParent());

  double translationArray[3] = { parameterNode->GetAdditionalModelLateralDisplacement(), parameterNode->GetAdditionalModelLongitudinalDisplacement(),
    parameterNode->GetAdditionalModelVerticalDisplacement() };

  vtkNew<vtkMatrix4x4> additionalCollimatorDeviceToCollimatorMatrix;
  additionalCollimatorDeviceToCollimatorMatrix->SetElement(0,3, translationArray[0]);
  additionalCollimatorDeviceToCollimatorMatrix->SetElement(1,3, translationArray[1]);
  additionalCollimatorDeviceToCollimatorMatrix->SetElement(2,3, translationArray[2]);
  additionalCollimatorDeviceToCollimatorTransform->SetMatrix(additionalCollimatorDeviceToCollimatorMatrix);
  additionalCollimatorDeviceToCollimatorTransform->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateAdditionalDevicesVisibility(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateAdditionalDevicesVisibility: Invalid parameter set node");
  }
  vtkMRMLModelNode* applicatorHolderModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(APPLICATORHOLDER_MODEL_NAME));
  if (!applicatorHolderModel)
  {
    vtkErrorMacro("UpdateAdditionalDevicesVisibility: Invalid applicator holder model node");
    return;
  }
  vtkMRMLModelNode* electronApplicatorModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(ELECTRONAPPLICATOR_MODEL_NAME));
  if (!electronApplicatorModel)
  {
    vtkErrorMacro("UpdateAdditionalDevicesVisibility: Invalid electron applicator model node");
    return;
  }

  if (parameterNode->GetElectronApplicatorVisibility())
  {
    electronApplicatorModel->GetDisplayNode()->VisibilityOn();
  }
  else
  {
    electronApplicatorModel->GetDisplayNode()->VisibilityOff();
  }

  if (parameterNode->GetApplicatorHolderVisibility())
  {
    applicatorHolderModel->GetDisplayNode()->VisibilityOn();
  }
  else
  {
    applicatorHolderModel->GetDisplayNode()->VisibilityOff();
  }
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
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Gantry, vtkSlicerIECTransformLogic::FixedReference);
  vtkMRMLLinearTransformNode* patientSupportToPatientSupportRotationTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupport, vtkSlicerIECTransformLogic::PatientSupportRotation);
  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry);
  vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTop, vtkSlicerIECTransformLogic::TableTopEccentricRotation);

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

  // If number of contacts between pieces of treatment room is greater than 0, the collision between which pieces
  // will be set to the output string and returned by the function.
  this->GantryTableTopCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(gantryToRasTransform));
  this->GantryTableTopCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(tableTopToRasTransform));
  this->GantryTableTopCollisionDetection->Update();
  if (this->GantryTableTopCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between gantry and table top\n";
  }

  this->GantryPatientSupportCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(gantryToRasTransform));
  this->GantryPatientSupportCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(patientSupportToRasTransform));
  this->GantryPatientSupportCollisionDetection->Update();
  if (this->GantryPatientSupportCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between gantry and patient support\n";
  }

  this->CollimatorTableTopCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(collimatorToRasTransform));
  this->CollimatorTableTopCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(tableTopToRasTransform));
  this->CollimatorTableTopCollisionDetection->Update();
  if (this->CollimatorTableTopCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between collimator and table top\n";
  }

  //TODO: Collision detection is disabled for additional devices, see SetupTreatmentMachineModels
  //this->AdditionalModelsTableTopCollisionDetection->Update();
  //if (this->AdditionalModelsTableTopCollisionDetection->GetNumberOfContacts() > 0)
  //{
  //  statusString = statusString + "Collision between additional devices and table top\n";
  //}

  //this->AdditionalModelsPatientSupportCollisionDetection->Update();
  //if (this->AdditionalModelsPatientSupportCollisionDetection->GetNumberOfContacts() > 0)
  //{
  //  statusString = statusString + "Collision between additional devices and patient support\n";
  //}

  // Get patient body poly data
  vtkNew<vtkPolyData> patientBodyPolyData;
  if (this->GetPatientBodyPolyData(parameterNode, patientBodyPolyData))
  {
    this->GantryPatientCollisionDetection->SetInput(1, patientBodyPolyData);
    this->GantryPatientCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(gantryToRasTransform));
    this->GantryPatientCollisionDetection->Update();
    if (this->GantryPatientCollisionDetection->GetNumberOfContacts() > 0)
    {
      statusString = statusString + "Collision between gantry and patient\n";
    }

    this->CollimatorPatientCollisionDetection->SetInput(1, patientBodyPolyData);
    this->CollimatorPatientCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(collimatorToRasTransform));
    this->CollimatorPatientCollisionDetection->Update();
    if (this->CollimatorPatientCollisionDetection->GetNumberOfContacts() > 0)
    {
      statusString = statusString + "Collision between collimator and patient\n";
    }
  }

  return statusString;
}
