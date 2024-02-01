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
  QString errorMessage = this->optimizePlanUsingEngine(planNode, resultOptimizationVolumeNode);
  
  return errorMessage;
}