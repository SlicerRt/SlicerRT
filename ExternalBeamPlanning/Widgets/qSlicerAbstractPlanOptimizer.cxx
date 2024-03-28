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

This file was originally developed by Niklas Wahl, German Cancer Research Center (DKFZ)

==============================================================================*/

// Optimization engines includes
#include "qSlicerAbstractPlanOptimizer.h"

// Plan
#include "vtkMRMLRTPlanNode.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"
#include "vtkSlicerIsodoseModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLColorTableNode.h>

// VTK includes
#include <vtkSmartPointer.h>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerAbstractModule.h"
#include "qSlicerModuleManager.h"
#include "qSlicerAbstractModuleWidget.h"

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
double qSlicerAbstractPlanOptimizer::DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM = 16.0;

//----------------------------------------------------------------------------
static const char* RESULT_DOSE_REFERENCE_ROLE = "ResultDoseRef";

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

  // Create output Optimization volume for beam
  vtkSmartPointer<vtkMRMLScalarVolumeNode> resultOptimizationVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  planNode->GetScene()->AddNode(resultOptimizationVolumeNode);
  // Give default name for result node (engine can give it a more meaningful name)
  std::string resultOptimizationNodeName = std::string(planNode->GetName()) + "_Optimization";
  resultOptimizationVolumeNode->SetName(resultOptimizationNodeName.c_str());

  // Optimize
  QString errorMessage = this->optimizePlanUsingOptimizer(planNode, resultOptimizationVolumeNode);
  
  if (errorMessage.isEmpty())
  {
      // Add result dose volume to beam
      this->addResultDose(resultOptimizationVolumeNode, planNode);
  }
  return errorMessage;
}


void qSlicerAbstractPlanOptimizer::addResultDose(vtkMRMLScalarVolumeNode* resultDose, vtkMRMLRTPlanNode* planNode, bool replace/*=true*/)
{    
    if (!resultDose)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid result dose";
        return;
    }
    if (!planNode)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid plan node";
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

    // // Remove already existing referenced dose volume if any
    // if (replace)
    // {
    //     std::vector<const char*> referencedDoseNodeIds;
    //     planNode->GetNodeReferenceIDs(RESULT_DOSE_REFERENCE_ROLE, referencedDoseNodeIds);
    //     for (std::vector<const char*>::iterator refIt = referencedDoseNodeIds.begin(); refIt != referencedDoseNodeIds.end(); ++refIt)
    //     {
    //         vtkMRMLNode* node = scene->GetNodeByID(*refIt);
    //         scene->RemoveNode(node);
    //     }
    // }

    // // Add reference in beam to result dose for later access
    // planNode->AddNodeReferenceID(RESULT_DOSE_REFERENCE_ROLE, resultDose->GetID());

    // // Set dose volume attribute so that it is identified as dose
    // resultDose->SetAttribute(vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

    // // Subject hierarchy related operations
    // vtkIdType beamShItemID = shNode->GetItemByDataNode(planNode);
    // if (beamShItemID)
    // {
    //     // Add result under beam in subject hierarchy
    //     shNode->CreateItem(beamShItemID, resultDose);

    //     // Set dose unit value to Gy if dose engine did not set it already (potentially to other unit)
    //     vtkIdType studyItemID = shNode->GetItemAncestorAtLevel(beamShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    //     if (!studyItemID)
    //     {
    //         qWarning() << Q_FUNC_INFO << ": Unable to find study item that contains the plan! Creating a study item and adding the reference dose and the plan under it is necessary in order for dose evaluation steps to work properly";
    //     }
    //     else if (shNode->GetItemAttribute(studyItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME).empty())
    //     {
    //         shNode->SetItemAttribute(studyItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME, "Gy");
    //     }
    // }

    // Set up display for dose volume
    resultDose->CreateDefaultDisplayNodes(); // Make sure display node is present
    if (resultDose->GetDisplayNode())
    {
        vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(resultDose->GetVolumeDisplayNode());

        // Set colormap to dose
        vtkMRMLColorTableNode* defaultDoseColorTable = vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(resultDose->GetScene());
        if (defaultDoseColorTable)
        {
            doseScalarVolumeDisplayNode->SetAndObserveColorNodeID(defaultDoseColorTable->GetID());
        }
        else
        {
            doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
            qCritical() << Q_FUNC_INFO << ": Failed to get default dose color table";
        }


        // Set window level based on prescription dose
        double rxDose = planNode->GetRxDose();
        doseScalarVolumeDisplayNode->AutoWindowLevelOff();
        doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, rxDose * 1.1);

        // Set threshold to hide very low dose values
        doseScalarVolumeDisplayNode->SetLowerThreshold(0.05 * rxDose);
        doseScalarVolumeDisplayNode->ApplyThresholdOn();

    }
    else
    {
        qWarning() << Q_FUNC_INFO << ": Display node is not available for dose volume node. The default color table will be used.";
    }
}