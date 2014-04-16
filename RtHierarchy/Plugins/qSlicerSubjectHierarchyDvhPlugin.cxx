/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// SlicerRt includes
#include "SlicerRtCommon.h"

// RTHierarchy Plugins includes
#include "qSlicerSubjectHierarchyDvhPlugin.h"

// SubjectHierarchy MRML includes
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"
#include "qSlicerSubjectHierarchyChartsPlugin.h"

// DoseVolumeHistogram includes
#include "vtkMRMLDoseVolumeHistogramNode.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLDoubleArrayNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QIcon>
#include <QStandardItem>

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// MRML widgets includes
#include "qMRMLNodeComboBox.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerSubjectHierarchyDvhPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyDvhPlugin);
protected:
  qSlicerSubjectHierarchyDvhPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyDvhPluginPrivate(qSlicerSubjectHierarchyDvhPlugin& object);
  ~qSlicerSubjectHierarchyDvhPluginPrivate();
public:
  QIcon DvhIcon;

  QIcon VisibleIcon;
  QIcon HiddenIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyDvhPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDvhPluginPrivate::qSlicerSubjectHierarchyDvhPluginPrivate(qSlicerSubjectHierarchyDvhPlugin& object)
 : q_ptr(&object)
{
  this->DvhIcon = QIcon(":Icons/DVH.png");

  this->VisibleIcon = QIcon(":Icons/VisibleOn.png");
  this->HiddenIcon = QIcon(":Icons/VisibleOff.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDvhPluginPrivate::~qSlicerSubjectHierarchyDvhPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDvhPlugin::qSlicerSubjectHierarchyDvhPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyDvhPluginPrivate(*this) )
{
  this->m_Name = QString("DVH");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDvhPlugin::~qSlicerSubjectHierarchyDvhPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyDvhPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // DVH
  if ( associatedNode && associatedNode->IsA("vtkMRMLDoubleArrayNode")
    && associatedNode->GetAttribute(SlicerRtCommon::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    return 1.0; // Only this plugin can handle this node
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyDvhPlugin::roleForPlugin()const
{
  return "Dose volume histogram";
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyDvhPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyDvhPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    item->setIcon(d->DvhIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDvhPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::setVisibilityIcon: NULL node or item given!";
    return;
  }

  Q_D(qSlicerSubjectHierarchyDvhPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    if (this->getDisplayVisibility(node))
    {
      item->setIcon(d->VisibleIcon);
    }
    else
    {
      item->setIcon(d->HiddenIcon);
    }
  }
  else
  {
    // For all other owned nodes the visibility icon is set as default
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setVisibilityIcon(node, item);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDvhPlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::setDisplayVisibility: NULL node!";
    return;
  }

  if (this->canOwnSubjectHierarchyNode(node))
  {
    // Get parameter set node for DVH, then chart from the parameter set node
    //TODO:
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(node, visible);
  }
}

//---------------------------------------------------------------------------
int qSlicerSubjectHierarchyDvhPlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::getDisplayVisibility: NULL node!";
    return -1;
  }

  if (this->canOwnSubjectHierarchyNode(node))
  {
    return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->getDisplayVisibility(node);
  }

  // Default
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->getDisplayVisibility(node);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDvhPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  qSlicerAbstractModuleWidget* moduleWidget = qSlicerSubjectHierarchyAbstractPlugin::switchToModule("DoseVolumeHistogram");
  if (moduleWidget)
  {
    // Get node selector combobox
    qMRMLNodeComboBox* nodeSelector = moduleWidget->findChild<qMRMLNodeComboBox*>("MRMLNodeComboBox_ParameterSet");

    // Get DVH parameter set node containing the current DVH array
    vtkMRMLDoseVolumeHistogramNode* parameterSetNode = this->GetDvhParameterSetNodeForDvhArray(
      vtkMRMLDoubleArrayNode::SafeDownCast(node->GetAssociatedDataNode()) );

    // Choose current data node
    if (nodeSelector)
    {
      nodeSelector->setCurrentNode(parameterSetNode);
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLDoseVolumeHistogramNode* qSlicerSubjectHierarchyDvhPlugin::GetDvhParameterSetNodeForDvhArray(vtkMRMLDoubleArrayNode* dvhArrayNode)
{
  //TODO:
  return NULL;
}
