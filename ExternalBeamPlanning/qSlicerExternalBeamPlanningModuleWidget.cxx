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

// SlicerQt includes
#include "qSlicerExternalBeamPlanningModuleWidget.h"
#include "ui_qSlicerExternalBeamPlanningModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// QSlicer includes
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h> 
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <qSlicerAbstractCoreModule.h>

// ExtensionTemplate Logic includes
#include <vtkSlicerCLIModuleLogic.h>

// SlicerRt includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLRTProtonBeamNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"
#include "vtkSlicerBeamsModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLLayoutNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QTime>
#include <QItemSelection>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerExternalBeamPlanningModuleWidgetPrivate: public Ui_qSlicerExternalBeamPlanningModule
{
  Q_DECLARE_PUBLIC(qSlicerExternalBeamPlanningModuleWidget);
protected:
  qSlicerExternalBeamPlanningModuleWidget* const q_ptr;
public:
  qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object);
  ~qSlicerExternalBeamPlanningModuleWidgetPrivate();
  vtkSlicerExternalBeamPlanningModuleLogic* logic() const;
public:
  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
}

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::~qSlicerExternalBeamPlanningModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic*
qSlicerExternalBeamPlanningModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerExternalBeamPlanningModuleWidget);
  return vtkSlicerExternalBeamPlanningModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidget::qSlicerExternalBeamPlanningModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerExternalBeamPlanningModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidget::~qSlicerExternalBeamPlanningModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find a plan node and select it if there is one in the scene
  if (scene && d->MRMLNodeComboBox_RtPlan->currentNode())
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
    if (node)
    {
      this->setPlanNode(node);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}
//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (d->logic() == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic!";
    return;
  }

  // Select RT plan node
  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (rtPlanNode == NULL)
  {
    // Try to find one in the scene
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
    if (node)
    {
      rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(node);
    }
  }

  // Update UI from plan node
  this->updateWidgetFromMRML();

  // This is not used?
  d->ModuleWindowInitialized = true;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setup()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Check for matlab dose calculation module //TODO: Re-enable when matlab dose adaptor is created
  //vtkSlicerExternalBeamPlanningModuleLogic* externalBeamPlanningModuleLogic =
  //  vtkSlicerExternalBeamPlanningModuleLogic::SafeDownCast(this->logic());
  //qSlicerAbstractCoreModule* matlabDoseCalculationModule =
  //  qSlicerCoreApplication::application()->moduleManager()->module("MatlabDoseCalculation");
  //if (matlabDoseCalculationModule)
  //{
  //  vtkSlicerCLIModuleLogic* matlabDoseCalculationModuleLogic =
  //    vtkSlicerCLIModuleLogic::SafeDownCast(matlabDoseCalculationModule->logic());
  //  externalBeamPlanningModuleLogic->SetMatlabDoseCalculationModuleLogic(matlabDoseCalculationModuleLogic);
  //}
  //else
  //{
  //  qWarning() << Q_FUNC_INFO << ": MatlabDoseCalculation module is not found!";
  //}
  
  // Make connections
  connect( d->MRMLNodeComboBox_RtPlan, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setPlanNode(vtkMRMLNode*)) );

  // Plan parameters section
  connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_PlanSegmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(planSegmentationNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_PlanPOIs, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(planPOIsNodeChanged(vtkMRMLNode*)) );
  connect( d->doubleSpinBox_RxDose, SIGNAL(valueChanged(double)), this, SLOT(rxDoseChanged(double)) );
  connect( d->comboBox_DoseEngineType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(doseEngineTypeChanged(const QString &)) );

  // Output section
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_DoseROI, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseROINodeChanged(vtkMRMLNode*)) );
  connect( d->lineEdit_DoseGridSpacing, SIGNAL(textChanged(const QString &)), this, SLOT(doseGridSpacingChanged(const QString &)) );

  // Beams section
  //connect( d->BeamsTableView, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(beamSelectionChanged(QItemSelection,QItemSelection) ) );
  connect( d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeamClicked()) );
  connect( d->pushButton_RemoveBeam, SIGNAL(clicked()), this, SLOT(removeBeamClicked()) );

  // Calculation buttons
  connect( d->pushButton_CalculateDose, SIGNAL(clicked()), this, SLOT(calculateDoseClicked()) );
  connect( d->pushButton_CalculateWED, SIGNAL(clicked()), this, SLOT(calculateWEDClicked()) );
  connect( d->pushButton_ClearDose, SIGNAL(clicked()), this, SLOT(clearDoseClicked()) );

  // Hide non-functional items //TODO:
  d->label_DoseROI->setVisible(false);
  d->MRMLNodeComboBox_DoseROI->setVisible(false);
  d->label_DoseGridSpacing->setVisible(false);
  d->lineEdit_DoseGridSpacing->setVisible(false);
  d->label_DosePoint->setVisible(false);
  d->MRMLCoordinatesWidget_DosePointCoordinates->setVisible(false);
  d->comboBox_DosePoint->setVisible(false);

  // Set status text to initial instruction
  d->label_CalculateDoseStatus->setText("Add plan and beam to start planning");

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());

  // Enable GUI only if a plan node is selected
  d->CollapsibleButton_PlanParameters->setEnabled(rtPlanNode);
  d->CollapsibleButton_Beams->setEnabled(rtPlanNode);
  
  if (!rtPlanNode)
  {
    return;
  }

  // None is enabled for the reference volume and segmentation comboboxes, and invalid selection
  // in plan node is set to GUI so that the user needs to select nodes that are then set to the beams.
  d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(rtPlanNode->GetReferenceVolumeNode());
  d->MRMLNodeComboBox_PlanSegmentation->setCurrentNode(rtPlanNode->GetSegmentationNode());
  if (rtPlanNode->GetPoisMarkupsFiducialNode())
  {
    d->MRMLNodeComboBox_PlanPOIs->setCurrentNode(rtPlanNode->GetPoisMarkupsFiducialNode());
  }
  d->doubleSpinBox_RxDose->setValue(rtPlanNode->GetRxDose());

  if (rtPlanNode->GetOutputTotalDoseVolumeNode())
  {
    d->MRMLNodeComboBox_DoseVolume->setCurrentNode(rtPlanNode->GetOutputTotalDoseVolumeNode());
  }

  double rdp[3] = {0.0,0.0,0.0};
  rtPlanNode->GetReferenceDosePoint(rdp);
  d->MRMLCoordinatesWidget_DosePointCoordinates->setCoordinates(rdp);

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setPlanNode(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(node);

  // Make sure the plan node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_RtPlan->setCurrentNode(rtPlanNode);

  // Set plan node in beams table
  d->BeamsTableView->setPlanNode(rtPlanNode);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect(rtPlanNode, vtkCommand::ModifiedEvent, this, SLOT(onRTPlanNodeModified()));

  // Create and select output dose volume if missing
  if (rtPlanNode && !rtPlanNode->GetOutputTotalDoseVolumeNode())
  {
    vtkSmartPointer<vtkMRMLScalarVolumeNode> newDoseVolume = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    std::string newDoseVolumeName = std::string(rtPlanNode->GetName()) + "_TotalDose";
    newDoseVolume->SetName(newDoseVolumeName.c_str());
    this->mrmlScene()->AddNode(newDoseVolume);
    rtPlanNode->SetAndObserveOutputTotalDoseVolumeNode(newDoseVolume);
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onRTPlanNodeModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onLogicModified()
{
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::planSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::planPOIsNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObservePoisMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();

  // Update beam transforms based on new isocenter
  std::vector<vtkMRMLRTBeamNode*> beams;
  rtPlanNode->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* beamNode = (*beamIt);
    d->logic()->GetBeamsLogic()->UpdateBeamTransform(beamNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rxDoseChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetRxDose(value);
  rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveOutputTotalDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseROINodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  qWarning() << Q_FUNC_INFO << ": Not implemented!";

  //TODO:
  // rtPlanNode->DisableModifiedEventOn();
  // rtPlanNode->SetAndObserveDoseROINode(vtkMRMLAnnotationsROINode::SafeDownCast(node));
  // rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseGridSpacingChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  qWarning() << Q_FUNC_INFO << ": Not implemented!";

  // TODO: to be implemented
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseEngineTypeChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  if (text.compare("Plastimatch") == 0)
  {
    rtPlanNode->SetDoseEngine(vtkMRMLRTPlanNode::Plastimatch);
  }
  else if (text.compare("PMH") == 0)
  {
    rtPlanNode->SetDoseEngine(vtkMRMLRTPlanNode::PMH);
  }
  else if (text.compare("Matlab") == 0)
  {
    rtPlanNode->SetDoseEngine(vtkMRMLRTPlanNode::Matlab);
  }
}

//-----------------------------------------------------------------------------
vtkMRMLRTBeamNode* qSlicerExternalBeamPlanningModuleWidget::currentBeamNode()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return NULL;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return NULL;
  }

  QStringList beamNodeIDs = d->BeamsTableView->selectedBeamNodeIDs();
  if (beamNodeIDs.isEmpty())
  {
    return NULL;
  }
  else if (beamNodeIDs.count() > 1)
  {
    qWarning() << Q_FUNC_INFO << ": Multiple beams selected, the first one is used for the current operation";
  }

  return vtkMRMLRTBeamNode::SafeDownCast( this->mrmlScene()->GetNodeByID(beamNodeIDs[0].toLatin1().constData()) );
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::addBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // Create new beam node by replicating currently selected beam
  vtkMRMLRTBeamNode* beamNode = NULL;
  if (this->currentBeamNode())
  {
    beamNode = d->logic()->CopyAndAddBeamToPlan(this->currentBeamNode(), rtPlanNode);
  }
  else
  {
    //TODO: Create the type of beam specified by the selected dose engine plugin
    beamNode = vtkMRMLRTProtonBeamNode::New();
    beamNode->SetName(rtPlanNode->GenerateNewBeamName().c_str());
    this->mrmlScene()->AddNode(beamNode);
    beamNode->CreateDefaultBeamModel();
    beamNode->Delete(); // Return ownership to scene only
    rtPlanNode->AddBeam(beamNode);
  }
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to add beam!";
    return;
  }

  // Clear instruction text
  d->label_CalculateDoseStatus->setText("");

  // Update beam visualization
  d->logic()->GetBeamsLogic()->UpdateBeamTransform(beamNode);
  d->logic()->GetBeamsLogic()->UpdateBeamGeometry(beamNode);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::removeBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  QStringList beamNodeIDs = d->BeamsTableView->selectedBeamNodeIDs();
  foreach (QString beamNodeID, beamNodeIDs)
  {
    // Remove beam
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(
      this->mrmlScene()->GetNodeByID(beamNodeID.toLatin1().constData()) );
    rtPlanNode->RemoveBeam(beamNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting dose calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    QString errorString("No RT plan node selected");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }
  // Make sure inputs were specified
  if (rtPlanNode->GetDoseEngine() != vtkMRMLRTPlanNode::Plastimatch)
  {
    QString errorString("Dose calculation is not available for this dose calculation engine");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }
  vtkMRMLScalarVolumeNode* referenceVolume = rtPlanNode->GetReferenceVolumeNode();
  if (!referenceVolume)
  {
    QString errorString("No reference anatomical volume is selected");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = rtPlanNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    QString errorString("No plan segmentation node is selected"); // MD Fix TODO -> dose could be computed without target
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }

  // Start timer
  QTime time;
  time.start();
  
  // The last verifications were fine so we can compute the dose
  // Dose Calculation - loop on all the beam and sum in a global dose matrix

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetBeams(beams);
  if (!beams) 
  {
    d->label_CalculateDoseStatus->setText("No beam found in the plan");
    return;
  }
  vtkMRMLRTProtonBeamNode* beamNode = NULL;

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  std::string errorMessage = d->logic()->InitializeAccumulatedDose(rtPlanNode);
  if (!errorMessage.empty())
  {
    d->label_CalculateDoseStatus->setText(errorMessage.c_str());
    QApplication::restoreOverrideCursor();
    return;
  }

  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      QString progressMessage = QString("Dose calculation in progress: %1").arg(beamNode->GetName());
      d->label_CalculateDoseStatus->setText(progressMessage);

      errorMessage = d->logic()->ComputeDose(rtPlanNode, beamNode);
      if (!errorMessage.empty())
      {
        d->label_CalculateDoseStatus->setText(errorMessage.c_str());
        break;
      }

      //TODO: sum in the final dose matrix with weights externalbeam for RxDose, and beamNode for beam weight
    }
    else
    {
      QString message = QString("Beam %1 not found").arg(beamNode->GetName());
      d->label_CalculateDoseStatus->setText(message);
    }
  }
  
  errorMessage = d->logic()->FinalizeAccumulatedDose(rtPlanNode);
  if (!errorMessage.empty())
  {
    d->label_CalculateDoseStatus->setText(errorMessage.c_str());
  }
  else
  {
    d->label_CalculateDoseStatus->setText("Dose calculation done");
  }
  QApplication::restoreOverrideCursor();

  qDebug() << Q_FUNC_INFO << ": Dose calculated in " << time.elapsed() << " ms";

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::clearDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  d->logic()->RemoveIntermediateDoseNodes(rtPlanNode);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateWEDClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting WED calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  /// GCS FIX TODO *** Implementation needs work ***

#if defined (commentout)
  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  // Make sure inputs were specified - only CT needed
  vtkMRMLScalarVolumeNode* referenceVolume = planNode->GetReferenceVolumeNode();
  if (!referenceVolume)
  {
    d->label_CalculateDoseStatus->setText("No reference image");
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Copy pertinent variable values from GUI to logic
  // Is this the right place for this?
  d->logic()->GetExternalBeamPlanningNode()->SetGantryAngle(d->SliderWidget_GantryAngle->value());

  // OK, we're good to go (well, not really, but let's pretend). 
  // Do the actual computation in the logic object
  d->logic()->ComputeWED();

  d->label_CalculateDoseStatus->setText("WED calculation done.");
  QApplication::restoreOverrideCursor();
#endif
}

//-----------------------------------------------------------------------------
bool qSlicerExternalBeamPlanningModuleWidget::setEditedNode(vtkMRMLNode* node, QString role/*=QString()*/, QString context/*=QString()*/)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  if (vtkMRMLRTPlanNode::SafeDownCast(node))
  {
    d->MRMLNodeComboBox_RtPlan->setCurrentNode(node);
    return true;
  }
  return false;
}
