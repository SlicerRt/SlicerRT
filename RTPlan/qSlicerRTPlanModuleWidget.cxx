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
#include "qSlicerRTPlanModuleWidget.h"
#include "ui_qSlicerRTPlanModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// SlicerRt includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanModuleNode.h"
#include "vtkSlicerRTPlanModuleLogic.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Contours
class qSlicerRTPlanModuleWidgetPrivate: public Ui_qSlicerRTPlanModule
{
  Q_DECLARE_PUBLIC(qSlicerRTPlanModuleWidget);
protected:
  qSlicerRTPlanModuleWidget* const q_ptr;
public:
  qSlicerRTPlanModuleWidgetPrivate(qSlicerRTPlanModuleWidget& object);
  ~qSlicerRTPlanModuleWidgetPrivate();
  vtkSlicerRTPlanModuleLogic* logic() const;
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
// qSlicerRTPlanModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerRTPlanModuleWidgetPrivate::qSlicerRTPlanModuleWidgetPrivate(qSlicerRTPlanModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
  currentBeamRow = -1;
  totalBeamRows = 0;
}

//-----------------------------------------------------------------------------
qSlicerRTPlanModuleWidgetPrivate::~qSlicerRTPlanModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerRTPlanModuleLogic*
qSlicerRTPlanModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerRTPlanModuleWidget);
  return vtkSlicerRTPlanModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerRTPlanModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerRTPlanModuleWidget::qSlicerRTPlanModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerRTPlanModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerRTPlanModuleWidget::~qSlicerRTPlanModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerRTPlanModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetRTPlanModuleNode() == 0)
    {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLRTPlanModuleNode");
    if (node)
      {
      this->setRTPlanModuleNode( vtkMRMLRTPlanModuleNode::SafeDownCast(node) );
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}
//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::onEnter()
{
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerRTPlanModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLRTPlanModuleNode* paramNode = d->logic()->GetRTPlanModuleNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLRTPlanModuleNode");
    if (node)
    {
      paramNode = vtkMRMLRTPlanModuleNode::SafeDownCast(node);
      d->logic()->SetAndObserveRTPlanModuleNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLRTPlanModuleNode> newNode = vtkSmartPointer<vtkMRMLRTPlanModuleNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveRTPlanModuleNode(newNode);
    }
  }

  this->updateWidgetFromMRML();
  d->ModuleWindowInitialized = true;
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerRTPlanModuleWidget);

  vtkMRMLRTPlanModuleNode* paramNode = d->logic()->GetRTPlanModuleNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
    if (paramNode->GetReferenceVolumeNodeID() && strcmp(paramNode->GetReferenceVolumeNodeID(),""))
    {
      d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(paramNode->GetReferenceVolumeNodeID());
    }
    else
    {
      this->referenceVolumeNodeChanged(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
    }

    if (paramNode->GetRTPlanNodeID() && strcmp(paramNode->GetRTPlanNodeID(),""))
    {
      d->MRMLNodeComboBox_RTPlan->setCurrentNode(paramNode->GetRTPlanNodeID());
    }
    else
    {
      this->RTPlanNodeChanged(d->MRMLNodeComboBox_RTPlan->currentNode());
    }
  }

}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::setup()
{
  Q_D(qSlicerRTPlanModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  this->connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setRTPlanModuleNode(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_RTPlan, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(RTPlanNodeChanged(vtkMRMLNode*)) );

  this->connect( d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeamClicked()) );
  this->connect( d->pushButton_RemoveBeam, SIGNAL(clicked()), this, SLOT(removeBeamClicked()) );

  this->connect( d->lineEdit_BeamName, SIGNAL(textChanged(const QString &)), this, SLOT(beamNameChanged(const QString &)) );
  this->connect( d->comboBox_BeamType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(beamTypeChanged(const QString &)) );
  this->connect( d->comboBox_RadiationType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(radiationTypeChanged(const QString &)) );
  this->connect( d->MRMLNodeComboBox_ISOCenter, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(ISOCenterNodeChanged(vtkMRMLNode*)) );
  this->connect( d->SliderWidget_GantryAngle, SIGNAL(valueChanged(double)), this, SLOT(gantryAngleChanged(double)) );
  this->connect( d->comboBox_CollimatorType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(collimatorTypeChanged(const QString &)) );
  //this->connect( d->comboBox_NominalEnergy, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(nominalEnergyChanged(const QString &)) );
  //this->connect( d->lineEdit_NominalmA, SIGNAL(textChanged(const QString &)), this, SLOT(nominalmAChanged(const QString &)) );
  //this->connect( d->lineEdit_RxDose, SIGNAL(textChanged(const QString &)), this, SLOT(RxDoseChanged(const QString &)) );
  //this->connect( d->lineEdit_BeamOnTime, SIGNAL(textChanged(const QString &)), this, SLOT(beamOnTimeChanged(const QString &)) );

  this->connect( d->tableWidget_Beams, SIGNAL(itemClicked(QtableWidgetItem *item)), this, SLOT(tableWidgetItemClicked(QtableWidgetItem *item)) );
  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::setRTPlanModuleNode(vtkMRMLNode *node)
{
  Q_D(qSlicerRTPlanModuleWidget);

  vtkMRMLRTPlanModuleNode* paramNode = vtkMRMLRTPlanModuleNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetRTPlanModuleNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveRTPlanModuleNode(paramNode);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::onLogicModified()
{
  this->updateRTBeamTableWidget();
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::updateRTBeamTableWidget()
{
  Q_D(qSlicerRTPlanModuleWidget);

  vtkMRMLRTPlanModuleNode* paramNode = d->logic()->GetRTPlanModuleNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  // Clear the table
  d->tableWidget_Beams->setRowCount(0);
  d->tableWidget_Beams->setColumnCount(0);
  d->tableWidget_Beams->clearContents();

  // get rt beam nodes for rtplan node
  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(d->logic()->GetRTPlanModuleNode()->GetRTPlanNodeID()));

  // a method to get a list of RTbeam node from rtplan node here
  if (!RTPlanNode)
  { // no rtplan node selected
    return;
  }
  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  RTPlanNode->GetRTBeamNodes(beams);
  
  // go through each rtbeam node
  beams->InitTraversal();
  if (beams->GetNumberOfItems() < 1)
  {
    std::cerr << "Warning: Selected RTPlan node has no children contour nodes!" << std::endl;
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
void qSlicerRTPlanModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerRTPlanModuleWidget);

  vtkMRMLRTPlanModuleNode* paramNode = d->logic()->GetRTPlanModuleNode();
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
void qSlicerRTPlanModuleWidget::RTPlanNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerRTPlanModuleWidget);

  vtkMRMLRTPlanModuleNode* paramNode = d->logic()->GetRTPlanModuleNode();
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
void qSlicerRTPlanModuleWidget::ISOCenterNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerRTPlanModuleWidget);

  vtkMRMLRTPlanModuleNode* paramNode = d->logic()->GetRTPlanModuleNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveISOCenterNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  //this->updateButtonsState();
  //this->updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::addBeamClicked()
{
  Q_D(qSlicerRTPlanModuleWidget);

  // Compute the DPH for the selected margin cal double array
  std::string errorMessage;
  d->currentBeamRow = d->totalBeamRows;
  d->totalBeamRows ++;
  d->logic()->AddBeam();
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::removeBeamClicked()
{
  Q_D(qSlicerRTPlanModuleWidget);

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
void qSlicerRTPlanModuleWidget::tableWidgetItemClicked(QTableWidgetItem *item)
{
  Q_D(qSlicerRTPlanModuleWidget);

  int row = d->tableWidget_Beams->currentRow();
  if (row != d->currentBeamRow)  
  {
    d->currentBeamRow = row;
    // need to update beam parameters panel
  } 
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::updateBeamParameters()
{
  Q_D(qSlicerRTPlanModuleWidget);

  // Compute the DPH for the selected margin cal double array
  std::string errorMessage;
  //d->logic()->UpdateBeam();

}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::beamNameChanged(const QString & text)
{
  Q_D(qSlicerRTPlanModuleWidget);

  //d->GantryAngle = text;

}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::beamTypeChanged(const QString & text)
{
  Q_D(qSlicerRTPlanModuleWidget);

}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::radiationTypeChanged(const QString & text)
{
  Q_D(qSlicerRTPlanModuleWidget);

}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModuleWidget::gantryAngleChanged(double value)
{
  Q_D(qSlicerRTPlanModuleWidget);

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
void qSlicerRTPlanModuleWidget::collimatorTypeChanged(const QString & text)
{
  Q_D(qSlicerRTPlanModuleWidget);

}
