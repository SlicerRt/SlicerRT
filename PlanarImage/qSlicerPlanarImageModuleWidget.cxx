/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerPlanarImageModuleWidget.h"
#include "ui_qSlicerPlanarImageModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// PlanarImage includes
#include "vtkSlicerPlanarImageModuleLogic.h"
#include "vtkMRMLPlanarImageNode.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_PlanarImage
class qSlicerPlanarImageModuleWidgetPrivate: public Ui_qSlicerPlanarImageModule
{
  Q_DECLARE_PUBLIC(qSlicerPlanarImageModuleWidget);
protected:
  qSlicerPlanarImageModuleWidget* const q_ptr;
public:
  qSlicerPlanarImageModuleWidgetPrivate(qSlicerPlanarImageModuleWidget& object);
  ~qSlicerPlanarImageModuleWidgetPrivate();
  vtkSlicerPlanarImageModuleLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerPlanarImageModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerPlanarImageModuleWidgetPrivate::qSlicerPlanarImageModuleWidgetPrivate(qSlicerPlanarImageModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerPlanarImageModuleWidgetPrivate::~qSlicerPlanarImageModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerPlanarImageModuleLogic*
qSlicerPlanarImageModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerPlanarImageModuleWidget);
  return vtkSlicerPlanarImageModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerPlanarImageModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerPlanarImageModuleWidget::qSlicerPlanarImageModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerPlanarImageModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerPlanarImageModuleWidget::~qSlicerPlanarImageModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerPlanarImageModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->MRMLNodeComboBox_ParameterSet->currentNodeID().isEmpty())
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLPlanarImageNode");
    if (node)
    {
      this->setPlanarImageNode( vtkMRMLPlanarImageNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerPlanarImageModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerPlanarImageModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << "qSlicerPlanarImageModuleWidget::onEnter: Invalid logic!";
    return;
  }
  vtkMRMLPlanarImageNode* paramNode = vtkMRMLPlanarImageNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLPlanarImageNode");
    if (node)
    {
      d->MRMLNodeComboBox_ParameterSet->setCurrentNode(node);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLPlanarImageNode> newNode = vtkSmartPointer<vtkMRMLPlanarImageNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->MRMLNodeComboBox_ParameterSet->setCurrentNode(newNode);
    }
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::setPlanarImageNode(vtkMRMLNode *node)
{
  Q_D(qSlicerPlanarImageModuleWidget);

  vtkMRMLPlanarImageNode* paramNode = vtkMRMLPlanarImageNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->MRMLNodeComboBox_ParameterSet->currentNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerPlanarImageModuleWidget);

  // Update MRML node combobox selections with the nodes referenced by the parameter set node
  // Note: The comboboxes need to have noneEnabled=True in order not to select the first node in the scene
  //       automatically, but to allow this function to select it properly
  vtkMRMLNode* paramNode = d->MRMLNodeComboBox_ParameterSet->currentNode();
  if (paramNode && this->mrmlScene())
  {
    if (paramNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_VOLUME_REFERENCE_ROLE.c_str()))
    {
      d->MRMLNodeComboBox_PlanarImageVolume->setCurrentNodeID(
        paramNode->GetNodeReferenceID(SlicerRtCommon::PLANARIMAGE_VOLUME_REFERENCE_ROLE.c_str()) );
    }
    else
    {
      this->planarImageVolumeNodeChanged(d->MRMLNodeComboBox_PlanarImageVolume->currentNode());
    }
    if (paramNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()))
    {
      d->MRMLNodeComboBox_DisplayedModel->setCurrentNodeID(
        paramNode->GetNodeReferenceID(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
    }
    else
    {
      this->displayedModelNodeChanged(d->MRMLNodeComboBox_DisplayedModel->currentNode());
    }
    if (paramNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_TEXTURE_REFERENCE_ROLE.c_str()))
    {
      d->MRMLNodeComboBox_TextureVolume->setCurrentNodeID(
        paramNode->GetNodeReferenceID(SlicerRtCommon::PLANARIMAGE_TEXTURE_REFERENCE_ROLE.c_str()) );
    }
    else
    {
      this->textureVolumeNodeChanged(d->MRMLNodeComboBox_TextureVolume->currentNode());
    }
  }

  this->refreshOutputBaseName();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::setup()
{
  Q_D(qSlicerPlanarImageModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_Error->setVisible(false);

  // Make connections
  connect( d->MRMLNodeComboBox_PlanarImageVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( planarImageVolumeNodeChanged(vtkMRMLNode*) ) );

  connect( d->MRMLNodeComboBox_DisplayedModel, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( displayedModelNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_TextureVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( textureVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setPlanarImageNode(vtkMRMLNode*) ) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::planarImageVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerPlanarImageModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerPlanarImageModuleWidget::planarImageVolumeNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLNode* paramNode = d->MRMLNodeComboBox_ParameterSet->currentNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetNthNodeReferenceID(SlicerRtCommon::PLANARIMAGE_VOLUME_REFERENCE_ROLE.c_str(), 0, node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
  this->refreshOutputBaseName();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::displayedModelNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerPlanarImageModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerPlanarImageModuleWidget::displayedModelNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLNode* paramNode = d->MRMLNodeComboBox_ParameterSet->currentNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetNthNodeReferenceID(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str(), 0, node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::textureVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerPlanarImageModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerPlanarImageModuleWidget::textureVolumeNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLNode* paramNode = d->MRMLNodeComboBox_ParameterSet->currentNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetNthNodeReferenceID(SlicerRtCommon::PLANARIMAGE_TEXTURE_REFERENCE_ROLE.c_str(), 0, node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::updateButtonsState()
{
  Q_D(qSlicerPlanarImageModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerPlanarImageModuleWidget::updateButtonsState: Invalid scene!";
    return;
  }

  vtkMRMLNode* paramNode = d->MRMLNodeComboBox_ParameterSet->currentNode();
  bool applyEnabled = paramNode
                   && paramNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_VOLUME_REFERENCE_ROLE.c_str())
                   && paramNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str())
                   && paramNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_TEXTURE_REFERENCE_ROLE.c_str());
  d->pushButton_Apply->setEnabled(applyEnabled);

  d->label_Error->setVisible(false);
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::applyClicked()
{
  Q_D(qSlicerPlanarImageModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  vtkMRMLPlanarImageNode* paramNode = vtkMRMLPlanarImageNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  d->logic()->CreateModelForPlanarImage(paramNode);

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModuleWidget::refreshOutputBaseName()
{
  Q_D(qSlicerPlanarImageModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerPlanarImageModuleWidget::refreshOutputBaseName: Invalid scene!";
    return;
  }

  vtkMRMLNode* paramNode = d->MRMLNodeComboBox_ParameterSet->currentNode();
  if (!paramNode)
  {
    return;
  }

  QString newDisplayedModelBaseName(SlicerRtCommon::PLANARIMAGE_MODEL_NODE_NAME_PREFIX.c_str());
  QString newTextureVolumeBaseName(SlicerRtCommon::PLANARIMAGE_TEXTURE_NODE_NAME_PREFIX.c_str());

  vtkMRMLNode* planarImageVolumeNode = paramNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_VOLUME_REFERENCE_ROLE.c_str());
  if (planarImageVolumeNode)
  {
    newDisplayedModelBaseName.append(planarImageVolumeNode->GetName());
    newTextureVolumeBaseName.append(planarImageVolumeNode->GetName());
  }

  d->MRMLNodeComboBox_DisplayedModel->setBaseName( newDisplayedModelBaseName );
  d->MRMLNodeComboBox_TextureVolume->setBaseName( newTextureVolumeBaseName );
}
