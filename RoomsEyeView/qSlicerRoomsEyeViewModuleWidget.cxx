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

// Beams includes
#include "vtkSlicerIECTransformLogic.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

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
#include <vtkMRMLViewNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// Qt includes
#include <QDebug>

// CTK includes
#include <ctkSliderWidget.h>

// VTK includes
#include <vtkCamera.h>
#include <vtkPolyData.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RoomsEyeView
class qSlicerRoomsEyeViewModuleWidgetPrivate : public Ui_qSlicerRoomsEyeViewModule
{
  Q_DECLARE_PUBLIC(qSlicerRoomsEyeViewModuleWidget);
protected:
  qSlicerRoomsEyeViewModuleWidget* const q_ptr;
public:
  qSlicerRoomsEyeViewModuleWidgetPrivate(qSlicerRoomsEyeViewModuleWidget& object);
  ~qSlicerRoomsEyeViewModuleWidgetPrivate()  = default;
  vtkSmartPointer<vtkSlicerRoomsEyeViewModuleLogic> logic() const;

  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerRoomsEyeViewModuleWidgetPrivate::qSlicerRoomsEyeViewModuleWidgetPrivate(qSlicerRoomsEyeViewModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
}

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
qSlicerRoomsEyeViewModuleWidget::~qSlicerRoomsEyeViewModuleWidget() = default;

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  this->Superclass::setMRMLScene(scene);
  qvtkReconnect(d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()));
  qvtkReconnect(d->logic(), scene, vtkMRMLScene::EndCloseEvent, this, SLOT(onSceneClosedEvent()));

  // Find parameters node or create it if there is none in the scene
  if (scene)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLRoomsEyeViewNode");
    if (d->MRMLNodeComboBox_ParameterSet->currentNode())
    {
      this->setParameterNode(d->MRMLNodeComboBox_ParameterSet->currentNode());
    }
    else if (node)
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
void qSlicerRoomsEyeViewModuleWidget::onSceneClosedEvent()
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
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  Q_D(qSlicerRoomsEyeViewModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic";
    return;
  }

  // Select or create parameter node
  this->setMRMLScene(this->mrmlScene());

  d->ModuleWindowInitialized = true;
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

  // Each time the node is modified, the UI widgets are updated
  qvtkReconnect(paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
  
  // Set selected MRML nodes in comboboxes in the parameter set if it was nullptr there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (paramNode)
  {
    if (!paramNode->GetBeamNode())
    {
      paramNode->SetAndObserveBeamNode(vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_Beam->currentNode()));
    }
    if (!paramNode->GetPatientBodySegmentationNode())
    {
      paramNode->SetAndObservePatientBodySegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(d->SegmentSelectorWidget_PatientBody->currentNode()));
    }
    if (!paramNode->GetPatientBodySegmentID() && !d->SegmentSelectorWidget_PatientBody->currentSegmentID().isEmpty())
    {
      paramNode->SetPatientBodySegmentID(d->SegmentSelectorWidget_PatientBody->currentSegmentID().toUtf8().constData());
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
    if (paramNode->GetBeamNode())
    {
      d->MRMLNodeComboBox_Beam->setCurrentNode(paramNode->GetBeamNode());
    }
    if (paramNode->GetPatientBodySegmentationNode())
    {
      d->SegmentSelectorWidget_PatientBody->setCurrentNode(paramNode->GetPatientBodySegmentationNode());
    }
    if (paramNode->GetPatientBodySegmentID())
    {
      d->SegmentSelectorWidget_PatientBody->setCurrentSegmentID(paramNode->GetPatientBodySegmentID());
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
    d->ApplicatorHolderCheckBox->setChecked(paramNode->GetApplicatorHolderVisibility());
    d->ElectronApplicatorCheckBox->setChecked(paramNode->GetElectronApplicatorVisibility());
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

  // Add treatment machine options
  d->TreatmentMachineComboBox->addItem("Varian TrueBeam STx", "VarianTrueBeamSTx");
  d->TreatmentMachineComboBox->addItem("Siemens Artiste", "SiemensArtiste");

  //
  // Make connections
  connect(d->LoadTreatmentMachineModelsButton, SIGNAL(clicked()), this, SLOT(onLoadTreatmentMachineModelsButtonClicked()));

  // Treatment machine components
  connect(d->CollimatorRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(onCollimatorRotationSliderValueChanged(double)));
  connect(d->GantryRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(onGantryRotationSliderValueChanged(double)));
  connect(d->ImagingPanelMovementSlider, SIGNAL(valueChanged(double)), this, SLOT(onImagingPanelMovementSliderValueChanged(double)));
  connect(d->PatientSupportRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(onPatientSupportRotationSliderValueChanged(double)));
  connect(d->VerticalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(onVerticalTableTopDisplacementSliderValueChanged(double)));
  connect(d->LongitudinalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(onLongitudinalTableTopDisplacementSliderValueChanged(double)));
  connect(d->LateralTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(onLateralTableTopDisplacementSliderValueChanged(double)));

  // Additional devices
  connect(d->LoadBasicCollimatorMountedDeviceButton, SIGNAL(clicked()), this, SLOT(onLoadBasicCollimatorMountedDeviceButtonClicked()));
  connect(d->LoadCustomCollimatorMountedDeviceButton, SIGNAL(clicked()), this, SLOT(onLoadCustomCollimatorMountedDeviceButtonClicked()));
  //TODO:
  //connect(d->ApplicatorHolderCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onAdditionalCollimatorMountedDevicesChecked(int)));
  //connect(d->ElectronApplicatorCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onAdditionalCollimatorMountedDevicesChecked(int)));
  //TODO: BrainLabSRSCheckBox
  connect(d->LateralTranslationSliderWidget, SIGNAL(valueChanged(double)), this, SLOT(onAdditionalModelLateralDisplacementSliderValueChanged(double)));
  connect(d->LongitudinalTranslationSliderWidget, SIGNAL(valueChanged(double)), this, SLOT(onAdditionalModelLongitudinalDisplacementSliderValueChanged(double)));
  connect(d->VerticalTranslationSliderWidget, SIGNAL(valueChanged(double)), this, SLOT(onAdditionalModelVerticalDisplacementSliderValueChanged(double)));
  
  connect(d->BeamsEyeViewButton, SIGNAL(clicked()), this, SLOT(onBeamsEyeViewButtonClicked()));

  connect(d->MRMLNodeComboBox_Beam, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onBeamNodeChanged(vtkMRMLNode*)));
  connect(d->SegmentSelectorWidget_PatientBody, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onPatientBodySegmentationNodeChanged(vtkMRMLNode*)));
  connect(d->SegmentSelectorWidget_PatientBody, SIGNAL(currentSegmentChanged(QString)), this, SLOT(onPatientBodySegmentChanged(QString)));

  // Handle scene change event if occurs
  qvtkConnect(d->logic(), vtkCommand::ModifiedEvent, this, SLOT(onLogicModified()));
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onBeamNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy";
    return;
  }

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(node);
  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveBeamNode(beamNode);
  paramNode->DisableModifiedEventOff();

  // Trigger update of transforms based on selected beam
  beamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);

  // Show only selected beam, hide others
  std::vector<vtkMRMLNode*> beamNodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLRTBeamNode", beamNodes);
  for (std::vector<vtkMRMLNode*>::iterator beamIt=beamNodes.begin(); beamIt!=beamNodes.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* currentBeamNode = vtkMRMLRTBeamNode::SafeDownCast(*beamIt);
    shNode->SetDisplayVisibilityForBranch(
      shNode->GetItemByDataNode(currentBeamNode), (currentBeamNode==beamNode ? 1 : 0) );
  }

  // Select patient segmentation
  vtkMRMLRTPlanNode* planNode = beamNode->GetParentPlanNode();
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access parent plan of beam " << beamNode->GetName();
    return;
  }
  d->SegmentSelectorWidget_PatientBody->setCurrentNode(planNode->GetSegmentationNode());
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onPatientBodySegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy";
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

  // Show only selected patient segmentation
  std::vector<vtkMRMLNode*> segmentationNodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLSegmentationNode", segmentationNodes);
  for (std::vector<vtkMRMLNode*>::iterator segIt=segmentationNodes.begin(); segIt!=segmentationNodes.end(); ++segIt)
  {
    vtkMRMLSegmentationNode* currentSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(*segIt);
    shNode->SetDisplayVisibilityForBranch(
      shNode->GetItemByDataNode(currentSegmentationNode), (currentSegmentationNode==node ? 1 : 0) );
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onPatientBodySegmentChanged(QString segmentID)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || segmentID.isEmpty() || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetPatientBodySegmentID(segmentID.toUtf8().constData());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLoadTreatmentMachineModelsButtonClicked()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  // Load and setup models
  QString treatmentMachineType(d->TreatmentMachineComboBox->currentData().toString());
  paramNode->SetTreatmentMachineType(treatmentMachineType.toUtf8().constData());
  d->logic()->LoadTreatmentMachineModels(paramNode);

  // Reset camera
  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  threeDView->resetCamera();

  // Set treatment machine dependent properties
  if (!treatmentMachineType.compare("VarianTrueBeamSTx"))
  {
    d->LateralTableTopDisplacementSlider->setMinimum(-230.0);
    d->LateralTableTopDisplacementSlider->setMaximum(230.0);
  }
  else if (!treatmentMachineType.compare("SiemensArtiste"))
  {
    d->LateralTableTopDisplacementSlider->setMinimum(-250.0);
    d->LateralTableTopDisplacementSlider->setMaximum(250.0);
  }

  // Set orientation marker
  //TODO: Add new option 'Treatment room' to orientation marker choices and merged model with actual colors (surface scalars?)
  //vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();
  //viewNode->SetOrientationMarkerHumanModelNodeID(this->mrmlScene()->GetFirstNodeByName("EBRTOrientationMarkerModel")->GetID());
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onCollimatorRotationSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetCollimatorRotationAngle(value);
  paramNode->DisableModifiedEventOff();
  
  // Update IEC transform
  d->logic()->UpdateCollimatorToGantryTransform(paramNode);

  // Update beam parameter
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(paramNode->GetBeamNode());
  if (beamNode)
  {
    beamNode->SetCollimatorAngle(value);
  }

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onGantryRotationSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetGantryRotationAngle(value);
  paramNode->DisableModifiedEventOff();

  // Update IEC transform
  d->logic()->UpdateGantryToFixedReferenceTransform(paramNode);

  // Update beam parameter
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(paramNode->GetBeamNode());
  if (beamNode)
  {
    beamNode->SetGantryAngle(value);
  }

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onImagingPanelMovementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetImagingPanelMovement(value);
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateImagingPanelMovementTransforms(paramNode);

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onPatientSupportRotationSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetPatientSupportRotationAngle(value);
  paramNode->DisableModifiedEventOff();

  // Update IEC transform
  d->logic()->UpdatePatientSupportRotationToFixedReferenceTransform(paramNode);

  // Update beam parameter
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(paramNode->GetBeamNode());
  if (beamNode)
  {
    beamNode->SetCouchAngle(value);
  }

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onVerticalTableTopDisplacementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetVerticalTableTopDisplacement(value);
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdatePatientSupportToPatientSupportRotationTransform(paramNode);
  d->logic()->UpdateTableTopToTableTopEccentricRotationTransform(paramNode);

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLongitudinalTableTopDisplacementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetLongitudinalTableTopDisplacement(value);
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateTableTopToTableTopEccentricRotationTransform(paramNode);

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLateralTableTopDisplacementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  Q_UNUSED(value);

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
  this->updateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLoadBasicCollimatorMountedDeviceButtonClicked()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  d->logic()->LoadBasicCollimatorMountedDevices();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLoadCustomCollimatorMountedDeviceButtonClicked()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  vtkSmartPointer<vtkCollection> loadedModelsCollection = vtkSmartPointer<vtkCollection>::New();
  ioManager->openDialog("ModelFile", qSlicerDataDialog::Read, qSlicerIO::IOProperties(), loadedModelsCollection);
  
  for (int modelIndex=0; modelIndex<loadedModelsCollection->GetNumberOfItems(); ++modelIndex)
  {
    vtkMRMLModelNode* loadedModelNode = vtkMRMLModelNode::SafeDownCast(
      loadedModelsCollection->GetItemAsObject(modelIndex) );
    vtkMRMLLinearTransformNode* collimatorModelTransforms = d->logic()->GetIECLogic()->GetTransformNodeBetween(
      vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry );
    loadedModelNode->SetAndObserveTransformNodeID(collimatorModelTransforms->GetID());
  }

  //TODO: Add function UpdateTreatmentOrientationMarker that merges the treatment machine components into a model that can be set as orientation marker,
  //TODO: Add new option 'Treatment room' to orientation marker choices and merged model with actual colors (surface scalars?)
  /**qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();
  viewNode->SetOrientationMarkerHumanModelNodeID(this->mrmlScene()->GetFirstNodeByName("EBRTOrientationMarkerModel")->GetID());**/
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onAdditionalCollimatorMountedDevicesChecked(int state)
{
  
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }
  paramNode->SetApplicatorHolderVisibility(state);
  paramNode->SetElectronApplicatorVisibility(state);
  d->logic()->UpdateAdditionalDevicesVisibility(paramNode);
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onAdditionalModelLateralDisplacementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAdditionalModelLateralDisplacement(value);
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateAdditionalCollimatorDevicesToCollimatorTransforms(paramNode);

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onAdditionalModelLongitudinalDisplacementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAdditionalModelLongitudinalDisplacement(value);
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateAdditionalCollimatorDevicesToCollimatorTransforms(paramNode);

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
}
//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onAdditionalModelVerticalDisplacementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAdditionalModelVerticalDisplacement(value);
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateAdditionalCollimatorDevicesToCollimatorTransforms(paramNode);

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onBeamsEyeViewButtonClicked()
{
  //TODO: Move feature to beams module

  Q_D(qSlicerRoomsEyeViewModuleWidget);

  // Get 3D view node
  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();
  //vtkCamera* beamsEyeCamera = vtkSmartPointer<vtkCamera>::New();

  // Get camera node for view
  vtkCollection* cameras = this->mrmlScene()->GetNodesByClass("vtkMRMLCameraNode");
  vtkMRMLCameraNode* cameraNode = nullptr;
  for (int i = 0; i < cameras->GetNumberOfItems(); i++)
  {
    cameraNode = vtkMRMLCameraNode::SafeDownCast(cameras->GetItemAsObject(i));
    if (cameraNode->GetActiveTag() == viewNode->GetID())
    {
      break;
    }
  }
  if (!cameraNode)
  {
    qCritical() << Q_FUNC_INFO << "Failed to find camera for view " << (viewNode ? viewNode->GetID() : "(null)");
    cameras->Delete();
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_Beam->currentNode());
  double sourcePosition[3] = {0.0, 0.0, 0.0};
  double isocenter[3] = {0.0, 0.0, 0.0};

  if (beamNode && beamNode->GetSourcePosition(sourcePosition))
  {
    vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
    vtkTransform* beamTransform = nullptr;
    vtkNew<vtkMatrix4x4> mat;
    mat->Identity();

    if (beamTransformNode)
    {
      beamTransform = vtkTransform::SafeDownCast(beamTransformNode->GetTransformToParent());
      beamTransform->GetMatrix(mat);
    }
    else
    {
      qCritical() << Q_FUNC_INFO << "Beam transform node is invalid";
      cameras->Delete();
      return;
    }

    double viewUpVector[4] = { -1., 0., 0., 0. }; // beam negative X-axis
    double vup[4];
  
    mat->MultiplyPoint( viewUpVector, vup);
    //vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetFirstNodeByName("CollimatorModel"));
    //vtkPolyData* collimatorModelPolyData = collimatorModel->GetPolyData();

    //double collimatorCenterOfRotation[3] = {0.0, 0.0, 0.0};
    //double collimatorModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

    //collimatorModelPolyData->GetBounds(collimatorModelBounds);
    //collimatorCenterOfRotation[0] = (collimatorModelBounds[0] + collimatorModelBounds[1]) / 2;
    //collimatorCenterOfRotation[1] = (collimatorModelBounds[2] + collimatorModelBounds[3]) / 2;
    //collimatorCenterOfRotation[2] = collimatorModelBounds[4];

    //cameraNode->GetCamera()->SetPosition(collimatorCenterOfRotation);
    cameraNode->GetCamera()->SetPosition(sourcePosition);
    if (beamNode->GetPlanIsocenterPosition(isocenter))
    {
      cameraNode->GetCamera()->SetFocalPoint(isocenter);
    }
    cameraNode->SetViewUp(vup);
  }
  
  cameraNode->GetCamera()->Elevation(-(d->GantryRotationSlider->value()));
  cameras->Delete();

  //TODO: Oblique slice updating real-time based on beam geometry
  //vtkMRMLSliceNode* redSliceNode = redSliceWidget->mrmlSliceNode();
  //redSliceNode->SetSliceVisible(1);

  //TODO: Camera roll also needs to be set to keep the field of view aligned with the beam's field
  //redSliceNode->SetWidgetNormalLockedToCamera(cameraNode->GetCamera()->GetID);
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
void qSlicerRoomsEyeViewModuleWidget::updateTreatmentOrientationMarker()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  // Get 3D view node
  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();

  // Update orientation marker if shown
  if (viewNode->GetOrientationMarkerType() == vtkMRMLViewNode::OrientationMarkerTypeHuman)
  {  
    vtkMRMLModelNode* orientationMarkerModel = d->logic()->UpdateTreatmentOrientationMarker();
    if (!orientationMarkerModel)
    {
      qCritical() << Q_FUNC_INFO << ": Failed to create orientation marker model";
      return;
    }

    // Make sure the orientation marker has the right model node
    viewNode->SetOrientationMarkerHumanModelNodeID(orientationMarkerModel->GetID());
    viewNode->Modified();
  }
}
