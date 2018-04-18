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

// RTHierarchy Plugins includes
#include "qSlicerSubjectHierarchyDoseVolumeHistogramPlugin.h"

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"
#include "qSlicerSubjectHierarchyChartsPlugin.h"

// DoseVolumeHistogram includes
#include "vtkMRMLDoseVolumeHistogramNode.h"
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLTableNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QIcon>
#include <QStandardItem>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerAbstractModule.h"
#include "qSlicerModuleManager.h"
#include "qSlicerAbstractModuleWidget.h"

// MRML widgets includes
#include "qMRMLNodeComboBox.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyDoseVolumeHistogramPlugin);
protected:
  qSlicerSubjectHierarchyDoseVolumeHistogramPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate(qSlicerSubjectHierarchyDoseVolumeHistogramPlugin& object);
  ~qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate();
public:
  QIcon DvhIcon;

  QIcon VisibleIcon;
  QIcon HiddenIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate::qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate(qSlicerSubjectHierarchyDoseVolumeHistogramPlugin& object)
 : q_ptr(&object)
{
  this->DvhIcon = QIcon(":Icons/DVH.png");

  this->VisibleIcon = QIcon(":Icons/VisibleOn.png");
  this->HiddenIcon = QIcon(":Icons/VisibleOff.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate::~qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::qSlicerSubjectHierarchyDoseVolumeHistogramPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate(*this) )
{
  this->m_Name = QString("DVH");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::~qSlicerSubjectHierarchyDoseVolumeHistogramPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::canOwnSubjectHierarchyItem(vtkIdType itemID)const
{
  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return 0.0;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return 0.0;
  }

  // DVH
  vtkMRMLNode* associatedNode = shNode->GetItemDataNode(itemID);
  if ( associatedNode && associatedNode->IsA("vtkMRMLDoubleArrayNode")
    && associatedNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    return 1.0; // Only this plugin can handle this node
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::roleForPlugin()const
{
  return "Dose volume histogram";
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::icon(vtkIdType itemID)
{
  Q_D(qSlicerSubjectHierarchyDoseVolumeHistogramPlugin);

  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return QIcon();
  }

  if (this->canOwnSubjectHierarchyItem(itemID))
  {
    return d->DvhIcon;
  }

  // Item unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::visibilityIcon(int visible)
{
  Q_D(qSlicerSubjectHierarchyDoseVolumeHistogramPlugin);

  if (visible)
  {
    return d->VisibleIcon;
  }
  else
  {
    return d->HiddenIcon;
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::setDisplayVisibility(vtkIdType itemID, int visible)
{
  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  if (this->canOwnSubjectHierarchyItem(itemID))
  {
    // Get chart for DVH array item
    vtkIdType chartShItemID = this->getChartShItemForDvhArray(itemID);
    if (!chartShItemID)
    {
      qCritical() << Q_FUNC_INFO << ": Unable to get chart item for DVH array item";
      return;
    }
    vtkMRMLChartNode* chartNode = vtkMRMLChartNode::SafeDownCast(shNode->GetItemDataNode(chartShItemID));

    // Get chart visibility
    int chartVisible = qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Charts")->getDisplayVisibility(chartShItemID);

    // Get DVH logic for DVH visibility setting
    qSlicerAbstractCoreModule* dvhModule = qSlicerApplication::application()->moduleManager()->module("DoseVolumeHistogram");
    vtkSlicerDoseVolumeHistogramModuleLogic* dvhLogic = vtkSlicerDoseVolumeHistogramModuleLogic::SafeDownCast(dvhModule->logic());

    // Show/hide the DVH plot
    vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(shNode->GetItemDataNode(itemID));
    if (visible)
    {
      if (!chartVisible)
      {
        // Show the chart itself
        qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Charts")->setDisplayVisibility(chartShItemID, visible);
      }

      dvhLogic->AddDvhToChart(chartNode, dvhArrayNode);
    }
    else if (chartVisible)
    {
      dvhLogic->RemoveDvhFromChart(chartNode, dvhArrayNode);
    }

    // Trigger icon update
    shNode->ItemModified(itemID);
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(itemID, visible);
  }
}

//---------------------------------------------------------------------------
int qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::getDisplayVisibility(vtkIdType itemID)const
{
  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return -1;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return -1;
  }

  if (this->canOwnSubjectHierarchyItem(itemID))
  {
    // Get chart for DVH array item
    vtkIdType chartShItemID = this->getChartShItemForDvhArray(itemID);
    if (!chartShItemID)
    {
      qCritical() << Q_FUNC_INFO << ": Unable to get chart item for DVH array item";
      return -1;
    }
    vtkMRMLChartNode* chartNode = vtkMRMLChartNode::SafeDownCast(shNode->GetItemDataNode(chartShItemID));

    // Get chart visibility
    int chartVisible = qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Charts")->getDisplayVisibility(chartShItemID);

    // Get DVH logic for DVH visibility
    qSlicerAbstractCoreModule* dvhModule = qSlicerApplication::application()->moduleManager()->module("DoseVolumeHistogram");
    vtkSlicerDoseVolumeHistogramModuleLogic* dvhLogic = vtkSlicerDoseVolumeHistogramModuleLogic::SafeDownCast(dvhModule->logic());

    // Only return true if the chart is visible and the DVH array is added in the chart
    vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(shNode->GetItemDataNode(itemID));
    return (chartVisible && dvhLogic->IsDvhAddedToChart(chartNode, dvhArrayNode));
  }

  // Default
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->getDisplayVisibility(itemID);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::editProperties(vtkIdType itemID)
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  qSlicerAbstractModuleWidget* moduleWidget = qSlicerSubjectHierarchyAbstractPlugin::switchToModule("DoseVolumeHistogram");
  if (moduleWidget)
  {
    // Get node selector combobox
    qMRMLNodeComboBox* nodeSelector = moduleWidget->findChild<qMRMLNodeComboBox*>("MRMLNodeComboBox_ParameterSet");

    // Get DVH parameter set node containing the current DVH array
    vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(shNode->GetItemDataNode(itemID));
    if (!dvhArrayNode)
    {
      qCritical() << Q_FUNC_INFO << ": Unable to get DVH array node";
      return;
    }

    // Choose parameter set node
    vtkMRMLDoseVolumeHistogramNode* parameterSetNode = this->getDvhParameterSetNodeForDvhArray(dvhArrayNode);
    if (nodeSelector)
    {
      nodeSelector->setCurrentNode(parameterSetNode);
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLDoseVolumeHistogramNode* qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::getDvhParameterSetNodeForDvhArray(vtkMRMLNode* dvhArrayNode)const
{
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
    return NULL;
  }

  vtkMRMLTableNode* metricsTableNode = vtkMRMLTableNode::SafeDownCast(
    dvhArrayNode->GetNodeReference(vtkMRMLDoseVolumeHistogramNode::DVH_METRICS_TABLE_REFERENCE_ROLE) );

  std::vector<vtkMRMLNode*> nodes;
  scene->GetNodesByClass("vtkMRMLDoseVolumeHistogramNode", nodes);
  for (std::vector<vtkMRMLNode*>::iterator nodeIt=nodes.begin(); nodeIt!=nodes.end(); ++nodeIt)
  {
    vtkMRMLDoseVolumeHistogramNode* dvhNode = vtkMRMLDoseVolumeHistogramNode::SafeDownCast(*nodeIt);
    if (dvhNode && dvhNode->GetMetricsTableNode() == metricsTableNode)
    {
      return dvhNode;
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
vtkIdType qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::getChartShItemForDvhArray(vtkIdType dvhArrayShItemID)const
{
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
    return 0;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return 0;
  }

  // Get parameter set node for DVH, then chart from the parameter set node
  vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(
    shNode->GetItemDataNode(dvhArrayShItemID) );
  if (!dvhArrayNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to get DVH array node";
    return 0;
  }

  vtkMRMLDoseVolumeHistogramNode* parameterSetNode = this->getDvhParameterSetNodeForDvhArray(dvhArrayNode);
  vtkMRMLChartNode* chartNode = parameterSetNode->GetChartNode();
  if (!chartNode)
  {
    qCritical() << Q_FUNC_INFO << ": Chart node must exist for DVH parameter set node";
    return 0;
  }

  // Get chart subject hierarchy item (at the same time make sure the chart is in the proper branch)
  vtkIdType seriesItemID = shNode->GetItemParent(dvhArrayShItemID);
  return shNode->CreateItem(seriesItemID, chartNode);
}
