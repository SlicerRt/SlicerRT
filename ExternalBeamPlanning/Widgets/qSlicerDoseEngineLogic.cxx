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

// Dose engines includes
#include "qSlicerDoseEngineLogic.h"

#include "qSlicerDoseEnginePluginHandler.h"
#include "qSlicerAbstractDoseEngine.h"

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
class qSlicerDoseEngineLogicPrivate
{
  Q_DECLARE_PUBLIC(qSlicerDoseEngineLogic);
protected:
  qSlicerDoseEngineLogic* const q_ptr;
public:
  qSlicerDoseEngineLogicPrivate(qSlicerDoseEngineLogic& object);
  ~qSlicerDoseEngineLogicPrivate();
  void loadApplicationSettings();
};

//-----------------------------------------------------------------------------
// qSlicerDoseEngineLogicPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseEngineLogicPrivate::qSlicerDoseEngineLogicPrivate(qSlicerDoseEngineLogic& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerDoseEngineLogicPrivate::~qSlicerDoseEngineLogicPrivate() = default;

//-----------------------------------------------------------------------------
void qSlicerDoseEngineLogicPrivate::loadApplicationSettings()
{
  //TODO: Implement if there are application settings (such as default dose engine)
  //      See qSlicerSubjectHierarchyPluginLogicPrivate::loadApplicationSettings
}

//-----------------------------------------------------------------------------
// qSlicerDoseEngineLogic methods

//----------------------------------------------------------------------------
qSlicerDoseEngineLogic::qSlicerDoseEngineLogic(QObject* parent)
  : QObject(parent)
{
}

//----------------------------------------------------------------------------
qSlicerDoseEngineLogic::~qSlicerDoseEngineLogic() = default;

//-----------------------------------------------------------------------------
void qSlicerDoseEngineLogic::setMRMLScene(vtkMRMLScene* scene)
{
  this->qSlicerObject::setMRMLScene(scene);

  // Connect scene node added event so that the new subject hierarchy nodes can be claimed by a plugin
  qvtkReconnect( scene, vtkMRMLScene::NodeAddedEvent, this, SLOT( onNodeAdded(vtkObject*,vtkObject*) ) );
  // Connect scene import ended event so that subject hierarchy nodes can be created for supported data nodes if missing (backwards compatibility)
  qvtkReconnect( scene, vtkMRMLScene::EndImportEvent, this, SLOT( onSceneImportEnded(vtkObject*) ) );
}

//-----------------------------------------------------------------------------
void qSlicerDoseEngineLogic::onNodeAdded(vtkObject* sceneObject, vtkObject* nodeObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
  {
    return;
  }

  if (nodeObject->IsA("vtkMRMLRTPlanNode"))
  {
    // Observe dose engine changed event so that default beam parameters
    // can be added for the newly selected engine in the beams contained by the plan
    vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(nodeObject);
    qvtkConnect( planNode, vtkMRMLRTPlanNode::DoseEngineChanged, this, SLOT( applyDoseEngineInPlan(vtkObject*) ) );
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseEngineLogic::onSceneImportEnded(vtkObject* sceneObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
  {
    return;
  }

  // Traverse all plan nodes in the scene and observe dose engine changed event so that default
  // beam parameters can be added for the newly selected engine in the beams contained by the plan
  std::vector<vtkMRMLNode*> planNodes;
  scene->GetNodesByClass("vtkMRMLRTPlanNode", planNodes);
  for (std::vector<vtkMRMLNode*>::iterator planNodeIt = planNodes.begin(); planNodeIt != planNodes.end(); ++planNodeIt)
  {
    vtkMRMLNode* planNode = (*planNodeIt);
    qvtkConnect( planNode, vtkMRMLRTPlanNode::DoseEngineChanged, this, SLOT( applyDoseEngineInPlan(vtkObject*) ) );
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseEngineLogic::applyDoseEngineInPlan(vtkObject* nodeObject)
{
  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(nodeObject);
  if (!planNode)
  {
    return;
  }

  // Get newly selected dose engine
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(planNode->GetDoseEngineName());
  if (!selectedEngine)
  {
    QString errorString = QString("Unable to access dose engine with name %1").arg(planNode->GetDoseEngineName() ? planNode->GetDoseEngineName() : "nullptr");
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }

  // Update beam parameter visibilities in tab widget
  foreach (qSlicerAbstractDoseEngine* currentEngine, qSlicerDoseEnginePluginHandler::instance()->registeredDoseEngines())
  {
    // Show parameters if engine is the current engine, hide otherwise
    currentEngine->setBeamParametersVisible(currentEngine == selectedEngine);
  }

  // Add engine-specific beam parameters to all beams contained by the plan.
  // Existing parameters are not overwritten, missing ones get the default values.
  std::vector<vtkMRMLRTBeamNode*> beams;
  planNode->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* beamNode = (*beamIt);
    selectedEngine->addBeamParameterAttributesToBeamNode(beamNode);
  }
}

//---------------------------------------------------------------------------
QString qSlicerDoseEngineLogic::calculateDose(vtkMRMLRTPlanNode* planNode)
{
  QString errorMessage("");
  if (!planNode || !planNode->GetScene())
  {
    errorMessage = QString("Invalid MRML scene or RT plan node");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  // Get selected dose engine
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(planNode->GetDoseEngineName());
  if (!selectedEngine)
  {
    QString errorString = QString("Unable to access dose engine with name %1").arg(planNode->GetDoseEngineName() ? planNode->GetDoseEngineName() : "nullptr");
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return errorMessage;
  }

  // Calculate dose for each beam under the plan
  std::vector<vtkMRMLRTBeamNode*> beams;
  planNode->GetBeams(beams);
  int numberOfBeams = beams.size();
  int currentBeamIndex = 0;
  double progress = 0.0;

  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt, ++currentBeamIndex)
  {
    vtkMRMLRTBeamNode* beamNode = (*beamIt);
    if (beamNode)
    {
      progress = (double)currentBeamIndex / (numberOfBeams+1);
      emit progressUpdated(progress);

      // Calculate dose for current beam
      errorMessage = selectedEngine->calculateDose(beamNode);
      if (!errorMessage.isEmpty())
      {
        qCritical() << Q_FUNC_INFO << ": " << errorMessage;
        return errorMessage;
      }
    }
    else
    {
      errorMessage = QString("Invalid beam!");
      qCritical() << Q_FUNC_INFO << ": " << errorMessage;
      return errorMessage;
    }
  }

  progress = (double)numberOfBeams / (numberOfBeams+1);
  emit progressUpdated(progress);

  // Accumulate calculated per-beam dose distributions into the total dose volume
  errorMessage = this->createAccumulatedDose(planNode);
  if (!errorMessage.isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  progress = 1.0;
  emit progressUpdated(progress);

  return QString();
}

//---------------------------------------------------------------------------
QString qSlicerDoseEngineLogic::createAccumulatedDose(vtkMRMLRTPlanNode* planNode)
{
  if (!planNode || !planNode->GetScene())
  {
    QString errorMessage("Invalid MRML scene or RT plan node");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    QString errorMessage("Unable to access reference volume");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  vtkMRMLScalarVolumeNode* totalDoseVolumeNode = planNode->GetOutputTotalDoseVolumeNode();
  if (!totalDoseVolumeNode)
  {
    QString errorMessage("Unable to access output dose volume");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(planNode->GetScene());
  if (!shNode)
  {
    QString errorMessage("Failed to access subject hierarchy node");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  // Get selected dose engine
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(planNode->GetDoseEngineName());
  if (!selectedEngine)
  {
    QString errorMessage = QString("Unable to access dose engine with name %1").arg(planNode->GetDoseEngineName() ? planNode->GetDoseEngineName() : "nullptr");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  // Create parameter node for dose accumulation
  vtkSmartPointer<vtkMRMLDoseAccumulationNode> doseAccumulationNode = vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New();
  planNode->GetScene()->AddNode(doseAccumulationNode);
  std::string doseAccumulationNodeName = std::string("DoseAccumulation_") + planNode->GetName();
  doseAccumulationNodeName = planNode->GetScene()->GenerateUniqueName(doseAccumulationNodeName);
  doseAccumulationNode->SetName(doseAccumulationNodeName.c_str());
  doseAccumulationNode->SetAndObserveAccumulatedDoseVolumeNode(totalDoseVolumeNode);
  doseAccumulationNode->SetAndObserveReferenceDoseVolumeNode(referenceVolumeNode); //TODO: CT seems to be the reference based on old code but dose accumulation code suggests it should be a dose

  // Collect per-beam dose volumes from beams under the plan
  std::vector<vtkMRMLRTBeamNode*> beams;
  planNode->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* beamNode = (*beamIt);
    if (!beamNode)
    {
      QString errorMessage("Beam not found");
      qCritical() << Q_FUNC_INFO << ": " << errorMessage;
      continue;
    }

    // Get calculation result dose volume from beam
    vtkMRMLScalarVolumeNode* perBeamDoseVolume = selectedEngine->getResultDoseForBeam(beamNode);
    if (!perBeamDoseVolume)
    {
      QString errorMessage = QString("No calculated dose found for beam %1").arg(beamNode->GetName());
      qCritical() << Q_FUNC_INFO << ": " << errorMessage;
      continue;
    }

    // Add dose volume to dose accumulation
    doseAccumulationNode->AddSelectedInputVolumeNode(perBeamDoseVolume, beamNode->GetBeamWeight());
  }

  // Accumulate dose
  vtkSmartPointer<vtkSlicerDoseAccumulationModuleLogic> doseAccumulationLogic = vtkSmartPointer<vtkSlicerDoseAccumulationModuleLogic>::New();
  doseAccumulationLogic->SetMRMLScene(planNode->GetScene());
  std::string errorMessage = doseAccumulationLogic->AccumulateDoseVolumes(doseAccumulationNode);
  if (!errorMessage.empty())
  {
    return QString(errorMessage.c_str());
  }

  // Add total dose volume to subject hierarchy under the study of the reference volume
  vtkIdType referenceVolumeShItemID = shNode->GetItemByDataNode(referenceVolumeNode);
  if (referenceVolumeShItemID)
  {
    vtkIdType studyItemID = shNode->GetItemAncestorAtLevel(referenceVolumeShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    if (studyItemID)
    {
      shNode->CreateItem(studyItemID, totalDoseVolumeNode);
    }
  }

  totalDoseVolumeNode->CreateDefaultDisplayNodes(); // Make sure display node is present
  if (totalDoseVolumeNode->GetVolumeDisplayNode())
  {
    // Set dose color table
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(totalDoseVolumeNode->GetDisplayNode());
    vtkMRMLColorTableNode* defaultDoseColorTable = vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(planNode->GetScene());
    if (defaultDoseColorTable)
    {
      doseScalarVolumeDisplayNode->SetAndObserveColorNodeID(defaultDoseColorTable->GetID());
    }
    else
    {
      doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
      qCritical() << Q_FUNC_INFO << ": Failed to get default dose color table!";
    }

    // Set window level based on prescription dose
    double rxDose = planNode->GetRxDose();
    doseScalarVolumeDisplayNode->AutoWindowLevelOff();
    doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, rxDose);

    // Set threshold to hide very low dose values
    doseScalarVolumeDisplayNode->SetLowerThreshold(0.05 * rxDose);
    doseScalarVolumeDisplayNode->ApplyThresholdOn();
  }
  else
  {
    qWarning() << Q_FUNC_INFO << ": Display node is not available for calculated dose volume node. The default color table will be used.";
  }

  // Show total dose in foreground
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (selectionNode)
  {
    // Make sure reference volume is shown in background
    selectionNode->SetReferenceActiveVolumeID(referenceVolumeNode->GetID());
    // Select as foreground volume
    selectionNode->SetReferenceSecondaryVolumeID(totalDoseVolumeNode->GetID());
    qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection(0);

    // Set opacity so that volume is visible
    vtkMRMLSliceCompositeNode* compositeNode = nullptr;
    int numberOfCompositeNodes = planNode->GetScene()->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");
    for (int i=0; i<numberOfCompositeNodes; i++)
    {
      compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast ( planNode->GetScene()->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
      if (compositeNode && compositeNode->GetForegroundOpacity() == 0.0)
      {
        compositeNode->SetForegroundOpacity(0.5);
      }
    }
  } 

  return QString();
}

//---------------------------------------------------------------------------
void qSlicerDoseEngineLogic::removeIntermediateResults(vtkMRMLRTPlanNode* planNode)
{
  if (!planNode)
  {
    return;
  }

  // Get selected dose engine
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(planNode->GetDoseEngineName());
  if (!selectedEngine)
  {
    QString errorString = QString("Unable to access dose engine with name %1").arg(planNode->GetDoseEngineName() ? planNode->GetDoseEngineName() : "nullptr");
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }

  std::vector<vtkMRMLRTBeamNode*> beams;
  planNode->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator it=beams.begin(); it!=beams.end(); ++it)
  {
    vtkMRMLRTBeamNode* currentBeam = (*it);

    // Remove intermediate results other than the per-beam dose volume
    selectedEngine->removeIntermediateResults(currentBeam);

    // Remove per-beam dose volume
    vtkMRMLScalarVolumeNode* currentDose = selectedEngine->getResultDoseForBeam(currentBeam);
    currentBeam->GetScene()->RemoveNode(currentDose);
  }
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* qSlicerDoseEngineLogic::createBeamInPlan(vtkMRMLRTPlanNode* planNode)
{
  if (!planNode || !planNode->GetScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid plan node!";
    return nullptr;
  }

  // Create beam and add to scene
  vtkSmartPointer<vtkMRMLRTBeamNode> beamNode;
  if (planNode->GetIonPlanFlag())
  {
    beamNode = vtkSmartPointer<vtkMRMLRTIonBeamNode>::New();
  }
  else
  {
    beamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  }

  beamNode->SetName(planNode->GenerateNewBeamName().c_str());
  planNode->GetScene()->AddNode(beamNode);

  // Add beam parameters specific to the current engine selected for plan
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(planNode->GetDoseEngineName());
  if (!selectedEngine)
  {
    QString errorString = QString("Unable to access dose engine with name %1").arg(planNode->GetDoseEngineName() ? planNode->GetDoseEngineName() : "nullptr");
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return nullptr;
  }
  selectedEngine->addBeamParameterAttributesToBeamNode(beamNode);

  // Add beam to plan
  planNode->AddBeam(beamNode);

  return beamNode;
}
