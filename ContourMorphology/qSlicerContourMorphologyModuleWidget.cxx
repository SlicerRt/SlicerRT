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
#include <QDebug>

// SlicerQt includes
#include "qSlicerContourMorphologyModuleWidget.h"
#include "ui_qSlicerContourMorphologyModule.h"

// Isodose includes
#include "vtkSlicerContourMorphologyModuleLogic.h"
#include "vtkMRMLContourMorphologyNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLContourNode.h>

// SlicerRT includes
#include "SlicerRtCommon.h"

// VTK includes
#include <vtkLookupTable.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

// STD includes
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerContourMorphologyModuleWidgetPrivate: public Ui_qSlicerContourMorphologyModule
{
  Q_DECLARE_PUBLIC(qSlicerContourMorphologyModuleWidget);
protected:
  qSlicerContourMorphologyModuleWidget* const q_ptr;
public:
  qSlicerContourMorphologyModuleWidgetPrivate(qSlicerContourMorphologyModuleWidget &object);
  ~qSlicerContourMorphologyModuleWidgetPrivate();
  vtkSlicerContourMorphologyModuleLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerContourMorphologyModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModuleWidgetPrivate::qSlicerContourMorphologyModuleWidgetPrivate(qSlicerContourMorphologyModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModuleWidgetPrivate::~qSlicerContourMorphologyModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerContourMorphologyModuleLogic*
qSlicerContourMorphologyModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerContourMorphologyModuleWidget);
  return vtkSlicerContourMorphologyModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerContourMorphologyModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModuleWidget::qSlicerContourMorphologyModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerContourMorphologyModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModuleWidget::~qSlicerContourMorphologyModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetContourMorphologyNode() == 0)
    {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLContourMorphologyNode");
    if (node)
      {
      this->setContourMorphologyNode( vtkMRMLContourMorphologyNode::SafeDownCast(node) );
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::onEnter()
{
  if (!this->mrmlScene())
    {
    return;
    }

  Q_D(qSlicerContourMorphologyModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
    {
    return;
    }
  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
    {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLContourMorphologyNode");
    if (node)
      {
      paramNode = vtkMRMLContourMorphologyNode::SafeDownCast(node);
      d->logic()->SetAndObserveContourMorphologyNode(paramNode);
      return;
      }
    else 
      {
      vtkSmartPointer<vtkMRMLContourMorphologyNode> newNode = vtkSmartPointer<vtkMRMLContourMorphologyNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveContourMorphologyNode(newNode);
      }
    }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetReferenceContourNodeID()))
    {
      d->MRMLNodeComboBox_ReferenceContour->setCurrentNode(paramNode->GetReferenceContourNodeID());
    }
    else
    {
      this->referenceContourNodeChanged(d->MRMLNodeComboBox_ReferenceContour->currentNode());
    }
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetInputContourNodeID()))
    {
      d->MRMLNodeComboBox_InputContour->setCurrentNode(paramNode->GetInputContourNodeID());
    }
    else
    {
      this->inputContourNodeChanged(d->MRMLNodeComboBox_InputContour->currentNode());
    }
    int op = paramNode->GetOperation();
    switch (op)
    {
      case SLICERRT_EXPAND:
        d->radioButton_Expand->setChecked(true);
        d->radioButton_Shrink->setChecked(false);
        d->radioButton_Union->setChecked(false);
        d->radioButton_Intersect->setChecked(false);
        d->radioButton_Subtract->setChecked(false);
        break;
      case SLICERRT_SHRINK:
        d->radioButton_Expand->setChecked(false);
        d->radioButton_Shrink->setChecked(true);
        d->radioButton_Union->setChecked(false);
        d->radioButton_Intersect->setChecked(false);
        d->radioButton_Subtract->setChecked(false);
        break;
      case SLICERRT_UNION:
        d->radioButton_Expand->setChecked(false);
        d->radioButton_Shrink->setChecked(false);
        d->radioButton_Union->setChecked(true);
        d->radioButton_Intersect->setChecked(false);
        d->radioButton_Subtract->setChecked(false);
        break;
      case SLICERRT_INTERSECT:
        d->radioButton_Expand->setChecked(false);
        d->radioButton_Shrink->setChecked(false);
        d->radioButton_Union->setChecked(false);
        d->radioButton_Intersect->setChecked(true);
        d->radioButton_Subtract->setChecked(false);
        break;
      case SLICERRT_SUBTRACT:
        d->radioButton_Expand->setChecked(false);
        d->radioButton_Shrink->setChecked(false);
        d->radioButton_Union->setChecked(false);
        d->radioButton_Intersect->setChecked(false);
        d->radioButton_Subtract->setChecked(true);
        break;
    }

    std::ostringstream sstream_xsize;
    sstream_xsize << paramNode->GetXSize();
    d->lineEdit_XSize->setText(QString(sstream_xsize.str().c_str()));

    std::ostringstream sstream_ysize;
    sstream_ysize << paramNode->GetYSize();
    d->lineEdit_YSize->setText(QString(sstream_ysize.str().c_str()));

    std::ostringstream sstream_zsize;
    sstream_zsize << paramNode->GetZSize();
    d->lineEdit_ZSize->setText(QString(sstream_zsize.str().c_str()));
  }
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setup()
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  this->connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setContourMorphologyNode(vtkMRMLNode*) ) );

  this->connect( d->MRMLNodeComboBox_ReferenceContour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( referenceContourNodeChanged(vtkMRMLNode*) ) );
  this->connect( d->MRMLNodeComboBox_InputContour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( inputContourNodeChanged(vtkMRMLNode*) ) );
  this->connect( d->MRMLNodeComboBox_OutputContour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( outputContourNodeChanged(vtkMRMLNode*) ) );

  this->connect( d->radioButton_Expand, SIGNAL(clicked()), this, SLOT(radioButtonExpandClicked()));
  this->connect( d->radioButton_Shrink, SIGNAL(clicked()), this, SLOT(radioButtonShrinkClicked()));
  this->connect( d->radioButton_Union, SIGNAL(clicked()), this, SLOT(radioButtonUnionClicked()));
  this->connect( d->radioButton_Intersect, SIGNAL(clicked()), this, SLOT(radioButtonIntersectClicked()));
  this->connect( d->radioButton_Subtract, SIGNAL(clicked()), this, SLOT(radioButtonSubtractClicked()));

  this->connect( d->lineEdit_XSize, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditXSizeChanged(const QString &)));
  this->connect( d->lineEdit_YSize, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditYSizeChanged(const QString &)));
  this->connect( d->lineEdit_ZSize, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditZSizeChanged(const QString &)));

  this->connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setContourMorphologyNode(vtkMRMLNode *node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = vtkMRMLContourMorphologyNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetContourMorphologyNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveContourMorphologyNode(paramNode);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::referenceContourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  vtkMRMLContourNode* referenceContourNode = vtkMRMLContourNode::SafeDownCast(node);
  if (referenceContourNode->GetActiveRepresentationType() != vtkMRMLContourNode::IndexedLabelmap)
  {
    d->label_NotLabelmapWarning->setText(tr("Reference contour representation is not labelmap!"));
    return;
  }
  else
  {
    d->label_NotLabelmapWarning->setText("");
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceContourNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::inputContourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  vtkMRMLContourNode* inputContourNode = vtkMRMLContourNode::SafeDownCast(node);
  if (inputContourNode->GetActiveRepresentationType() != vtkMRMLContourNode::IndexedLabelmap)
  {
    d->label_NotLabelmapWarning->setText(tr("Input contour representation is not labelmap!"));
    return;
  }
  else
  {
    d->label_NotLabelmapWarning->setText("");
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveInputContourNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::outputContourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveOutputContourNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonExpandClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToExpand();
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonShrinkClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToShrink();
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonUnionClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToUnion();
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonIntersectClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToIntersect();
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonSubtractClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToSubtract();
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::lineEditXSizeChanged(const QString & text)
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  
  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetXSize(text.toDouble());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::lineEditYSizeChanged(const QString & text)
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  
  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetYSize(text.toDouble());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::lineEditZSizeChanged(const QString & text)
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  
  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetZSize(text.toDouble());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::updateButtonsState()
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  if (!this->mrmlScene())
  {
    return;
  }

  bool applyEnabled = d->logic()->GetContourMorphologyNode()
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourMorphologyNode()->GetReferenceContourNodeID())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourMorphologyNode()->GetInputContourNodeID())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourMorphologyNode()->GetOutputContourNodeID());
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::applyClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // 
  d->logic()->MorphContour();

  QApplication::restoreOverrideCursor();
}

