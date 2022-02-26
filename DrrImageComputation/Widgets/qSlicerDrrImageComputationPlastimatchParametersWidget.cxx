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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// VTK includes
#include <vtkWeakPointer.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLNode.h>

// SlicerRT Beams MRML includes
#include <vtkMRMLRTBeamNode.h>

// SlicerRT DrrImageComputation MRML includes
#include <vtkMRMLDrrImageComputationNode.h>

// Qt includes
#include <QDebug>

// PlastimatchParameters Widgets includes
#include "qSlicerDrrImageComputationPlastimatchParametersWidget.h"
#include "ui_qSlicerDrrImageComputationPlastimatchParametersWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DrrImageComputation
class qSlicerDrrImageComputationPlastimatchParametersWidgetPrivate : public Ui_qSlicerDrrImageComputationPlastimatchParametersWidget
{
  Q_DECLARE_PUBLIC(qSlicerDrrImageComputationPlastimatchParametersWidget);
protected:
  qSlicerDrrImageComputationPlastimatchParametersWidget* const q_ptr;

public:
  qSlicerDrrImageComputationPlastimatchParametersWidgetPrivate(
    qSlicerDrrImageComputationPlastimatchParametersWidget& object);
  virtual void setupUi(qSlicerDrrImageComputationPlastimatchParametersWidget*);
  void init();
  
  /// RT Image MRML node containing shown parameters
  vtkWeakPointer<vtkMRMLDrrImageComputationNode> ParameterNode;
};

// --------------------------------------------------------------------------
qSlicerDrrImageComputationPlastimatchParametersWidgetPrivate::qSlicerDrrImageComputationPlastimatchParametersWidgetPrivate(
  qSlicerDrrImageComputationPlastimatchParametersWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidgetPrivate::setupUi(qSlicerDrrImageComputationPlastimatchParametersWidget* widget)
{
  this->Ui_qSlicerDrrImageComputationPlastimatchParametersWidget::setupUi(widget);
}

// --------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidgetPrivate::init()
{
  Q_Q(qSlicerDrrImageComputationPlastimatchParametersWidget);

  // Range widgets
  QObject::connect( this->RangeWidget_IntensityRange, SIGNAL(valuesChanged( double, double)), 
    q, SLOT(onAutoscaleIntensityRangeChanged( double, double)));

  // Slicer widgets
  QObject::connect( this->SliderWidget_HounsfieldThreshold, SIGNAL(valueChanged(double)), 
    q, SLOT(onHUThresholdChanged(double)));

  // Buttons
  QObject::connect( this->CheckBox_UseExponentialMapping, SIGNAL(toggled(bool)), 
    q, SLOT(onUseExponentialMappingToggled(bool)));
  QObject::connect( this->CheckBox_AutoscaleIntensity, SIGNAL(toggled(bool)), 
    q, SLOT(onAutoscalePixelsRangeToggled(bool)));
  QObject::connect( this->CheckBox_InvertIntensity, SIGNAL(toggled(bool)), 
    q, SLOT(onInvertIntensityToggled(bool)));

  // Combo Boxes
  QObject::connect( this->ComboBox_ReconstructionAlgorithm, SIGNAL(currentIndexChanged(int)), 
    q, SLOT(onReconstructionAlgorithmChanged(int)));
  QObject::connect( this->ComboBox_Threading, SIGNAL(currentIndexChanged(int)), 
    q, SLOT(onThreadingChanged(int)));
  QObject::connect( this->ComboBox_HounsfieldConversion, SIGNAL(currentIndexChanged(int)), 
    q, SLOT(onHUConversionChanged(int)));
}

//-----------------------------------------------------------------------------
// qSlicerDrrImageComputationPlastimatchParametersWidget methods

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationPlastimatchParametersWidget::qSlicerDrrImageComputationPlastimatchParametersWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerDrrImageComputationPlastimatchParametersWidgetPrivate(*this) )
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);
  d->setupUi(this);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationPlastimatchParametersWidget::~qSlicerDrrImageComputationPlastimatchParametersWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::setParameterNode(vtkMRMLNode* node)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  vtkMRMLDrrImageComputationNode* parameterNode = vtkMRMLDrrImageComputationNode::SafeDownCast(node);
  // Each time the node is modified, the UI widgets are updated
  qvtkReconnect( d->ParameterNode, parameterNode, vtkCommand::ModifiedEvent, 
    this, SLOT( updateWidgetFromMRML() ) );

  d->ParameterNode = parameterNode;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  d->plainTextEdit_PlastimatchDrrArguments->clear();

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  // Update widgets info from parameter node and update plastimatch drr command
  d->CheckBox_UseExponentialMapping->setChecked(d->ParameterNode->GetExponentialMappingFlag());
  d->CheckBox_AutoscaleIntensity->setChecked(d->ParameterNode->GetAutoscaleFlag());
  d->CheckBox_InvertIntensity->setChecked(d->ParameterNode->GetInvertIntensityFlag());

  float autoscaleRange[2] = { 0.f, 255.f };
  d->ParameterNode->GetAutoscaleRange(autoscaleRange);
  d->RangeWidget_IntensityRange->setValues( autoscaleRange[0], autoscaleRange[1]);
  d->SliderWidget_HounsfieldThreshold->setValue(double(d->ParameterNode->GetHUThresholdBelow()));

  switch (d->ParameterNode->GetAlgorithmReconstuction())
  {
    case vtkMRMLDrrImageComputationNode::Exact:
      d->ComboBox_ReconstructionAlgorithm->setCurrentIndex(0);
      break;
    case vtkMRMLDrrImageComputationNode::Uniform:
      d->ComboBox_ReconstructionAlgorithm->setCurrentIndex(1);
      break;
    default:
      break;
  }

  switch (d->ParameterNode->GetHUConversion())
  {
    case vtkMRMLDrrImageComputationNode::Preprocess:
      d->ComboBox_HounsfieldConversion->setCurrentIndex(0);
      break;
    case vtkMRMLDrrImageComputationNode::Inline:
      d->ComboBox_HounsfieldConversion->setCurrentIndex(1);
      break;
    case vtkMRMLDrrImageComputationNode::None:
      d->ComboBox_HounsfieldConversion->setCurrentIndex(2);
      break;
    default:
      break;
  }

  switch (d->ParameterNode->GetThreading())
  {
    case vtkMRMLDrrImageComputationNode::CPU:
      d->ComboBox_Threading->setCurrentIndex(0);
      break;
    case vtkMRMLDrrImageComputationNode::CUDA:
      d->ComboBox_Threading->setCurrentIndex(1);
      break;
    case vtkMRMLDrrImageComputationNode::OpenCL:
      d->ComboBox_Threading->setCurrentIndex(2);
      break;
    default:
      break;
  }
  this->updatePlastimatchDrrArguments();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onReconstructionAlgorithmChanged(int id)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  switch (id)
  {
  case 0:
    d->ParameterNode->SetAlgorithmReconstuction(vtkMRMLDrrImageComputationNode::Exact);
    break;
  case 1:
    d->ParameterNode->SetAlgorithmReconstuction(vtkMRMLDrrImageComputationNode::Uniform);
    break;
  default:
    qWarning() << Q_FUNC_INFO << ": Invalid reconstruct algorithm button id";
    break;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onThreadingChanged(int id)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  switch (id)
  {
  case 0:
    d->ParameterNode->SetThreading(vtkMRMLDrrImageComputationNode::CPU);
    break;
  case 1:
    d->ParameterNode->SetThreading(vtkMRMLDrrImageComputationNode::CUDA);
    break;
  case 2:
    d->ParameterNode->SetThreading(vtkMRMLDrrImageComputationNode::OpenCL);
    break;
  default:
    qWarning() << Q_FUNC_INFO << ": Invalid threading button id";
    break;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onHUConversionChanged(int id)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  switch (id)
  {
  case 0:
    d->ParameterNode->SetHUConversion(vtkMRMLDrrImageComputationNode::None);
    break;
  case 1:
    d->ParameterNode->SetHUConversion(vtkMRMLDrrImageComputationNode::Inline);
    break;
  case 2:
    d->ParameterNode->SetHUConversion(vtkMRMLDrrImageComputationNode::Preprocess);
    break;
  default:
    qWarning() << Q_FUNC_INFO << ": Invalid Hounsfield units conversion button id";
    break;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onUseExponentialMappingToggled(bool value)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  d->ParameterNode->SetExponentialMappingFlag(value);
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onAutoscalePixelsRangeToggled(bool value)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  d->ParameterNode->SetAutoscaleFlag(value);
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onHUThresholdChanged(double value)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  d->ParameterNode->DisableModifiedEventOn();
  d->ParameterNode->SetHUThresholdBelow(static_cast<int>(value));
  d->ParameterNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onInvertIntensityToggled(bool value)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  d->ParameterNode->SetInvertIntensityFlag(value);
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onAutoscaleIntensityRangeChanged( double min, double max)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  d->ParameterNode->SetAutoscaleRange( min, max);
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::updatePlastimatchDrrArguments()
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = d->ParameterNode->GetBeamNode();
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT Beam node";
    return;
  }

  std::ostringstream command;
  command << "plastimatch drr ";
  switch (d->ParameterNode->GetThreading())
  {
    case vtkMRMLDrrImageComputationNode::CPU:
      command << "-A cpu \\\n";
      break;
    case vtkMRMLDrrImageComputationNode::CUDA:
      command << "-A cuda \\\n";
      break;
    case vtkMRMLDrrImageComputationNode::OpenCL:
      command << "-A opencl \\\n";
      break;
    default:
      break;
  }

  double n[3], vup[3]; // normal and view up in LPS coordinate system
  d->ParameterNode->GetNormalVector(n);
  d->ParameterNode->GetViewUpVector(vup);
  command << "\t--nrm" << " \"" << n[0] << " " << n[1] << " " << n[2] << "\" \\" << "\n";
  command << "\t--vup" << " \"" << vup[0] << " " << vup[1] << " " << vup[2] << "\" \\" << "\n";

  int imagerResolution[2];
  double imagerSpacing[2];
  double imageCenter[2];
  double isocenterPosition[3];
  d->ParameterNode->GetImagerResolution(imagerResolution);
  d->ParameterNode->GetImagerSpacing(imagerSpacing);
  d->ParameterNode->GetImageCenter(imageCenter);
  d->ParameterNode->GetIsocenterPositionLPS(isocenterPosition);

  command << "\t--sad " << beamNode->GetSAD() << " --sid " \
    << beamNode->GetSAD() + d->ParameterNode->GetIsocenterImagerDistance() << " \\" << "\n";
  command << "\t-r" << " \"" << imagerResolution[1] << " " \
    << imagerResolution[0] << "\" \\" << "\n";
  command << "\t-z" << " \"" << imagerResolution[1] * imagerSpacing[1] << " " \
    << imagerResolution[0] * imagerSpacing[0] << "\" \\" << "\n";
  command << "\t-c" << " \"" << imageCenter[1] << " " << imageCenter[0] << "\" \\" << "\n";

  int imageWindow[4];
  d->ParameterNode->GetImageWindow(imageWindow);
  // Isocenter LPS position
  command << "\t-o" << " \"" << isocenterPosition[0] << " " \
    << isocenterPosition[1] << " " << isocenterPosition[2] << "\" \\" << "\n";
  if (d->ParameterNode->GetImageWindowFlag())
  {
    command << "\t-w" << " \"" << imageWindow[1] << " " << imageWindow[3] << " " \
      << imageWindow[0] << " " << imageWindow[2] << "\" \\" << "\n";
  }

  if (d->ParameterNode->GetExponentialMappingFlag())
  {
    command << "\t-e ";
  }
  else
  {
    command << "\t ";
  }

  if (d->ParameterNode->GetAutoscaleFlag())
  {
    command << "--autoscale ";
  }
  float autoscaleRange[2];
  d->ParameterNode->GetAutoscaleRange(autoscaleRange);
  command << "--autoscale-range \"" << autoscaleRange[0] << " " << autoscaleRange[1] << "\" \\\n\t ";

  switch (d->ParameterNode->GetAlgorithmReconstuction())
  {
    case vtkMRMLDrrImageComputationNode::Exact:
      command << "-i exact ";
      break;
    case vtkMRMLDrrImageComputationNode::Uniform:
      command << "-i uniform ";
      break;
    default:
      break;
  }

  switch (d->ParameterNode->GetHUConversion())
  {
    case vtkMRMLDrrImageComputationNode::None:
      command << "-P none ";
      break;
    case vtkMRMLDrrImageComputationNode::Preprocess:
      command << "-P preprocess ";
      break;
    case vtkMRMLDrrImageComputationNode::Inline:
      command << "-P inline ";
      break;
    default:
      break;
  }

  command << "-O Out -t raw";

  d->plainTextEdit_PlastimatchDrrArguments->setPlainText(QString::fromStdString(command.str()));
}
