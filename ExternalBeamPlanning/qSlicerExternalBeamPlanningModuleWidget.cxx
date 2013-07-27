/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// SlicerQt includes
#include "qSlicerExternalBeamPlanningModuleWidget.h"
#include "ui_qSlicerExternalBeamPlanningModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// SlicerRt includes
#include "vtkMRMLContourNode.h"
#include "vtkMRMLExternalBeamPlanningNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Contours
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
  /// List of currently selected contour nodes. Contains the selected
  /// contour node or the children of the selected contour hierarchy node

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;

  int currentBeamRow;
  int totalBeamRows;

  QString BeamName;
  vtkMRMLRTBeamNode::RTBeamType BeamType;
  vtkMRMLRTBeamNode::RTRadiationType RadiationType;
  double GantryAngle;
  QString CollimatorType;
};

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
  currentBeamRow = -1;
  totalBeamRows = 0;
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

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetExternalBeamPlanningNode() == 0)
    {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLExternalBeamPlanningNode");
    if (node)
      {
      this->setExternalBeamPlanningNode( vtkMRMLExternalBeamPlanningNode::SafeDownCast(node) );
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
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLExternalBeamPlanningNode");
    if (node)
    {
      paramNode = vtkMRMLExternalBeamPlanningNode::SafeDownCast(node);
      d->logic()->SetAndObserveExternalBeamPlanningNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLExternalBeamPlanningNode> newNode = vtkSmartPointer<vtkMRMLExternalBeamPlanningNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveExternalBeamPlanningNode(newNode);
    }
  }

  this->updateWidgetFromMRML();
  d->ModuleWindowInitialized = true;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
    if (paramNode->GetReferenceVolumeNodeID() && strcmp(paramNode->GetReferenceVolumeNodeID(),""))
    {
      d->MRMLNodeComboBox_ReferenceVolume->setCurrentNodeID(paramNode->GetReferenceVolumeNodeID());
    }
    else
    {
      this->referenceVolumeNodeChanged(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
    }

    if (paramNode->GetRTPlanNodeID() && strcmp(paramNode->GetRTPlanNodeID(),""))
    {
      d->MRMLNodeComboBox_RTPlan->setCurrentNodeID(paramNode->GetRTPlanNodeID());
    }
    else
    {
      this->RTPlanNodeChanged(d->MRMLNodeComboBox_RTPlan->currentNode());
    }

    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetProtonTargetVolumeNodeID()))
    {
      d->MRMLNodeComboBox_ProtonTargetVolume->setCurrentNodeID(paramNode->GetProtonTargetVolumeNodeID());
    }
    else
    {
      this->protonTargetVolumeNodeChanged(d->MRMLNodeComboBox_ProtonTargetVolume->currentNode());
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setup()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  this->connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setExternalBeamPlanningNode(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_RTPlan, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(RTPlanNodeChanged(vtkMRMLNode*)) );

  this->connect( d->tableWidget_Beams, SIGNAL(itemClicked(QtableWidgetItem *item)), this, SLOT(tableWidgetItemClicked(QtableWidgetItem *item)) );

  this->connect( d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeamClicked()) );
  this->connect( d->pushButton_RemoveBeam, SIGNAL(clicked()), this, SLOT(removeBeamClicked()) );

  /* Beam global parameters */
  this->connect( d->lineEdit_BeamName, SIGNAL(textChanged(const QString &)), this, SLOT(beamNameChanged(const QString &)) );
  this->connect( d->comboBox_RadiationType, SIGNAL(currentIndexChanged(int)), this, SLOT(radiationTypeChanged(int)) );

  /* Task buttons */
  this->connect( d->pushButton_BeamPrescription, SIGNAL(clicked()), this, SLOT(beamPrescriptionButtonClicked()) );
  this->connect( d->pushButton_BeamGeometry, SIGNAL(clicked()), this, SLOT(beamGeometryButtonClicked()) );

  /* Make prescription the default task */
  this->beamPrescriptionButtonClicked();

  /* Prescription page */
  this->connect( d->comboBox_BeamType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(beamTypeChanged(const QString &)) );

  /* Geometry page */
  this->connect( d->MRMLNodeComboBox_ProtonTargetVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(protonTargetVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_Isocenter, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(IsocenterNodeChanged(vtkMRMLNode*)) );
  this->connect( d->SliderWidget_GantryAngle, SIGNAL(valueChanged(double)), this, SLOT(gantryAngleChanged(double)) );
  this->connect( d->comboBox_CollimatorType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(collimatorTypeChanged(const QString &)) );
  //this->connect( d->comboBox_NominalEnergy, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(nominalEnergyChanged(const QString &)) );
  //this->connect( d->lineEdit_NominalmA, SIGNAL(textChanged(const QString &)), this, SLOT(nominalmAChanged(const QString &)) );
  //this->connect( d->lineEdit_RxDose, SIGNAL(textChanged(const QString &)), this, SLOT(RxDoseChanged(const QString &)) );
  //this->connect( d->lineEdit_BeamOnTime, SIGNAL(textChanged(const QString &)), this, SLOT(beamOnTimeChanged(const QString &)) );

  /* Calculation buttons */
  this->connect( d->pushButton_CalculateDose, SIGNAL(clicked()), this, SLOT(calculateDoseClicked()) );

  /* Disable unused buttons in prescription task */
  this->radiationTypeChanged(0);

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setExternalBeamPlanningNode(vtkMRMLNode *node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = vtkMRMLExternalBeamPlanningNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetExternalBeamPlanningNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveExternalBeamPlanningNode(paramNode);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onLogicModified()
{
  this->updateRTBeamTableWidget();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  // Clear the table
  d->tableWidget_Beams->setRowCount(0);
  d->tableWidget_Beams->setColumnCount(0);
  d->tableWidget_Beams->clearContents();

  // get rt beam nodes for ExternalBeamPlanning node
  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(d->logic()->GetExternalBeamPlanningNode()->GetRTPlanNodeID()));

  // a method to get a list of RTbeam node from ExternalBeamPlanning node here
  if (!rtPlanNode)
  { // no ExternalBeamPlanning node selected
    return;
  }
  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
  
  // go through each rtbeam node
  beams->InitTraversal();
  if (beams->GetNumberOfItems() < 1)
  {
    std::cerr << "Warning: Selected ExternalBeamPlanning node has no children contour nodes!" << std::endl;
    return;
  }

  // Set up the table
  d->tableWidget_Beams->setColumnCount(4);
  QStringList headerLabels;
  headerLabels << "" << "Name" << "Type" << "Radiation";

  d->tableWidget_Beams->setColumnWidth(0, 24);
  d->tableWidget_Beams->setHorizontalHeaderLabels(headerLabels);
  d->tableWidget_Beams->setRowCount(beams->GetNumberOfItems());

  // Fill the table
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      d->tableWidget_Beams->setItem(i, 1, new QTableWidgetItem( QString(beamNode->GetBeamName()) ) );
    }
  }
  if (d->currentBeamRow >=0)
  {
    d->tableWidget_Beams->selectRow(d->currentBeamRow);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  qDebug ("EBP:: rvnode changed %p, %p, %p",
          paramNode, this->mrmlScene(), node);
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceVolumeNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  //this->updateButtonsState();
  //this->updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::RTPlanNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveRTPlanNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  //this->updateButtonsState();
  //this->updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::IsocenterNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveIsocenterNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  //this->updateButtonsState();
  //this->updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::addBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // Compute the DPH for the selected margin cal double array
  std::string errorMessage;
  d->currentBeamRow = d->totalBeamRows;
  d->totalBeamRows ++;
  d->logic()->AddBeam();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::removeBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  QTableWidgetItem *item = NULL;
  char beamName[100];
  item = d->tableWidget_Beams->item(d->currentBeamRow, 1);

  if (item)
  {
    strcpy(beamName, item->text().toStdString().c_str());
    d->logic()->RemoveBeam(beamName);
    d->totalBeamRows--;
    d->currentBeamRow = d->totalBeamRows-1;
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::tableWidgetItemClicked(QTableWidgetItem *item)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  int row = d->tableWidget_Beams->currentRow();
  if (row != d->currentBeamRow)  
  {
    d->currentBeamRow = row;
    // need to update beam parameters panel
  } 
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::protonTargetVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  qDebug ("EBP:: ptvnode changed %p, %p, %p",
          paramNode, this->mrmlScene(), node);

  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveProtonTargetVolumeNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

#if defined (commentout)
  /* This is just debugging */
  vtkMRMLContourNode* contourNode;
  vtkMRMLContourNode::ContourRepresentationType contourRep;
  contourNode = vtkMRMLContourNode::SafeDownCast(node);
  contourRep = contourNode->GetActiveRepresentationType();
  qDebug("Contour representation is %d\n", contourRep);
#endif

#if defined (commentout)
  // TODO GCS FIX: Update GUI to set range & modulation, etc.
  // Update UI from selected contours nodes list
  this->updateWidgetFromMRML();
#endif
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting dose calculation...");

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();

  /* Make sure inputs were specified */
  const char *refVolID = paramNode->GetReferenceVolumeNodeID();
  const char *tgtVolID = paramNode->GetProtonTargetVolumeNodeID();
  if (SlicerRtCommon::IsStringNullOrEmpty(refVolID))
  {
    d->label_CalculateDoseStatus->setText("No reference image");
    return;
  }
  if (SlicerRtCommon::IsStringNullOrEmpty(tgtVolID))
  {
    d->label_CalculateDoseStatus->setText("No proton target volume");
    return;
  }

  /* Make sure contour is in labelmap form */
  vtkMRMLNode *node;
  vtkMRMLContourNode* contourNode;
  vtkMRMLContourNode::ContourRepresentationType contourRep;

  node = this->mrmlScene()->GetNodeByID(tgtVolID);
  contourNode = vtkMRMLContourNode::SafeDownCast(node);
  contourRep = contourNode->GetActiveRepresentationType();
  qDebug("Contour representation is %d\n", contourRep);

  if (contourRep != vtkMRMLContourNode::IndexedLabelmap)
  {
    d->label_CalculateDoseStatus->setText("Proton target must be labelmap");
    return;
  }

  /* Copy pertinent variable values from GUI to logic */
  /* Is this the right place for this? */
  bool ok;
  d->logic()->GetExternalBeamPlanningNode()->SetGantryAngle (
    d->SliderWidget_GantryAngle->value());

  float smearing = d->lineEdit_Smearing->text().toFloat(&ok);
  if (!ok) {
    d->label_CalculateDoseStatus->setText("Proton target must be labelmap");
    return;
  }
  d->logic()->GetExternalBeamPlanningNode()->SetSmearing (smearing);

  /* OK, we're good to go (well, not really, but let's pretend). 
     Do the actual computation in the logic object */
  d->logic()->ComputeDose();

  d->label_CalculateDoseStatus->setText("Dose calculation done.");
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateBeamParameters()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // Compute the DPH for the selected margin cal double array
  std::string errorMessage;
  //d->logic()->UpdateBeam();

}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamNameChanged(const QString & text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  //d->GantryAngle = text;

}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::radiationTypeChanged(int index)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  qDebug ("Radiation type changed (%d)\n", index);
  if (index == -1) {
    return;
  }

  QString text = d->comboBox_RadiationType->currentText();

  if (text == "Photon") {
    d->label_NominalEnergy->setEnabled (true);
    d->comboBox_NominalEnergy->setEnabled (true);
    d->label_BeamOnTime->setEnabled (true);
    d->lineEdit_BeamOnTime->setEnabled (true);
    d->label_NominalmA->setEnabled (true);
    d->lineEdit_NominalmA->setEnabled (true);
    d->label_CollimatorType->setEnabled (true);
    d->comboBox_CollimatorType->setEnabled (true);
  } else {
    d->label_NominalEnergy->setEnabled (false);
    d->comboBox_NominalEnergy->setEnabled (false);
    d->label_BeamOnTime->setEnabled (false);
    d->lineEdit_BeamOnTime->setEnabled (false);
    d->label_NominalmA->setEnabled (false);
    d->lineEdit_NominalmA->setEnabled (false);
    d->label_CollimatorType->setEnabled (false);
    d->comboBox_CollimatorType->setEnabled (false);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamPrescriptionButtonClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->pushButton_BeamPrescription->setChecked (true);
  d->pushButton_BeamGeometry->setChecked (false);

  d->stackedWidget->setCurrentIndex (0);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamGeometryButtonClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->pushButton_BeamPrescription->setChecked (false);
  d->pushButton_BeamGeometry->setChecked (true);

  d->stackedWidget->setCurrentIndex (1);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamTypeChanged(const QString & text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::gantryAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  QTableWidgetItem *item = NULL;
  char beamName[100];
  item = d->tableWidget_Beams->item(d->currentBeamRow, 1);

  if (item)
  {
    strcpy(beamName, item->text().toStdString().c_str());
    d->logic()->UpdateBeam(beamName, value);
  }

}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::collimatorTypeChanged(const QString & text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

}
