/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

#ifndef __qSlicerSegmentComparisonModuleWidget_h
#define __qSlicerSegmentComparisonModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerSegmentComparisonModuleExport.h"

class qSlicerSegmentComparisonModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_QtModules_SegmentComparison
class Q_SLICER_QTMODULES_SEGMENTCOMPARISON_EXPORT qSlicerSegmentComparisonModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerSegmentComparisonModuleWidget(QWidget *parent=0);
  virtual ~qSlicerSegmentComparisonModuleWidget();

  virtual void enter();

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Set current parameter node
  void setSegmentComparisonNode(vtkMRMLNode *node);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void referenceSegmentationNodeChanged(vtkMRMLNode*);
  void referenceSegmentChanged(QString);
  void compareSegmentationNodeChanged(vtkMRMLNode*);
  void compareSegmentChanged(QString);

  /// Updates button states
  void updateButtonsState();

  /// Compute Dice similarity metrics and display results
  void computeDiceClicked();

  /// Compute Dice similarity metrics and display results
  void computeHausdorffClicked();

  void onLogicModified();

protected:
  /// Invalidate the Dice similarity results (change the values with 'N/A')
  void invalidateDiceResults();

  /// Invalidate the Hausdorff distance results (change the values with 'N/A')
  void invalidateHausdorffResults();

protected:
  QScopedPointer<qSlicerSegmentComparisonModuleWidgetPrivate> d_ptr;
  
  virtual void setup();
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentComparisonModuleWidget);
  Q_DISABLE_COPY(qSlicerSegmentComparisonModuleWidget);
};

#endif
