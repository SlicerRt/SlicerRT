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

// Segmentations Module Logic includes
#include <vtkSlicerSegmentationsModuleLogic.h>

// SlicerRT includes
#include <vtkSlicerBeamsModuleLogic.h>
#include <vtkSlicerMLCPositionLogic.h>

#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTIonBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLTableNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLMarkupsClosedCurveNode.h> // for convex hull curve
#include <vtkMRMLSegmentationNode.h> // for RTSTRUCT sermentation data

// VTK includes
#include <vtkWeakPointer.h>

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <qSlicerAbstractCoreModule.h>

// Qt includes
#include <QDebug>
#include <QLineEdit>

//----------------------------------------------------------------------------
const char* qMRMLBeamParametersTabWidget::BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY = "BeamParameterNodeAttribute";
const char* qMRMLBeamParametersTabWidget::DEPENDENT_PARAMETER_NAMES_PROPERTY = "DependentParameterNames";
const char* qMRMLBeamParametersTabWidget::USED_BY_CURRENT_ENGINE_PROPERTY = "UsedByCurrentEngine";

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

  /// Logic for MLC position calculation
  vtkSlicerMLCPositionLogic* MLCPositionLogic;
};

//-----------------------------------------------------------------------------
qMRMLBeamParametersTabWidgetPrivate::qMRMLBeamParametersTabWidgetPrivate(qMRMLBeamParametersTabWidget& object)
  : q_ptr(&object)
  , BeamNode(nullptr)
  , MLCPositionLogic(nullptr)
{
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidgetPrivate::init()
{
  Q_Q(qMRMLBeamParametersTabWidget);
  this->setupUi(q);

  // Geometry page
  QObject::connect( this->MRMLNodeComboBox_MLCBoundaryAndPositionTable, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(mlcBoundaryAndPositionTableNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->doubleSpinBox_SAD, SIGNAL(valueChanged(double)), q, SLOT(sourceDistanceChanged(double)) );
  QObject::connect( this->doubleSpinBox_DistanceMLC, SIGNAL(valueChanged(double)), q, SLOT(mlcDistanceChanged(double)) );
  QObject::connect( this->RangeWidget_XJawsPosition, SIGNAL(valuesChanged(double, double)), q, SLOT(xJawsPositionValuesChanged(double, double)) );
  QObject::connect( this->RangeWidget_YJawsPosition, SIGNAL(valuesChanged(double, double)), q, SLOT(yJawsPositionValuesChanged(double, double)) );
  QObject::connect( this->SliderWidget_CollimatorAngle, SIGNAL(valueChanged(double)), q, SLOT(collimatorAngleChanged(double)) );
  QObject::connect( this->SliderWidget_GantryAngle, SIGNAL(valueChanged(double)), q, SLOT(gantryAngleChanged(double)) );
  QObject::connect( this->SliderWidget_CouchAngle, SIGNAL(valueChanged(double)), q, SLOT(couchAngleChanged(double)) );

  // Visualization page
  QObject::connect( this->pushButton_UpdateDRR, SIGNAL(clicked()), q, SLOT(updateDRRClicked()) );
  QObject::connect( this->checkBox_BeamsEyeView, SIGNAL(clicked(bool)), q, SLOT(beamEyesViewClicked(bool)) );
  QObject::connect( this->checkBox_ContoursInBEW, SIGNAL(clicked(bool)), q, SLOT(contoursInBEWClicked(bool)) );

  // Multi Leaf Collimator page
  QObject::connect( this->pushButton_GenerateMLCBoundary, SIGNAL(clicked()), q, SLOT(generateMLCboundaryClicked()) );
  QObject::connect( this->pushButton_UpdateMLCBoundary, SIGNAL(clicked()), q, SLOT(updateMLCboundaryClicked()) );
  QObject::connect( this->pushButton_CalculateMLCPosition, SIGNAL(clicked()), q, SLOT(calculateMLCPositionClicked()) );

  // Load MLC position calculation logic
  qSlicerCoreApplication* app = qSlicerCoreApplication::application();
  if (!app)
  {
    qCritical() << Q_FUNC_INFO << ": MLC Position logic is not found (not a Slicer core application instance)";
  }
  else
  {
    vtkSlicerBeamsModuleLogic* beamsLogic = vtkSlicerBeamsModuleLogic::SafeDownCast(app->moduleLogic("Beams"));
    if (beamsLogic)
    {
      this->MLCPositionLogic = beamsLogic->GetMLCPositionLogic();
    }
  }

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
qMRMLBeamParametersTabWidget::~qMRMLBeamParametersTabWidget() = default;

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::setBeamNode(vtkMRMLNode* node)
{
  Q_D(qMRMLBeamParametersTabWidget);

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(node);

  // Connect display modified event to population of the table
  qvtkReconnect( d->BeamNode, beamNode, vtkCommand::ModifiedEvent,
                 this, SLOT( updateWidgetFromMRML() ) );

  d->BeamNode = beamNode;
  this->updateWidgetFromMRML();
  this->setCurrentIndex(0);
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

  // Update values into geometry tab
  d->doubleSpinBox_SAD->setValue(d->BeamNode->GetSAD());
  d->RangeWidget_XJawsPosition->setValues(d->BeamNode->GetX1Jaw(), d->BeamNode->GetX2Jaw());
  d->RangeWidget_YJawsPosition->setValues(d->BeamNode->GetY1Jaw(), d->BeamNode->GetY2Jaw());
  d->SliderWidget_CollimatorAngle->setValue(d->BeamNode->GetCollimatorAngle());
  d->SliderWidget_GantryAngle->blockSignals(true);
  d->SliderWidget_GantryAngle->setValue(d->BeamNode->GetGantryAngle());
  d->SliderWidget_GantryAngle->blockSignals(false);
  d->SliderWidget_CouchAngle->setValue(d->BeamNode->GetCouchAngle());

  d->MRMLNodeComboBox_MLCBoundaryAndPositionTable->setMRMLScene(d->BeamNode->GetScene());

  // Check for MLC table and enable combo box
  if (vtkMRMLTableNode* mlcTable = d->BeamNode->GetMultiLeafCollimatorTableNode())
  {
    d->MRMLNodeComboBox_MLCBoundaryAndPositionTable->setCurrentNode(mlcTable);
    d->pushButton_UpdateMLCBoundary->setEnabled(true);
  }
  else
  {
    d->MRMLNodeComboBox_MLCBoundaryAndPositionTable->setCurrentNode(nullptr);
    d->MRMLNodeComboBox_MLCBoundaryAndPositionTable->setEnabled(false);
    d->pushButton_UpdateMLCBoundary->setEnabled(false);
  }

  // Update engine-specific values
  foreach (QWidget* tabWidget, d->BeamParametersTabWidgets)
  {
    QFormLayout* currentLayout = qobject_cast<QFormLayout*>(tabWidget->layout());
    if (!currentLayout)
    {
      // Tab layout must be form layout
      qWarning() << Q_FUNC_INFO << ": Invalid layout in beams parameter tab";
      continue;
    }

    // Go through the layout rows to determine which parameter belongs to the current engine
    for (int currentRow=0; currentRow<currentLayout->rowCount(); ++currentRow)
    {
      QWidget* currentParameterFieldWidget = currentLayout->itemAt(currentRow, QFormLayout::FieldRole)->widget();

      if (currentParameterFieldWidget->property(USED_BY_CURRENT_ENGINE_PROPERTY).toBool())
      {
        QString attributeName = currentParameterFieldWidget->property(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString();
        QString parameterValue = d->BeamNode->GetAttribute(attributeName.toUtf8().constData());

        // Set value to supported widget types
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(currentParameterFieldWidget);
        ctkSliderWidget* slider = qobject_cast<ctkSliderWidget*>(currentParameterFieldWidget);
        QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(currentParameterFieldWidget);
        QComboBox* comboBox = qobject_cast<QComboBox*>(currentParameterFieldWidget);
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(currentParameterFieldWidget);
        if (lineEdit)
        {
          lineEdit->blockSignals(true);
          lineEdit->setText(parameterValue);
          lineEdit->blockSignals(false);
        }
        else if (slider)
        {
          slider->blockSignals(true);
          slider->setValue(parameterValue.toDouble());
          slider->blockSignals(false);
        }
        else if (spinBox)
        {
          spinBox->blockSignals(true);
          spinBox->setValue(parameterValue.toDouble());
          spinBox->blockSignals(false);
        }
        else if (comboBox)
        {
          comboBox->blockSignals(true);
          comboBox->setCurrentIndex(parameterValue.toInt());
          comboBox->blockSignals(false);
        }
        else if (checkBox)
        {
          checkBox->blockSignals(true);
          checkBox->setChecked(QVariant(parameterValue).toBool());
          checkBox->blockSignals(false);

          // Enable/disable dependent parameters
          this->updateDependentParameterWidgetsForCheckbox(checkBox);
        }
      }
    } // For each row
  } // For each tab

  // Update RTBeamNode specific widgets according to the type of rt beam node
  vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(d->BeamNode);
  if (d->BeamNode && !ionBeamNode)
  {
    connect( d->doubleSpinBox_SAD, SIGNAL(valueChanged(double)), this, SLOT(sourceDistanceChanged(double)) );
    disconnect( d->doubleSpinBox_VSADx, SIGNAL(valueChanged(double)), this, SLOT(virtualSourceAxisXDistanceChanged(double)) );
    disconnect( d->doubleSpinBox_VSADy, SIGNAL(valueChanged(double)), this, SLOT(virtualSourceAxisYDistanceChanged(double)) );
    // rename some labels
    d->label_DistanceMLC->setText(tr("Source to MLC distance (mm):"));

    // hide widgets from rt beam
    d->label_VSADx->hide();
    d->doubleSpinBox_VSADx->hide();
    d->label_VSADy->hide();
    d->doubleSpinBox_VSADy->hide();
    d->label_ScanSpotParameters->hide();
    d->MRMLNodeComboBox_ScanSpotParametersTable->hide();

    // show widgets for rt beam
    d->doubleSpinBox_SAD->show();
    d->label_SAD->show();
  }
  else if (ionBeamNode)
  {
    disconnect( d->doubleSpinBox_SAD, SIGNAL(valueChanged(double)), this, SLOT(sourceDistanceChanged(double)) );
    connect( d->doubleSpinBox_VSADx, SIGNAL(valueChanged(double)), this, SLOT(virtualSourceAxisXDistanceChanged(double)) );
    connect( d->doubleSpinBox_VSADy, SIGNAL(valueChanged(double)), this, SLOT(virtualSourceAxisYDistanceChanged(double)) );

    // Check for ScanSpot table and enable combo box
    if (vtkMRMLTableNode* scanspotTable = ionBeamNode->GetScanSpotTableNode())
    {
      d->MRMLNodeComboBox_ScanSpotParametersTable->setMRMLScene(d->BeamNode->GetScene());
      d->MRMLNodeComboBox_ScanSpotParametersTable->setCurrentNode(scanspotTable);
    }
    else
    {
      d->MRMLNodeComboBox_ScanSpotParametersTable->setCurrentNode(nullptr);
      d->MRMLNodeComboBox_ScanSpotParametersTable->setEnabled(false);
    }

    // rename some labels
    d->label_DistanceMLC->setText(tr("Isocenter to MLC distance (mm):"));

    // hide widgets from rt ion beam
    d->doubleSpinBox_SAD->hide();
    d->label_SAD->hide();

    // show widgets for rt ion beam
    d->label_VSADx->show();
    d->doubleSpinBox_VSADx->show();
    d->label_VSADy->show();
    d->doubleSpinBox_VSADy->show();
    d->label_ScanSpotParameters->show();
    d->MRMLNodeComboBox_ScanSpotParametersTable->show();
  }
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::updateDependentParameterWidgetsForCheckbox(QCheckBox* checkBox)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!checkBox)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid checkbox!";
    return;
  }

  QStringList dependentParameterNames = checkBox->property(DEPENDENT_PARAMETER_NAMES_PROPERTY).toStringList();
  bool enabled = checkBox->isChecked();

  // Enable/disable each dependent parameters
  foreach (QString parameter, dependentParameterNames)
  {
    // Find widget for parameter
    foreach (QWidget* tabWidget, d->BeamParametersTabWidgets)
    {
      QFormLayout* currentLayout = qobject_cast<QFormLayout*>(tabWidget->layout());
      if (!currentLayout)
      {
        // Tab layout must be form layout
        qWarning() << Q_FUNC_INFO << ": Invalid layout in beams parameter tab";
        continue;
      }

      for (int currentRow=0; currentRow<currentLayout->rowCount(); ++currentRow)
      {
        QWidget* currentParameterFieldWidget = currentLayout->itemAt(currentRow, QFormLayout::FieldRole)->widget();
        if ( !dependentParameterNames.contains(
          currentParameterFieldWidget->property(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString()) )
        {
          continue;
        }

        // The current parameter depends on the input checkbox. Set its enabled state
        QWidget* currentParameterLabelWidget = currentLayout->itemAt(currentRow, QFormLayout::LabelRole)->widget();
        currentParameterLabelWidget->setEnabled(enabled);
        currentParameterFieldWidget->setEnabled(enabled);
      }
    }
  } // For each dependent parameter
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
  QWidget* tabWidget = nullptr;
  for (int index=0; index<this->count(); ++index)
  {
    if (!this->tabText(index).compare(tabName))
    {
      tabWidget = this->widget(index);
      if (!qobject_cast<QFormLayout*>(tabWidget->layout()))
      {
        // Tab layout must be form layout
        qCritical() << Q_FUNC_INFO << ": Invalid layout in tab named " << tabName;
        return nullptr;
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
    ctkSliderWidget* slider = new ctkSliderWidget(tabWidget);
    slider->setToolTip(tooltip);
    slider->setRange(minimumValue, maximumValue);
    slider->setValue(defaultValue);
    slider->setSingleStep(stepSize);
    slider->setProperty(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY, parameterName);
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
    spinBox->setProperty(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY, parameterName);
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
  comboBox->setProperty(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY, parameterName);
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
  checkBox->setProperty(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY, parameterName);
  checkBox->setProperty(DEPENDENT_PARAMETER_NAMES_PROPERTY, dependentParameterNames);
  connect( checkBox, SIGNAL(stateChanged(int)), this, SLOT(booleanBeamParameterChanged(int)) );
  tabLayout->addRow(parameterLabel, checkBox);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::addBeamParameterLineEdit(
  QString tabName, QString parameterName, QString parameterLabel,
  QString tooltip, QString defaultValue )
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

  QLineEdit* lineEdit = new QLineEdit(defaultValue, tabWidget);
  lineEdit->setToolTip(tooltip);
  lineEdit->setProperty(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY, parameterName);
  connect( lineEdit, SIGNAL(textChanged(QString)), this, SLOT(stringBeamParameterChanged(QString)) );
  tabLayout->addRow(parameterLabel, lineEdit);
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
  QString attributeName = this->sender()->property(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString();

  // Set parameter as attribute in beam node
  d->BeamNode->DisableModifiedEventOn();
  d->BeamNode->SetAttribute(
    attributeName.toUtf8().constData(),
    QString::number(newValue).toUtf8().constData() );
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
  QString attributeName = this->sender()->property(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString();

  // Set parameter as attribute in beam node
  d->BeamNode->DisableModifiedEventOn();
  d->BeamNode->SetAttribute(
    attributeName.toUtf8().constData(),
    QString::number(newValue).toUtf8().constData() );
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
  QString attributeName = this->sender()->property(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString();

  // Set parameter as attribute in beam node
  d->BeamNode->DisableModifiedEventOn();
  d->BeamNode->SetAttribute(
    attributeName.toUtf8().constData(),
    QVariant(newValue != 0).toString().toUtf8().constData() );
  d->BeamNode->DisableModifiedEventOff();

  // Enable/disable dependent parameters
  this->updateDependentParameterWidgetsForCheckbox( qobject_cast<QCheckBox*>(this->sender()) );
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::stringBeamParameterChanged(QString newValue)
{
  Q_D(qMRMLBeamParametersTabWidget);
  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node, cannot set parameter!";
    return;
  }

  // Get attribute name that belongs to the widget in which the value was changed
  QString attributeName = this->sender()->property(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString();

  // Set parameter as attribute in beam node
  d->BeamNode->DisableModifiedEventOn();
  d->BeamNode->SetAttribute(
    attributeName.toUtf8().constData(),
    newValue.toUtf8().constData() );
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
      if ( parameterName ==
        currentParameterFieldWidget->property(BEAM_PARAMETER_NODE_ATTRIBUTE_PROPERTY).toString() )
      {
        // Widget for parameter found. Set visibility for that and the label
        currentParameterFieldWidget->setVisible(visible);
        currentParameterFieldWidget->setProperty(USED_BY_CURRENT_ENGINE_PROPERTY, QVariant(visible)); // Property is needed instead of simply querying visibility as it is set to false when tab is removed
        QWidget* currentParameterLabelWidget = currentLayout->itemAt(currentRow, QFormLayout::LabelRole)->widget();
        currentParameterLabelWidget->setVisible(visible);

        // Visibility set, task completed
        return true;
      }
    }
  }

  qCritical() << Q_FUNC_INFO << ": Failed to find widget for beam parameter named '" << parameterName << "'";
  return false;
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::updateTabVisibility()
{
  Q_D(qMRMLBeamParametersTabWidget);

  // Go through all tabs to update visibility
  foreach (QString tabName, d->BeamParametersTabWidgets.keys())
  {
    QWidget* tabWidget = d->BeamParametersTabWidgets[tabName];
    QFormLayout* currentLayout = qobject_cast<QFormLayout*>(tabWidget->layout());
    if (!currentLayout)
    {
      // Tab layout must be form layout
      qWarning() << Q_FUNC_INFO << ": Invalid layout in beams parameter tab";
      continue;
    }

    // Go through the layout rows to determine if any parameters are visible
    bool tabVisible = false;
    for (int currentRow=0; currentRow<currentLayout->rowCount(); ++currentRow)
    {
      QWidget* currentParameterFieldWidget = currentLayout->itemAt(currentRow, QFormLayout::FieldRole)->widget();

      if (currentParameterFieldWidget->property(USED_BY_CURRENT_ENGINE_PROPERTY).toBool())
      {
        tabVisible = true;
        break;
      }
    }

    // Show tab if was empty but not any more
    int tabIndex = this->indexOf(tabWidget);
    if (tabVisible && tabIndex == -1)
    {
      this->addTab(tabWidget, tabName);
    }
    // Hide tab if became empty
    else if (!tabVisible && tabIndex > -1)
    {
      this->removeTab(tabIndex);
    }
  }
}

//-----------------------------------------------------------------------------
// Default beam parameter handler functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::mlcBoundaryAndPositionTableNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLBeamParametersTabWidget);

  d->pushButton_UpdateMLCBoundary->setEnabled(node);
  d->pushButton_CalculateMLCPosition->setEnabled(node);

  if (!d->MLCPositionLogic)
  {
    qCritical() << Q_FUNC_INFO << ": MLC position calculation logic is invalid!";
    return;
  }
  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Beam node is invalid!";
    return;
  }

  if (vtkMRMLTableNode* mlcTable = vtkMRMLTableNode::SafeDownCast(node))
  {
    d->BeamNode->SetAndObserveMultiLeafCollimatorTableNode(mlcTable);
    d->MLCPositionLogic->SetParentForMultiLeafCollimatorTableNode(d->BeamNode);
  }
  else
  {
    d->BeamNode->SetAndObserveMultiLeafCollimatorTableNode(nullptr);
    qCritical() << Q_FUNC_INFO << ": MLC boundary and position table node is invalid, set nullptr value by default!";
  }
  d->BeamNode->UpdateGeometry();

  // GCS FIX TODO *** Come back to this later ***
  Q_UNUSED(node);
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
void qMRMLBeamParametersTabWidget::generateMLCboundaryClicked()
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->MLCPositionLogic)
  {
    qCritical() << Q_FUNC_INFO << ": MLC position calculation logic is invalid!";
    return;
  }

  bool mlcType = d->radioButton_MLCX->isChecked();
  int nofPairs = static_cast<int>(d->SliderWidget_NumberOfLeafPairs->value());
  double leafPairSize = d->SliderWidget_LeafPairBoundarySize->value();
  double offset = d->SliderWidget_IsocenterOffset->value();

  vtkMRMLTableNode* mlcTable = d->MLCPositionLogic->CreateMultiLeafCollimatorTableNodeBoundaryData(
    mlcType, nofPairs, leafPairSize, offset);
  if (mlcTable)
  {
    const char* beamName = d->BeamNode->GetName();
    const char* mlcName = mlcTable->GetName();
    std::string newName = std::string(mlcName) + ": " + beamName;
    mlcTable->SetName(newName.c_str());
    // enable MRML combobox if it was disabled
    d->MRMLNodeComboBox_MLCBoundaryAndPositionTable->setEnabled(true);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Unable to create MLC boundary data table!";
  }
  if (!d->checkBox_ParallelBeam->isChecked())
  {
    d->MLCPositionLogic->CalculateLeavesProjection( d->BeamNode, mlcTable);
  }
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::updateMLCboundaryClicked()
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->MLCPositionLogic)
  {
    qCritical() << Q_FUNC_INFO << ": MLC position calculation logic is invalid!";
    return;
  }

  bool mlcType = d->radioButton_MLCX->isChecked();
  int nofPairs = static_cast<int>(d->SliderWidget_NumberOfLeafPairs->value());
  double leafPairSize = d->SliderWidget_LeafPairBoundarySize->value();
  double offset = d->SliderWidget_IsocenterOffset->value();

  vtkMRMLNode* node = d->MRMLNodeComboBox_MLCBoundaryAndPositionTable->currentNode();
  vtkMRMLTableNode* mlcTableNode = vtkMRMLTableNode::SafeDownCast(node);
  if (mlcTableNode)
  {
    d->MLCPositionLogic->UpdateMultiLeafCollimatorTableNodeBoundaryData( mlcTableNode,
    mlcType, nofPairs, leafPairSize, offset);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Unable to update MLC boundary data table!";
  }
  if (!d->checkBox_ParallelBeam->isChecked())
  {
    d->MLCPositionLogic->CalculateLeavesProjection( d->BeamNode, mlcTableNode);
  }
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::calculateMLCPositionClicked()
{
  Q_D(qMRMLBeamParametersTabWidget);
  vtkMRMLNode* mlcTable = d->MRMLNodeComboBox_MLCBoundaryAndPositionTable->currentNode();

  if (!d->MLCPositionLogic)
  {
    qCritical() << Q_FUNC_INFO << ": MLC position calculation logic is invalid!";
    return;
  }
  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }
  vtkMRMLRTPlanNode* planNode = d->BeamNode->GetParentPlanNode();
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": No plan node for current beam!";
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = planNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* volumeNode = planNode->GetReferenceVolumeNode();
  const char* targetID = planNode->GetTargetSegmentID();

  if (!segmentationNode || !volumeNode || !targetID)
  {
    qCritical() << Q_FUNC_INFO << ": No segmentation, reference volume or target id data!";
    return;
  }

  vtkPolyData* targetPoly = segmentationNode->GetClosedSurfaceInternalRepresentation(std::string(targetID));
  if (targetPoly)
  {
    vtkMRMLMarkupsCurveNode* convexHullCurve = d->MLCPositionLogic->CalculatePositionConvexHullCurve( d->BeamNode, targetPoly);
    if (convexHullCurve)
    {
      const char* beamName = d->BeamNode->GetName();
      const char* curveName = convexHullCurve->GetName();
      std::string newName = std::string(curveName) + ": " + beamName;
      convexHullCurve->SetName(newName.c_str());

      vtkMRMLTransformNode* beamTransformNode = d->BeamNode->GetParentTransformNode();

      vtkMRMLTableNode* mlcTableNode = vtkMRMLTableNode::SafeDownCast(mlcTable);
      if (mlcTableNode && d->MLCPositionLogic->CalculateMultiLeafCollimatorPosition( mlcTableNode, convexHullCurve)
        && d->MLCPositionLogic->CalculateMultiLeafCollimatorPosition( d->BeamNode, mlcTableNode, targetPoly))
      {
        d->BeamNode->SetAndObserveMultiLeafCollimatorTableNode(mlcTableNode);
        d->MLCPositionLogic->SetParentForMultiLeafCollimatorTableNode(d->BeamNode);

        double area = d->MLCPositionLogic->CalculateMultiLeafCollimatorPositionArea(d->BeamNode);
        qDebug() << Q_FUNC_INFO << ": MLC position area (mm^2) = " << area;

        area = d->MLCPositionLogic->CalculateCurvePolygonArea(convexHullCurve);
        qDebug() << Q_FUNC_INFO << ": Convex hull closed curve area (mm^2) = " << area;

        d->MLCPositionLogic->SetParentForMultiLeafCollimatorCurve( d->BeamNode, convexHullCurve);

        convexHullCurve->SetAndObserveTransformNodeID(beamTransformNode->GetID());
      }
    }
  }
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

  // Do not disable modifier events as transforms need to be updated
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

  // Do not disable modifier events as transforms need to be updated
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

  // Do not disable modifier events as transforms need to be updated
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

  // Do not disable modifier events as geometry needs to be updated
  d->BeamNode->SetSAD(value);
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::virtualSourceAxisXDistanceChanged(double value)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(d->BeamNode);
  if (ionBeamNode)
  {
    ionBeamNode->SetVSADx(value);
    // Explicit geometry update
    ionBeamNode->UpdateGeometry();
  }
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::virtualSourceAxisYDistanceChanged(double value)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(d->BeamNode);
  if (ionBeamNode)
  {
    ionBeamNode->SetVSADy(value);
    // Explicit geometry update
    ionBeamNode->UpdateGeometry();
  }
}

//-----------------------------------------------------------------------------
void qMRMLBeamParametersTabWidget::mlcDistanceChanged(double value)
{
  Q_D(qMRMLBeamParametersTabWidget);

  if (!d->BeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node!";
    return;
  }

  vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(d->BeamNode);
  // Do not disable modifier events as geometry needs to be updated
  if (d->BeamNode && !ionBeamNode) // RTBeam
  {
    d->BeamNode->SetSourceToMultiLeafCollimatorDistance(value);
    // Explicit geometry update
    d->BeamNode->UpdateGeometry();
  }
  else if (ionBeamNode) // RTIonBeam
  {
    ionBeamNode->SetIsocenterToMultiLeafCollimatorDistance(value);
    // Explicit geometry update
    ionBeamNode->UpdateGeometry();
  }
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

  Q_UNUSED(checked);
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
