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
#include "qSlicerSubjectHierarchyRtImagePlugin.h"

// SubjectHierarchy MRML includes
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"
#include "qSlicerSubjectHierarchyVolumesPlugin.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLModelNode.h>
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
class qSlicerSubjectHierarchyRtImagePluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyRtImagePlugin);
protected:
  qSlicerSubjectHierarchyRtImagePlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyRtImagePluginPrivate(qSlicerSubjectHierarchyRtImagePlugin& object);
  ~qSlicerSubjectHierarchyRtImagePluginPrivate();
public:
  QIcon PlanarImageIcon;

  QIcon VisibleIcon;
  QIcon HiddenIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRtImagePluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtImagePluginPrivate::qSlicerSubjectHierarchyRtImagePluginPrivate(qSlicerSubjectHierarchyRtImagePlugin& object)
 : q_ptr(&object)
{
  this->PlanarImageIcon = QIcon(":Icons/PlanarImage.png");

  this->VisibleIcon = QIcon(":Icons/VisibleOn.png");
  this->HiddenIcon = QIcon(":Icons/VisibleOff.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtImagePluginPrivate::~qSlicerSubjectHierarchyRtImagePluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtImagePlugin::qSlicerSubjectHierarchyRtImagePlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRtImagePluginPrivate(*this) )
{
  this->m_Name = QString("RtImage");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtImagePlugin::~qSlicerSubjectHierarchyRtImagePlugin()
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerSubjectHierarchyRtImagePlugin::dependencies()const
{
  return QStringList() << "Volumes";
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRtImagePlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtImagePlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // RT Image
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode")
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    return 1.0; // Only this plugin can handle this node
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyRtImagePlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyRtImagePlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyRtImagePlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    item->setIcon(d->PlanarImageIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtImagePlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyRtImagePlugin::setVisibilityIcon: NULL node or item given!";
    return;
  }

  Q_D(qSlicerSubjectHierarchyRtImagePlugin);

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (!associatedNode)
  {
    qCritical() << "qSlicerSubjectHierarchyRtImagePlugin::setVisibilityIcon: Invalid associated node for subject hierarchy node " << node->GetName();
    return;
  }
  
  // RT image (show regular eye icon (because it can be shown and hidden)
  if (this->canOwnSubjectHierarchyNode(node))
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
      associatedNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
    if (!modelNode)
    {
      qCritical() << "qSlicerSubjectHierarchyRtImagePlugin::setVisibilityIcon: No displayed model found for planar image '" << associatedNode->GetName() << "'!";
      return;
    }
    if (modelNode->GetDisplayVisibility())
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
void qSlicerSubjectHierarchyRtImagePlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtImagePlugin::setDisplayVisibility: NULL node!";
    return;
  }

  vtkMRMLScalarVolumeNode* associatedVolumeNode =
    vtkMRMLScalarVolumeNode::SafeDownCast(node->GetAssociatedDataNode());
  if (associatedVolumeNode)
  {
    // RT Image: show/hide is available. Not propagated to possible children
    if (this->canOwnSubjectHierarchyNode(node))
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        associatedVolumeNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
      if (!modelNode)
      {
        qCritical() << "qSlicerSubjectHierarchyRtImagePlugin::setDisplayVisibility: No displayed model found for planar image '" << associatedVolumeNode->GetName() << "'!";
        return;
      }
      modelNode->SetDisplayVisibility(visible);
      node->Modified(); // Triggers icon refresh in subject hierarchy tree
    }
    // Default
    else
    {
      qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(node, visible);
    }
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(node, visible);
  }
}

//---------------------------------------------------------------------------
int qSlicerSubjectHierarchyRtImagePlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtImagePlugin::getDisplayVisibility: NULL node!";
    return -1;
  }

  vtkMRMLScalarVolumeNode* associatedVolumeNode =
    vtkMRMLScalarVolumeNode::SafeDownCast(node->GetAssociatedDataNode());
  if (associatedVolumeNode)
  {
    // RT Image: show/hide is available. Not propagated to possible children
    if (this->canOwnSubjectHierarchyNode(node))
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        associatedVolumeNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
      if (!modelNode)
      {
        qCritical() << "qSlicerSubjectHierarchyRtImagePlugin::setDisplayVisibility: No displayed model found for planar image '" << associatedVolumeNode->GetName() << "'!";
        return -1;
      }
      return modelNode->GetDisplayVisibility();
    }
  }

  // Default
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->getDisplayVisibility(node);
}
