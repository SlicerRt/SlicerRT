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
#include "qSlicerSubjectHierarchyRtDoseVolumePlugin.h"

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"
#include "qSlicerSubjectHierarchyVolumesPlugin.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QIcon>
#include <QStandardItem>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerSubjectHierarchyRtDoseVolumePluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyRtDoseVolumePlugin);
protected:
  qSlicerSubjectHierarchyRtDoseVolumePlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyRtDoseVolumePluginPrivate(qSlicerSubjectHierarchyRtDoseVolumePlugin& object);
  ~qSlicerSubjectHierarchyRtDoseVolumePluginPrivate();
public:
  QIcon DoseVolumeIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRtDoseVolumePluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePluginPrivate::qSlicerSubjectHierarchyRtDoseVolumePluginPrivate(qSlicerSubjectHierarchyRtDoseVolumePlugin& object)
 : q_ptr(&object)
{
  this->DoseVolumeIcon = QIcon(":Icons/DoseVolume.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePluginPrivate::~qSlicerSubjectHierarchyRtDoseVolumePluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePlugin::qSlicerSubjectHierarchyRtDoseVolumePlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRtDoseVolumePluginPrivate(*this) )
{
  this->m_Name = QString("RtDoseVolume");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePlugin::~qSlicerSubjectHierarchyRtDoseVolumePlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRtDoseVolumePlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // RT Dose
  if ( node->IsLevel(vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && associatedNode && SlicerRtCommon::IsDoseVolumeNode(associatedNode) )
  {
    return 1.0; // Only this plugin can handle this node
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyRtDoseVolumePlugin::roleForPlugin()const
{
  return "RT dose volume";
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyRtDoseVolumePlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyRtDoseVolumePlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    item->setIcon(d->DoseVolumeIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::setVisibilityIcon: NULL node or item given!";
    return;
  }

  if (this->canOwnSubjectHierarchyNode(node))
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->setVisibilityIcon(node, item);
  }
  else
  {
    // For all other owned nodes the visibility icon is set as default
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setVisibilityIcon(node, item);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::setDisplayVisibility: NULL node!";
    return;
  }

  if (this->canOwnSubjectHierarchyNode(node))
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->setDisplayVisibility(node, visible);
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(node, visible);
  }
}

//---------------------------------------------------------------------------
int qSlicerSubjectHierarchyRtDoseVolumePlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::getDisplayVisibility: NULL node!";
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
void qSlicerSubjectHierarchyRtDoseVolumePlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->editProperties(node);
}
