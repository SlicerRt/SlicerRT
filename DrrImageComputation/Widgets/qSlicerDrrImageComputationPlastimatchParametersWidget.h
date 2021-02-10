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

#ifndef __qSlicerDrrImageComputationPlastimatchParametersWidget_h
#define __qSlicerDrrImageComputationPlastimatchParametersWidget_h

// Qt includes
#include <QWidget>

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

// DrrImageComputation Widgets includes
#include "qSlicerDrrImageComputationModuleWidgetsExport.h"

class vtkMRMLNode;
class vtkMRMLDrrImageComputationNode;
class qSlicerDrrImageComputationPlastimatchParametersWidgetPrivate;

/// \ingroup Slicer_QtModules_DrrImageComputation
class Q_SLICER_MODULE_DRRIMAGECOMPUTATION_WIDGETS_EXPORT qSlicerDrrImageComputationPlastimatchParametersWidget
  : public QWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef QWidget Superclass;
  qSlicerDrrImageComputationPlastimatchParametersWidget(QWidget *parent=0);
  ~qSlicerDrrImageComputationPlastimatchParametersWidget() override;

public slots:
  /// Set DrrImageComputation MRML node (Parameter node)
  void setParameterNode(vtkMRMLNode* node);
  /// Update widget GUI from RT Image parameters node
  void updateWidgetFromMRML();

protected slots:
  /// Exponential mapping flag
  void onUseExponentialMappingToggled(bool);
  /// Autoscale flag
  void onAutoscalePixelsRangeToggled(bool);
  /// Invert intensity flag
  void onInvertIntensityToggled(bool);
  /// Type of reconstruct algorithm
  void onReconstructionAlgorithmChanged(int);
  /// Type of computation threading
  void onThreadingChanged(int);
  /// Type Hounsfield Units conversion
  void onHUConversionChanged(int);
  void onAutoscaleIntensityRangeChanged(double, double);
  /// HU threshold value for processing
  /// @param thresholdBelow - Air HU value (-1000) below that threshold
  void onHUThresholdChanged(double thresholdBelow);

protected:
  QScopedPointer<qSlicerDrrImageComputationPlastimatchParametersWidgetPrivate> d_ptr;
  void updatePlastimatchDrrArguments();

private:
  Q_DECLARE_PRIVATE(qSlicerDrrImageComputationPlastimatchParametersWidget);
  Q_DISABLE_COPY(qSlicerDrrImageComputationPlastimatchParametersWidget);
};

#endif
