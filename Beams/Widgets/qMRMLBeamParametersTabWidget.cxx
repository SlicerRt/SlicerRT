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

// Beams includes
#include "qMRMLBeamParametersTabWidget.h"
#include "ui_qMRMLBeamParametersTabWidget.h"

#include "vtkMRMLRTBeamNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkWeakPointer.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
class qMRMLBeamParametersTabWidgetPrivate: public Ui_qMRMLBeamParametersTabWidget
{
  Q_DECLARE_PUBLIC(qMRMLBeamParametersTabWidget);

protected:
  qMRMLBeamParametersTabWidget* const q_ptr;
public:
  qMRMLBeamParametersTabWidgetPrivate(qMRMLBeamParametersTabWidget& object);
  void init();

public:
  /// RT beam MRML node containing shown parameters
  vtkWeakPointer<vtkMRMLRTBeamNode> BeamNode;
};

//-----------------------------------------------------------------------------
qMRMLBeamParametersTabWidgetPrivate::qMRMLBeamParametersTabWidgetPrivate(qMRMLBeamParametersTabWidget& object)
  : q_ptr(&object)
  , BeamNode(NULL)
{
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidgetPrivate::init()
{
  Q_Q(qMRMLBeamParametersTabWidget);
  this->setupUi(q);

  // Geometry page
  QObject::connect( this->MRMLNodeComboBox_MLCPositionDoubleArray, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(mlcPositionDoubleArrayNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->doubleSpinBox_SAD, SIGNAL(valueChanged(double)), q, SLOT(sourceDistanceChanged(double)) );
  QObject::connect( this->RangeWidget_XJawsPosition, SIGNAL(valuesChanged(double, double)), q, SLOT(xJawsPositionValuesChanged(double, double)) );
  QObject::connect( this->RangeWidget_YJawsPosition, SIGNAL(valuesChanged(double, double)), q, SLOT(yJawsPositionValuesChanged(double, double)) );
  QObject::connect( this->SliderWidget_CollimatorAngle, SIGNAL(valueChanged(double)), q, SLOT(collimatorAngleChanged(double)) );
  QObject::connect( this->SliderWidget_GantryAngle, SIGNAL(valueChanged(double)), q, SLOT(gantryAngleChanged(double)) );
  QObject::connect( this->SliderWidget_CouchAngle, SIGNAL(valueChanged(double)), q, SLOT(couchAngleChanged(double)) );

  // Visualization page
  QObject::connect( this->pushButton_UpdateDRR, SIGNAL(clicked()), q, SLOT(updateDRRClicked()) );
  QObject::connect( this->checkBox_BeamsEyeView, SIGNAL(clicked(bool)), q, SLOT(beamEyesViewClicked(bool)) );
  QObject::connect( this->checkBox_ContoursInBEW, SIGNAL(clicked(bool)), q, SLOT(contoursInBEWClicked(bool)) );
}


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qMRMLBeamParametersTabWidget methods

//-----------------------------------------------------------------------------
qMRMLBeamParametersTabWidget::qMRMLBeamParametersTabWidget(QWidget* _parent)
  : QTabWidget(_parent)
  , d_ptr(new qMRMLBeamParametersTabWidgetPrivate(*this))
{
  Q_D(qMRMLBeamParametersTabWidget);
  d->init();
}

//-----------------------------------------------------------------------------
qMRMLBeamParametersTabWidget::~qMRMLBeamParametersTabWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::setBeamNode(vtkMRMLNode* node)
{
  Q_D(qMRMLBeamParametersTabWidget);

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(node);

  // Connect display modified event to population of the table
  //qvtkReconnect( d->SegmentationNode, segmentationNode, vtkMRMLDisplayableNode::DisplayModifiedEvent,
  //               this, SLOT( updateWidgetFromMRML() ) );

  d->BeamNode = beamNode;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLBeamParametersTabWidget::beamNode()
{
  Q_D(qMRMLBeamParametersTabWidget);

  return d->BeamNode;
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    return;
  }

  // Set values into geometry tab
  d->doubleSpinBox_SAD->setValue(d->BeamNode->GetSAD());
  d->RangeWidget_XJawsPosition->setValues(d->BeamNode->GetX1Jaw(), d->BeamNode->GetX2Jaw());
  d->RangeWidget_YJawsPosition->setValues(d->BeamNode->GetY1Jaw(), d->BeamNode->GetY2Jaw());
  d->SliderWidget_CollimatorAngle->setValue(d->BeamNode->GetCollimatorAngle());
  d->SliderWidget_GantryAngle->blockSignals(true);
  d->SliderWidget_GantryAngle->setValue(d->BeamNode->GetGantryAngle());
  d->SliderWidget_GantryAngle->blockSignals(false);
  d->SliderWidget_CouchAngle->setValue(d->BeamNode->GetCouchAngle());

  //TODO:
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::mlcPositionDoubleArrayNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLBeamParametersTabWidget);

  // GCS FIX TODO *** Come back to this later ***
#if defined (commentout)
  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  // Get rt plan node for ExternalBeamPlanning node
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetExternalBeamPlanningNode()->GetRtPlanNode();
  if (!rtPlanNode)
  { 
    qCritical() << Q_FUNC_INFO << ": Invalid plan node!";
    return;
  }

  paramNode->SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Inputs are not initialized!";
    return;
  }

  beamNode->DisableModifiedEventOn();
  beamNode->SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode::SafeDownCast(node));
  beamNode->DisableModifiedEventOff();
#endif
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::xJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  // Do not disable modifier events as geometry need to be updated
  d->BeamNode->SetX1Jaw(minVal);
  d->BeamNode->SetX2Jaw(maxVal);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::yJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  // Do not disable modifier events as geometry need to be updated
  d->BeamNode->SetY1Jaw(minVal);
  d->BeamNode->SetY2Jaw(maxVal);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::gantryAngleChanged(double value)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  // Do not disable modifier events as geometry need to be updated
  d->BeamNode->SetGantryAngle(value);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::collimatorAngleChanged(double value)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  // Do not disable modifier events as geometry need to be updated
  d->BeamNode->SetCollimatorAngle(value);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::couchAngleChanged(double value)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  // Do not disable modifier events as geometry need to be updated
  //TODO: Does not seem to be doing anything (IEC logic needs to be used!)
  d->BeamNode->SetCouchAngle(value);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::sourceDistanceChanged(double value)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  // Do not disable modifier events as geometry need to be updated
  d->BeamNode->SetSAD(value);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::beamEyesViewClicked(bool checked)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  //TODO:
  //if (checked)
  //{
  //  qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutTwoOverTwoView);
  //}
  //else
  //{
  //  qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutFourUpView);
  //}

  //TODO: Set camera
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::contoursInBEWClicked(bool checked)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  // TODO: add the logic to check if contours should be included in the DRR view
  // right now the contours are included always. 
  if (checked)
  {
  }
  else
  {
  }
  qWarning() << Q_FUNC_INFO << ": Not implemented!";
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::updateDRRClicked()
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  //TODO: Fix DRR code (it is in EBP logic and it is commented out)
  //d->logic()->UpdateDRR(beamNode->GetName());
  qCritical() << Q_FUNC_INFO << ": Not implemented!";
}
