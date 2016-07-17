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
#include "qSlicerAbstractDoseEngine.h"

// Beams includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
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

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
double qSlicerAbstractDoseEngine::DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM = 16.0;

//----------------------------------------------------------------------------
static const char* INTERMEDIATE_RESULT_REFERENCE_ROLE = "IntermediateResultRef";
static const char* RESULT_DOSE_REFERENCE_ROLE = "ResultDoseRef";

//----------------------------------------------------------------------------
qSlicerAbstractDoseEngine::qSlicerAbstractDoseEngine(QObject* parent)
  : QObject(parent)
  , m_Name(QString())
{
}

//----------------------------------------------------------------------------
qSlicerAbstractDoseEngine::~qSlicerAbstractDoseEngine()
{
}

//----------------------------------------------------------------------------
QString qSlicerAbstractDoseEngine::name() const
{
  return m_Name;
}

//----------------------------------------------------------------------------
QString qSlicerAbstractDoseEngine::calculateDose(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    QString errorMessage("Invalid beam node");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  vtkMRMLRTPlanNode* parentPlanNode = beamNode->GetParentPlanNode();
  if (!parentPlanNode)
  {
    QString errorMessage = QString("Unable to access parent node for beam %1").arg(beamNode->GetName());
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  // Add RT plan to the same branch where the reference volume is
  vtkMRMLScalarVolumeNode* referenceVolumeNode = parentPlanNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    QString errorMessage("Unable to access reference volume");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  vtkMRMLSubjectHierarchyNode* referenceVolumeShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(referenceVolumeNode);
  if (referenceVolumeShNode)
  {
    vtkMRMLSubjectHierarchyNode* planShNode = parentPlanNode->GetPlanSubjectHierarchyNode();
    if (planShNode)
    {
      planShNode->SetParentNodeID(referenceVolumeShNode->GetParentNodeID());
    }
    else
    {
      qCritical() << Q_FUNC_INFO << ": Failed to access RT plan subject hierarchy node, although it should always be available!";
    }
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access reference volume subject hierarchy node!";
  }

  // Remove past intermediate results for beam before calculating dose again
  this->removeIntermediateResults(beamNode);

  // Create output dose volume for beam
  vtkSmartPointer<vtkMRMLScalarVolumeNode> resultDoseVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  beamNode->GetScene()->AddNode(resultDoseVolumeNode);
  // Give default name for result node (engine can give it a more meaningful name)
  std::string resultDoseNodeName = std::string(beamNode->GetName()) + "_Dose";
  resultDoseVolumeNode->SetName(resultDoseNodeName.c_str());
  
  // Calculate dose
  QString errorMessage = this->calculateDoseUsingEngine(beamNode, resultDoseVolumeNode);
  if (errorMessage.isEmpty())
  {
    // Add result dose volume to beam
    this->addResultDose(resultDoseVolumeNode, beamNode);
  }

  return errorMessage;
}

//---------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::addIntermediateResult(vtkMRMLNode* result, vtkMRMLRTBeamNode* beamNode)
{
  if (!result)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid intermediate result";
    return;
  }
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    return;
  }

  // Add reference in beam to result for later access
  beamNode->AddNodeReferenceID(INTERMEDIATE_RESULT_REFERENCE_ROLE, result->GetID());

  // Add result under beam in subject hierarchy
  vtkMRMLSubjectHierarchyNode* beamShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(beamNode);
  if (beamShNode)
  {
    vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      beamNode->GetScene(), beamShNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(), 
      result->GetName(), result );
  }
}

//---------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::addResultDose(vtkMRMLScalarVolumeNode* resultDose, vtkMRMLRTBeamNode* beamNode, bool replace/*=true*/)
{
  if (!resultDose)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid result dose";
    return;
  }
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    return;
  }

  // Remove already existing referenced dose volume if any
  if (replace)
  {
    vtkMRMLScene* scene = beamNode->GetScene();
    std::vector<const char*> referencedDoseNodeIds;
    beamNode->GetNodeReferenceIDs(RESULT_DOSE_REFERENCE_ROLE, referencedDoseNodeIds);
    for (std::vector<const char*>::iterator refIt=referencedDoseNodeIds.begin(); refIt != referencedDoseNodeIds.end(); ++refIt)
    {
      vtkMRMLNode* node = scene->GetNodeByID(*refIt);
      scene->RemoveNode(node);
    }
  }

  // Add reference in beam to result dose for later access
  beamNode->AddNodeReferenceID(RESULT_DOSE_REFERENCE_ROLE, resultDose->GetID());

  // Set dose volume attribute so that it is identified as dose
  resultDose->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Subject hierarchy related operations
  vtkMRMLSubjectHierarchyNode* beamShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(beamNode);
  if (beamShNode)
  {
    // Add result under beam in subject hierarchy
    vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      beamNode->GetScene(), beamShNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(), 
      resultDose->GetName(), resultDose );

    // Set dose unit value to Gy if dose engine did not set it already (potentially to other unit)
    vtkMRMLSubjectHierarchyNode* studyNode = beamShNode->GetAncestorAtLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    if (!studyNode)
    {
      qWarning() << Q_FUNC_INFO << ": Unable to find study node that contains the plan! Creating a study node and adding the reference dose and the plan under it is necessary in order for dose evaluation steps to work properly";
    }
    else if (!studyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str()))
    {
      studyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), "Gy");
    }
  }

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
      qCritical() << Q_FUNC_INFO << ": Failed to get default dose color table!";
    }

    vtkMRMLRTPlanNode* planNode = beamNode->GetParentPlanNode();
    if (planNode)
    {
      // Set window level based on prescription dose
      double rxDose = planNode->GetRxDose();
      doseScalarVolumeDisplayNode->AutoWindowLevelOff();
      doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, rxDose*1.1);

      // Set threshold to hide very low dose values
      doseScalarVolumeDisplayNode->SetLowerThreshold(0.05 * rxDose);
      doseScalarVolumeDisplayNode->ApplyThresholdOn();
    }
    else
    {
      doseScalarVolumeDisplayNode->AutoWindowLevelOff();
      doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, qSlicerAbstractDoseEngine::DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM);
    }
  }
  else
  {
    qWarning() << Q_FUNC_INFO << ": Display node is not available for dose volume node. The default color table will be used.";
  }
}

//---------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* qSlicerAbstractDoseEngine::getResultDoseForBeam(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    return NULL;
  }

  // Get last node reference if there are more than one, so that the dose calculated last is returned
  return vtkMRMLScalarVolumeNode::SafeDownCast(
    beamNode->GetNthNodeReference(RESULT_DOSE_REFERENCE_ROLE, beamNode->GetNumberOfNodeReferences(RESULT_DOSE_REFERENCE_ROLE)-1) );
}

//---------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::removeIntermediateResults(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    return;
  }

  vtkMRMLScene* scene = beamNode->GetScene();
  std::vector<const char*> referencedNodeIds;
  beamNode->GetNodeReferenceIDs(INTERMEDIATE_RESULT_REFERENCE_ROLE, referencedNodeIds);
  for (std::vector<const char*>::iterator refIt=referencedNodeIds.begin(); refIt != referencedNodeIds.end(); ++refIt)
  {
    vtkMRMLNode* node = scene->GetNodeByID(*refIt);
    scene->RemoveNode(node);
  }
}

//-----------------------------------------------------------------------------
QString qSlicerAbstractDoseEngine::parameter(vtkMRMLRTBeamNode* beamNode, QString name)
{
  if (!beamNode)
  {
    return QString();
  }

  // Get effect-specific prefixed parameter first
  QString attributeName = QString("%1.%2").arg(this->name()).arg(name);
  const char* value = beamNode->GetAttribute(attributeName.toLatin1().constData());
  // Look for common parameter if effect-specific one is not found
  if (!value)
  {
    value = beamNode->GetAttribute(name.toLatin1().constData());
  }
  if (!value)
  {
    qCritical() << Q_FUNC_INFO << ": Parameter named " << name << " cannot be found for beam " << beamNode->GetName();
    return QString();
  }

  return QString(value);
}

//-----------------------------------------------------------------------------
int qSlicerAbstractDoseEngine::integerParameter(vtkMRMLRTBeamNode* beamNode, QString name)
{
  if (!beamNode)
  {
    return 0;
  }

  QString parameterStr = this->parameter(beamNode, name);
  bool ok = false;
  int parameterInt = parameterStr.toInt(&ok);
  if (!ok)
  {
    qCritical() << Q_FUNC_INFO << ": Parameter named " << name << " cannot be converted to integer!";
    return 0;
  }

  return parameterInt;
}

//-----------------------------------------------------------------------------
double qSlicerAbstractDoseEngine::doubleParameter(vtkMRMLRTBeamNode* beamNode, QString name)
{
  if (!beamNode)
  {
    return 0.0;
  }

  QString parameterStr = this->parameter(beamNode, name);
  bool ok = false;
  double parameterDouble = parameterStr.toDouble(&ok);
  if (!ok)
  {
    qCritical() << Q_FUNC_INFO << ": Parameter named " << name << " cannot be converted to floating point number!";
    return 0.0;
  }

  return parameterDouble;
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setParameter(vtkMRMLRTBeamNode* beamNode, QString name, QString value)
{
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node!";
    return;
  }

  const char* oldValue = beamNode->GetAttribute(name.toLatin1().constData());
  if (oldValue == NULL && value.isEmpty())
    {
    // no change
    return;
    }
  if (value == QString(oldValue))
    {
    // no change
    return;
    }

  //TODO: Decide whether modified events are needed and handle custom modified events if necessary
  //      (e.g. could create setGeometryParameter or add bool arguments to specify special parameter type)
  // Disable full modified events in all cases (observe EffectParameterModified instead if necessary)
  //int disableState = beamNode->GetDisableModifiedEvent();
  //beamNode->SetDisableModifiedEvent(1);

  // Set parameter as attribute
  beamNode->SetAttribute(name.toLatin1().constData(), value.toLatin1().constData());

  // Re-enable full modified events for parameter node
  //beamNode->SetDisableModifiedEvent(disableState);

  // Emit parameter modified event if requested
  // Don't pass parameter name as char pointer, as custom modified events may be compressed and invoked after EndModify()
  // and by that time the pointer may not be valid anymore.
  //beamNode->InvokeCustomModifiedEvent(vtkMRMLSegmentEditorNode::EffectParameterModified);
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setParameter(vtkMRMLRTBeamNode* beamNode, QString name, int value)
{
  this->setParameter(beamNode, name, QString::number(value));
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setParameter(vtkMRMLRTBeamNode* beamNode, QString name, double value)
{
  this->setParameter(beamNode, name, QString::number(value));
}
