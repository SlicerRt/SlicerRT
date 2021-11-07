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

// DoseVolumeHistogram includes
#include "vtkMRMLDoseVolumeHistogramNode.h"
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLPlotChartNode.h>
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
qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate::~qSlicerSubjectHierarchyDoseVolumeHistogramPluginPrivate() = default;

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
qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::~qSlicerSubjectHierarchyDoseVolumeHistogramPlugin() = default;

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
  if ( associatedNode && associatedNode->IsA("vtkMRMLPlotSeriesNode")
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
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Plots")->setDisplayVisibility(itemID, visible);
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
    return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Plots")->getDisplayVisibility(itemID);
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

    // Get DVH parameter set node containing the current DVH table
    vtkMRMLTableNode* dvhTableNode = vtkMRMLTableNode::SafeDownCast(shNode->GetItemDataNode(itemID));
    if (!dvhTableNode)
    {
      qCritical() << Q_FUNC_INFO << ": Unable to get DVH table node";
      return;
    }

    // Choose parameter set node
    vtkMRMLDoseVolumeHistogramNode* parameterSetNode = this->getDvhParameterSetNodeForDvhTable(dvhTableNode);
    if (nodeSelector)
    {
      nodeSelector->setCurrentNode(parameterSetNode);
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLDoseVolumeHistogramNode* qSlicerSubjectHierarchyDoseVolumeHistogramPlugin::getDvhParameterSetNodeForDvhTable(vtkMRMLNode* dvhTableNode)const
{
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
    return nullptr;
  }

  vtkMRMLTableNode* metricsTableNode = vtkMRMLTableNode::SafeDownCast(
    dvhTableNode->GetNodeReference(vtkMRMLDoseVolumeHistogramNode::DVH_METRICS_TABLE_REFERENCE_ROLE) );

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

  return nullptr;
}
