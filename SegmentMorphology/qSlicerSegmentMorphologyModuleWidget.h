/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

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

#ifndef __qSlicerSegmentMorphologyModuleWidget_h
#define __qSlicerSegmentMorphologyModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerSegmentMorphologyModuleExport.h"

class qSlicerSegmentMorphologyModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_QtModules_SegmentMorphology
class Q_SLICER_QTMODULES_SEGMENTMORPHOLOGY_EXPORT qSlicerSegmentMorphologyModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerSegmentMorphologyModuleWidget(QWidget *parent=0);
  virtual ~qSlicerSegmentMorphologyModuleWidget();

  virtual void enter();

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Set current parameter node
  void setSegmentMorphologyNode(vtkMRMLNode*);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void segmentationANodeChanged(vtkMRMLNode* node);
  void segmentAChanged(QString);
  void segmentationBNodeChanged(vtkMRMLNode* node);
  void segmentBChanged(QString);

  void outputSegmentationNodeChanged(vtkMRMLNode* node);
  void radioButtonExpandClicked();
  void radioButtonShrinkClicked();
  void radioButtonUnionClicked();
  void radioButtonIntersectClicked();
  void radioButtonSubtractClicked();

  void checkBoxUniformCheckedStateChanged(int state);
  void doubleSpinBoxXSizeChanged(double value);
  void doubleSpinBoxYSizeChanged(double value);
  void doubleSpinBoxZSizeChanged(double value);

  void applyClicked();

  void onLogicModified();

  /// Updates button states
  void updateButtonsState();

protected:
  QScopedPointer<qSlicerSegmentMorphologyModuleWidgetPrivate> d_ptr;
  
  virtual void setup();
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentMorphologyModuleWidget);
  Q_DISABLE_COPY(qSlicerSegmentMorphologyModuleWidget);
};

#endif
