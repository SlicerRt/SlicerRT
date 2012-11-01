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

// Qt includes
#include <QCheckBox>

// SlicerQt includes
#include "qSlicerIsodoseModuleWidget.h"
#include "ui_qSlicerIsodoseModule.h"

// Isodose includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLIsodoseNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLProceduralColorNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

// STD includes
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Isodose
class qSlicerIsodoseModuleWidgetPrivate: public Ui_qSlicerIsodoseModule
{
  Q_DECLARE_PUBLIC(qSlicerIsodoseModuleWidget);
protected:
  qSlicerIsodoseModuleWidget* const q_ptr;
public:
  qSlicerIsodoseModuleWidgetPrivate(qSlicerIsodoseModuleWidget &object);
  ~qSlicerIsodoseModuleWidgetPrivate();
  vtkSlicerIsodoseModuleLogic* logic() const;
  void setDefaultColorNode();
};

//-----------------------------------------------------------------------------
// qSlicerIsodoseModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidgetPrivate::qSlicerIsodoseModuleWidgetPrivate(qSlicerIsodoseModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidgetPrivate::~qSlicerIsodoseModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic*
qSlicerIsodoseModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerIsodoseModuleWidget);
  return vtkSlicerIsodoseModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerIsodoseModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidget::qSlicerIsodoseModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerIsodoseModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidget::~qSlicerIsodoseModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerIsodoseModuleWidget);

  this->Superclass::setMRMLScene(scene);

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetIsodoseNode() == 0)
    {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
    if (node)
      {
      this->setIsodoseNode( vtkMRMLIsodoseNode::SafeDownCast(node) );
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onEnter()
{
  if (!this->mrmlScene())
    {
    return;
    }

  Q_D(qSlicerIsodoseModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
    {
    return;
    }
  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
    {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
    if (node)
      {
      paramNode = vtkMRMLIsodoseNode::SafeDownCast(node);
      d->logic()->SetAndObserveIsodoseNode(paramNode);
      return;
      }
    else 
      {
      vtkSmartPointer<vtkMRMLIsodoseNode> newNode = vtkSmartPointer<vtkMRMLIsodoseNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveIsodoseNode(newNode);
      }
    }

  // set up default color node
  d->setDefaultColorNode();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
    if (paramNode->GetDoseVolumeNodeId() && strcmp(paramNode->GetDoseVolumeNodeId(),""))
    {
      d->MRMLNodeComboBox_DoseVolume->setCurrentNode(paramNode->GetDoseVolumeNodeId());
    }
    else
    {
      this->doseVolumeNodeChanged(d->MRMLNodeComboBox_DoseVolume->currentNode());
    }
    d->setDefaultColorNode();
    vtkSmartPointer<vtkMRMLColorTableNode> colorTableNode = vtkMRMLColorTableNode::SafeDownCast(
      this->mrmlScene()->GetNodeByID(paramNode->GetColorTableNodeId()));       
    d->spinBox_NumberOfLevels->setValue(colorTableNode->GetNumberOfColors());
    d->checkBox_Isoline->setChecked(paramNode->GetShowIsodoseLines());
    d->checkBox_Isosurface->setChecked(paramNode->GetShowIsodoseSurfaces());
  }

}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onLogicModified()
{
  Q_D(qSlicerIsodoseModuleWidget);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidgetPrivate::setDefaultColorNode()
{
  Q_Q(qSlicerIsodoseModuleWidget);
  if (!q->mrmlScene())
  {
    return;
  }
  const char *defaultID = this->logic()->GetDefaultLabelMapColorTableNodeId();
  vtkMRMLColorTableNode *defaultNode = vtkMRMLColorTableNode::SafeDownCast(
    q->mrmlScene()->GetNodeByID(defaultID));
  this->tableView_IsodoseLevels->setMRMLColorNode(defaultNode);
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setup()
{
  Q_D(qSlicerIsodoseModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setIsodoseNode(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->spinBox_NumberOfLevels, SIGNAL(valueChanged(int)), this, SLOT(setNumberOfLevels(int)));

  connect( d->checkBox_Isoline, SIGNAL(toggled(bool)), this, SLOT( setIsolineVisibility(bool) ) );
  connect( d->checkBox_Isosurface, SIGNAL(toggled(bool)), this, SLOT( setIsosurfaceVisibility(bool) ) );

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  // Select the default color node
  d->setDefaultColorNode();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsodoseNode(vtkMRMLNode *node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetIsodoseNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveIsodoseNode(paramNode);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  if (d->logic()->DoseVolumeContainsDose())
  {
    d->label_NotDoseVolumeWarning->setText("");
  }
  else
  {
    d->label_NotDoseVolumeWarning->setText(tr(" Selected volume is not a dose"));
  }

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setNumberOfLevels(int newNumber)
{
  Q_D(qSlicerIsodoseModuleWidget);
  if (!d->spinBox_NumberOfLevels->isEnabled())
  {
    return;
  }

  const char *defaultID = d->logic()->GetDefaultLabelMapColorTableNodeId();
  vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(defaultID));

  colorTableNode->SetNumberOfColors(newNumber);
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::outputHierarchyNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveOutputHierarchyNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
QString qSlicerIsodoseModuleWidget::generateNewIsodoseLevel() const
{
  Q_D(const qSlicerIsodoseModuleWidget);

  QString newIsodoseLevelBase("New level");
  QString newIsodoseLevel(newIsodoseLevelBase);
  return newIsodoseLevel;
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsolineVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);
  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetShowIsodoseLines(visible);
  paramNode->DisableModifiedEventOff();

  vtkMRMLModelHierarchyNode* modelHierarchyNode = NULL;
  modelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetOutputHierarchyNodeId()));
  if(!modelHierarchyNode)
  {
    return;
  }

  vtkSmartPointer<vtkCollection> childModelNodes = vtkSmartPointer<vtkCollection>::New();
  modelHierarchyNode->GetChildrenModelNodes(childModelNodes);
  childModelNodes->InitTraversal();
  for (int i=0; i<childModelNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(childModelNodes->GetItemAsObject(i));
    modelNode->GetDisplayNode()->SetSliceIntersectionVisibility(visible);
  }
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsosurfaceVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);
  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetShowIsodoseSurfaces(visible);
  paramNode->DisableModifiedEventOff();

  vtkMRMLModelHierarchyNode* modelHierarchyNode = NULL;
  modelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetOutputHierarchyNodeId()));
  if(!modelHierarchyNode)
  {
    return;
  }

  vtkSmartPointer<vtkCollection> childModelNodes = vtkSmartPointer<vtkCollection>::New();
  modelHierarchyNode->GetChildrenModelNodes(childModelNodes);
  childModelNodes->InitTraversal();
  for (int i=0; i<childModelNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(childModelNodes->GetItemAsObject(i));
    modelNode->GetDisplayNode()->SetVisibility(visible);
  }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::applyClicked()
{
  Q_D(qSlicerIsodoseModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the iso dose surface for the selected dose volume
  d->logic()->ComputeIsodose();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateButtonsState()
{
  Q_D(qSlicerIsodoseModuleWidget);
  if (!this->mrmlScene())
  {
    return;
  }

  const char *defaultID = d->logic()->GetDefaultLabelMapColorTableNodeId();
  vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(defaultID));
  bool applyEnabled = d->logic()->GetIsodoseNode()
                   && d->logic()->GetIsodoseNode()->GetDoseVolumeNodeId()
                   && strcmp(d->logic()->GetIsodoseNode()->GetDoseVolumeNodeId(), "")
                   && colorTableNode->GetNumberOfColors() > 0;
  d->pushButton_Apply->setEnabled(applyEnabled);
}
