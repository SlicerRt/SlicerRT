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

#ifndef __qSlicerProtonDoseModuleWidget_h
#define __qSlicerProtonDoseModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerProtonDoseModuleExport.h"

class qSlicerProtonDoseModuleWidgetPrivate;
class vtkMRMLNode;
class QTableWidgetItem;

/// \ingroup Slicer_QtModules_ProtonDose
class Q_SLICER_QTMODULES_PROTONDOSE_EXPORT qSlicerProtonDoseModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerProtonDoseModuleWidget(QWidget *parent=0);
  virtual ~qSlicerProtonDoseModuleWidget();

  virtual void enter();

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Set current parameter node
  void setProtonDoseNode(vtkMRMLNode *node);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void doseVolumeNodeChanged(vtkMRMLNode*);
  void storeSelectedTableItemText(QTableWidgetItem* selectedItem, QTableWidgetItem* previousItem);
  void outputHierarchyNodeChanged(vtkMRMLNode*);

  void applyClicked();

  void onLogicModified();

protected:
  QScopedPointer<qSlicerProtonDoseModuleWidgetPrivate> d_ptr;
  
  virtual void setup();
  void onEnter();

  // Generates a new ProtonDose level name
  QString generateNewProtonDoseLevel() const;
  
  /// Updates button states
  void updateButtonsState();

private:
  Q_DECLARE_PRIVATE(qSlicerProtonDoseModuleWidget);
  Q_DISABLE_COPY(qSlicerProtonDoseModuleWidget);
};

#endif
