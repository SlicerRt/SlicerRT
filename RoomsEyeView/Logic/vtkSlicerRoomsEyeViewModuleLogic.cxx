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

// SlicerQt includes
//TODO: These should not be included here, move these to the widget
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qSlicerFileDialog.h>
#include <qSlicerDataDialog.h>
#include <qSlicerIOManager.h>
#include <qMRMLSliceWidget.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLThreeDView.h>

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
const char* vtkSlicerRoomsEyeViewModuleLogic::COLLIMATOR_MODEL_NAME = "CollimatorModel";
const char* vtkSlicerRoomsEyeViewModuleLogic::GANTRY_MODEL_NAME = "GantryModel";
const char* vtkSlicerRoomsEyeViewModuleLogic::IMAGINGPANELLEFT_MODEL_NAME = "ImagingPanelLeftModel";
const char* vtkSlicerRoomsEyeViewModuleLogic::IMAGINGPANELRIGHT_MODEL_NAME = "ImagingPanelRightModel";
const char* vtkSlicerRoomsEyeViewModuleLogic::LINACBODY_MODEL_NAME = "LinacBodyModel";
const char* vtkSlicerRoomsEyeViewModuleLogic::PATIENTSUPPORT_MODEL_NAME = "PatientSupportModel";
const char* vtkSlicerRoomsEyeViewModuleLogic::TABLETOP_MODEL_NAME = "TableTopModel";
const char* vtkSlicerRoomsEyeViewModuleLogic::APPLICATORHOLDER_MODEL_NAME = "ApplicatorHolderModel";
const char* vtkSlicerRoomsEyeViewModuleLogic::ELECTRONAPPLICATOR_MODEL_NAME = "ElectronApplicatorModel";

const char* vtkSlicerRoomsEyeViewModuleLogic::ORIENTATION_MARKER_MODEL_NODE_NAME = "RoomsEyeViewOrientationMarkerModel";

// Transform names
//TODO: Add this dynamically to the IEC transform map
static const char* ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME = "AdditionalCollimatorDevicesToCollimatorTransform";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerRoomsEyeViewModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::vtkSlicerRoomsEyeViewModuleLogic()
  : GantryPatientCollisionDetection(NULL)
  , GantryTableTopCollisionDetection(NULL)
  , GantryPatientSupportCollisionDetection(NULL)
  , CollimatorPatientCollisionDetection(NULL)
  , CollimatorTableTopCollisionDetection(NULL)
  , AdditionalModelsTableTopCollisionDetection(NULL)
  , AdditionalModelsPatientSupportCollisionDetection(NULL)
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
    this->IECLogic = NULL;
  }

  if (this->GantryPatientCollisionDetection)
  {
    this->GantryPatientCollisionDetection->Delete();
    this->GantryPatientCollisionDetection = NULL;
  }
  if (this->GantryTableTopCollisionDetection)
  {
    this->GantryTableTopCollisionDetection->Delete();
    this->GantryTableTopCollisionDetection = NULL;
  }
  if (this->GantryPatientSupportCollisionDetection)
  {
    this->GantryPatientSupportCollisionDetection->Delete();
    this->GantryPatientSupportCollisionDetection = NULL;
  }
  if (this->CollimatorPatientCollisionDetection)
  {
    this->CollimatorPatientCollisionDetection->Delete();
    this->CollimatorPatientCollisionDetection = NULL;
  }
  if (this->CollimatorTableTopCollisionDetection)
  {
    this->CollimatorTableTopCollisionDetection->Delete();
    this->CollimatorTableTopCollisionDetection = NULL;
  }
  if (this->AdditionalModelsTableTopCollisionDetection)
  {
    this->AdditionalModelsTableTopCollisionDetection->Delete();
    this->AdditionalModelsTableTopCollisionDetection = NULL;
  }
  if (this->AdditionalModelsPatientSupportCollisionDetection)
  {
    this->AdditionalModelsPatientSupportCollisionDetection->Delete();
    this->AdditionalModelsPatientSupportCollisionDetection = NULL;
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
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRoomsEyeViewNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  this->Superclass::SetMRMLSceneInternal(newScene);

  this->IECLogic->SetMRMLScene(newScene);
}

//---------------------------------------------------------------------------
vtkSlicerIECTransformLogic* vtkSlicerRoomsEyeViewModuleLogic::GetIECLogic()
{
  return this->IECLogic;
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
  if (!scene->GetFirstNodeByName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME))
  {
    additionalCollimatorDevicesToCollimatorTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    additionalCollimatorDevicesToCollimatorTransformNode->SetName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME);
    additionalCollimatorDevicesToCollimatorTransformNode->SetHideFromEditors(1);
    scene->AddNode(additionalCollimatorDevicesToCollimatorTransformNode);
  }
  else
  {
    additionalCollimatorDevicesToCollimatorTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME));
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
void vtkSlicerRoomsEyeViewModuleLogic::LoadLinacModels()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("LoadLinacModels: Invalid scene");
    return;
  }

  std::string moduleShareDirectory = this->GetModuleShareDirectory();

  // Make sure the transform hierarchy is in place
  this->BuildRoomsEyeViewTransformHierarchy();

  //TODO: Only the Varian TrueBeam STx models are supported right now.
  //      Allow loading multiple types of machines
  std::string treatmentMachineModelsDirectory = moduleShareDirectory + "/" + "VarianTrueBeamSTx";
  std::string additionalDevicesDirectory = moduleShareDirectory + "/" + "AdditionalTreatmentModels";

  // Create a models logic for convenient loading of components
  vtkSmartPointer<vtkSlicerModelsLogic> modelsLogic = vtkSmartPointer<vtkSlicerModelsLogic>::New();
  modelsLogic->SetMRMLScene(this->GetMRMLScene());

  // Create model hierarchy so that the treatment machine can be shown/hidden easily
  vtkSmartPointer<vtkMRMLModelHierarchyNode> rootModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  this->GetMRMLScene()->AddNode(rootModelHierarchyNode);
  rootModelHierarchyNode->SetName("Varian TrueBeam STx linac components"); //TODO: Change when multiple models are supported

  vtkSmartPointer<vtkMRMLModelDisplayNode> rootModelHierarchyDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  this->GetMRMLScene()->AddNode(rootModelHierarchyDisplayNode);
  rootModelHierarchyNode->SetAndObserveDisplayNodeID( rootModelHierarchyDisplayNode->GetID() );

  //
  // Load supported treatment machine models
  std::string collimatorModelFilePath = treatmentMachineModelsDirectory + "/" + COLLIMATOR_MODEL_NAME + ".stl";
  vtkMRMLModelNode* collimatorModelNode = modelsLogic->AddModel(collimatorModelFilePath.c_str());
  vtkSmartPointer<vtkMRMLModelHierarchyNode> collimatorModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  this->GetMRMLScene()->AddNode(collimatorModelHierarchyNode);
  collimatorModelHierarchyNode->SetModelNodeID(collimatorModelNode->GetID());
  collimatorModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
  collimatorModelHierarchyNode->HideFromEditorsOn();

  std::string gantryModelFilePath = treatmentMachineModelsDirectory + "/" + GANTRY_MODEL_NAME + ".stl";
  vtkMRMLModelNode* gantryModelNode = modelsLogic->AddModel(gantryModelFilePath.c_str());
  vtkSmartPointer<vtkMRMLModelHierarchyNode> gantryModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  this->GetMRMLScene()->AddNode(gantryModelHierarchyNode);
  gantryModelHierarchyNode->SetModelNodeID(gantryModelNode->GetID());
  gantryModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
  gantryModelHierarchyNode->HideFromEditorsOn();

  std::string imagingPanelLeftModelFilePath = treatmentMachineModelsDirectory + "/" + IMAGINGPANELLEFT_MODEL_NAME + ".stl";
  vtkMRMLModelNode* imagingPanelLeftModelNode = modelsLogic->AddModel(imagingPanelLeftModelFilePath.c_str());
  vtkSmartPointer<vtkMRMLModelHierarchyNode> imagingPanelLeftModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  this->GetMRMLScene()->AddNode(imagingPanelLeftModelHierarchyNode);
  imagingPanelLeftModelHierarchyNode->SetModelNodeID(imagingPanelLeftModelNode->GetID());
  imagingPanelLeftModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
  imagingPanelLeftModelHierarchyNode->HideFromEditorsOn();

  std::string imagingPanelRightModelFilePath = treatmentMachineModelsDirectory + "/" + IMAGINGPANELRIGHT_MODEL_NAME + ".stl";
  vtkMRMLModelNode* imagingPanelRightModelNode = modelsLogic->AddModel(imagingPanelRightModelFilePath.c_str());
  vtkSmartPointer<vtkMRMLModelHierarchyNode> imagingPanelRightModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  this->GetMRMLScene()->AddNode(imagingPanelRightModelHierarchyNode);
  imagingPanelRightModelHierarchyNode->SetModelNodeID(imagingPanelRightModelNode->GetID());
  imagingPanelRightModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
  imagingPanelRightModelHierarchyNode->HideFromEditorsOn();

  std::string linacBodyModelFilePath = treatmentMachineModelsDirectory + "/" + LINACBODY_MODEL_NAME + ".stl";
  vtkMRMLModelNode* linacBodyModelNode = modelsLogic->AddModel(linacBodyModelFilePath.c_str());
  vtkSmartPointer<vtkMRMLModelHierarchyNode> linacBodyModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  this->GetMRMLScene()->AddNode(linacBodyModelHierarchyNode);
  linacBodyModelHierarchyNode->SetModelNodeID(linacBodyModelNode->GetID());
  linacBodyModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
  linacBodyModelHierarchyNode->HideFromEditorsOn();

  std::string patientSupportModelFilePath = treatmentMachineModelsDirectory + "/" + PATIENTSUPPORT_MODEL_NAME + ".stl";
  vtkMRMLModelNode* patientSupportModelNode = modelsLogic->AddModel(patientSupportModelFilePath.c_str());
  vtkSmartPointer<vtkMRMLModelHierarchyNode> patientSupportModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  this->GetMRMLScene()->AddNode(patientSupportModelHierarchyNode);
  patientSupportModelHierarchyNode->SetModelNodeID(patientSupportModelNode->GetID());
  patientSupportModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
  patientSupportModelHierarchyNode->HideFromEditorsOn();

  std::string tableTopModelFilePath = treatmentMachineModelsDirectory + "/" + TABLETOP_MODEL_NAME + ".stl";
  vtkMRMLModelNode* tableTopModelNode = modelsLogic->AddModel(tableTopModelFilePath.c_str());
  vtkSmartPointer<vtkMRMLModelHierarchyNode> tableTopModelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  this->GetMRMLScene()->AddNode(tableTopModelHierarchyNode);
  tableTopModelHierarchyNode->SetModelNodeID(tableTopModelNode->GetID());
  tableTopModelHierarchyNode->SetParentNodeID(rootModelHierarchyNode->GetID());
  tableTopModelHierarchyNode->HideFromEditorsOn();

  if ( !collimatorModelNode || !gantryModelNode || !imagingPanelLeftModelNode || !imagingPanelRightModelNode
    || !patientSupportModelNode || !tableTopModelNode )
  {
    vtkErrorMacro("LoadLinacModels: Failed to load every treatment machine component");
    return;
  }

  //TODO: Move these to LoadAdditionalDevices, as these are not linac models
  std::string applicatorHolderModelFilePath = additionalDevicesDirectory + "/" + APPLICATORHOLDER_MODEL_NAME + ".stl";
  modelsLogic->AddModel(applicatorHolderModelFilePath.c_str());
  std::string electronApplicatorModelFilePath = additionalDevicesDirectory + "/" + ELECTRONAPPLICATOR_MODEL_NAME + ".stl";
  modelsLogic->AddModel(electronApplicatorModelFilePath.c_str());

  // Setup treatment machine model display and transforms
  this->SetupTreatmentMachineModels();
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LoadAdditionalDevices()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("LoadLinacModels: Invalid scene");
    return;
  }

  //TODO: This should not be in the logic, move to widget
  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  vtkSmartPointer<vtkCollection> loadedModelsCollection = vtkSmartPointer<vtkCollection>::New();
  ioManager->openDialog("ModelFile", qSlicerDataDialog::Read, qSlicerIO::IOProperties(), loadedModelsCollection);
  
  vtkMRMLModelNode* loadedModelNode = vtkMRMLModelNode::New();
  loadedModelNode = vtkMRMLModelNode::SafeDownCast(loadedModelsCollection->GetNextItemAsObject());
  vtkMRMLLinearTransformNode* collimatorModelTransforms = this->IECLogic->GetTransformNodeBetween(
    vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry );
  loadedModelNode->SetAndObserveTransformNodeID(collimatorModelTransforms->GetID());
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::SetupTreatmentMachineModels()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Invalid scene");
    return;
  }

  // Display all pieces of the treatment room and sets each piece a color to provide realistic representation
  vtkMRMLModelNode* linacBodyModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(LINACBODY_MODEL_NAME) );
  if (!linacBodyModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access linac body model");
    return;
  }
  vtkMRMLLinearTransformNode* fixedReferenceToRasTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::FixedReference, vtkSlicerIECTransformLogic::RAS);
  linacBodyModel->SetAndObserveTransformNodeID(fixedReferenceToRasTransformNode->GetID());
  linacBodyModel->CreateDefaultDisplayNodes();
  linacBodyModel->GetDisplayNode()->SetColor(0.9, 0.9, 0.9);

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

  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME) );
  if (!leftImagingPanelModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access left imaging panel model");
    return;
  }
  vtkMRMLLinearTransformNode* leftImagingPanelToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::LeftImagingPanel, vtkSlicerIECTransformLogic::Gantry);
  leftImagingPanelModel->SetAndObserveTransformNodeID(leftImagingPanelToGantryTransformNode->GetID());
  leftImagingPanelModel->CreateDefaultDisplayNodes();
  leftImagingPanelModel->GetDisplayNode()->SetColor(0.95, 0.95, 0.95);

  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME) );
  if (!rightImagingPanelModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access right imaging panel model");
    return;
  }
  vtkMRMLLinearTransformNode* rightImagingPanelToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::RightImagingPanel, vtkSlicerIECTransformLogic::Gantry);
  rightImagingPanelModel->SetAndObserveTransformNodeID(rightImagingPanelToGantryTransformNode->GetID());
  rightImagingPanelModel->CreateDefaultDisplayNodes();
  rightImagingPanelModel->GetDisplayNode()->SetColor(0.95, 0.95, 0.95);

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
  vtkSmartPointer<vtkTransform> identityTransform = vtkSmartPointer<vtkTransform>::New();
  identityTransform->Identity();
  this->GantryPatientCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(identityTransform));
  this->CollimatorPatientCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(identityTransform));


  //
  // Additional devices
  //TODO: Separate to a function and call it from LoadAdditionalDevices
  vtkMRMLModelNode* applicatorHolderModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(APPLICATORHOLDER_MODEL_NAME));
  if (!applicatorHolderModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access applicator holder model");
    return;
  }
  vtkMRMLLinearTransformNode* applicatorHolderModelTransformNode = vtkMRMLLinearTransformNode::SafeDownCast( //TODO:
    this->GetMRMLScene()->GetFirstNodeByName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME));
  applicatorHolderModel->SetAndObserveTransformNodeID(applicatorHolderModelTransformNode->GetID());
  applicatorHolderModel->CreateDefaultDisplayNodes();
  applicatorHolderModel->GetDisplayNode()->VisibilityOff();

  vtkMRMLModelNode* electronApplicatorModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(ELECTRONAPPLICATOR_MODEL_NAME));
  if (!electronApplicatorModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access electron applicator model");
    return;
  }
  vtkMRMLLinearTransformNode* electronApplicatorModelTransformNode = vtkMRMLLinearTransformNode::SafeDownCast( //TODO:
    this->GetMRMLScene()->GetFirstNodeByName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME));
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
void vtkSlicerRoomsEyeViewModuleLogic::UpdateTreatmentOrientationMarker()
{
  vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();

  vtkMRMLModelNode* gantryModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(GANTRY_MODEL_NAME));
  vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_MODEL_NAME));
  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME));
  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME));
  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  vtkMRMLModelNode* tableTopModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOP_MODEL_NAME));

  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = 
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Gantry, vtkSlicerIECTransformLogic::FixedReference);
  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry);
  vtkMRMLLinearTransformNode* leftImagingPanelToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::LeftImagingPanel, vtkSlicerIECTransformLogic::Gantry);
  vtkMRMLLinearTransformNode* rightImagingPanelToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::RightImagingPanel, vtkSlicerIECTransformLogic::Gantry);
  vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupport, vtkSlicerIECTransformLogic::FixedReference);
  vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTop, vtkSlicerIECTransformLogic::TableTopEccentricRotation);

  vtkSmartPointer<vtkPolyData> gantryModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  gantryModelPolyData->DeepCopy(gantryModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> collimatorModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  collimatorModelPolyData->DeepCopy(collimatorModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> leftImagingPanelModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  leftImagingPanelModelPolyData->DeepCopy(leftImagingPanelModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> rightImagingPanelModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  rightImagingPanelModelPolyData->DeepCopy(rightImagingPanelModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> patientSupportModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  patientSupportModelPolyData->DeepCopy(patientSupportModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> tableTopModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  tableTopModelPolyData->DeepCopy(tableTopModel->GetPolyData());

  vtkSmartPointer<vtkTransformFilter> gantryTransformFilter = vtkSmartPointer<vtkTransformFilter>::New();
  gantryTransformFilter->SetInputData(gantryModelPolyData);
  vtkSmartPointer<vtkGeneralTransform> gantryToFixedReferenceTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  gantryToFixedReferenceTransformNode->GetTransformFromWorld(gantryToFixedReferenceTransform);
  gantryToFixedReferenceTransform->Inverse();
  gantryTransformFilter->SetTransform(gantryToFixedReferenceTransform);
  gantryTransformFilter->Update();
  gantryModelPolyData = vtkPolyData::SafeDownCast(gantryTransformFilter->GetOutput());

  vtkSmartPointer<vtkTransformFilter> collimatorTransformFilter = vtkSmartPointer<vtkTransformFilter>::New();
  collimatorTransformFilter->SetInputData(collimatorModelPolyData);
  vtkSmartPointer<vtkGeneralTransform> collimatorToGantryTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  collimatorToGantryTransformNode->GetTransformFromWorld(collimatorToGantryTransform);
  collimatorToGantryTransform->Inverse();
  collimatorTransformFilter->SetTransform(collimatorToGantryTransform);
  collimatorTransformFilter->Update();
  collimatorModelPolyData = vtkPolyData::SafeDownCast(collimatorTransformFilter->GetOutput());

  vtkSmartPointer<vtkTransformFilter> leftImagingPanelTransformFilter = vtkSmartPointer<vtkTransformFilter>::New();
  leftImagingPanelTransformFilter->SetInputData(leftImagingPanelModelPolyData);
  vtkSmartPointer<vtkGeneralTransform> leftImagingPanelToGantryTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  leftImagingPanelToGantryTransformNode->GetTransformFromWorld(leftImagingPanelToGantryTransform);
  leftImagingPanelToGantryTransform->Inverse();
  leftImagingPanelTransformFilter->SetTransform(leftImagingPanelToGantryTransform);
  leftImagingPanelTransformFilter->Update();
  leftImagingPanelModelPolyData = vtkPolyData::SafeDownCast(leftImagingPanelTransformFilter->GetOutput());

  vtkSmartPointer<vtkTransformFilter> rightImagingPanelTransformFilter = vtkSmartPointer<vtkTransformFilter>::New();
  rightImagingPanelTransformFilter->SetInputData(rightImagingPanelModelPolyData);
  vtkSmartPointer<vtkGeneralTransform> rightImagingPanelToGantryTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  rightImagingPanelToGantryTransformNode->GetTransformFromWorld(rightImagingPanelToGantryTransform);
  rightImagingPanelToGantryTransform->Inverse();
  rightImagingPanelTransformFilter->SetTransform(rightImagingPanelToGantryTransform);
  rightImagingPanelTransformFilter->Update();
  rightImagingPanelModelPolyData = vtkPolyData::SafeDownCast(rightImagingPanelTransformFilter->GetOutput());

  vtkSmartPointer<vtkTransformFilter> patientSupportTransformFilter = vtkSmartPointer<vtkTransformFilter>::New();
  patientSupportTransformFilter->SetInputData(patientSupportModelPolyData);
  vtkSmartPointer<vtkGeneralTransform> patientSupportToFixedReferenceTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  patientSupportToFixedReferenceTransformNode->GetTransformFromWorld(patientSupportToFixedReferenceTransform);
  patientSupportToFixedReferenceTransform->Inverse();
  patientSupportTransformFilter->SetTransform(patientSupportToFixedReferenceTransform);
  patientSupportTransformFilter->Update();
  patientSupportModelPolyData = vtkPolyData::SafeDownCast(patientSupportTransformFilter->GetOutput());

  vtkSmartPointer<vtkTransformFilter> tableTopTransformFilter = vtkSmartPointer<vtkTransformFilter>::New();
  tableTopTransformFilter->SetInputData(tableTopModelPolyData);
  vtkSmartPointer<vtkGeneralTransform> tableTopModelTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  tableTopToTableTopEccentricRotationTransformNode->GetTransformFromWorld(tableTopModelTransform);
  tableTopModelTransform->Inverse();
  tableTopTransformFilter->SetTransform(tableTopModelTransform);
  tableTopTransformFilter->Update();
  tableTopModelPolyData = vtkPolyData::SafeDownCast(tableTopTransformFilter->GetOutput());

  vtkPolyData* inputs[] = { gantryModelPolyData, collimatorModelPolyData, leftImagingPanelModelPolyData, rightImagingPanelModelPolyData, patientSupportModelPolyData, tableTopModelPolyData};
  vtkSmartPointer<vtkPolyData> orientationMarkerPolyData = vtkSmartPointer<vtkPolyData>::New();
  appendFilter->ExecuteAppend(orientationMarkerPolyData,inputs,6);

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

  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();
  viewNode->SetOrientationMarkerHumanModelNodeID(orientationMarkerModel->GetID());  
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
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(
    collimatorToGantryTransformNode->GetTransformToParent() );

  collimatorToGantryTransform->Identity();
  collimatorToGantryTransform->RotateZ(parameterNode->GetCollimatorRotationAngle());
  collimatorToGantryTransform->Modified();
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
  vtkTransform* gantryToFixedReferenceTransform = vtkTransform::SafeDownCast(
    gantryToFixedReferenceTransformNode->GetTransformToParent() );
  
  gantryToFixedReferenceTransform->Identity();
  gantryToFixedReferenceTransform->RotateY(parameterNode->GetGantryRotationAngle() * (-1.0));
  gantryToFixedReferenceTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry);
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(
    collimatorToGantryTransformNode->GetTransformToParent() );
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
    vtkErrorMacro("UpdateLeftImagingPanelToGantryTransform: Invalid MRML model node");
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

  vtkSmartPointer<vtkTransform> leftImagingPanelToRasTransform = vtkSmartPointer<vtkTransform>::New();
  leftImagingPanelToRasTransform->Identity();
  leftImagingPanelToRasTransform->Translate(leftImagingPanelTranslationToOrigin);

  // Rotation
  vtkSmartPointer<vtkTransform> rasToRotatedRasTransform = vtkSmartPointer<vtkTransform>::New();
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
  vtkSmartPointer<vtkTransform> rotatedRasToRotatedLeftImagingPanelTransform = vtkSmartPointer<vtkTransform>::New();
  rotatedRasToRotatedLeftImagingPanelTransform->Translate(leftImagingPanelTranslationFromOrigin);

  // Translation
  vtkSmartPointer<vtkTransform> rotatedLeftImagingPanelToGantryTransform = vtkSmartPointer<vtkTransform>::New();
  if (panelMovement > 0)
  {
    double translationArray[3] = { 0, -(panelMovement), 0 };
    rotatedLeftImagingPanelToGantryTransform->Translate(translationArray);
  }

  // Assemble transform and update node
  vtkMRMLLinearTransformNode* leftImagingPanelToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::LeftImagingPanel, vtkSlicerIECTransformLogic::Gantry);
  vtkTransform* leftImagingPanelToGantryTransform = vtkTransform::SafeDownCast(
    leftImagingPanelToGantryTransformNode->GetTransformToParent() );
  leftImagingPanelToGantryTransform->Identity();
  leftImagingPanelToGantryTransform->PostMultiply();
  leftImagingPanelToGantryTransform->Concatenate(leftImagingPanelToRasTransform);
  leftImagingPanelToGantryTransform->Concatenate(rasToRotatedRasTransform);
  leftImagingPanelToGantryTransform->Concatenate(rotatedRasToRotatedLeftImagingPanelTransform);
  leftImagingPanelToGantryTransform->Concatenate(rotatedLeftImagingPanelToGantryTransform);
  leftImagingPanelToGantryTransform->Modified();
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
    vtkErrorMacro("UpdateRightImagingPanelToGantryTransform: Invalid MRML model node");
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

  vtkSmartPointer<vtkTransform> rightImagingPanelToRasTransform = vtkSmartPointer<vtkTransform>::New();
  rightImagingPanelToRasTransform->Identity();
  rightImagingPanelToRasTransform->Translate(rightImagingPanelTranslationToOrigin);

  // Rotation
  vtkSmartPointer<vtkTransform> rasToRotatedRasTransform = vtkSmartPointer<vtkTransform>::New();
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
  vtkSmartPointer<vtkTransform> rotatedRasToRotatedRightImagingPanelTransform = vtkSmartPointer<vtkTransform>::New();
  rotatedRasToRotatedRightImagingPanelTransform->Translate(rightImagingPanelTranslationFromOrigin);

  // Translation
  vtkSmartPointer<vtkTransform> rotatedRightImagingPanelToGantryTransform = vtkSmartPointer<vtkTransform>::New();
  if (panelMovement > 0)
  {
    double translationArray[3] = { 0, -(panelMovement), 0 };
    rotatedRightImagingPanelToGantryTransform->Translate(translationArray);
  }

  // Assemble transform and update node
  vtkMRMLLinearTransformNode* rightImagingPanelToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::RightImagingPanel, vtkSlicerIECTransformLogic::Gantry);
  vtkTransform* rightImagingPanelToGantryTransform = vtkTransform::SafeDownCast(
    rightImagingPanelToGantryTransformNode->GetTransformToParent() );
  rightImagingPanelToGantryTransform->Identity();
  rightImagingPanelToGantryTransform->PostMultiply();
  rightImagingPanelToGantryTransform->Concatenate(rightImagingPanelToRasTransform);
  rightImagingPanelToGantryTransform->Concatenate(rasToRotatedRasTransform);
  rightImagingPanelToGantryTransform->Concatenate(rotatedRasToRotatedRightImagingPanelTransform);
  rightImagingPanelToGantryTransform->Concatenate(rotatedRightImagingPanelToGantryTransform);
  rightImagingPanelToGantryTransform->Modified();
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
  vtkTransform* patientSupportToRotatedPatientSupportTransform = vtkTransform::SafeDownCast(
    patientSupportRotationToFixedReferenceTransformNode->GetTransformToParent() );
  
  double rotationAngle = parameterNode->GetPatientSupportRotationAngle();
  patientSupportToRotatedPatientSupportTransform->Identity();
  patientSupportToRotatedPatientSupportTransform->RotateZ(rotationAngle);
  patientSupportToRotatedPatientSupportTransform->Modified();
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
  vtkSmartPointer<vtkTransform> patientSupportRotationToRasTransform = vtkSmartPointer<vtkTransform>::New();
  patientSupportRotationToRasTransform->Identity();
  double patientSupportTranslationToOrigin[3] = { 0, 0, (-1.0) * patientSupportModelBounds[4]}; //TODO: Subtract [1]?
  patientSupportRotationToRasTransform->Translate(patientSupportTranslationToOrigin);

  // Vertical scaling
  double scaleFactor = fabs((patientSupportModelBounds[4] + patientSupportModelBounds[5]) / 2);
  double tableTopDisplacement = parameterNode->GetVerticalTableTopDisplacement();

  vtkSmartPointer<vtkTransform> rasToScaledRasTransform = vtkSmartPointer<vtkTransform>::New();
  rasToScaledRasTransform->Identity();
  //TODO: Magic number
  rasToScaledRasTransform->Scale(1, 1, ((fabs(patientSupportModelBounds[5]) + tableTopDisplacement*0.525) / fabs(patientSupportModelBounds[5]))); //TODO: Subtract [2]?

  // Translation back from origin after in-place scaling
  vtkSmartPointer<vtkTransform> scaledRasToFixedReferenceTransform = vtkSmartPointer<vtkTransform>::New();
  scaledRasToFixedReferenceTransform->Identity();
  double patientSupportTranslationFromOrigin[3] = { 0, 0, patientSupportModelBounds[4] }; //TODO: Subtract [1]?
  scaledRasToFixedReferenceTransform->Translate(patientSupportTranslationFromOrigin);

  // Assemble transform and update node
  vtkMRMLLinearTransformNode* patientSupportToPatientSupportRotationTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupport, vtkSlicerIECTransformLogic::PatientSupportRotation);
  vtkTransform* patientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(
    patientSupportToPatientSupportRotationTransformNode->GetTransformToParent() );
  patientSupportToFixedReferenceTransform->Identity();
  patientSupportToFixedReferenceTransform->PostMultiply();
  patientSupportToFixedReferenceTransform->Concatenate(patientSupportRotationToRasTransform);
  patientSupportToFixedReferenceTransform->Concatenate(rasToScaledRasTransform);
  patientSupportToFixedReferenceTransform->Concatenate(scaledRasToFixedReferenceTransform);
  patientSupportToFixedReferenceTransform->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateTableTopEccentricRotationToPatientSupportRotationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
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

  tableTopEccentricRotationToPatientSupportTransform->Identity();
  double translationArray[3] =
    { parameterNode->GetLateralTableTopDisplacement(), parameterNode->GetLongitudinalTableTopDisplacement(), parameterNode->GetVerticalTableTopDisplacement() };
  tableTopEccentricRotationToPatientSupportTransform->Translate(translationArray);
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
    this->GetMRMLScene()->GetFirstNodeByName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME) );
  vtkTransform* additionalCollimatorDeviceToCollimatorTransform = vtkTransform::SafeDownCast(
    additionalCollimatorDeviceToCollimatorTransformNode->GetTransformToParent());
  
  double translationArray[3] = { parameterNode->GetAdditionalModelLateralDisplacement(), parameterNode->GetAdditionalModelLongitudinalDisplacement(),
    parameterNode->GetAdditionalModelVerticalDisplacement() };

  additionalCollimatorDeviceToCollimatorTransform->Identity();
  additionalCollimatorDeviceToCollimatorTransform->Translate(translationArray);
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
  vtkSmartPointer<vtkGeneralTransform> gantryToRasGeneralTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  gantryToFixedReferenceTransformNode->GetTransformToWorld(gantryToRasGeneralTransform);
  vtkSmartPointer<vtkTransform> gantryToRasTransform = vtkSmartPointer<vtkTransform>::New();

  vtkSmartPointer<vtkGeneralTransform> patientSupportToRasGeneralTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  patientSupportToPatientSupportRotationTransformNode->GetTransformToWorld(patientSupportToRasGeneralTransform);
  vtkSmartPointer<vtkTransform> patientSupportToRasTransform = vtkSmartPointer<vtkTransform>::New();

  vtkSmartPointer<vtkGeneralTransform> collimatorToRasGeneralTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  collimatorToGantryTransformNode->GetTransformToWorld(collimatorToRasGeneralTransform);
  vtkSmartPointer<vtkTransform> collimatorToRasTransform = vtkSmartPointer<vtkTransform>::New();

  vtkSmartPointer<vtkGeneralTransform> tableTopToRasGeneralTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  tableTopToTableTopEccentricRotationTransformNode->GetTransformToWorld(tableTopToRasGeneralTransform);
  vtkSmartPointer<vtkTransform> tableTopToRasTransform = vtkSmartPointer<vtkTransform>::New();

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
  vtkSmartPointer<vtkPolyData> patientBodyPolyData = vtkSmartPointer<vtkPolyData>::New();
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

/** This function was added to help with the issue of the vtkMRMLRTBeamNode::CalculateSourcePosition function not accounting for beam transformations, but has been unsuccessful thus far
bool vtkSlicerRoomsEyeViewModuleLogic::CalculateNewSourcePosition(vtkMRMLRTBeamNode* beamNode, double oldSourcePosition[3], double newSourcePosition[3]){
  vtkMatrix4x4* sourcePositionTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkTransform* sourcePositionTransform = vtkSmartPointer<vtkTransform>::New();
  vtkMRMLLinearTransformNode* sourcePositionTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName("NewBeam_7_Transform"));
  sourcePositionTransform = vtkTransform::SafeDownCast(sourcePositionTransformNode->GetTransformFromParent());
  sourcePositionTransform->TransformPoint(oldSourcePosition, newSourcePosition);

  return true;
}**/
