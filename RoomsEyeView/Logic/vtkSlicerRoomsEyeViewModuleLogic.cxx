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
  : CollimatorToWorldTransformMatrix(NULL)
  , TableTopToWorldTransformMatrix(NULL)
  , GantryPatientCollisionDetection(NULL)
  , GantryTableTopCollisionDetection(NULL)
  , GantryPatientSupportCollisionDetection(NULL)
  , CollimatorPatientCollisionDetection(NULL)
  , CollimatorTableTopCollisionDetection(NULL)
  , AdditionalModelsTableTopCollisionDetection(NULL)
  , AdditionalModelsPatientSupportCollisionDetection(NULL)
{
  this->IECLogic = vtkSlicerIECTransformLogic::New();

  this->CollimatorToWorldTransformMatrix = vtkMatrix4x4::New();
  this->TableTopToWorldTransformMatrix = vtkMatrix4x4::New();

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

  if (this->CollimatorToWorldTransformMatrix)
  {
    this->CollimatorToWorldTransformMatrix->Delete();
    this->CollimatorToWorldTransformMatrix = NULL;
  }
  if (this->TableTopToWorldTransformMatrix)
  {
    this->TableTopToWorldTransformMatrix->Delete();
    this->TableTopToWorldTransformMatrix = NULL;
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
  vtkMRMLLinearTransformNode* tableTopEccentricRotatedToPatientSupportTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTopEccentricRotated, vtkSlicerIECTransformLogic::PatientSupport);
  if (!tableTopEccentricRotatedToPatientSupportTransformNode)
  {
    vtkErrorMacro("BuildRoomsEyeViewTransformHierarchy: Failed to access tableTopEccentricRotatedToPatientSupportTransformNode");
    return;
  }

  // Organize transforms into hierarchy
  additionalCollimatorDevicesToCollimatorTransformNode->SetAndObserveTransformNodeID(collimatorToGantryTransformNode->GetID());

  // Get member transform matrices from transform nodes
  //TODO: This does not look right, need to review (we should not need these members)
  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
  tableTopEccentricRotatedToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
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
  // Load supported treatment machine models
  vtkSmartPointer<vtkSlicerModelsLogic> modelsLogic = vtkSmartPointer<vtkSlicerModelsLogic>::New();
  modelsLogic->SetMRMLScene(this->GetMRMLScene());

  std::string collimatorModelFilePath = treatmentMachineModelsDirectory + "/" + COLLIMATOR_MODEL_NAME + ".stl";
  modelsLogic->AddModel(collimatorModelFilePath.c_str());
  std::string gantryModelFilePath = treatmentMachineModelsDirectory + "/" + GANTRY_MODEL_NAME + ".stl";
  modelsLogic->AddModel(gantryModelFilePath.c_str());
  std::string imagingPanelLeftModelFilePath = treatmentMachineModelsDirectory + "/" + IMAGINGPANELLEFT_MODEL_NAME + ".stl";
  modelsLogic->AddModel(imagingPanelLeftModelFilePath.c_str());
  std::string imagingPanelRightModelFilePath = treatmentMachineModelsDirectory + "/" + IMAGINGPANELRIGHT_MODEL_NAME + ".stl";
  modelsLogic->AddModel(imagingPanelRightModelFilePath.c_str());
  std::string linacBodyModelFilePath = treatmentMachineModelsDirectory + "/" + LINACBODY_MODEL_NAME + ".stl";
  modelsLogic->AddModel(linacBodyModelFilePath.c_str());
  std::string patientSupportModelFilePath = treatmentMachineModelsDirectory + "/" + PATIENTSUPPORT_MODEL_NAME + ".stl";
  modelsLogic->AddModel(patientSupportModelFilePath.c_str());
  std::string tableTopModelFilePath = treatmentMachineModelsDirectory + "/" + TABLETOP_MODEL_NAME + ".stl";
  modelsLogic->AddModel(tableTopModelFilePath.c_str());

  //TODO: Move these to LoadAdditionalDevices, as these are not linac models
  std::string applicatorHolderModelFilePath = additionalDevicesDirectory + "/" + APPLICATORHOLDER_MODEL_NAME + ".stl";
  modelsLogic->AddModel(applicatorHolderModelFilePath.c_str());
  std::string electronApplicatorModelFilePath = additionalDevicesDirectory + "/" + ELECTRONAPPLICATOR_MODEL_NAME + ".stl";
  modelsLogic->AddModel(electronApplicatorModelFilePath.c_str());
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
  vtkCollection* loadedModelsCollection = vtkCollection::New();
  ioManager->openDialog("ModelFile", qSlicerDataDialog::Read, qSlicerIO::IOProperties(), loadedModelsCollection);
  
  vtkMRMLModelNode* loadedModelNode = vtkMRMLModelNode::New();
  loadedModelNode = vtkMRMLModelNode::SafeDownCast(loadedModelsCollection->GetNextItemAsObject());
  vtkMRMLLinearTransformNode* collimatorModelTransforms = this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry);
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

  // TODO: Uncomment once Additional Devices STL models are created
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
  vtkMRMLLinearTransformNode* patientSupportModelTransformNode = //TODO:
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupportPositiveVerticalTranslated, vtkSlicerIECTransformLogic::PatientSupportScaled);
  vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransform = this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupport, vtkSlicerIECTransformLogic::FixedReference);
  patientSupportModel->SetAndObserveTransformNodeID(patientSupportModelTransformNode->GetID());
  patientSupportModel->CreateDefaultDisplayNodes();
  patientSupportModel->GetDisplayNode()->SetColor(0.85, 0.85, 0.85);

  vtkMRMLModelNode* tableTopModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOP_MODEL_NAME) );
  if (!tableTopModel)
  {
    vtkErrorMacro("SetupTreatmentMachineModels: Unable to access table top model");
    return;
  }
  vtkMRMLLinearTransformNode* tableTopModelTransformNode = //TODO:
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTopEccentricRotated, vtkSlicerIECTransformLogic::PatientSupport);
  tableTopModel->SetAndObserveTransformNodeID(tableTopModelTransformNode->GetID());
  tableTopModel->CreateDefaultDisplayNodes();
  tableTopModel->GetDisplayNode()->SetColor(0, 0, 0);

  // Set up collision detection between components
  this->GantryTableTopCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());
  this->GantryTableTopCollisionDetection->SetTransform(0,
    vtkLinearTransform::SafeDownCast(gantryToFixedReferenceTransformNode->GetTransformToParent()) );
  this->GantryTableTopCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->GantryTableTopCollisionDetection->Update();

  this->GantryPatientSupportCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryPatientSupportCollisionDetection->SetInput(1, patientSupportModel->GetPolyData());
  this->GantryPatientSupportCollisionDetection->SetTransform(0,
    vtkLinearTransform::SafeDownCast(gantryToFixedReferenceTransformNode->GetTransformToParent()) );
  this->GantryPatientSupportCollisionDetection->SetTransform(1,
    vtkLinearTransform::SafeDownCast(patientSupportToFixedReferenceTransform->GetTransformToParent()) );
  this->GantryPatientSupportCollisionDetection->Update();

  this->CollimatorTableTopCollisionDetection->SetInput(0, collimatorModel->GetPolyData());
  this->CollimatorTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());
  this->CollimatorTableTopCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  this->CollimatorTableTopCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->CollimatorTableTopCollisionDetection->Update();

  //TODO: Whole patient (segmentation, CT) will need to be transformed when the table top is transformed
  //vtkMRMLLinearTransformNode* patientModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
  //  this->GetMRMLScene()->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  //patientModel->SetAndObserveTransformNodeID(patientModelTransforms->GetID());

  this->GantryPatientCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryPatientCollisionDetection->SetTransform(0,
    vtkLinearTransform::SafeDownCast(gantryToFixedReferenceTransformNode->GetTransformToParent()) );
  this->GantryPatientCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->GantryPatientCollisionDetection->Update();

  this->CollimatorPatientCollisionDetection->SetInput(0, collimatorModel->GetPolyData());
  this->CollimatorPatientCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  this->CollimatorPatientCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->CollimatorPatientCollisionDetection->Update();

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
  vtkSmartPointer<vtkAppendPolyData> append = vtkSmartPointer<vtkAppendPolyData>::New();

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
  vtkMRMLLinearTransformNode* patientSupportModelTransformNode = //TODO:
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupportPositiveVerticalTranslated, vtkSlicerIECTransformLogic::PatientSupportScaled);
  vtkMRMLLinearTransformNode* tableTopModelTransformNode = //TODO:
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTopEccentricRotated, vtkSlicerIECTransformLogic::PatientSupport);

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

  vtkTransformFilter* gantryTransformFilter = vtkTransformFilter::New();
  gantryTransformFilter->SetInputData(gantryModelPolyData);
  vtkGeneralTransform* gantryToFixedReferenceTransform = vtkGeneralTransform::New();
  gantryToFixedReferenceTransformNode->GetTransformFromWorld(gantryToFixedReferenceTransform);
  gantryToFixedReferenceTransform->Inverse();
  gantryTransformFilter->SetTransform(gantryToFixedReferenceTransform);
  gantryTransformFilter->Update();
  gantryModelPolyData = vtkPolyData::SafeDownCast(gantryTransformFilter->GetOutput());

  vtkTransformFilter* collimatorTransformFilter = vtkTransformFilter::New();
  collimatorTransformFilter->SetInputData(collimatorModelPolyData);
  vtkGeneralTransform* collimatorToGantryTransform = vtkGeneralTransform::New();
  collimatorToGantryTransformNode->GetTransformFromWorld(collimatorToGantryTransform);
  collimatorToGantryTransform->Inverse();
  collimatorTransformFilter->SetTransform(collimatorToGantryTransform);
  collimatorTransformFilter->Update();
  collimatorModelPolyData = vtkPolyData::SafeDownCast(collimatorTransformFilter->GetOutput());

  vtkTransformFilter* leftImagingPanelTransformFilter = vtkTransformFilter::New();
  leftImagingPanelTransformFilter->SetInputData(leftImagingPanelModelPolyData);
  vtkGeneralTransform* leftImagingPanelToGantryTransform = vtkGeneralTransform::New();
  leftImagingPanelToGantryTransformNode->GetTransformFromWorld(leftImagingPanelToGantryTransform);
  leftImagingPanelToGantryTransform->Inverse();
  leftImagingPanelTransformFilter->SetTransform(leftImagingPanelToGantryTransform);
  leftImagingPanelTransformFilter->Update();
  leftImagingPanelModelPolyData = vtkPolyData::SafeDownCast(leftImagingPanelTransformFilter->GetOutput());

  vtkTransformFilter* rightImagingPanelTransformFilter = vtkTransformFilter::New();
  rightImagingPanelTransformFilter->SetInputData(rightImagingPanelModelPolyData);
  vtkGeneralTransform* rightImagingPanelToGantryTransform = vtkGeneralTransform::New();
  rightImagingPanelToGantryTransformNode->GetTransformFromWorld(rightImagingPanelToGantryTransform);
  rightImagingPanelToGantryTransform->Inverse();
  rightImagingPanelTransformFilter->SetTransform(rightImagingPanelToGantryTransform);
  rightImagingPanelTransformFilter->Update();
  rightImagingPanelModelPolyData = vtkPolyData::SafeDownCast(rightImagingPanelTransformFilter->GetOutput());

  vtkTransformFilter* patientSupportTransformFilter = vtkTransformFilter::New();
  patientSupportTransformFilter->SetInputData(patientSupportModelPolyData);
  vtkGeneralTransform* patientSupportModelTransform = vtkGeneralTransform::New();
  patientSupportModelTransformNode->GetTransformFromWorld(patientSupportModelTransform);
  patientSupportModelTransform->Inverse();
  patientSupportTransformFilter->SetTransform(patientSupportModelTransform);
  patientSupportTransformFilter->Update();
  patientSupportModelPolyData = vtkPolyData::SafeDownCast(patientSupportTransformFilter->GetOutput());

  vtkTransformFilter* tableTopTransformFilter = vtkTransformFilter::New();
  tableTopTransformFilter->SetInputData(tableTopModelPolyData);
  vtkGeneralTransform* tableTopModelTransform = vtkGeneralTransform::New();
  tableTopModelTransformNode->GetTransformFromWorld(tableTopModelTransform);
  tableTopModelTransform->Inverse();
  tableTopTransformFilter->SetTransform(tableTopModelTransform);
  tableTopTransformFilter->Update();
  tableTopModelPolyData = vtkPolyData::SafeDownCast(tableTopTransformFilter->GetOutput());

  vtkPolyData* inputs[] = { gantryModelPolyData, collimatorModelPolyData, leftImagingPanelModelPolyData, rightImagingPanelModelPolyData, patientSupportModelPolyData, tableTopModelPolyData};
  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();
  append->ExecuteAppend(output,inputs,6);

  // Get or create orientation marker model node
  vtkSmartPointer<vtkMRMLModelNode> orientationMarkerModel =
    vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(ORIENTATION_MARKER_MODEL_NODE_NAME));
  if (!orientationMarkerModel)
  {
    orientationMarkerModel = vtkSmartPointer<vtkMRMLModelNode>::New();
    orientationMarkerModel->SetName(ORIENTATION_MARKER_MODEL_NODE_NAME);
    this->GetMRMLScene()->AddNode(orientationMarkerModel);
  }
  orientationMarkerModel->SetAndObservePolyData(output);

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

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
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

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
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
  vtkSmartPointer<vtkTransform> rotatedLeftImagingPanelToLeftImagingPanelTransform = vtkSmartPointer<vtkTransform>::New();
  if (panelMovement > 0)
  {
    double translationArray[3] = { 0, -(panelMovement), 0 };
    rotatedLeftImagingPanelToLeftImagingPanelTransform->Translate(translationArray);
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
  leftImagingPanelToGantryTransform->Concatenate(rotatedLeftImagingPanelToLeftImagingPanelTransform);
  leftImagingPanelToGantryTransform->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME));
  if (!rightImagingPanelModel)
  {
    vtkErrorMacro("UpdateRightImagingPanelToGantryTransform: Invalid MRML model node");
  }

  // Translation to origin for in-place rotation
  vtkPolyData* rightImagingPanelModelPolyData = rightImagingPanelModel->GetPolyData();

  double rightImagingPanelModelBounds[6] = { 0, 0, 0, 0, 0, 0 };
  rightImagingPanelModelPolyData->GetBounds(rightImagingPanelModelBounds);

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
  vtkSmartPointer<vtkTransform> rotatedRightImagingPanelToRightImagingPanelTransform = vtkSmartPointer<vtkTransform>::New();
  if (panelMovement > 0)
  {
    double translationArray[3] = { 0, -(panelMovement), 0 };
    rotatedRightImagingPanelToRightImagingPanelTransform->Translate(translationArray);
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
  rightImagingPanelToGantryTransform->Concatenate(rotatedRightImagingPanelToRightImagingPanelTransform);
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
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdatePatientSupportToFixedReferenceTransform: Invalid parameter set node");
    return;
  }

  double rotationAngle = parameterNode->GetPatientSupportRotationAngle();

  vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupport, vtkSlicerIECTransformLogic::FixedReference);
  vtkTransform* patientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(
    patientSupportToFixedReferenceTransformNode->GetTransformToParent() );

  patientSupportToFixedReferenceTransform->Identity();
  patientSupportToFixedReferenceTransform->RotateZ(rotationAngle);
  patientSupportToFixedReferenceTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTopEccentricRotated, vtkSlicerIECTransformLogic::PatientSupport);
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateTableTopEccentricRotationToPatientSupportTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateTableTopEccentricRotationToPatientSupportTransform: Invalid parameter set node");
    return;
  }

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTopEccentricRotated, vtkSlicerIECTransformLogic::PatientSupport);
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransform->Identity();
  double translationArray[3] =
    { parameterNode->GetLateralTableTopDisplacement(), parameterNode->GetLongitudinalTableTopDisplacement(), parameterNode->GetVerticalTableTopDisplacement() };
  tableTopEccentricRotationToPatientSupportTransform->Translate(translationArray);
  tableTopEccentricRotationToPatientSupportTransform->Modified();

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportScaledTranslatedToTableTopVerticalTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  if (!patientSupportModel)
  {
    vtkErrorMacro("patientSupportModel: Invalid MRML model node");
  }

  vtkPolyData* patientSupportModelPolyData = patientSupportModel->GetPolyData();

  double patientSupportModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  patientSupportModelPolyData->GetBounds(patientSupportModelBounds);

  //TODO: This method does not use any input
  vtkMRMLLinearTransformNode* patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupportScaledTranslated, vtkSlicerIECTransformLogic::PatientSupport);
  vtkTransform* patientSupportScaledTranslatedToTableTopVerticalTranslationTransform = vtkTransform::SafeDownCast(
    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->GetTransformToParent() );

  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Identity();
  double translationArray[3] = { 0, 0, patientSupportModelBounds[4]};
  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Translate(translationArray);
  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTopEccentricRotated, vtkSlicerIECTransformLogic::PatientSupport);
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportScaledByTableTopVerticalMovementTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdatePatientSupportScaledByTableTopVerticalMovementTransform: Invalid parameter set node");
    return;
  }

  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  if (!patientSupportModel)
  {
    vtkErrorMacro("patientSupportModel: Invalid MRML model node");
  }

  vtkPolyData* patientSupportModelPolyData = patientSupportModel->GetPolyData();

  double patientSupportModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  patientSupportModelPolyData->GetBounds(patientSupportModelBounds);
  double scaleFactor = abs((patientSupportModelBounds[4] + patientSupportModelBounds[5]) / 2);

  double tableTopDisplacement = parameterNode->GetVerticalTableTopDisplacement();

  vtkMRMLLinearTransformNode* patientSupportScaledByTableTopVerticalMovementTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupportScaled, vtkSlicerIECTransformLogic::PatientSupportScaledTranslated);
  vtkTransform* patientSupportScaledByTableTopVerticalMovementTransform = vtkTransform::SafeDownCast(
    patientSupportScaledByTableTopVerticalMovementTransformNode->GetTransformToParent() );

  patientSupportScaledByTableTopVerticalMovementTransform->Identity();
  patientSupportScaledByTableTopVerticalMovementTransform->Scale(1, 1, ((abs(patientSupportModelBounds[5]) + tableTopDisplacement*0.525) / abs(patientSupportModelBounds[5])));
  patientSupportScaledByTableTopVerticalMovementTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTopEccentricRotated, vtkSlicerIECTransformLogic::PatientSupport);
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportPositiveVerticalTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdatePatientSupportScaledByTableTopVerticalMovementTransform: Invalid parameter set node");
    return;
  }

  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  if (!patientSupportModel)
  {
    vtkErrorMacro("patientSupportModel: Invalid MRML model node");
  }

  vtkPolyData* patientSupportModelPolyData = patientSupportModel->GetPolyData();

  double patientSupportModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  patientSupportModelPolyData->GetBounds(patientSupportModelBounds);

  //TODO: This method does not use any input
  vtkMRMLLinearTransformNode* patientSupportPositiveVerticalTranslationTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::PatientSupportPositiveVerticalTranslated, vtkSlicerIECTransformLogic::PatientSupportScaled);
  vtkTransform* patientSupportPositiveVerticalTranslationTransform = vtkTransform::SafeDownCast(
    patientSupportPositiveVerticalTranslationTransformNode->GetTransformToParent() );

  patientSupportPositiveVerticalTranslationTransform->Identity();
  double translationArray[3] = { 0, 0, patientSupportModelBounds[4]*-1};
  patientSupportPositiveVerticalTranslationTransform->Translate(translationArray);
  patientSupportPositiveVerticalTranslationTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::TableTopEccentricRotated, vtkSlicerIECTransformLogic::PatientSupport);
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateVerticalDisplacementTransforms(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateVerticalDisplacementTransforms: Invalid parameter set node");
    return;
  }

  this->UpdateTableTopEccentricRotationToPatientSupportTransform(parameterNode);
  this->UpdatePatientSupportScaledTranslatedToTableTopVerticalTranslationTransform(parameterNode);
  this->UpdatePatientSupportScaledByTableTopVerticalMovementTransform(parameterNode);
  this->UpdatePatientSupportPositiveVerticalTranslationTransform(parameterNode);
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

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->IECLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry);
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(
    collimatorToGantryTransformNode->GetTransformToParent() );

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
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

  // If number of contacts between pieces of treatment room is greater than 0, the collision between which pieces
  // will be set to the output string and returned by the function.
  this->GantryTableTopCollisionDetection->Update();
  if (this->GantryTableTopCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between gantry and table top\n";
  }

  this->GantryPatientSupportCollisionDetection->Update();
  if (this->GantryPatientSupportCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between gantry and patient support\n";
  }

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
    this->GantryPatientCollisionDetection->Update();
    if (this->GantryPatientCollisionDetection->GetNumberOfContacts() > 0)
    {
      statusString = statusString + "Collision between gantry and patient\n";
    }

    this->CollimatorPatientCollisionDetection->SetInput(1, patientBodyPolyData);
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
