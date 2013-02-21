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
#include "qSlicerProtonDoseBeamParameters.h"

// library includes
#include <vector>

// classes
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

  // actions from the QPush Button  
  void beamChanged();
  void beamNameChanged();
  
  void addBeam();
  void deleteBeam();
  void loadBeam();
  void saveBeam();

  // actions from the editing of the editlines for isocenter, jaw and energy
  void editIso_x();
  void editIso_y();
  void editIso_z();
  void editRange();
  void editMod();
  void editXJaw();
  void editYJaw();

  // actions when change the rotations of the SpinBox
  void gantryChanged(double);
  void collimatorChanged(double);
  void couchRotationChanged(double);
  void couchPitchChanged(double);
  void couchRollChanged(double);

  // Update of the data beam on the window
  void beamQtUpdate();

  // Creation of the configuration file
  void configFileCreation();

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
  std::vector<ProtonBeamParameters> beam; // vector of beam classes - each case contains a beam with its parameter
  int beam_max, beam_actual; // two parameters to manage the beam that appears on the screen (beam_actual) - max beam is used for adding and deleting a beam
};

#endif
