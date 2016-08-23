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

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLSegmentationNode.h>

// Qt includes
#include <QDebug>

// CTK includes
#include <ctkSliderWidget.h>

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

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

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
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setup()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->setupUi(this);

  this->Superclass::setup();

  this->setMRMLScene(this->mrmlScene());

  connect(d->loadModelButton, SIGNAL(clicked()), this, SLOT(loadModelButtonClicked()));
  connect(d->CollimatorRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(collimatorRotationSliderValueChanged()));
  connect(d->GantryRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(gantryRotationSliderValueChanged()));
  connect(d->ImagingPanelMovementSlider, SIGNAL(valueChanged(double)), this, SLOT(imagingPanelMovementSliderValueChanged()));
  connect(d->PatientSupportRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(patientSupportRotationSliderValueChanged()));
  connect(d->VerticalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(verticalTableTopDisplacementSliderValueChanged()));
  connect(d->LongitudinalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(longitudinalTableTopDisplacementSliderValueChanged()));
  connect(d->LateralTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(lateralTableTopDisplacementSliderValueChanged()));

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

  d->logic()->LoadLinacModels();

  //TODO: Move this a more central place after integrating REV into EBP
  // Initialize logic
  d->logic()->InitializeIEC();
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

  d->logic()->UpdatePatientSupportToFixedReferenceTransform(paramNode);

  this->checkForCollisions();
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

  d->logic()->UpdateVerticalDisplacementTransforms(paramNode);

  this->checkForCollisions();
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

  d->logic()->UpdateTableTopEccentricRotationToPatientSupportTransform(paramNode);

  this->checkForCollisions();
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

  d->logic()->UpdateTableTopEccentricRotationToPatientSupportTransform(paramNode);

  this->checkForCollisions();
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
