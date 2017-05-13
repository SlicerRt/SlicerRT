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

// Room's Eye View includes
#include "qSlicerRoomsEyeViewModuleWidget.h"
#include "ui_qSlicerRoomsEyeViewModule.h"

#include "vtkSlicerRoomsEyeViewModuleLogic.h"
#include "vtkMRMLRoomsEyeViewNode.h"

// Slicer includes
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qSlicerIOManager.h>
#include <qSlicerDataDialog.h>
#include <qSlicerSaveDataDialog.h>
#include <qMRMLSliceWidget.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLThreeDView.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLCameraNode.h>
#include <vtkMRMLRTBeamNode.h>
#include <vtkMRMLViewNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLTransformNode.h>

// Qt includes
#include <QDebug>

// CTK includes
#include <ctkSliderWidget.h>

// VTK includes
#include <vtkCamera.h>
#include <vtkPolyData.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RoomsEyeView
class qSlicerRoomsEyeViewModuleWidgetPrivate : public Ui_qSlicerRoomsEyeViewModule
{
  Q_DECLARE_PUBLIC(qSlicerRoomsEyeViewModuleWidget);
protected:
  qSlicerRoomsEyeViewModuleWidget* const q_ptr;
public:
  qSlicerRoomsEyeViewModuleWidgetPrivate(qSlicerRoomsEyeViewModuleWidget& object) : q_ptr(&object) { };
  ~qSlicerRoomsEyeViewModuleWidgetPrivate() { };
  vtkSmartPointer<vtkSlicerRoomsEyeViewModuleLogic> logic() const;

  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkSlicerRoomsEyeViewModuleLogic> qSlicerRoomsEyeViewModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerRoomsEyeViewModuleWidget);
  return vtkSlicerRoomsEyeViewModuleLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerRoomsEyeViewModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerRoomsEyeViewModuleWidget::qSlicerRoomsEyeViewModuleWidget(QWidget* _parent)
: Superclass(_parent)
, d_ptr(new qSlicerRoomsEyeViewModuleWidgetPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerRoomsEyeViewModuleWidget::~qSlicerRoomsEyeViewModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  this->Superclass::setMRMLScene(scene);
  qvtkReconnect(d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()));

  // Find parameters node or create it if there is none in the scene
  if (scene && d->MRMLNodeComboBox_ParameterSet->currentNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLRoomsEyeViewNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else
    {
      vtkSmartPointer<vtkMRMLRoomsEyeViewNode> newNode = vtkSmartPointer<vtkMRMLRoomsEyeViewNode>::New();
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  Q_D(qSlicerRoomsEyeViewModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic!";
    return;
  }

  d->ModuleWindowInitialized = true;
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(node);
  //vtkMRMLModelNode* additionalModelNode = vtkMRMLModelNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
  //d->MRMLNodeComboBox_AddtionalTreatmentModels_->setCurrentNode(additionalModelNode);

  // Each time the node is modified, the UI widgets are updated
  qvtkReconnect(paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
  
  // Set selected MRML nodes in comboboxes in the parameter set if it was NULL there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (paramNode)
  {
    if (!paramNode->GetPatientBodySegmentationNode())
    {
      paramNode->SetAndObservePatientBodySegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(d->SegmentSelectorWidget->currentNode()));
    }
    if (!paramNode->GetPatientBodySegmentID() && !d->SegmentSelectorWidget->currentSegmentID().isEmpty())
    {
      paramNode->SetPatientBodySegmentID(d->SegmentSelectorWidget->currentSegmentID().toLatin1().constData());
    }

    paramNode->SetApplicatorHolderVisibility(0);
    paramNode->SetElectronApplicatorVisibility(0);
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  if (paramNode && this->mrmlScene())
  {
    if (paramNode->GetPatientBodySegmentationNode())
    {
      d->SegmentSelectorWidget->setCurrentNode(paramNode->GetPatientBodySegmentationNode());
    }
    if (paramNode->GetPatientBodySegmentID())
    {
      d->SegmentSelectorWidget->setCurrentSegmentID(paramNode->GetPatientBodySegmentID());
    }

    d->GantryRotationSlider->setValue(paramNode->GetGantryRotationAngle());
    d->CollimatorRotationSlider->setValue(paramNode->GetCollimatorRotationAngle());
    d->PatientSupportRotationSlider->setValue(paramNode->GetPatientSupportRotationAngle());
    d->ImagingPanelMovementSlider->setValue(paramNode->GetImagingPanelMovement());
    d->VerticalTableTopDisplacementSlider->setValue(paramNode->GetVerticalTableTopDisplacement());
    d->LateralTableTopDisplacementSlider->setValue(paramNode->GetLateralTableTopDisplacement());
    d->LongitudinalTableTopDisplacementSlider->setValue(paramNode->GetLongitudinalTableTopDisplacement());
    d->VerticalTranslationSliderWidget->setValue(paramNode->GetAdditionalModelVerticalDisplacement());
    d->LongitudinalTranslationSliderWidget->setValue(paramNode->GetAdditionalModelLongitudinalDisplacement());
    d->LateralTranslationSliderWidget->setValue(paramNode->GetAdditionalModelLateralDisplacement());
    d->applicatorHolderCheckBox->setChecked(paramNode->GetApplicatorHolderVisibility());
    d->electronApplicatorCheckBox->setChecked(paramNode->GetElectronApplicatorVisibility());
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setup()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->setupUi(this);

  this->Superclass::setup();

  this->setMRMLScene(this->mrmlScene());

  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  ioManager->registerDialog(new qSlicerDataDialog(this));
  ioManager->registerDialog(new qSlicerSaveDataDialog(this));


  connect(d->loadModelButton, SIGNAL(clicked()), this, SLOT(loadModelButtonClicked()));
  //TODO: Rename function to onSomethingChanged, and have them use the passed argument
  connect(d->applicatorHolderCheckBox, SIGNAL(stateChanged(int)), this, SLOT(additionalCollimatorDevicesChecked()));
  connect(d->electronApplicatorCheckBox, SIGNAL(stateChanged(int)), this, SLOT(additionalCollimatorDevicesChecked()));
  connect(d->CollimatorRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(collimatorRotationSliderValueChanged()));
  connect(d->GantryRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(gantryRotationSliderValueChanged()));
  connect(d->ImagingPanelMovementSlider, SIGNAL(valueChanged(double)), this, SLOT(imagingPanelMovementSliderValueChanged()));
  connect(d->PatientSupportRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(patientSupportRotationSliderValueChanged()));
  connect(d->VerticalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(verticalTableTopDisplacementSliderValueChanged()));
  connect(d->LongitudinalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(longitudinalTableTopDisplacementSliderValueChanged()));
  connect(d->LateralTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(lateralTableTopDisplacementSliderValueChanged()));

  connect(d->loadAdditionalDevicesButton, SIGNAL(clicked()), this, SLOT(loadAdditionalDevicesButtonClicked()));
  connect(d->LateralTranslationSliderWidget, SIGNAL(valueChanged(double)), this, SLOT(additionalModelLateralDisplacementSliderValueChanged()));
  connect(d->LongitudinalTranslationSliderWidget, SIGNAL(valueChanged(double)), this, SLOT(additionalModelLongitudinalDisplacementSliderValueChanged()));
  connect(d->VerticalTranslationSliderWidget, SIGNAL(valueChanged(double)), this, SLOT(additionalModelVerticalDisplacementSliderValueChanged()));

  
  connect(d->beamsEyeViewButton, SIGNAL(clicked()), this, SLOT(beamsEyeViewButtonClicked()));

  connect(d->SegmentSelectorWidget, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(patientBodySegmentationNodeChanged(vtkMRMLNode*)));
  connect(d->SegmentSelectorWidget, SIGNAL(currentSegmentChanged(QString)), this, SLOT(patientBodySegmentChanged(QString)));

  // Handle scene change event if occurs
  qvtkConnect(d->logic(), vtkCommand::ModifiedEvent, this, SLOT(onLogicModified()));
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::loadModelButtonClicked()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  // Load and setup models
  d->logic()->LoadLinacModels();

  // Reset camera
  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  threeDView->resetCamera();

  // Set orientation marker
  //TODO: Add new option 'Treatment room' to orientation marker choices and merged model with actual colors (surface scalars?)
  //vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();
  //viewNode->SetOrientationMarkerHumanModelNodeID(this->mrmlScene()->GetFirstNodeByName("EBRTOrientationMarkerModel")->GetID());
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::loadAdditionalDevicesButtonClicked()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  d->logic()->LoadAdditionalDevices();

  //TODO: Add function UpdateTreatmentOrientationMarker that merges the treatment machine components into a model that can be set as orientation marker,
  //TODO: Add new option 'Treatment room' to orientation marker choices and merged model with actual colors (surface scalars?)
  /**qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();
  viewNode->SetOrientationMarkerHumanModelNodeID(this->mrmlScene()->GetFirstNodeByName("EBRTOrientationMarkerModel")->GetID());**/
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::additionalCollimatorDevicesChecked(){
  
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }
  paramNode->SetApplicatorHolderVisibility(d->applicatorHolderCheckBox->isChecked());
  paramNode->SetElectronApplicatorVisibility(d->electronApplicatorCheckBox->isChecked());
  d->logic()->UpdateAdditionalDevicesVisibility(paramNode);
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::patientBodySegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObservePatientBodySegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::patientBodySegmentChanged(QString segmentID)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || segmentID.isEmpty() || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetPatientBodySegmentID(segmentID.toLatin1().constData());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::gantryRotationSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetGantryRotationAngle(d->GantryRotationSlider->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateGantryToFixedReferenceTransform(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::imagingPanelMovementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetImagingPanelMovement(d->ImagingPanelMovementSlider->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateImagingPanelMovementTransforms(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::collimatorRotationSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetCollimatorRotationAngle(d->CollimatorRotationSlider->value());
  paramNode->DisableModifiedEventOff();
  
  d->logic()->UpdateCollimatorToGantryTransform(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::patientSupportRotationSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetPatientSupportRotationAngle(d->PatientSupportRotationSlider->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdatePatientSupportRotationToFixedReferenceTransform(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::verticalTableTopDisplacementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetVerticalTableTopDisplacement(d->VerticalTableTopDisplacementSlider->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdatePatientSupportToPatientSupportRotationTransform(paramNode);
  d->logic()->UpdateTableTopToTableTopEccentricRotationTransform(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::longitudinalTableTopDisplacementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetLongitudinalTableTopDisplacement(d->LongitudinalTableTopDisplacementSlider->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateTableTopToTableTopEccentricRotationTransform(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::lateralTableTopDisplacementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetLateralTableTopDisplacement(d->LateralTableTopDisplacementSlider->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateTableTopToTableTopEccentricRotationTransform(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::additionalModelLateralDisplacementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAdditionalModelLateralDisplacement(d->LateralTranslationSliderWidget->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateAdditionalCollimatorDevicesToCollimatorTransforms(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::additionalModelLongitudinalDisplacementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAdditionalModelLongitudinalDisplacement(d->LongitudinalTranslationSliderWidget->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateAdditionalCollimatorDevicesToCollimatorTransforms(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::additionalModelVerticalDisplacementSliderValueChanged()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAdditionalModelVerticalDisplacement(d->VerticalTranslationSliderWidget->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateAdditionalCollimatorDevicesToCollimatorTransforms(paramNode);

  this->checkForCollisions();
  d->logic()->UpdateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::checkForCollisions()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  std::string collisionString = d->logic()->CheckForCollisions(paramNode);

  if (collisionString.length() > 0)
  {
    d->CollisionsDetected->setText(QString::fromStdString(collisionString));
    d->CollisionsDetected->setStyleSheet("color: red");
  }
  else
  {
    d->CollisionsDetected->setText(QString::fromStdString("No collisions detected"));
    d->CollisionsDetected->setStyleSheet("color: green");
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::beamsEyeViewButtonClicked()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  //qMRMLSliceWidget* redSliceWidget = layoutManager->sliceWidget("Red");
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();
  vtkCamera* beamsEyeCamera = vtkSmartPointer<vtkCamera>::New();
  
  vtkCollection* cameras = this->mrmlScene()->GetNodesByClass("vtkMRMLCameraNode");
  vtkMRMLCameraNode* cameraNode;
  for (int i = 0; i < cameras->GetNumberOfItems(); i++){
    cameraNode = vtkMRMLCameraNode::SafeDownCast(cameras->GetItemAsObject(i));
    if (cameraNode->GetActiveTag() == viewNode->GetID()){
        break;
    }
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(this->mrmlScene()->GetFirstNodeByName("NewBeam_7"));
  double sourcePosition[3];
  double isocenter[3];

  if (beamNode->CalculateSourcePosition(sourcePosition))
  {
    vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetFirstNodeByName("CollimatorModel"));
    vtkPolyData* collimatorModelPolyData = collimatorModel->GetPolyData();

    double collimatorCenterOfRotation[3];
    double collimatorModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

    collimatorModelPolyData->GetBounds(collimatorModelBounds);
    collimatorCenterOfRotation[0] = (collimatorModelBounds[0] + collimatorModelBounds[1]) / 2;
    collimatorCenterOfRotation[1] = (collimatorModelBounds[2] + collimatorModelBounds[3]) / 2;
    collimatorCenterOfRotation[2] = collimatorModelBounds[4];
    double newSourcePosition[3];

    //TODO: Determine a way to properly transform source position
    // Currently just using the beam source position does not work to create a proper BEV because the function CalculateSourcePosition in vtkMRMLRTBeamNode
    // does not account beam transformations. It currently takes the isocenter, SAD, gantry angle, and transforms the point in the RA plane. The beam ends up being
    // transformed and oriented vertically that requires the source position to be transformed in the RS plane. Upon trying to alter the CalculateSourcePosition to do this,
    // the transformation was unsuccessful. This will need to be investigated. This for the interim I have elected to use the center of the collimator's bottom face as the position
    // of the camera.
    //d->logic()->CalculateNewSourcePosition(beamNode,sourcePosition, newSourcePosition);
    cameraNode->GetCamera()->SetPosition(collimatorCenterOfRotation);
    if (beamNode->GetPlanIsocenterPosition(isocenter)){
      cameraNode->GetCamera()->SetFocalPoint(isocenter);
    }
  }
  
  cameraNode->GetCamera()->Elevation(-(d->GantryRotationSlider->value()));

  //TODO: Oblique slice updating real-time based on beam geometry
  //vtkMRMLSliceNode* redSliceNode = redSliceWidget->mrmlSliceNode();
  //redSliceNode->SetSliceVisible(1);

  //TODO: Camera roll also needs to be set to keep the field of view aligned with the beam's field
  //redSliceNode->SetWidgetNormalLockedToCamera(cameraNode->GetCamera()->GetID);
}
