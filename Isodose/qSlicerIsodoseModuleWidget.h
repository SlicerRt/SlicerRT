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

  This file was originally developed by Kevin Wang, RMP, PMH
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerIsodoseModuleWidget_h
#define __qSlicerIsodoseModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerIsodoseModuleExport.h"

class qSlicerIsodoseModuleWidgetPrivate;
class vtkMRMLNode;
class QTableWidgetItem;

/// \ingroup Slicer_QtModules_Isodose
class Q_SLICER_QTMODULES_ISODOSE_EXPORT qSlicerIsodoseModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerIsodoseModuleWidget(QWidget *parent=0);
  virtual ~qSlicerIsodoseModuleWidget();

  virtual void enter();

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Set current parameter node
  void setIsodoseNode(vtkMRMLNode *node);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

  ///
  void setNumberOfLevels(int newNumber);

protected slots:
  void doseVolumeNodeChanged(vtkMRMLNode*);
  void onTextEdited(QString changedString);
  void storeSelectedTableItemText(QTableWidgetItem* selectedItem, QTableWidgetItem* previousItem);
  void outputHierarchyNodeChanged(vtkMRMLNode*);
  void setIsolineVisibility(bool);
  void setIsosurfaceVisibility(bool);
  void applyClicked();

  void onLogicModified();

protected:
  QScopedPointer<qSlicerIsodoseModuleWidgetPrivate> d_ptr;
  
  virtual void setup();
  void onEnter();

  // Generates a new isodose level name
  QString generateNewIsodoseLevel() const;
  
  /// Updates button states
  void updateButtonsState();

private:
  Q_DECLARE_PRIVATE(qSlicerIsodoseModuleWidget);
  Q_DISABLE_COPY(qSlicerIsodoseModuleWidget);
};

#endif
