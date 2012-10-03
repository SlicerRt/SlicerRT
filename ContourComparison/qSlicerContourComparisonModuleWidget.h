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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerContourComparisonModuleWidget_h
#define __qSlicerContourComparisonModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerContourComparisonModuleExport.h"

class qSlicerContourComparisonModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_ContourComparison
class Q_SLICER_QTMODULES_CONTOURCOMPARISON_EXPORT qSlicerContourComparisonModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerContourComparisonModuleWidget(QWidget *parent=0);
  virtual ~qSlicerContourComparisonModuleWidget();

  virtual void enter();

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Set current parameter node
  void setContourComparisonNode(vtkMRMLNode *node);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void referenceContourNodeChanged(vtkMRMLNode*);
  void compareContourNodeChanged(vtkMRMLNode*);
  void referenceVolumeNodeChanged(vtkMRMLNode*);

  void applyClicked();

  void onLogicModified();

protected:
  /// Updates button states
  void updateButtonsState();

  /// Invalidate the results (change the values with 'N/A')
  void invalidateResults();

protected:
  QScopedPointer<qSlicerContourComparisonModuleWidgetPrivate> d_ptr;
  
  virtual void setup();
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerContourComparisonModuleWidget);
  Q_DISABLE_COPY(qSlicerContourComparisonModuleWidget);
};

#endif
