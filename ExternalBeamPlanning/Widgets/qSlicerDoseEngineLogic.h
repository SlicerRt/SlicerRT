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

#ifndef __qSlicerDoseEngineLogic_h
#define __qSlicerDoseEngineLogic_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// SlicerQt includes
#include "qSlicerObject.h"

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

// Qt includes
#include <QObject>

class vtkMRMLScene;
class vtkMRMLRTPlanNode;
class vtkMRMLRTBeamNode;
class qSlicerDoseEngineLogicPrivate;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract dose calculation algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class for specific dose engine plugins
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerDoseEngineLogic :
  public QObject, public virtual qSlicerObject
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerDoseEngineLogic(QObject* parent=nullptr);
  /// Destructor
  ~qSlicerDoseEngineLogic() override;

public:
  /// Set the current MRML scene to the widget
  Q_INVOKABLE virtual void setMRMLScene(vtkMRMLScene* scene);

  /// Calculate dose for a plan
  Q_INVOKABLE QString calculateDose(vtkMRMLRTPlanNode* planNode);

  /// Accumulate per-beam dose volumes for each beam under given plan. The accumulated
  /// total dose is
  Q_INVOKABLE QString createAccumulatedDose(vtkMRMLRTPlanNode* planNode);

  /// Remove MRML nodes created by dose calculation for the current RT plan,
  /// such as apertures, range compensators, and doses
  Q_INVOKABLE void removeIntermediateResults(vtkMRMLRTPlanNode* planNode);

  /// Create a beam for a plan (with beam parameters defined by the dose engine of the plan)
  /// @param planNode - node of the current plan
  Q_INVOKABLE vtkMRMLRTBeamNode* createBeamInPlan(vtkMRMLRTPlanNode* planNode);

signals:
  /// Signals for dose calculation progress update
  /// \param progress Value between 0 and 1
  void progressUpdated(double progress);

public slots:
  /// Called when the dose engine of a plan is changed.
  /// The beam parameters specific to the new engine are added to all the beams
  /// under the plan containing default values
  void applyDoseEngineInPlan(vtkObject* nodeObject);

protected slots:
  /// Called when a node is added to the scene
  void onNodeAdded(vtkObject* scene, vtkObject* nodeObject);

  /// Called when scene import is finished
  void onSceneImportEnded(vtkObject* sceneObject);

protected:
  QScopedPointer<qSlicerDoseEngineLogic> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDoseEngineLogic);
  Q_DISABLE_COPY(qSlicerDoseEngineLogic);
};

#endif
