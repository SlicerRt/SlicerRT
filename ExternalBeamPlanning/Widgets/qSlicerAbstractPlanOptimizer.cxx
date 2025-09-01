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

// ExternalBeamPlanning includes
#include "qSlicerAbstractPlanOptimizer.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLColorTableNode.h>
#include "vtkMRMLRTObjectiveNode.h"
#include <vtkMRMLSelectionNode.h>

// Slicer includes
#include <qSlicerCoreApplication.h>
#include "vtkSlicerApplicationLogic.h"

// VTK includes
#include <vtkSmartPointer.h>

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
double qSlicerAbstractPlanOptimizer::DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM = 16.0;

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerAbstractPlanOptimizerPrivate
{
  Q_DECLARE_PUBLIC(qSlicerAbstractPlanOptimizer);
protected:
  qSlicerAbstractPlanOptimizer* const q_ptr;
public:
  qSlicerAbstractPlanOptimizerPrivate(qSlicerAbstractPlanOptimizer& object);
};

//-----------------------------------------------------------------------------
// qSlicerAbstractPlanOptimizerPrivate methods

//-----------------------------------------------------------------------------
qSlicerAbstractPlanOptimizerPrivate::qSlicerAbstractPlanOptimizerPrivate(qSlicerAbstractPlanOptimizer& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
// qSlicerAbstractPlanOptimizer methods

//----------------------------------------------------------------------------
qSlicerAbstractPlanOptimizer::qSlicerAbstractPlanOptimizer(QObject* parent)
  : Superclass(parent)
  , m_Name(QString())
  , d_ptr( new qSlicerAbstractPlanOptimizerPrivate(*this) )
{
}

//----------------------------------------------------------------------------
qSlicerAbstractPlanOptimizer::~qSlicerAbstractPlanOptimizer() = default;

//----------------------------------------------------------------------------
QString qSlicerAbstractPlanOptimizer::name() const
{
  if (m_Name.isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": Empty Optimization engine name";
  }
  return this->m_Name;
}

//-----------------------------------------------------------------------------
void qSlicerAbstractPlanOptimizer::setName(QString name)
{
  Q_UNUSED(name);
  qCritical() << Q_FUNC_INFO << ": Cannot set Optimization engine name by method, only in constructor";
}

//----------------------------------------------------------------------------
QString qSlicerAbstractPlanOptimizer::optimizePlan(vtkMRMLRTPlanNode* planNode)
{
  if (!planNode)
  {
    QString errorMessage = QString("Invalid Plan Node");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  // Add RT plan to the same branch where the reference volume is
  vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    QString errorMessage("Unable to access reference volume");
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
  vtkIdType referenceVolumeShItemID = shNode->GetItemByDataNode(referenceVolumeNode);
  if (referenceVolumeShItemID)
  {
    vtkIdType planShItemID = planNode->GetPlanSubjectHierarchyItemID();
    if (planShItemID)
    {
      shNode->SetItemParent(planShItemID, shNode->GetItemParent(referenceVolumeShItemID));
    }
    else
    {
      qCritical() << Q_FUNC_INFO << ": Failed to access RT plan subject hierarchy item, although it should always be available";
    }
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access reference volume subject hierarchy item";
  }

  // Get saved objectives
  std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> objectives = this->getSavedObjectiveNodes();

  // Get or create output optimization volume for plan
  vtkSmartPointer<vtkMRMLScalarVolumeNode> resultOptimizationVolumeNode = planNode->GetOutputTotalDoseVolumeNode();
  if (!resultOptimizationVolumeNode)
  {
   resultOptimizationVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
      // Give default name for result node (engine can give it a more meaningful name)
      std::string resultOptimizationNodeName = std::string(planNode->GetName()) + "_Optimization";
      resultOptimizationVolumeNode->SetName(resultOptimizationNodeName.c_str());
  }
  planNode->GetScene()->AddNode(resultOptimizationVolumeNode);

  // Optimize
  QString errorMessage = this->optimizePlanUsingOptimizer(planNode, objectives, resultOptimizationVolumeNode);

  if (errorMessage.isEmpty())
  {
   // Add result dose volume to plan
      this->addResultOptimizedDose(resultOptimizationVolumeNode, planNode);
  }
  return errorMessage;
}

//----------------------------------------------------------------------------
std::vector<qSlicerAbstractPlanOptimizer::ObjectiveStruct> qSlicerAbstractPlanOptimizer::getAvailableObjectives()
{
  return this->availableObjectives;
}

//----------------------------------------------------------------------------
void qSlicerAbstractPlanOptimizer::setAvailableObjectives()
{
  qCritical() << Q_FUNC_INFO << ": no available Objectives ";
}

//----------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> qSlicerAbstractPlanOptimizer::getSavedObjectiveNodes()
{
  return this->savedObjectiveNodes;
}

//----------------------------------------------------------------------------
void qSlicerAbstractPlanOptimizer::saveObjectiveNodeInOptimizer(vtkSmartPointer<vtkMRMLRTObjectiveNode> objectiveNode)
{
  this->savedObjectiveNodes.push_back(objectiveNode);
}

//----------------------------------------------------------------------------
void qSlicerAbstractPlanOptimizer::removeAllObjectiveNodes()
{
  this->savedObjectiveNodes.clear();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractPlanOptimizer::addResultOptimizedDose(vtkMRMLScalarVolumeNode* resultOptimizedDose, vtkMRMLRTPlanNode* planNode, bool replace/*=true*/)
{    
  if (!resultOptimizedDose)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid result dose";
    return;
  }
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid plan node";
    return;
  }
  vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": " << "Unable to access reference volume";
    return;
  }
  vtkMRMLScene* scene = planNode->GetScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  // Set optimized dose volume attribute so that it is identified as dose
  resultOptimizedDose->SetAttribute(vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Subject hierarchy related operations
  vtkIdType beamShItemID = shNode->GetItemByDataNode(planNode);
  if (beamShItemID)
  {
    // Add result under beam in subject hierarchy
    shNode->CreateItem(beamShItemID, resultOptimizedDose);

    // Set dose unit value to Gy if dose engine did not set it already (potentially to other unit)
    vtkIdType studyItemID = shNode->GetItemAncestorAtLevel(beamShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    if (!studyItemID)
    {
      qWarning() << Q_FUNC_INFO << ": Unable to find study item that contains the plan! Creating a study item and adding the reference dose and the plan under it is necessary in order for dose evaluation steps to work properly";
    }
    else if (shNode->GetItemAttribute(studyItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME).empty())
    {
      shNode->SetItemAttribute(studyItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME, "Gy");
    }
  }
    
  resultOptimizedDose->CreateDefaultDisplayNodes(); // Make sure display node is present
  if (resultOptimizedDose->GetDisplayNode())
  {
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(resultOptimizedDose->GetVolumeDisplayNode());

    doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeDose_ColorTable_Relative");

    // Set window level based on prescription dose
    double rxDose = planNode->GetRxDose();
    doseScalarVolumeDisplayNode->AutoWindowLevelOn();

    // Set threshold to hide very low dose values
    doseScalarVolumeDisplayNode->SetLowerThreshold(0.05 * rxDose);
    doseScalarVolumeDisplayNode->ApplyThresholdOn();
  }
  else
  {
    qWarning() << Q_FUNC_INFO << ": Display node is not available for dose volume node. The default color table will be used.";
  }

  // Show total dose in foreground
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (selectionNode)
  {
    // Make sure reference volume is shown in background
    selectionNode->SetReferenceActiveVolumeID(referenceVolumeNode->GetID());
    // Select as foreground volume
    selectionNode->SetReferenceSecondaryVolumeID(resultOptimizedDose->GetID());
    qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection(0);
    
    // Set opacity so that volume is visible
    vtkMRMLSliceCompositeNode* compositeNode = nullptr;
    int numberOfCompositeNodes = planNode->GetScene()->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");
    for (int i = 0; i < numberOfCompositeNodes; i++)
    {
      compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast(planNode->GetScene()->GetNthNodeByClass(i, "vtkMRMLSliceCompositeNode"));
      if (compositeNode && compositeNode->GetForegroundOpacity() == 0.0)
      {
        compositeNode->SetForegroundOpacity(0.5);
      }
    }
  }
}
