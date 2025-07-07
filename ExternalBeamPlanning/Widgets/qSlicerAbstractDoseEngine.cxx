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
#include "qMRMLBeamParametersTabWidget.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"
#include "vtkSlicerIsodoseModuleLogic.h"
#include "qSlicerDoseEnginePluginHandler.h"

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
#include <QTabWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>

//----------------------------------------------------------------------------
double qSlicerAbstractDoseEngine::DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM = 16.0;

QSet<qMRMLBeamParametersTabWidget*> qSlicerAbstractDoseEngine::m_BeamParametersTabWidgets = QSet<qMRMLBeamParametersTabWidget*>();

//----------------------------------------------------------------------------
static const char* INTERMEDIATE_RESULT_REFERENCE_ROLE = "IntermediateResultRef";
static const char* RESULT_DOSE_REFERENCE_ROLE = "ResultDoseRef";

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerAbstractDoseEnginePrivate
{
  Q_DECLARE_PUBLIC(qSlicerAbstractDoseEngine);
protected:
  qSlicerAbstractDoseEngine* const q_ptr;
public:
  qSlicerAbstractDoseEnginePrivate(qSlicerAbstractDoseEngine& object);
public:
  /// Engine-specific parameters defined in \sa defineBeamParameters.
  /// Key is the parameter name (without engine name prefix), value is the default
  QMap<QString,QVariant> BeamParameters;
};

//-----------------------------------------------------------------------------
// qSlicerAbstractDoseEnginePrivate methods

//-----------------------------------------------------------------------------
qSlicerAbstractDoseEnginePrivate::qSlicerAbstractDoseEnginePrivate(qSlicerAbstractDoseEngine& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
// qSlicerAbstractDoseEngine methods

//----------------------------------------------------------------------------
qSlicerAbstractDoseEngine::qSlicerAbstractDoseEngine(QObject* parent)
  : Superclass(parent)
  , m_Name(QString())
  , d_ptr( new qSlicerAbstractDoseEnginePrivate(*this) )
{
}

//----------------------------------------------------------------------------
qSlicerAbstractDoseEngine::~qSlicerAbstractDoseEngine() = default;

//----------------------------------------------------------------------------
QString qSlicerAbstractDoseEngine::name() const
{
  if (m_Name.isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": Empty dose engine name";
  }
  return this->m_Name;
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setName(QString name)
{
  Q_UNUSED(name);
  qCritical() << Q_FUNC_INFO << ": Cannot set dose engine name by method, only in constructor";
}

//----------------------------------------------------------------------------
bool qSlicerAbstractDoseEngine::isInverse() const
{
    return this->m_IsInverse;
}

void qSlicerAbstractDoseEngine::setIsInverse(bool isInverse)
{
    // TODO: is this correct to avoid setting the value except for Python engines?
    qCritical() << Q_FUNC_INFO << ": Cannot set dose engine name by method, only in constructor";
}

//----------------------------------------------------------------------------
bool qSlicerAbstractDoseEngine::canDoIonPlan() const
{
	return this->m_CanDoIonPlan;
}

void qSlicerAbstractDoseEngine::setCanDoIonPlan(bool canDoIonPlan)
{
	qCritical() << Q_FUNC_INFO << ": Cannot set dose engine name by method, only in constructor";
}


//----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::registerBeamParametersTabWidget(qMRMLBeamParametersTabWidget* tabWidget)
{
  qSlicerAbstractDoseEngine::m_BeamParametersTabWidgets.insert(tabWidget);

  // Make sure beam parameters specified by engines are defined in the tab widget
  qSlicerDoseEnginePluginHandler::DoseEngineListType engines =
    qSlicerDoseEnginePluginHandler::instance()->registeredDoseEngines();
  for (qSlicerDoseEnginePluginHandler::DoseEngineListType::iterator engineIt = engines.begin(); engineIt != engines.end(); ++engineIt)
  {
    (*engineIt)->defineBeamParameters();
  }
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
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(beamNode->GetScene());
  if (!shNode)
  {
    QString errorMessage("Failed to access subject hierarchy node");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  vtkIdType referenceVolumeShItemID = shNode->GetItemByDataNode(referenceVolumeNode);
  if (referenceVolumeShItemID)
  {
    vtkIdType planShItemID = parentPlanNode->GetPlanSubjectHierarchyItemID();
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

//----------------------------------------------------------------------------
QString qSlicerAbstractDoseEngine::calculateDoseInfluenceMatrix(vtkMRMLRTBeamNode* beamNode)
{
    if (!this->isInverse())
    {
		QString errorMessage("Dose engine lacks functionality for calculating a dose influence matrix for inverse planning");
		qCritical() << Q_FUNC_INFO << ": " << errorMessage;
		return errorMessage;
    }
    
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
    vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(beamNode->GetScene());
    if (!shNode)
    {
        QString errorMessage("Failed to access subject hierarchy node");
        qCritical() << Q_FUNC_INFO << ": " << errorMessage;
        return errorMessage;
    }
    vtkIdType referenceVolumeShItemID = shNode->GetItemByDataNode(referenceVolumeNode);
    if (referenceVolumeShItemID)
    {
        vtkIdType planShItemID = parentPlanNode->GetPlanSubjectHierarchyItemID();
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
    
    // Remove past intermediate results for beam before calculating dose again
    this->removeIntermediateResults(beamNode);

    // Calculate dose
    QString errorMessage = this->calculateDoseInfluenceMatrixUsingEngine(beamNode);

    return errorMessage;
}

//---------------------------------------------------------------------------
QString qSlicerAbstractDoseEngine::calculateDoseInfluenceMatrixUsingEngine(vtkMRMLRTBeamNode* beamNode)
{
	qCritical() << Q_FUNC_INFO << ": Inverse dose calculation not implemented";
    return QString();
}
//---------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::addIntermediateResult(vtkMRMLNode* result, vtkMRMLRTBeamNode* beamNode)
{
  if (!result)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid intermediate result";
    return;
  }
  if (!beamNode || !beamNode->GetScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(beamNode->GetScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  // Add reference in beam to result for later access
  beamNode->AddNodeReferenceID(INTERMEDIATE_RESULT_REFERENCE_ROLE, result->GetID());

  // Add result under beam in subject hierarchy
  vtkIdType beamShItemID = shNode->GetItemByDataNode(beamNode);
  if (beamShItemID)
  {
    shNode->CreateItem(beamShItemID, result);
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
  vtkMRMLScene* scene = beamNode->GetScene();
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

  // Remove already existing referenced dose volume if any
  if (replace)
  {
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
  resultDose->SetAttribute(vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Subject hierarchy related operations
  vtkIdType beamShItemID = shNode->GetItemByDataNode(beamNode);
  if (beamShItemID)
  {
    // Add result under beam in subject hierarchy
    shNode->CreateItem(beamShItemID, resultDose);

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
    return nullptr;
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

//---------------------------------------------------------------------------
// Beam parameter definition functions.
// Need to be called from the implemented \sa defineBeamParameters method.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::addBeamParameterSpinBox(
  QString tabName, QString parameterName, QString parameterLabel,
  QString tooltip, double minimumValue, double maximumValue,
  double defaultValue, double stepSize, int precision )
{
  Q_D(qSlicerAbstractDoseEngine);

  // Add parameter to container
  d->BeamParameters[parameterName] = QVariant(defaultValue);

  for (auto beamParametersTabWidget : m_BeamParametersTabWidgets)
  {
    // Add beam parameter to tab widget
    beamParametersTabWidget->addBeamParameterFloatingPointNumber(
      tabName, this->assembleEngineParameterName(parameterName), parameterLabel,
      tooltip, minimumValue, maximumValue, defaultValue, stepSize, precision, false );
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::addBeamParameterSlider(
  QString tabName, QString parameterName, QString parameterLabel,
  QString tooltip, double minimumValue, double maximumValue,
  double defaultValue, double stepSize, int precision )
{
  Q_D(qSlicerAbstractDoseEngine);

  // Add parameter to container
  d->BeamParameters[parameterName] = QVariant(defaultValue);

  for (auto beamParametersTabWidget : m_BeamParametersTabWidgets)
  {
    // Add beam parameter to tab widget
    beamParametersTabWidget->addBeamParameterFloatingPointNumber(
      tabName, this->assembleEngineParameterName(parameterName), parameterLabel,
      tooltip, minimumValue, maximumValue, defaultValue, stepSize, precision, true );
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::addBeamParameterComboBox(
  QString tabName, QString parameterName, QString parameterLabel,
  QString tooltip, QStringList options, int defaultIndex)
{
  Q_D(qSlicerAbstractDoseEngine);

  // Add parameter to container
  d->BeamParameters[parameterName] = QVariant(defaultIndex);

  for (auto beamParametersTabWidget : m_BeamParametersTabWidgets)
  {
    // Add beam parameter to tab widget
    beamParametersTabWidget->addBeamParameterComboBox(
      tabName, this->assembleEngineParameterName(parameterName), parameterLabel,
      tooltip, options, defaultIndex );
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::updateBeamParameterComboBox(
    QString tabName, QString parameterName, QString parameterLabel,
    QString tooltip, QStringList options, int defaultIndex)
{
    Q_D(qSlicerAbstractDoseEngine);

    // Add parameter to container
    d->BeamParameters[parameterName] = QVariant(defaultIndex);

    for (auto beamParametersTabWidget : m_BeamParametersTabWidgets)
    {
        // Update beam parameter in tab widget
        beamParametersTabWidget->updateBeamParameterComboBox(
            tabName, this->assembleEngineParameterName(parameterName), parameterLabel,
            tooltip, options, defaultIndex);
    }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::addBeamParameterCheckBox(
  QString tabName, QString parameterName, QString parameterLabel,
  QString tooltip, bool defaultValue, QStringList dependentParameterNames/*=QStringList()*/)
{
  Q_D(qSlicerAbstractDoseEngine);

  // Add parameter to container
  d->BeamParameters[parameterName] = QVariant(defaultValue);

  // Assemble dependent parameter names to be prefixed with dose engine name
  QStringList assembledDependentParameterNames = QStringList();
  foreach (QString parameter, dependentParameterNames)
  {
    assembledDependentParameterNames << this->assembleEngineParameterName(parameter);
  }

  for (auto beamParametersTabWidget : m_BeamParametersTabWidgets)
  {
    // Add beam parameter to tab widget
    beamParametersTabWidget->addBeamParameterCheckBox(
      tabName, this->assembleEngineParameterName(parameterName), parameterLabel,
      tooltip, defaultValue, assembledDependentParameterNames );
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::addBeamParameterLineEdit(
  QString tabName, QString parameterName, QString parameterLabel,
  QString tooltip, QString defaultValue )
{
  Q_D(qSlicerAbstractDoseEngine);

  // Add parameter to container
  d->BeamParameters[parameterName] = QVariant(defaultValue);

  for (auto beamParametersTabWidget : m_BeamParametersTabWidgets)
  {
    // Add beam parameter to tab widget
    beamParametersTabWidget->addBeamParameterLineEdit(
      tabName, this->assembleEngineParameterName(parameterName), parameterLabel,
      tooltip, defaultValue );
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setBeamParametersVisible(bool visible)
{
  Q_D(qSlicerAbstractDoseEngine);

  for (auto beamParametersTabWidget : m_BeamParametersTabWidgets)
  {
    // Set visibility for all beam parameters specific to this engine
    foreach (QString parameterName, d->BeamParameters.keys())
    {
      bool success = beamParametersTabWidget->setBeamParameterVisible(
        this->assembleEngineParameterName(parameterName), visible );
      if (!success)
      {
        qCritical() << Q_FUNC_INFO << ": Failed to " << (visible?"show":"hide") << " beam parameter widget for parameter '" << parameterName << "'";
        return;
      }
    }

    // Update tab visibility in parameters widget
    beamParametersTabWidget->updateTabVisibility();
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::addBeamParameterAttributesToBeamNode(vtkMRMLRTBeamNode* beamNode)
{
  Q_D(qSlicerAbstractDoseEngine);

  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    return;
  }

  // Add each beam parameter if missing
  foreach (QString parameterName, d->BeamParameters.keys())
  {
    if ( !beamNode->GetAttribute(
      this->assembleEngineParameterName(parameterName).toUtf8().constData()) )
    {
      beamNode->SetAttribute(
        this->assembleEngineParameterName(parameterName).toUtf8().constData(),
        d->BeamParameters[parameterName].toString().toUtf8().constData() );
    }
  }
}

//---------------------------------------------------------------------------
// Beam parameter get/set functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
QString qSlicerAbstractDoseEngine::parameter(vtkMRMLRTBeamNode* beamNode, QString parameterName)
{
  if (!beamNode)
  {
    return QString();
  }

  // Get effect-specific prefixed parameter first
  QString attributeName = this->assembleEngineParameterName(parameterName);
  const char* value = beamNode->GetAttribute(attributeName.toUtf8().constData());
  if (!value)
  {
    qCritical() << Q_FUNC_INFO << ": Parameter named " << parameterName << " cannot be found for beam " << beamNode->GetName();
    return QString();
  }

  return QString(value);
}

//-----------------------------------------------------------------------------
int qSlicerAbstractDoseEngine::integerParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName)
{
  if (!beamNode)
  {
    return 0;
  }

  QString parameterStr = this->parameter(beamNode, parameterName);
  bool ok = false;
  int parameterInt = parameterStr.toInt(&ok);
  if (!ok)
  {
    qCritical() << Q_FUNC_INFO << ": Parameter named " << parameterName << " cannot be converted to integer";
    return 0;
  }

  return parameterInt;
}

//-----------------------------------------------------------------------------
double qSlicerAbstractDoseEngine::doubleParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName)
{
  if (!beamNode)
  {
    return 0.0;
  }

  QString parameterStr = this->parameter(beamNode, parameterName);
  bool ok = false;
  double parameterDouble = parameterStr.toDouble(&ok);
  if (!ok)
  {
    qCritical() << Q_FUNC_INFO << ": Parameter named " << parameterName << " cannot be converted to floating point number";
    return 0.0;
  }

  return parameterDouble;
}

//-----------------------------------------------------------------------------
bool qSlicerAbstractDoseEngine::booleanParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName)
{
  if (!beamNode)
  {
    return false;
  }

  QString parameterStr = this->parameter(beamNode, parameterName);
  if (parameterStr == QVariant(true).toString())
  {
    return true;
  }
  else if (parameterStr == QVariant(false).toString())
  {
    return false;
  }

  qCritical() << Q_FUNC_INFO << ": Parameter named " << parameterName << " contains invalid boolean value '" << parameterStr << "'";
  return false;
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName, QString parameterValue)
{
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node";
    return;
  }

  QString attributeName = this->assembleEngineParameterName(parameterName);
  const char* oldValue = beamNode->GetAttribute(attributeName.toUtf8().constData());
  if (oldValue == nullptr && parameterValue.isEmpty())
    {
    // no change
    return;
    }
  if (parameterValue == QString(oldValue))
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
  beamNode->SetAttribute(attributeName.toUtf8().constData(), parameterValue.toUtf8().constData());

  // Re-enable full modified events for parameter node
  //beamNode->SetDisableModifiedEvent(disableState);

  // Emit parameter modified event if requested
  // Don't pass parameter name as char pointer, as custom modified events may be compressed and invoked after EndModify()
  // and by that time the pointer may not be valid anymore.
  //beamNode->InvokeCustomModifiedEvent(vtkMRMLSegmentEditorNode::EffectParameterModified);
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName, int parameterValue)
{
  this->setParameter(beamNode, parameterName, QString::number(parameterValue));
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName, double parameterValue)
{
  this->setParameter(beamNode, parameterName, QString::number(parameterValue));
}

//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName, bool parameterValue)
{
  this->setParameter(beamNode, parameterName, QVariant(parameterValue).toString());
}

//-----------------------------------------------------------------------------
QString qSlicerAbstractDoseEngine::assembleEngineParameterName(QString parameterName)
{
  return QString("%1.%2").arg(this->m_Name).arg(parameterName);
}

//-----------------------------------------------------------------------------
qMRMLBeamParametersTabWidget* qSlicerAbstractDoseEngine::beamParametersTabWidgetFromBeamsModule()
{
  // Get tab widget from beams module widget
  qSlicerAbstractCoreModule* module = qSlicerApplication::application()->moduleManager()->module("Beams");
  if (!module)
  {
    return nullptr;
  }
  qSlicerAbstractModuleWidget* moduleWidget = dynamic_cast<qSlicerAbstractModuleWidget*>(module->widgetRepresentation());
  if (!moduleWidget)
  {
    return nullptr;
  }
  return moduleWidget->findChild<qMRMLBeamParametersTabWidget*>("BeamParametersTabWidget");
}

/*
//-----------------------------------------------------------------------------
void qSlicerAbstractDoseEngine::setDoseEngineTypeToBeam(vtkMRMLRTBeamNode* beamNode)
{
  //TODO: Needed?
  qCritical() << Q_FUNC_INFO << ": Not implemented";
}
*/

//-----------------------------------------------------------------------------
void  qSlicerAbstractDoseEngine::updateBeamParametersForIonPlan(bool isIonPlanActive)
{
    qCritical() << Q_FUNC_INFO << ": updateBeamParamtersForIonPlan not implemented";
}
