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

// SlicerRt includes
#include "vtkSlicerRtCommon.h"
#include "vtkMRMLPlanarImageNode.h"

// RTHierarchy Plugins includes
#include "qSlicerSubjectHierarchyRtImagePlugin.h"

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

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
qSlicerSubjectHierarchyRtImagePluginPrivate::~qSlicerSubjectHierarchyRtImagePluginPrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRtImagePlugin methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtImagePlugin::qSlicerSubjectHierarchyRtImagePlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRtImagePluginPrivate(*this) )
{
  this->m_Name = QString("RtImage");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtImagePlugin::~qSlicerSubjectHierarchyRtImagePlugin() = default;

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRtImagePlugin::canOwnSubjectHierarchyItem(vtkIdType itemID)const
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

  // RT Image
  vtkMRMLNode* associatedNode = shNode->GetItemDataNode(itemID);
  if ( associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode")
    && !shNode->GetItemAttribute(itemID, vtkSlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME).empty() )
  {
    return 1.0; // Only this plugin can handle this node
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyRtImagePlugin::roleForPlugin()const
{
  return "RT image";
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRtImagePlugin::icon(vtkIdType itemID)
{
  Q_D(qSlicerSubjectHierarchyRtImagePlugin);

  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return QIcon();
  }

  if (this->canOwnSubjectHierarchyItem(itemID))
  {
    return d->PlanarImageIcon;
  }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRtImagePlugin::visibilityIcon(int visible)
{
  Q_D(qSlicerSubjectHierarchyRtImagePlugin);

  // RT image (show regular eye icon (because it can be shown and hidden)
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
void qSlicerSubjectHierarchyRtImagePlugin::setDisplayVisibility(vtkIdType itemID, int visible)
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

  vtkMRMLScalarVolumeNode* associatedVolumeNode =
    vtkMRMLScalarVolumeNode::SafeDownCast(shNode->GetItemDataNode(itemID) );
  if (associatedVolumeNode)
  {
    // RT Image: show/hide is available. Not propagated to possible children
    if (this->canOwnSubjectHierarchyItem(itemID))
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        associatedVolumeNode->GetNodeReference(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
      if (!modelNode)
      {
        qCritical() << Q_FUNC_INFO << ": No displayed model found for planar image '" << associatedVolumeNode->GetName() << "'";
        return;
      }
      modelNode->SetDisplayVisibility(visible);
      shNode->ItemModified(itemID); // Triggers icon refresh in subject hierarchy tree
    }
    // Default
    else
    {
      qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(itemID, visible);
    }
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(itemID, visible);
  }
}

//---------------------------------------------------------------------------
int qSlicerSubjectHierarchyRtImagePlugin::getDisplayVisibility(vtkIdType itemID)const
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

  vtkMRMLScalarVolumeNode* associatedVolumeNode =
    vtkMRMLScalarVolumeNode::SafeDownCast(shNode->GetItemDataNode(itemID) );
  if (associatedVolumeNode)
  {
    // RT Image: show/hide is available. Not propagated to possible children
    if (this->canOwnSubjectHierarchyItem(itemID))
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        associatedVolumeNode->GetNodeReference(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
      if (!modelNode)
      {
        qCritical() << Q_FUNC_INFO << ": No displayed model found for planar image '" << associatedVolumeNode->GetName() << "'!";
        return -1;
      }
      return modelNode->GetDisplayVisibility();
    }
  }

  // Default
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->getDisplayVisibility(itemID);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtImagePlugin::editProperties(vtkIdType itemID)
{
  Q_UNUSED(itemID);
  //TODO: Switch to external beam planning module when it supports RT images
}
