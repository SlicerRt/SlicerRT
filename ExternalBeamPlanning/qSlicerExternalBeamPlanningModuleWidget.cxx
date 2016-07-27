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

// CLI logic includes
//#include <vtkSlicerCLIModuleLogic.h>

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

// Beams includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkSlicerBeamsModuleLogic.h"

// ExternalBeamPlanning
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"

// DoseEngines includes
#include "qSlicerDoseEnginePluginHandler.h"
#include "qSlicerAbstractDoseEngine.h"
#include "qSlicerDoseEngineLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLSliceNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QTime>
#include <QItemSelection>
#include <QMessageBox>

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
  /// Dose engine logic for dose calculation related functions
  qSlicerDoseEngineLogic* DoseEngineLogic;
};

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
  , DoseEngineLogic(NULL)
{
  this->DoseEngineLogic = new qSlicerDoseEngineLogic(&object);
}

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::~qSlicerExternalBeamPlanningModuleWidgetPrivate()
{
  //TODO: Leak?
  //if (this->DoseEngineLogic)
  //{
  //  delete this->DoseEngineLogic;
  //  this->DoseEngineLogic = NULL;
  //}
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

  // Set scene to dose engine logic
  d->DoseEngineLogic->setMRMLScene(scene);

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
  connect( d->MRMLNodeComboBox_PlanSegmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(segmentationNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_PlanPOIs, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(poisMarkupsNodeChanged(vtkMRMLNode*)) );

  connect( d->MRMLCoordinatesWidget_IsocenterCoordinates, SIGNAL(coordinatesChanged(double*)), this, SLOT(isocenterCoordinatesChanged(double *)));
  connect( d->pushButton_CenterViewToIsocenter, SIGNAL(clicked()), this, SLOT(centerViewToIsocenterClicked()) );

  connect( d->MRMLSegmentSelectorWidget_TargetStructure, SIGNAL(currentSegmentChanged(QString)), this, SLOT(targetSegmentChanged(const QString&)) );
  connect( d->checkBox_IsocenterAtTargetCenter, SIGNAL(stateChanged(int)), this, SLOT(isocenterAtTargetCenterCheckboxStateChanged(int)));

  connect( d->comboBox_DoseEngine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(doseEngineChanged(const QString&)) );
  connect( d->doubleSpinBox_RxDose, SIGNAL(valueChanged(double)), this, SLOT(rxDoseChanged(double)) );

  // Output section
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_DoseROI, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseROINodeChanged(vtkMRMLNode*)) );
  connect( d->lineEdit_DoseGridSpacing, SIGNAL(textChanged(const QString&)), this, SLOT(doseGridSpacingChanged(const QString&)) );

  // Beams section
  //connect( d->BeamsTableView, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(beamSelectionChanged(QItemSelection,QItemSelection) ) );
  connect( d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeamClicked()) );
  connect( d->pushButton_RemoveBeam, SIGNAL(clicked()), this, SLOT(removeBeamClicked()) );

  // Calculation buttons
  connect( d->pushButton_CalculateDose, SIGNAL(clicked()), this, SLOT(calculateDoseClicked()) );
  connect( d->pushButton_CalculateWED, SIGNAL(clicked()), this, SLOT(calculateWEDClicked()) );
  connect( d->pushButton_ClearDose, SIGNAL(clicked()), this, SLOT(clearDoseClicked()) );

  // Connect to progress event
  connect( d->DoseEngineLogic, SIGNAL(progressUpdated(double)), this, SLOT(onProgressUpdated(double)) );

  // Hide non-functional items //TODO:
  d->label_DoseROI->setVisible(false);
  d->MRMLNodeComboBox_DoseROI->setVisible(false);
  d->label_DoseGridSpacing->setVisible(false);
  d->lineEdit_DoseGridSpacing->setVisible(false);

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
  if (rtPlanNode->GetOutputTotalDoseVolumeNode())
  {
    d->MRMLNodeComboBox_DoseVolume->setCurrentNode(rtPlanNode->GetOutputTotalDoseVolumeNode());
  }

  // Set target segment
  d->MRMLSegmentSelectorWidget_TargetStructure->setCurrentNode(rtPlanNode->GetSegmentationNode());
  d->MRMLSegmentSelectorWidget_TargetStructure->setCurrentSegmentID(rtPlanNode->GetTargetSegmentID());

  // Update isocenter specification
  d->checkBox_IsocenterAtTargetCenter->setChecked(rtPlanNode->GetIsocenterSpecification() == vtkMRMLRTPlanNode::CenterOfTarget);
  // Update isocenter controls based on plan isocenter position
  this->updateIsocenterPosition();

  // Populate dose engines combobox
  this->updateDoseEngines();

  // Set prescription
  d->doubleSpinBox_RxDose->setValue(rtPlanNode->GetRxDose());

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
  qvtkReconnect(rtPlanNode, vtkMRMLRTPlanNode::IsocenterModifiedEvent, this, SLOT(updateIsocenterPosition()));

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
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    }
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::segmentationNodeChanged(vtkMRMLNode* node)
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
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    }
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();

  // Set segmentation node to target selector
  d->MRMLSegmentSelectorWidget_TargetStructure->setCurrentNode(node);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::poisMarkupsNodeChanged(vtkMRMLNode* node)
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
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    }
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
    beamNode->UpdateTransform();
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
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    }
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
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    }
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
void qSlicerExternalBeamPlanningModuleWidget::targetSegmentChanged(const QString& segment)
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

  // Set target segment ID
  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetTargetSegmentID(segment.toLatin1().constData());
  rtPlanNode->DisableModifiedEventOff();

  if (rtPlanNode->GetIsocenterSpecification() == vtkMRMLRTPlanNode::CenterOfTarget)
  {
    rtPlanNode->SetIsocenterToTargetCenter();
    this->centerViewToIsocenterClicked();
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterAtTargetCenterCheckboxStateChanged(int state)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  if (state > 0)
  {
    rtPlanNode->SetIsocenterSpecification(vtkMRMLRTPlanNode::CenterOfTarget);
  }
  else
  {
    rtPlanNode->SetIsocenterSpecification(vtkMRMLRTPlanNode::ArbitraryPoint);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterCoordinatesChanged(double* coords)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // If isocenter specification is CenterOfTarget, then reset it to previous isocenter
  if (rtPlanNode->GetIsocenterSpecification() == vtkMRMLRTPlanNode::CenterOfTarget)
  {
    double isocenter[3] = {0.0,0.0,0.0};
    if (!rtPlanNode->GetIsocenterPosition(isocenter))
    {
      qCritical() << Q_FUNC_INFO << ": Failed to get plan isocenter for plan " << rtPlanNode->GetName();
    }
    d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(true);
    d->MRMLCoordinatesWidget_IsocenterCoordinates->setCoordinates(isocenter);
    d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(false);
  }
  else // Otherwise (if ArbitraryPoint) set coordinates as isocenter position
  {
    rtPlanNode->DisableModifiedEventOn();
    rtPlanNode->SetIsocenterPosition(coords);
    rtPlanNode->DisableModifiedEventOff();
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::centerViewToIsocenterClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // Get isocenter position
  double isocenter[3] = {0.0,0.0,0.0};
  if (!rtPlanNode->GetIsocenterPosition(isocenter))
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get plan isocenter for plan " << rtPlanNode->GetName();
  }

  // Navigate slice views to position
  this->mrmlScene()->InitTraversal();
  vtkMRMLNode *currentNode = this->mrmlScene()->GetNextNodeByClass("vtkMRMLSliceNode");
  while (currentNode)
  {
    vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(currentNode);
    sliceNode->JumpSlice(isocenter[0], isocenter[1], isocenter[2]);
    currentNode = this->mrmlScene()->GetNextNodeByClass("vtkMRMLSliceNode");
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateIsocenterPosition()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // Get isocenter position
  double isocenter[3] = {0.0,0.0,0.0};
  if (!rtPlanNode->GetIsocenterPosition(isocenter))
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get plan isocenter for plan " << rtPlanNode->GetName();
  }

  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(true);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->setCoordinates(isocenter);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateDoseEngines()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  qSlicerDoseEnginePluginHandler::DoseEngineListType engines =
    qSlicerDoseEnginePluginHandler::instance()->registeredDoseEngines();
  if (engines.size() == d->comboBox_DoseEngine->count())
  {
    return;
  }

  QString selectedEngineName = d->comboBox_DoseEngine->currentText();

  d->comboBox_DoseEngine->blockSignals(true);
  d->comboBox_DoseEngine->clear();

  for (qSlicerDoseEnginePluginHandler::DoseEngineListType::iterator engineIt = engines.begin();
    engineIt != engines.end(); ++engineIt)
  {
    d->comboBox_DoseEngine->addItem((*engineIt)->name());
  }

  // Select previously selected engine
  int index = d->comboBox_DoseEngine->findText(selectedEngineName);
  if (index != -1)
  {
    d->comboBox_DoseEngine->setCurrentIndex(index);
  }
  // If previous selection not found (e.g. no selection has been made yet), then select first engine
  else
  {
    d->comboBox_DoseEngine->setCurrentIndex(0);
  }
  // Apply engine selection (signals are blocked, plus if first index has been selected and it has
  // not been applied, then it needs to be done now)
  this->doseEngineChanged(d->comboBox_DoseEngine->currentText());

  d->comboBox_DoseEngine->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseEngineChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  if (rtPlanNode->GetDoseEngineName() && !text.compare(rtPlanNode->GetDoseEngineName()))
  {
    return;
  }

  // Hide parameters belonging to the previously selected dose engine from user interface
  if (rtPlanNode->GetDoseEngineName())
  {
    qSlicerAbstractDoseEngine* previousEngine =
      qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(rtPlanNode->GetDoseEngineName());
    if (!previousEngine)
    {
      qCritical() << Q_FUNC_INFO << ": Failed to access dose engine with name" << rtPlanNode->GetDoseEngineName();
      return;
    }
    previousEngine->setBeamParametersVisible(false);
  }

  // Get newly selected dose engine
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(text.toLatin1().constData());
  if (!selectedEngine)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access dose engine with name" << text;
    return;
  }

  // Add engine-specific beam parameters to user interface
  selectedEngine->setBeamParametersVisible(true);

  qDebug() << "Dose engine selection changed to " << text;
  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetDoseEngineName(selectedEngine->name().toLatin1().constData());
  rtPlanNode->DisableModifiedEventOff();
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
  vtkMRMLRTBeamNode* beamNode = d->DoseEngineLogic->createBeamInPlan(rtPlanNode);
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to add beam!";
    return;
  }

  // Add engine-specific beam parameters to newly created beam node
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(rtPlanNode->GetDoseEngineName());
  if (!selectedEngine)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access dose engine with name" << rtPlanNode->GetDoseEngineName();
    return;
  }
  selectedEngine->addBeamParameterAttributesToBeamNode(beamNode);

  // Clear instruction text
  d->label_CalculateDoseStatus->setText("");
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

  // Start timer
  QTime time;
  time.start();
  // Set busy cursor
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Get selected dose engine
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(rtPlanNode->GetDoseEngineName());
  if (!selectedEngine)
  {
    QString errorString = QString("Unable to access dose engine with name %1").arg(rtPlanNode->GetDoseEngineName() ? rtPlanNode->GetDoseEngineName() : "NULL");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }
  // Calculate dose
  QString errorMessage = d->DoseEngineLogic->calculateDose(rtPlanNode);

  if (errorMessage.isEmpty())
  {
    QString message = QString("Dose calculated successfully in %1 s").arg(time.elapsed()/1000.0);
    qDebug() << Q_FUNC_INFO << ": " << message;
    d->label_CalculateDoseStatus->setText(message);
  }
  else
  {
    QString message = QString("ERROR: %1").arg(errorMessage);
    qCritical() << Q_FUNC_INFO << ": " << message;
    d->label_CalculateDoseStatus->setText(message);
  }

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onProgressUpdated(double progress)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  int progressPercent = (int)(progress * 100.0);
  QString progressMessage = QString("Dose calculation in progress: %1 %").arg(progressPercent);
  d->label_CalculateDoseStatus->setText(progressMessage);
  QApplication::processEvents();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::clearDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  d->DoseEngineLogic->removeIntermediateResults(rtPlanNode);
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
