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

==============================================================================*/

#ifndef __qSlicerDrrImageComputationModuleWidget_h
#define __qSlicerDrrImageComputationModuleWidget_h

// Slicer includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDrrImageComputationModuleExport.h"

class qSlicerDrrImageComputationModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_DrrImageComputation
class Q_SLICER_QTMODULES_DRRIMAGECOMPUTATION_EXPORT qSlicerDrrImageComputationModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDrrImageComputationModuleWidget(QWidget *parent=0);
  ~qSlicerDrrImageComputationModuleWidget() override;

  void exit() override;
  void enter() override;

public slots:
  void setMRMLScene(vtkMRMLScene*) override;
  void setParameterNode(vtkMRMLNode*);
  void onSceneImportedEvent();
  void onSceneClosedEvent();

  void onParameterNodeChanged(vtkMRMLNode*);
  void onRTBeamNodeChanged(vtkMRMLNode*);
  void onCtVolumeNodeChanged(vtkMRMLNode*);
  void onCameraNodeChanged(vtkMRMLNode*);

  void onLogicModified();
  void onIsocenterImagerDistanceValueChanged(double);
  void onImageWindowColumnsValuesChanged( double, double);
  void onImageWindowRowsValuesChanged( double, double);
  void onImagerSpacingChanged(double*);
  void onImagerResolutionChanged(double*);
  void onShowMarkupsToggled(bool);
  void onUseImageWindowToggled(bool);
  void onUpdateBeamFromCameraClicked();
  void onComputeDrrClicked();
  void onMarkupsControlPointSelectionChanged(int index);

  /// Update widget GUI from RT Image parameters node
  void updateWidgetFromMRML();

protected:
  QScopedPointer<qSlicerDrrImageComputationModuleWidgetPrivate> d_ptr;

  void setup() override;
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerDrrImageComputationModuleWidget);
  Q_DISABLE_COPY(qSlicerDrrImageComputationModuleWidget);
};

#endif
