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

//----------------------------------------------------------------------------
const char* qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY = "BeamParameterNodeAttribute";
const char* qMRMLBeamParametersTabWidget::DEPENDENT_PARAMETER_NAMES_PROPERTY = "DependentParameterNames";

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

  /// Map storing beam parameter tab widgets by name
  QMap<QString,QWidget*> BeamParametersTabWidgets;
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

  // Remove Visualization tab as it is not yet functional
  //TODO: Re-enable when works
  q->removeTab(1);
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
void qMRMLBeamParametersTabWidget::addBeamParameterFloatingPointNumber(
  QString tabName, QString parameterName, QString parameterLabel,
  QString tooltip, double minimumValue, double maximumValue,
  double defaultValue, double stepSize, int precision, bool slider/*=false*/)
{
  // Get tab to which the widget needs to be added
  QWidget* tabWidget = this->beamParametersTab(tabName);
  if (!tabWidget)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to access widget for beam parameters tab named " << tabName;
    return;
  }
  QFormLayout* tabLayout = qobject_cast<QFormLayout*>(tabWidget->layout());
  if (!tabLayout)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid layout in beam parameters tab named " << tabName;
    return;
  }

  if (slider)
  {
    QSlider* slider = new QSlider(tabWidget);
    slider->setToolTip(tooltip);
    slider->setRange(minimumValue, maximumValue);
    slider->setValue(defaultValue);
    slider->setSingleStep(stepSize);
    slider->setProperty(qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY, parameterName);
    connect( slider, SIGNAL(valueChanged(double)), this, SLOT(doubleBeamParameterChanged(double)) );
    tabLayout->addRow(parameterLabel, slider);
  }
  else
  {
    QDoubleSpinBox* spinBox = new QDoubleSpinBox(tabWidget);
    spinBox->setToolTip(tooltip);
    spinBox->setRange(minimumValue, maximumValue);
    spinBox->setValue(defaultValue);
    spinBox->setSingleStep(stepSize);
    spinBox->setDecimals(precision);
    spinBox->setProperty(qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY, parameterName);
    connect( spinBox, SIGNAL(valueChanged(double)), this, SLOT(doubleBeamParameterChanged(double)) );
    tabLayout->addRow(parameterLabel, spinBox);
  }
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::addBeamParameterComboBox(
  QString tabName, QString parameterName, QString parameterLabel,
  QString tooltip, QStringList options, int defaultIndex )
{
  // Get tab to which the spin box needs to be added
  QWidget* tabWidget = this->beamParametersTab(tabName);
  if (!tabWidget)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to access widget for beam parameters tab named " << tabName;
    return;
  }
  QFormLayout* tabLayout = qobject_cast<QFormLayout*>(tabWidget->layout());
  if (!tabLayout)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid layout in beam parameters tab named " << tabName;
    return;
  }

  QComboBox* comboBox = new QComboBox(tabWidget);
  foreach (QString option, options)
  {
    comboBox->addItem(option);
  }
  comboBox->setToolTip(tooltip);
  comboBox->setCurrentIndex(defaultIndex);
  comboBox->setProperty(qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY, parameterName);
  connect( comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(integerBeamParameterChanged(int)) );
  tabLayout->addRow(parameterLabel, comboBox);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::addBeamParameterCheckBox(
  QString tabName, QString parameterName, QString parameterLabel,
  QString tooltip, bool defaultValue, QStringList dependentParameterNames/*=QStringList()*/ )
{
  // Get tab to which the spin box needs to be added
  QWidget* tabWidget = this->beamParametersTab(tabName);
  if (!tabWidget)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to access widget for beam parameters tab named " << tabName;
    return;
  }
  QFormLayout* tabLayout = qobject_cast<QFormLayout*>(tabWidget->layout());
  if (!tabLayout)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid layout in beam parameters tab named " << tabName;
    return;
  }

  QCheckBox* checkBox = new QCheckBox(tabWidget);
  checkBox->setToolTip(tooltip);
  checkBox->setChecked(defaultValue);
  checkBox->setProperty(qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY, parameterName);
  checkBox->setProperty(qMRMLBeamParametersTabWidget::DEPENDENT_PARAMETER_NAMES_PROPERTY, dependentParameterNames);
  connect( checkBox, SIGNAL(stateChanged(int)), this, SLOT(booleanBeamParameterChanged(int)) );
  tabLayout->addRow(parameterLabel, checkBox);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::doubleBeamParameterChanged(double newValue)
{
  Q_D(qMRMLBeamParametersTabWidget);
  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node, cannot set parameter!";
    return;
  }

  // Get attribute name that belongs to the widget in which the value was changed
  QString attributeName = this->sender()->property(qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString();

  // Set parameter as attribute in beam node
  d->BeamNode->DisableModifiedEventOn();
  d->BeamNode->SetAttribute(
    attributeName.toLatin1().constData(),
    QString::number(newValue).toLatin1().constData() );
  d->BeamNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::integerBeamParameterChanged(int newValue)
{
  Q_D(qMRMLBeamParametersTabWidget);
  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node, cannot set parameter!";
    return;
  }

  // Get attribute name that belongs to the widget in which the value was changed
  QString attributeName = this->sender()->property(qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString();

  // Set parameter as attribute in beam node
  d->BeamNode->DisableModifiedEventOn();
  d->BeamNode->SetAttribute(
    attributeName.toLatin1().constData(),
    QString::number(newValue).toLatin1().constData() );
  d->BeamNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::booleanBeamParameterChanged(int newValue)
{
  Q_D(qMRMLBeamParametersTabWidget);
  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node, cannot set parameter!";
    return;
  }

  // Get attribute name that belongs to the widget in which the value was changed
  QString attributeName = this->sender()->property(qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString();

  // Set parameter as attribute in beam node
  d->BeamNode->DisableModifiedEventOn();
  d->BeamNode->SetAttribute(
    attributeName.toLatin1().constData(),
    QString::number(newValue == 0 ? 0 : 1).toLatin1().constData() );
  d->BeamNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
bool qMRMLBeamParametersTabWidget::setBeamParameterVisible(QString parameterName, bool visible)
{
  Q_D(qMRMLBeamParametersTabWidget);

  // Go through all tabs to find the widget for the specified parameter
  foreach (QWidget* tabWidget, d->BeamParametersTabWidgets)
  {
    QFormLayout* currentLayout = qobject_cast<QFormLayout*>(tabWidget->layout());
    if (!currentLayout)
    {
      // Tab layout must be form layout
      qWarning() << Q_FUNC_INFO << ": Invalid layout in beams parameter tab";
      continue;
    }

    // Go through the layout rows (because both the parameter widget and its label need to be hidden)
    for (int currentRow=0; currentRow<currentLayout->rowCount(); ++currentRow)
    {
      QWidget* currentParameterFieldWidget = currentLayout->itemAt(currentRow, QFormLayout::FieldRole)->widget();
      if ( currentParameterFieldWidget->property(qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString()
        == parameterName )
      {
        // Widget for parameter found. Set visibility for that and the label
        currentParameterFieldWidget->setVisible(visible);
        QWidget* currentParameterLabelWidget = currentLayout->itemAt(currentRow, QFormLayout::LabelRole)->widget();
        currentParameterLabelWidget->setVisible(visible);

        // Hide tab if became empty, show tab if was empty but not any more
        //TODO: Use removeTab(index) and addTab(widget, title)

        // Visibility set, task completed
        return true;
      }
    }
  }

  qCritical() << Q_FUNC_INFO << ": Failed to find widget for beam parameter named '" << parameterName << "'";
  return false;
}

//-----------------------------------------------------------------------------
QWidget* qMRMLBeamParametersTabWidget::beamParametersTab(QString tabName)
{
  Q_D(qMRMLBeamParametersTabWidget);

  // Check if the tab is cached already
  if (d->BeamParametersTabWidgets.contains(tabName))
  {
    return d->BeamParametersTabWidgets[tabName];
  }

  // Find tab with given name
  QWidget* tabWidget = NULL;
  for (int index=0; index<this->count(); ++index)
  {
    if (!this->tabText(index).compare(tabName))
    {
      tabWidget = this->widget(index);
      if (!qobject_cast<QFormLayout*>(tabWidget->layout()))
      {
        // Tab layout must be form layout
        qCritical() << Q_FUNC_INFO << ": Invalid layout in tab named " << tabName;
        return NULL;
      }
      break;
    }
  }

  // If tab was not found, then create a new one with given name
  if (!tabWidget)
  {
    tabWidget = new QWidget(this);
    QFormLayout* tabLayout = new QFormLayout(tabWidget);
    tabWidget->setLayout(tabLayout);

    this->addTab(tabWidget, tabName);
  }

  // Add tab widget to cache
  d->BeamParametersTabWidgets[tabName] = tabWidget;
  return tabWidget;
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
}

//-----------------------------------------------------------------------------
// Default beam parameter handler functions
//-----------------------------------------------------------------------------

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
