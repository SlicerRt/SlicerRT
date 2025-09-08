/*==============================================================================

  Copyright (c) German Cancer Research Center (DKFZ),
  Heidelberg, Germany. All Rights Reserved..

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Niklas Wahl, German Cancer Research Center (DKFZ).

==============================================================================*/

// ExternalBeamPlanning includes
#include "qSlicerPlanOptimizerLogic.h"
#include "qSlicerPlanOptimizerPluginHandler.h"
#include "qSlicerAbstractPlanOptimizer.h"

// Beams includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTIonBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// SlicerRT includes
#include "vtkMRMLDoseAccumulationNode.h"
#include "vtkSlicerDoseAccumulationModuleLogic.h"
#include "vtkSlicerIsodoseModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSelectionNode.h>

// Slicer includes
#include "qSlicerCoreApplication.h"
#include "vtkSlicerApplicationLogic.h"

// VTK includes
#include <vtkSmartPointer.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy
class qSlicerPlanOptimizerLogicPrivate
{
  Q_DECLARE_PUBLIC(qSlicerPlanOptimizerLogic);
protected:
  qSlicerPlanOptimizerLogic* const q_ptr;
public:
  qSlicerPlanOptimizerLogicPrivate(qSlicerPlanOptimizerLogic& object);
  ~qSlicerPlanOptimizerLogicPrivate();
  void loadApplicationSettings();
};

//-----------------------------------------------------------------------------
// qSlicerPlanOptimizerLogicPrivate methods

//-----------------------------------------------------------------------------
qSlicerPlanOptimizerLogicPrivate::qSlicerPlanOptimizerLogicPrivate(qSlicerPlanOptimizerLogic& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerPlanOptimizerLogicPrivate::~qSlicerPlanOptimizerLogicPrivate() = default;

//-----------------------------------------------------------------------------
void qSlicerPlanOptimizerLogicPrivate::loadApplicationSettings()
{
  //TODO: Implement if there are application settings (such as default optimization engine)
  //      See qSlicerSubjectHierarchyPluginLogicPrivate::loadApplicationSettings
}

//-----------------------------------------------------------------------------
// qSlicerPlanOptimizerLogic methods

//----------------------------------------------------------------------------
qSlicerPlanOptimizerLogic::qSlicerPlanOptimizerLogic(QObject* parent)
  : QObject(parent)
{
}

//----------------------------------------------------------------------------
qSlicerPlanOptimizerLogic::~qSlicerPlanOptimizerLogic() = default;

//-----------------------------------------------------------------------------
QString qSlicerPlanOptimizerLogic::optimizePlan(vtkMRMLRTPlanNode* planNode)
{
  QString errorMessage("");
  if (!planNode || !planNode->GetScene())
  {
    errorMessage = QString("Invalid MRML scene or RT plan node");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  // Get selected plan optimizer
  qSlicerAbstractPlanOptimizer* selectedEngine =
    qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
  if (!selectedEngine)
  {
    QString errorString = QString("Unable to access optimizer with name %1").arg(planNode->GetPlanOptimizerName() ? planNode->GetPlanOptimizerName() : "nullptr");
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return errorMessage;
  }

  connect(selectedEngine, SIGNAL(progressInfoUpdated(QString)), this, SIGNAL(progressInfoUpdated(QString)));

  emit progressInfoUpdated("starting");

  errorMessage = selectedEngine->optimizePlan(planNode);
  if (!errorMessage.isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  disconnect(selectedEngine, SIGNAL(progressInfoUpdated(QString)), this, SIGNAL(progressInfoUpdated(QString)));

  return QString();
}

//-----------------------------------------------------------------------------
void qSlicerPlanOptimizerLogic::setMRMLScene(vtkMRMLScene* scene)
{
  this->qSlicerObject::setMRMLScene(scene);

  // Connect scene node added event so that the new subject hierarchy nodes can be claimed by a plugin
  qvtkReconnect(scene, vtkMRMLScene::NodeAddedEvent, this, SLOT(onNodeAdded(vtkObject*, vtkObject*)));
  // Connect scene import ended event so that subject hierarchy nodes can be created for supported data nodes if missing (backwards compatibility)
  qvtkReconnect(scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportEnded(vtkObject*)));
}

//-----------------------------------------------------------------------------
void qSlicerPlanOptimizerLogic::onNodeAdded(vtkObject* sceneObject, vtkObject* nodeObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
  {
    return;
  }

  if (nodeObject->IsA("vtkMRMLRTPlanNode"))
  {
    // Observe plan optimizer changed event ...
    vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(nodeObject);
    qvtkConnect(planNode, vtkMRMLRTPlanNode::PlanOptimizerChanged, this, SLOT(applyPlanOptimizerInPlan(vtkObject*)));
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlanOptimizerLogic::onSceneImportEnded(vtkObject* sceneObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
  {
    return;
  }

  // Traverse all plan nodes in the scene and observe plan optimizer changed event ...
  std::vector<vtkMRMLNode*> planNodes;
  scene->GetNodesByClass("vtkMRMLRTPlanNode", planNodes);
  for (std::vector<vtkMRMLNode*>::iterator planNodeIt = planNodes.begin(); planNodeIt != planNodes.end(); ++planNodeIt)
  {
    vtkMRMLNode* planNode = (*planNodeIt);
    qvtkConnect(planNode, vtkMRMLRTPlanNode::PlanOptimizerChanged, this, SLOT(applyPlanOptimizerInPlan(vtkObject*)));
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlanOptimizerLogic::applyPlanOptimizerInPlan(vtkObject* nodeObject)
{
  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(nodeObject);
  if (!planNode)
  {
    return;
  }

  // Get newly selected plan optimizer
  qSlicerAbstractPlanOptimizer* selectedEngine =
    qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
  if (!selectedEngine)
  {
    QString errorString = QString("Unable to access plan optimizer with name %1").arg(planNode->GetPlanOptimizerName() ? planNode->GetPlanOptimizerName() : "nullptr");
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }
}
