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

  This file was originally developed by Kevin Wang, RMP, PMH
  
==============================================================================*/

// Qt includes
#include <QCheckBox>

// SlicerQt includes
#include "qSlicerProtonDoseModuleWidget.h"
#include "ui_qSlicerProtonDoseModule.h"

// ProtonDose includes
#include "vtkSlicerProtonDoseModuleLogic.h"
#include "vtkMRMLProtonDoseNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>

// STD includes
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ProtonDose
class qSlicerProtonDoseModuleWidgetPrivate: public Ui_qSlicerProtonDoseModule
{
  Q_DECLARE_PUBLIC(qSlicerProtonDoseModuleWidget);
protected:
  qSlicerProtonDoseModuleWidget* const q_ptr;
public:
  qSlicerProtonDoseModuleWidgetPrivate(qSlicerProtonDoseModuleWidget &object);
  ~qSlicerProtonDoseModuleWidgetPrivate();
  vtkSlicerProtonDoseModuleLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerProtonDoseModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerProtonDoseModuleWidgetPrivate::qSlicerProtonDoseModuleWidgetPrivate(qSlicerProtonDoseModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerProtonDoseModuleWidgetPrivate::~qSlicerProtonDoseModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerProtonDoseModuleLogic*
qSlicerProtonDoseModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerProtonDoseModuleWidget);
  return vtkSlicerProtonDoseModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerProtonDoseModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerProtonDoseModuleWidget::qSlicerProtonDoseModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerProtonDoseModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerProtonDoseModuleWidget::~qSlicerProtonDoseModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerProtonDoseModuleWidget);

  this->Superclass::setMRMLScene(scene);

  //qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetProtonDoseNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLProtonDoseNode");
    if (node)
    {
      this->setProtonDoseNode( vtkMRMLProtonDoseNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    return;
  }

  Q_D(qSlicerProtonDoseModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLProtonDoseNode");
    if (node)
    {
      paramNode = vtkMRMLProtonDoseNode::SafeDownCast(node);
      d->logic()->SetAndObserveProtonDoseNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLProtonDoseNode> newNode = vtkSmartPointer<vtkMRMLProtonDoseNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveProtonDoseNode(newNode);
    }
  }

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerProtonDoseModuleWidget);

  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
#if defined (commentout)
    if (paramNode->GetDoseVolumeNodeId() && strcmp(paramNode->GetDoseVolumeNodeId(),""))
    {
      d->MRMLNodeComboBox_DoseVolume->setCurrentNode(paramNode->GetDoseVolumeNodeId());
    }
    else
    {
      this->doseVolumeNodeChanged(d->MRMLNodeComboBox_DoseVolume->currentNode());
    }
#endif
  }
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::onLogicModified()
{
  Q_D(qSlicerProtonDoseModuleWidget);

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::setup()
{
  Q_D(qSlicerProtonDoseModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setProtonDoseNode(vtkMRMLNode*) ) );
#if defined (commentout)
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
#endif

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_OutputHierarchy, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( outputHierarchyNodeChanged(vtkMRMLNode*) ) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::setProtonDoseNode(vtkMRMLNode *node)
{
  Q_D(qSlicerProtonDoseModuleWidget);

  vtkMRMLProtonDoseNode* paramNode = vtkMRMLProtonDoseNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetProtonDoseNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveProtonDoseNode(paramNode);
  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerProtonDoseModuleWidget);

  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::outputHierarchyNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerProtonDoseModuleWidget);

  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveOutputHierarchyNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::storeSelectedTableItemText(QTableWidgetItem* selectedItem, QTableWidgetItem* previousItem)
{
  Q_D(qSlicerProtonDoseModuleWidget);

}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::applyClicked()
{
  Q_D(qSlicerProtonDoseModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the iso dose surface for the selected dose volume
  d->logic()->ComputeProtonDose();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::updateButtonsState()
{
  Q_D(qSlicerProtonDoseModuleWidget);

  bool applyEnabled = d->logic()->GetProtonDoseNode()
                   && d->logic()->GetProtonDoseNode()->GetDoseVolumeNodeId()
                   && strcmp(d->logic()->GetProtonDoseNode()->GetDoseVolumeNodeId(), "");
  d->pushButton_Apply->setEnabled(applyEnabled);
}

