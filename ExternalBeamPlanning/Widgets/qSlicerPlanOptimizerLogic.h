/*==============================================================================

  Copyright (c) German Cancer Research Center (DKFZ),
  Heidelberg, Germany. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Niklas Wahl, German Cancer Research Center (DKFZ).

==============================================================================*/

#ifndef __qSlicerPlanOptimizerLogic_h
#define __qSlicerPlanOptimizerLogic_h

// ExternalBeamPlanning includes
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
class qSlicerPlanOptimizerLogicPrivate;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract optimization algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class inverse treatment plan optimization
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerPlanOptimizerLogic :
  public QObject, public virtual qSlicerObject
{
  Q_OBJECT
    QVTK_OBJECT

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerPlanOptimizerLogic(QObject* parent = nullptr);
  /// Destructor
  ~qSlicerPlanOptimizerLogic() override;

public:
  /// Set the current MRML scene to the widget
  Q_INVOKABLE virtual void setMRMLScene(vtkMRMLScene* scene);

  /// Calculate dose for a plan
  Q_INVOKABLE QString optimizePlan(vtkMRMLRTPlanNode* planNode);


signals:
  /// Signals for optimization progress update
  /// \param progress Value between 0 and 1
  void progressInfoUpdated(QString info);

protected slots:
  /// Called when a node is added to the scene
  void onNodeAdded(vtkObject* scene, vtkObject* nodeObject);

  /// Called when scene import is finished
  void onSceneImportEnded(vtkObject* sceneObject);

protected:
  QScopedPointer<qSlicerPlanOptimizerLogic> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlanOptimizerLogic);
  Q_DISABLE_COPY(qSlicerPlanOptimizerLogic);
};

#endif
