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

  int CurrentBeamRow;
  int NumberOfBeamRows;
};

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
  CurrentBeamRow = -1;
  NumberOfBeamRows = 0;
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
  if (scene && d->logic()->GetRTPlanNode())
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
    if (node)
    {
      this->rtPlanNodeChanged(node);
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
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (rtPlanNode == NULL)
  {
    // If not, try to find one in the scene
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
  this->connect( d->MRMLNodeComboBox_RtPlan, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(rtPlanNodeChanged(vtkMRMLNode*)) );

  // RT plan section
  this->connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_PlanSegmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(planSegmentationNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_PlanPOIs, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(planPOIsNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_DoseROI, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseROINodeChanged(vtkMRMLNode*)) );
  this->connect( d->lineEdit_DoseGridSpacing, SIGNAL(textChanged(const QString &)), this, SLOT(doseGridSpacingChanged(const QString &)) );
  this->connect( d->comboBox_DoseEngineType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(doseEngineTypeChanged(const QString &)) );

  // Beams section
  this->connect( d->tableWidget_Beams, SIGNAL(cellClicked(int, int)), this, SLOT(tableWidgetCellClicked(int, int)) );
  this->connect( d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeamClicked()) );
  this->connect( d->pushButton_RemoveBeam, SIGNAL(clicked()), this, SLOT(removeBeamClicked()) );

  // Calculation buttons
  this->connect( d->pushButton_CalculateDose, SIGNAL(clicked()), this, SLOT(calculateDoseClicked()) );
  this->connect( d->pushButton_CalculateWED, SIGNAL(clicked()), this, SLOT(calculateWEDClicked()) );
  this->connect( d->pushButton_ClearDose, SIGNAL(clicked()), this, SLOT(clearDoseClicked()) );

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

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  d->MRMLNodeComboBox_RtPlan->setCurrentNode(rtPlanNode);

  // Enable GUI only if a plan node is selected
  d->CollapsibleButton_PlanParameters->setEnabled(rtPlanNode);
  d->CollapsibleButton_Beams->setEnabled(rtPlanNode);
  
  if (!rtPlanNode)
  {
    // GCS FIX TODO: Still need to wipe the GUI in this case
    return;
  }

  if (rtPlanNode->GetReferenceVolumeNode())
  {
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(rtPlanNode->GetReferenceVolumeNode());
  }
  if (rtPlanNode->GetSegmentationNode())
  {
    d->MRMLNodeComboBox_PlanSegmentation->setCurrentNode(rtPlanNode->GetSegmentationNode());
  }
  if (rtPlanNode->GetDoseVolumeNode())
  {
    d->MRMLNodeComboBox_DoseVolume->setCurrentNode(rtPlanNode->GetDoseVolumeNode());
  }
  if (rtPlanNode->GetMarkupsFiducialNode())
  {
    d->MRMLNodeComboBox_PlanPOIs->setCurrentNode(rtPlanNode->GetMarkupsFiducialNode());
  }

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rtPlanNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(node);
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect(d->logic()->GetRTPlanNode(), rtPlanNode, vtkCommand::ModifiedEvent, this, SLOT(onRTPlanNodeModified()));

  d->logic()->SetAndObserveRTPlanNode(rtPlanNode);

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
  this->updateRTBeamTableWidget();
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
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
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
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
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
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();

  // GCS FIX TODO: Update beams isocenters
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
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
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
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  qWarning() << Q_FUNC_INFO << ": Not implemented!";

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
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
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

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
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

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return NULL;
  }

  QTableWidgetItem *item = NULL;
  item = d->tableWidget_Beams->item(d->CurrentBeamRow, 0);
  if (!item)
  {
    return NULL;
  }
  int beamNumber = item->text().toInt();
  vtkMRMLRTBeamNode* beamNode = rtPlanNode->GetBeamByNumber(beamNumber);

  return beamNode;
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
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
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

  // Disconnect events for currently selected row
  qvtkDisconnect(beamNode, vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()));

  // Make new beam current in the table
  d->CurrentBeamRow = d->NumberOfBeamRows++;
  d->tableWidget_Beams->selectRow(d->CurrentBeamRow);

  // Clear instruction text
  d->label_CalculateDoseStatus->setText("");

  // GCS TODO FIX -- this should be called when logic is modified, maybe it gets called twice?
  this->updateRTBeamTableWidget();

  // Update UI
  this->updateWidgetFromMRML();

  // Update beam visualization
  d->logic()->GetBeamsLogic()->UpdateBeamTransform(beamNode);

  // Update beam visualization
  d->logic()->GetBeamsLogic()->UpdateBeamGeometry(beamNode);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::removeBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    return;
  }

  // Disconnect events for currently selected row
  qvtkDisconnect( beamNode, vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()) );

  // Remove beam
  rtPlanNode->RemoveBeam(beamNode);

  // Select a different beam as current
  d->NumberOfBeamRows--;
  if (d->CurrentBeamRow >= d->NumberOfBeamRows)
  {
    d->CurrentBeamRow = d->NumberOfBeamRows-1;
  }

  // Update UI table
  d->tableWidget_Beams->selectRow(d->CurrentBeamRow);
  this->updateRTBeamTableWidget();

  // Update lower UI tabs
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Clear the beam table
  d->tableWidget_Beams->setRowCount(0);
  d->tableWidget_Beams->setColumnCount(0);
  d->tableWidget_Beams->clearContents();

  // Get RT plan node
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  { 
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // Get beam nodes for the plan
  std::vector<vtkMRMLRTBeamNode*> beams;
  rtPlanNode->GetBeams(beams);

  // Set up the table
  d->tableWidget_Beams->setColumnCount(4);
  QStringList headerLabels;
  headerLabels << "Beam#" << "Name" << "Type" << "Radiation";

  d->tableWidget_Beams->blockSignals(true);
  d->tableWidget_Beams->setColumnWidth(0, 24);
  d->tableWidget_Beams->setHorizontalHeaderLabels(headerLabels);
  d->tableWidget_Beams->setRowCount(beams.size());

  // Fill the table
  int i=0;
  for (std::vector<vtkMRMLRTBeamNode*>::iterator it=beams.begin(); it!=beams.end(); ++it, ++i)
  {
    vtkMRMLRTBeamNode* beamNode = *it;
    if (beamNode)
    {
      d->tableWidget_Beams->setItem(i, 0, new QTableWidgetItem( QString::number(beamNode->GetBeamNumber()) ) );
      d->tableWidget_Beams->setItem(i, 1, new QTableWidgetItem( QString(beamNode->GetName()) ) );
      d->tableWidget_Beams->setItem(i, 2, new QTableWidgetItem( QString::number(beamNode->GetGantryAngle()) ) );
    }
  }

  d->tableWidget_Beams->blockSignals(false);
  if (d->CurrentBeamRow >= 0)
  {
    d->tableWidget_Beams->selectRow(d->CurrentBeamRow);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::tableWidgetCellClicked(int row, int column)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(column);

  // Don't do anything when clicking on already selected beam
  if (row == d->CurrentBeamRow)
  {
    return;
  }

  // Disconnect events for currently selected row
  qvtkDisconnect (this->currentBeamNode(), vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()));

  // Make sure inputs are initialized
  d->CurrentBeamRow = row;
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    qCritical() << "tableWidgetItemClicked: Inputs are not initialized!";
    return;
  }

  // Copy data from beam into tabs UI
  this->updateWidgetFromMRML();
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

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
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

  std::string errorMessage = d->logic()->InitializeAccumulatedDose();
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

      errorMessage = d->logic()->ComputeDose(beamNode);
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
  
  errorMessage = d->logic()->FinalizeAccumulatedDose();
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

  d->logic()->RemoveDoseNodes();
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
