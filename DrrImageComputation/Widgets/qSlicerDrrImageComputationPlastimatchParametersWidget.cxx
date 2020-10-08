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

  // Buttons
  QObject::connect( this->CheckBox_UseExponentialMapping, SIGNAL(toggled(bool)), q, SLOT(onUseExponentialMappingToggled(bool)));
  QObject::connect( this->CheckBox_AutoscaleIntensity, SIGNAL(toggled(bool)), q, SLOT(onAutoscalePixelsRangeToggled(bool)));
  QObject::connect( this->CheckBox_InvertIntensity, SIGNAL(toggled(bool)), q, SLOT(onInvertIntensityToggled(bool)));

  // Button groups
  QObject::connect( this->ButtonGroup_ReconstructAlgorithm, SIGNAL(buttonClicked(int)), q, SLOT(onReconstructionAlgorithmChanged(int)));
  QObject::connect( this->ButtonGroup_Threading, SIGNAL(buttonClicked(int)), q, SLOT(onThreadingChanged(int)));
  QObject::connect( this->ButtonGroup_HuConversion, SIGNAL(buttonClicked(int)), q, SLOT(onHUConversionChanged(int)));
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

  switch (d->ParameterNode->GetAlgorithmReconstuction())
  {
    case vtkMRMLDrrImageComputationNode::PlastimatchAlgorithmReconstuctionType::EXACT:
      d->RadioButton_Exact->setChecked(true);
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchAlgorithmReconstuctionType::UNIFORM:
      d->RadioButton_Uniform->setChecked(true);
      break;
    default:
      break;
  }

  switch (d->ParameterNode->GetHUConversion())
  {
    case vtkMRMLDrrImageComputationNode::PlastimatchHounsfieldUnitsConversionType::PREPROCESS:
      d->RadioButton_Preprocess->setChecked(true);
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchHounsfieldUnitsConversionType::INLINE:
      d->RadioButton_Inline->setChecked(true);
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchHounsfieldUnitsConversionType::NONE:
      d->RadioButton_None->setChecked(true);
      break;
    default:
      break;
  }

  switch (d->ParameterNode->GetThreading())
  {
    case vtkMRMLDrrImageComputationNode::PlastimatchThreadingType::CPU:
      d->RadioButton_CPU->setChecked(true);
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchThreadingType::CUDA:
      d->RadioButton_CUDA->setChecked(true);
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchThreadingType::OPENCL:
      d->RadioButton_OpenCL->setChecked(true);
      break;
    default:
      break;
  }
  this->updatePlastimatchDrrArguments();
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onReconstructionAlgorithmChanged(int button_id)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  QAbstractButton* button = d->ButtonGroup_ReconstructAlgorithm->button(button_id);
  QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);

  if (rbutton == d->RadioButton_Exact)
  {
    d->ParameterNode->SetAlgorithmReconstuction(vtkMRMLDrrImageComputationNode::PlastimatchAlgorithmReconstuctionType::EXACT);
  }
  else if (rbutton == d->RadioButton_Uniform)
  {
    d->ParameterNode->SetAlgorithmReconstuction(vtkMRMLDrrImageComputationNode::PlastimatchAlgorithmReconstuctionType::UNIFORM);
  }
  else
  {
    qWarning() << Q_FUNC_INFO << ": Invalid reconstruct algorithm button id";
    return;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onThreadingChanged(int button_id)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  QAbstractButton* button = d->ButtonGroup_Threading->button(button_id);
  QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);

  if (rbutton == d->RadioButton_CPU)
  {
    d->ParameterNode->SetThreading(vtkMRMLDrrImageComputationNode::PlastimatchThreadingType::CPU);
  }
  else if (rbutton == d->RadioButton_CUDA)
  {
    d->ParameterNode->SetThreading(vtkMRMLDrrImageComputationNode::PlastimatchThreadingType::CUDA);
  }
  else if (rbutton == d->RadioButton_OpenCL)
  {
    d->ParameterNode->SetThreading(vtkMRMLDrrImageComputationNode::PlastimatchThreadingType::OPENCL);
  }
  else
  {
    qWarning() << Q_FUNC_INFO << ": Invalid threading button id";
    return;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationPlastimatchParametersWidget::onHUConversionChanged(int button_id)
{
  Q_D(qSlicerDrrImageComputationPlastimatchParametersWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  QAbstractButton* button = d->ButtonGroup_HuConversion->button(button_id);
  QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);

  if (rbutton == d->RadioButton_None)
  {
    d->ParameterNode->SetHUConversion(vtkMRMLDrrImageComputationNode::PlastimatchHounsfieldUnitsConversionType::NONE);
  }
  else if (rbutton == d->RadioButton_Inline)
  {
    d->ParameterNode->SetHUConversion(vtkMRMLDrrImageComputationNode::PlastimatchHounsfieldUnitsConversionType::INLINE);
  }
  else if (rbutton == d->RadioButton_Preprocess)
  {
    d->ParameterNode->SetHUConversion(vtkMRMLDrrImageComputationNode::PlastimatchHounsfieldUnitsConversionType::PREPROCESS);
  }
  else
  {
    qWarning() << Q_FUNC_INFO << ": Invalid Hounsfield units conversion button id";
    return;
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
    case vtkMRMLDrrImageComputationNode::PlastimatchThreadingType::CPU:
      command << "-A cpu \\\n";
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchThreadingType::CUDA:
      command << "-A cuda \\\n";
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchThreadingType::OPENCL:
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
    case vtkMRMLDrrImageComputationNode::PlastimatchAlgorithmReconstuctionType::EXACT:
      command << "-i exact ";
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchAlgorithmReconstuctionType::UNIFORM:
      command << "-i uniform ";
      break;
    default:
      break;
  }

  switch (d->ParameterNode->GetHUConversion())
  {
    case vtkMRMLDrrImageComputationNode::PlastimatchHounsfieldUnitsConversionType::NONE:
      command << "-P none ";
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchHounsfieldUnitsConversionType::PREPROCESS:
      command << "-P preprocess ";
      break;
    case vtkMRMLDrrImageComputationNode::PlastimatchHounsfieldUnitsConversionType::INLINE:
      command << "-P inline ";
      break;
    default:
      break;
  }

  command << "-O Out";

  d->plainTextEdit_PlastimatchDrrArguments->setPlainText(QString::fromStdString(command.str()));
}
