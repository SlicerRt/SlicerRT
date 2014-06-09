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
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h" 

// SlicerRt includes
#include "vtkMRMLContourNode.h"
#include "vtkMRMLExternalBeamPlanningNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"

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
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::onEnter: Invalid logic!";
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
    if (paramNode->GetReferenceVolumeNode())
    {
      d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(paramNode->GetReferenceVolumeNode());
    }
    else
    {
      this->referenceVolumeNodeChanged(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
    }

    if (paramNode->GetRtPlanNode())
    {
      d->MRMLNodeComboBox_RtPlan->setCurrentNode(paramNode->GetRtPlanNode());
    }
    else
    {
      this->rtPlanNodeChanged(d->MRMLNodeComboBox_RtPlan->currentNode());
    }

	if (paramNode->GetMLCPositionDoubleArrayNode())
    {
      d->MRMLNodeComboBox_MLCPositionDoubleArray->setCurrentNode(paramNode->GetMLCPositionDoubleArrayNode());
    }
    else
    {
		this->MLCPositionDoubleArrayNodeChanged(d->MRMLNodeComboBox_MLCPositionDoubleArray->currentNode());
    }

    if (paramNode->GetProtonTargetContourNode())
    {
      d->MRMLNodeComboBox_ProtonTargetContour->setCurrentNode(paramNode->GetProtonTargetContourNode());
    }
    else
    {
      this->protonTargetContourNodeChanged(d->MRMLNodeComboBox_ProtonTargetContour->currentNode());
    }

	if (paramNode->GetIsocenterFiducialNode())
    {
      d->MRMLNodeComboBox_IsocenterFiducial->setCurrentNode(paramNode->GetIsocenterFiducialNode());
    }
    else
    {
		this->isocenterFiducialNodeChanged(d->MRMLNodeComboBox_IsocenterFiducial->currentNode());
    }

	if (paramNode->GetBeamName())
    {
	  d->lineEdit_BeamName->setText(paramNode->GetBeamName());
	}

	if (paramNode->GetIsocenterFiducialNode())
    {
	  d->RangeWidget_XJawsPosition->setValues(-paramNode->GetX1Jaw(), paramNode->GetX2Jaw());
	}

	d->RangeWidget_YJawsPosition->setValues(-paramNode->GetY1Jaw(), paramNode->GetY2Jaw());
	d->SliderWidget_GantryAngle->setValue(paramNode->GetGantryAngle());
	d->SliderWidget_CollimatorAngle->setValue(paramNode->GetCollimatorAngle());
	d->SliderWidget_CouchAngle->setValue(paramNode->GetCouchAngle());
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setup()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Set up contour selector widget
  d->ContourSelectorWidget->setAcceptContourHierarchies(true);
  d->ContourSelectorWidget->setRequiredRepresentation(vtkMRMLContourNode::ClosedSurfaceModel);

  // Make connections
  this->connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setExternalBeamPlanningNode(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->ContourSelectorWidget, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(planContourSetNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_RtPlan, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(rtPlanNodeChanged(vtkMRMLNode*)) );

  this->connect( d->tableWidget_Beams, SIGNAL(itemClicked(QtableWidgetItem *item)), this, SLOT(tableWidgetItemClicked(QtableWidgetItem *item)) );
  this->connect( d->tableWidget_Beams, SIGNAL(itemSelectionChanged()), this, SLOT(tableWidgetItemSelectionChanged()) );

  this->connect( d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeamClicked()) );
  this->connect( d->pushButton_RemoveBeam, SIGNAL(clicked()), this, SLOT(removeBeamClicked()) );

  /* Beam global parameters */
  this->connect( d->lineEdit_BeamName, SIGNAL(textChanged(const QString &)), this, SLOT(beamNameChanged(const QString &)) );
  this->connect( d->comboBox_RadiationType, SIGNAL(currentIndexChanged(int)), this, SLOT(radiationTypeChanged(int)) );

  /* Photon prescription page */
  this->connect( d->comboBox_BeamType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(beamTypeChanged(const QString &)) );
  this->connect( d->comboBox_NominalEnergy, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(nominalEnergyChanged(const QString&)) );
  this->connect( d->lineEdit_NominalmA, SIGNAL(textChanged(const QString &)), this, SLOT(nominalmAChanged(const QString &)) );
  this->connect( d->lineEdit_RxDose, SIGNAL(textChanged(const QString &)), this, SLOT(RxDoseChanged(const QString &)) );
  this->connect( d->lineEdit_BeamOnTime, SIGNAL(textChanged(const QString &)), this, SLOT(beamOnTimeChanged(const QString &)) );

  this->connect( d->MRMLNodeComboBox_MLCPositionDoubleArray, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(MLCPositionDoubleArrayNodeChanged(vtkMRMLNode*)) );
  this->connect( d->RangeWidget_XJawsPosition, SIGNAL(valuesChanged(double, double)), this, SLOT(XJawsPositionValuesChanged(double, double)) );
  this->connect( d->RangeWidget_YJawsPosition, SIGNAL(valuesChanged(double, double)), this, SLOT(YJawsPositionValuesChanged(double, double)) );

  /* Geometry page */
  this->connect( d->MRMLNodeComboBox_ProtonTargetContour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(protonTargetContourNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_IsocenterFiducial, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(isocenterFiducialNodeChanged(vtkMRMLNode*)) );
  this->connect( d->SliderWidget_GantryAngle, SIGNAL(valueChanged(double)), this, SLOT(gantryAngleChanged(double)) );
  this->connect( d->SliderWidget_CollimatorAngle, SIGNAL(valueChanged(double)), this, SLOT(collimatorAngleChanged(double)) );
  this->connect( d->SliderWidget_CouchAngle, SIGNAL(valueChanged(double)), this, SLOT(couchAngleChanged(double)) );

  /* Calculation buttons */
  this->connect( d->pushButton_CalculateDose, SIGNAL(clicked()), this, SLOT(calculateDoseClicked()) );
  this->connect( d->pushButton_CalculateWED, SIGNAL(clicked()), this, SLOT(calculateWEDClicked()) );

  /* Beam visualization */
  this->connect( d->pushButton_UpdateDRR, SIGNAL(clicked()), this, SLOT(updateDRRClicked()) );
  this->connect( d->checkBox_BeamEyesView, SIGNAL(clicked(bool)), this, SLOT(beamEyesViewClicked(bool)) );
  this->connect( d->checkBox_ContoursInBEW, SIGNAL(clicked(bool)), this, SLOT(contoursInBEWClicked(bool)) );

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

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget: Invalid parameter node!";
    return;
  }

  // Clear the beam table
  d->tableWidget_Beams->setRowCount(0);
  d->tableWidget_Beams->setColumnCount(0);
  d->tableWidget_Beams->clearContents();

  // Get rt plan node for ExternalBeamPlanning node
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetExternalBeamPlanningNode()->GetRtPlanNode();
  if (!rtPlanNode)
  { 
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget: Invalid rtplan node!";
    return;
  }
  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
  
  // Go through each RT beam node
  beams->InitTraversal();
  if (beams->GetNumberOfItems() < 1)
  {
    qWarning() << "qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget: Selected rtplan node has no children rtbeam nodes!";
    return;
  }

  // Set up the table
  d->tableWidget_Beams->setColumnCount(4);
  QStringList headerLabels;
  headerLabels << "Beam#" << "Name" << "Type" << "Radiation";

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

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::referenceVolumeNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::planContourSetNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::planContoursNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObservePlanContourSetNode(node);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rtPlanNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::rtPlanNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveRtPlanNode(vtkMRMLRTPlanNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
vtkMRMLRTBeamNode* qSlicerExternalBeamPlanningModuleWidget::getCurrentBeamNode(vtkMRMLExternalBeamPlanningNode* paramNode)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  QTableWidgetItem *item = NULL;
  char beamName[100];
  item = d->tableWidget_Beams->item(d->currentBeamRow, 1);
  if (!item)
  {
    return NULL;
  }
  strcpy(beamName, item->text().toStdString().c_str());

  vtkMRMLRTPlanNode* rtPlanNode = paramNode->GetRtPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::isocenterFiducialNodeChanged: Invalid rtplan node!";
    return NULL;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
  if (!beams) 
  {
    return NULL;
  }
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      if ( strcmp(beamNode->GetBeamName(), beamName) == 0)
      {
        break;
      }
    }
  }
  
  return beamNode;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::MLCPositionDoubleArrayNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::MLCPositionDoubleArrayNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  // Get rt plan node for ExternalBeamPlanning node
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetExternalBeamPlanningNode()->GetRtPlanNode();
  if (!rtPlanNode)
  { 
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget: Invalid rtplan node!";
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "couchAngleChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode::SafeDownCast(node));
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterFiducialNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::IsocenterFiducialNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveIsocenterFiducialNode(vtkMRMLMarkupsFiducialNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "couchAngleChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetAndObserveIsocenterFiducialNode(vtkMRMLMarkupsFiducialNode::SafeDownCast(node));
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::protonTargetContourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::protonTargetContourNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveProtonTargetContourNode(vtkMRMLContourNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "couchAngleChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetAndObserveProtonTargetContourNode(vtkMRMLContourNode::SafeDownCast(node));

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
void qSlicerExternalBeamPlanningModuleWidget::addBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::protonTargetContourNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  // TODO
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
  if (!item)
  {
    return;
  }
  strcpy(beamName, item->text().toStdString().c_str());

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

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  int row = d->tableWidget_Beams->currentRow();
  if (row != d->currentBeamRow)  
  {
    d->currentBeamRow = row;
  }

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "tableWidgetItemClicked: Inputs are not initialized!";
    return;
  }

  paramNode->SetBeamName(beamNode->GetBeamName());

  paramNode->SetX1Jaw(beamNode->GetX1Jaw());
  paramNode->SetX2Jaw(beamNode->GetX2Jaw());
  paramNode->SetY1Jaw(beamNode->GetY1Jaw());
  paramNode->SetY2Jaw(beamNode->GetY2Jaw());
  paramNode->SetAndObserveMLCPositionDoubleArrayNode(beamNode->GetMLCPositionDoubleArrayNode());
  paramNode->SetAndObserveIsocenterFiducialNode(beamNode->GetIsocenterFiducialNode());
  // paramNode->SetAndObserveProtonTargetContourNode(beamNode->GetProtonTargetContourNode());

  paramNode->SetGantryAngle(beamNode->GetGantryAngle());
  paramNode->SetCollimatorAngle(beamNode->GetCollimatorAngle());
  paramNode->SetCouchAngle(beamNode->GetCouchAngle());

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::tableWidgetItemSelectionChanged()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  int row = d->tableWidget_Beams->currentRow();
  if (row != d->currentBeamRow)  
  {
    d->currentBeamRow = row;
  }

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "tableWidgetItemSelectionChanged: Inputs are not initialized!";
    return;
  }

  paramNode->SetBeamName(beamNode->GetBeamName());

  paramNode->SetX1Jaw(beamNode->GetX1Jaw());
  paramNode->SetX2Jaw(beamNode->GetX2Jaw());
  paramNode->SetY1Jaw(beamNode->GetY1Jaw());
  paramNode->SetY2Jaw(beamNode->GetY2Jaw());
  paramNode->SetAndObserveMLCPositionDoubleArrayNode(beamNode->GetMLCPositionDoubleArrayNode());
  paramNode->SetAndObserveIsocenterFiducialNode(beamNode->GetIsocenterFiducialNode());
  // paramNode->SetAndObserveProtonTargetContourNode(beamNode->GetProtonTargetContourNode());

  paramNode->SetGantryAngle(beamNode->GetGantryAngle());
  paramNode->SetCollimatorAngle(beamNode->GetCollimatorAngle());
  paramNode->SetCouchAngle(beamNode->GetCouchAngle());

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting dose calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked: Invalid paramNode!";
    return;
  }

  /* Make sure inputs were specified */
  vtkMRMLScalarVolumeNode* referenceVolume = paramNode->GetReferenceVolumeNode();
  if (!referenceVolume)
  {
    d->label_CalculateDoseStatus->setText("No reference image");
    return;
  }

  vtkMRMLContourNode* contourNode = paramNode->GetProtonTargetContourNode();
  if (!contourNode)
  {
    d->label_CalculateDoseStatus->setText("No proton target volume");
    return;
  }

  /* Make sure contour has a labelmap form */
  if ( !contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) )
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
    d->label_CalculateDoseStatus->setText("Smearing radius must be a float");
    return;
  }
  d->logic()->GetExternalBeamPlanningNode()->SetSmearing (smearing);

  float proximal_margin = d->lineEdit_ProximalMargin->text().toFloat(&ok);
  if (!ok) {
    d->label_CalculateDoseStatus->setText("Proximal margin must be a float");
    return;
  }
  d->logic()->GetExternalBeamPlanningNode()->SetProximalMargin (proximal_margin);

  float distal_margin = d->lineEdit_DistalMargin->text().toFloat(&ok);
  if (!ok) {
    d->label_CalculateDoseStatus->setText("Distal margin must be a float");
    return;
  }
  d->logic()->GetExternalBeamPlanningNode()->SetDistalMargin (distal_margin);

  /* OK, we're good to go (well, not really, but let's pretend). 
     Do the actual computation in the logic object */
  d->logic()->ComputeDose();

  d->label_CalculateDoseStatus->setText("Dose calculation done.");
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateWEDClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting WED calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::calculateWEDClicked: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  /* Make sure inputs were specified - only CT needed*/
  vtkMRMLScalarVolumeNode* referenceVolume = paramNode->GetReferenceVolumeNode();
  if (!referenceVolume)
  {
    d->label_CalculateDoseStatus->setText("No reference image");
    return;
  }

  /* Copy pertinent variable values from GUI to logic */
  /* Is this the right place for this? */
  d->logic()->GetExternalBeamPlanningNode()->SetGantryAngle(d->SliderWidget_GantryAngle->value());

  /* OK, we're good to go (well, not really, but let's pretend). 
     Do the actual computation in the logic object */
  d->logic()->ComputeWED();

  d->label_CalculateDoseStatus->setText("WED calculation done.");
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateBeamParameters()
{
  //Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // TODO:
  std::string errorMessage;
  //d->logic()->UpdateBeam();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamNameChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::beamNameChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->SetBeamName(text.toStdString().c_str());

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "couchAngleChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetBeamName(text.toStdString().c_str());

  this->updateRTBeamTableWidget();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::radiationTypeChanged(int index)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::radiationTypeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  if (index == -1)
  {
    return;
  }

  QString text = d->comboBox_RadiationType->currentText();

  if (text == "Photon")
  {
    d->label_NominalEnergy->setEnabled (true);
    d->comboBox_NominalEnergy->setEnabled (true);
    d->label_BeamOnTime->setEnabled (true);
    d->lineEdit_BeamOnTime->setEnabled (true);
    d->label_NominalmA->setEnabled (true);
    d->lineEdit_NominalmA->setEnabled (true);
    //d->label_CollimatorType->setEnabled (true);
    //d->comboBox_CollimatorType->setEnabled (true);
  }
  else
  {
    d->label_NominalEnergy->setEnabled (false);
    d->comboBox_NominalEnergy->setEnabled (false);
    d->label_BeamOnTime->setEnabled (false);
    d->lineEdit_BeamOnTime->setEnabled (false);
    d->label_NominalmA->setEnabled (false);
    d->lineEdit_NominalmA->setEnabled (false);
    //d->label_CollimatorType->setEnabled (false);
    //d->comboBox_CollimatorType->setEnabled (false);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamTypeChanged(const QString &text)
{
  //Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  //TODO:
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::UpdateBeamTransform()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  QTableWidgetItem *item = NULL;
  char beamName[100];
  item = d->tableWidget_Beams->item(d->currentBeamRow, 1);
  if (!item)
  {
    return;
  }
  else 
  {
    strcpy(beamName, item->text().toStdString().c_str());
    d->logic()->UpdateBeamTransform(beamName);
  }
  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::UpdateBeamGeometryModel()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  QTableWidgetItem *item = NULL;
  char beamName[100];
  item = d->tableWidget_Beams->item(d->currentBeamRow, 1);
  if (!item)
  {
    return;
  }
  else 
  {
    strcpy(beamName, item->text().toStdString().c_str());
    d->logic()->UpdateBeamGeometryModel(beamName);
  }
  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::gantryAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::gantryAngleChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }
  paramNode->SetGantryAngle(value);

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "gantryAngleChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetGantryAngle(value);

  // Update beam visualization
  this->UpdateBeamTransform();

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::collimatorAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::collimatorAngleChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }
  paramNode->SetCollimatorAngle(value);

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "collimatorAngleChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetCollimatorAngle(value);

  // Update beam visualization
  this->UpdateBeamTransform();

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::couchAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::couchAngleChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }
  paramNode->SetCouchAngle(value);

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "couchAngleChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetCouchAngle(value);

  // Update beam visualization
  this->UpdateBeamTransform();

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::XJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::XJawPositionsValueChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::XJawPositionsValueChanged: Invalid module parameter!";
    return;
  }

  // Update parameter node value
  paramNode->SetX1Jaw(-minVal);
  paramNode->SetX2Jaw( maxVal);

  // Update beam node value
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() <<"XJawsPositionValuesChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetX1Jaw(-minVal);
  beamNode->SetX2Jaw( maxVal);

  // Update beam visualization
  this->UpdateBeamGeometryModel();

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::YJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::YJawPositionsValueChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }
  paramNode->SetY1Jaw(-minVal);
  paramNode->SetY2Jaw( maxVal);

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "YJawsPositionValuesChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetY1Jaw(-minVal);
  beamNode->SetY2Jaw( maxVal);

  // Update beam visualization
  this->UpdateBeamGeometryModel();

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::collimatorTypeChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::collimatorTypeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }
  
  //TODO:
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamEyesViewClicked(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (checked)
  {
    qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutTwoOverTwoView);
  }
  else
  {
    qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutFourUpView);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::contoursInBEWClicked(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // Todo: add the logic to check if contours should be included in the DRR view
  // right now the contours are included always. 
  if (checked)
  {
  }
  else
  {
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateDRRClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateDRRClicked: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  QTableWidgetItem *item = NULL;
  char beamName[100];
  item = d->tableWidget_Beams->item(d->currentBeamRow, 1);

  if (item)
  {
    strcpy(beamName, item->text().toStdString().c_str());
    d->logic()->UpdateDRR(beamName);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::nominalEnergyChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  // TODO: to be implemented
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::nominalmAChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  // TODO: to be implemented
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::RxDoseChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  // TODO: to be implemented
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamOnTimeChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  // TODO: to be implemented
}
