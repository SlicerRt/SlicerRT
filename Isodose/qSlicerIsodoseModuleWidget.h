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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __qSlicerIsodoseModuleWidget_h
#define __qSlicerIsodoseModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerIsodoseModuleExport.h"

class qSlicerIsodoseModuleWidgetPrivate;
class vtkMRMLNode;
class QTableWidgetItem;

/// \ingroup SlicerRt_QtModules_Isodose
class Q_SLICER_QTMODULES_ISODOSE_EXPORT qSlicerIsodoseModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerIsodoseModuleWidget(QWidget *parent=nullptr);
  ~qSlicerIsodoseModuleWidget() override;

  void enter() override;

  /// Allows other modules to select input and output nodes in the module's GUI.
  /// There may be multiple node selectors in a module widget, you can select between them
  /// using the role argument.
  /// Context can be specified to make a selection within that node (for example, a markup list
  /// node may contain multiple markups; context can be used to select a specific item).
  /// Returns true if the selection was successful.
  bool setEditedNode(vtkMRMLNode* node, QString role = QString(), QString context = QString()) override;

public slots:
  /// Set the current MRML scene to the widget
  void setMRMLScene(vtkMRMLScene*) override;

  /// Process loaded scene
  void onSceneImportedEvent();
  /// Process is scene is closed
  void onSceneClosedEvent();

  /// Set current parameter node
  void setParameterNode(vtkMRMLNode *node);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

  /// Set number of levels
  void setNumberOfLevels(int newNumber);

protected slots:
  /// Slot handling change of dose volume node
  void setDoseVolumeNode(vtkMRMLNode*);

  /// Slot handling change of isodose model node
  void setIsodoseModelNode(vtkMRMLNode*);

  /// Slot handling change of show dose only checkbox
  void showDoseVolumesOnlyCheckboxChanged(int);

  /// Slot for changing isoline visibility
  void setIsolineVisibility(bool);

  /// Slot for changing isosurface visibility
  void setIsosurfaceVisibility(bool);

  /// Slot handling clicking the Apply button
  void applyClicked();

  /// Slot called on change in logic
  void onLogicModified();

  /// Slot called on modify of the color table
  void updateScalarBarsFromSelectedColorTable();

  /// Slot setting type of isolevels representation
  void setRelativeIsolevelsFlag(bool useRelativeIsolevels);

  /// Slot called to set reference dose value
  void setReferenceDoseValue(double value);

  /// Updates color legend widget
  void updateColorLegendFromMRML();

protected:
  // Generates a new isodose level name
  QString generateNewIsodoseLevel() const;

  /// Updates button states
  void updateButtonsState();

protected:
  QScopedPointer<qSlicerIsodoseModuleWidgetPrivate> d_ptr;
  
  void setup() override;
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerIsodoseModuleWidget);
  Q_DISABLE_COPY(qSlicerIsodoseModuleWidget);
};

#endif
