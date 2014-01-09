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

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyVolumesPlugin.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// Slicer includes
#include "qSlicerCoreApplication.h"
#include "vtkSlicerApplicationLogic.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLSliceCompositeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QStandardItem>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class qSlicerSubjectHierarchyVolumesPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyVolumesPlugin);
protected:
  qSlicerSubjectHierarchyVolumesPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyVolumesPluginPrivate(qSlicerSubjectHierarchyVolumesPlugin& object);
  ~qSlicerSubjectHierarchyVolumesPluginPrivate();
public:
  QIcon LabelmapIcon;
  QIcon ShowInViewersIcon;
  QIcon VolumeIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyVolumesPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyVolumesPluginPrivate::qSlicerSubjectHierarchyVolumesPluginPrivate(qSlicerSubjectHierarchyVolumesPlugin& object)
: q_ptr(&object)
{
  this->LabelmapIcon = QIcon(":Icons/Labelmap.png");
  this->ShowInViewersIcon = QIcon(":Icons/ShowInViewers.png");
  this->VolumeIcon = QIcon(":Icons/Volume.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyVolumesPluginPrivate::~qSlicerSubjectHierarchyVolumesPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyVolumesPlugin::qSlicerSubjectHierarchyVolumesPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyVolumesPluginPrivate(*this) )
{
  this->m_Name = QString("Volumes");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyVolumesPlugin::~qSlicerSubjectHierarchyVolumesPlugin()
{
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyVolumesPlugin::canAddNodeToSubjectHierarchy(vtkMRMLNode* node, vtkMRMLSubjectHierarchyNode* parent/*=NULL*/)
{
  // The Volumes Subject Hierarchy plugin does not perform steps additional
  // to the default when adding nodes to the hierarchy from outside
  return 0.0;
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyVolumesPlugin::canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* node, vtkMRMLSubjectHierarchyNode* parent)
{
  // The Volumes Subject Hierarchy plugin does not perform steps additional
  // to the default when reparenting inside the hierarchy
  return 0.0;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyVolumesPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    return 0.5; // There are other plugins that can handle special volume nodes better, thus the relatively low value
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyVolumesPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  Q_D(qSlicerSubjectHierarchyVolumesPlugin);

  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::setIcon: NULL node or item given!";
    return false;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    const char* labelmapAttribute = associatedNode->GetAttribute("LabelMap");
    if (labelmapAttribute && strcmp(labelmapAttribute, "0"))
    {
      item->setIcon(d->LabelmapIcon);
    }
    else
    {
      item->setIcon(d->VolumeIcon);
    }
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::setVisibilityIcon: NULL node or item given!";
    return;
  }

  Q_D(qSlicerSubjectHierarchyVolumesPlugin);

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (!associatedNode || !associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::setVisibilityIcon: Invalid associated node for subject hierarchy node " << node->GetName();
    return;
  }

  item->setIcon(d->ShowInViewersIcon);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::setDisplayVisibility: NULL node!";
    return;
  }

  vtkMRMLScalarVolumeNode* associatedVolumeNode =
    vtkMRMLScalarVolumeNode::SafeDownCast(node->GetAssociatedDataNode());
  // Volume
  if (associatedVolumeNode)
  {
    this->showVolume(associatedVolumeNode);
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(node, visible);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::showVolume(vtkMRMLScalarVolumeNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::showVolume: NULL node!";
    return;
  }

  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::showVolume: Unable to get selection node to show volume node " << node->GetName();
    return;
  }

  vtkMRMLScalarVolumeNode* volumeNode = NULL;
  if ((volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node)) == NULL)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::showVolume: Note to show node is not a volume node: " << node->GetName();
    return;
  }

  vtkMRMLSliceCompositeNode *compositeNode = NULL;
  const int numberOfCompositeNodes = volumeNode->GetScene()->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");

  const char* labelmapAttribute = volumeNode->GetAttribute("LabelMap");
  if (labelmapAttribute && strcmp(labelmapAttribute, "0"))
  {
    selectionNode->SetActiveLabelVolumeID(volumeNode->GetID());
    qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
  }
  else
  {
    // Determine labelmap selection (if the selection is different in the slice viewers, then the first one is set)
    // TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
    std::string selectedLabelmapID("");
    for (int i=0; i<numberOfCompositeNodes; i++)
    {
      compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast ( volumeNode->GetScene()->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
      if (compositeNode && compositeNode->GetLabelVolumeID())
      {
        if (selectedLabelmapID.empty())
        {
          selectedLabelmapID = std::string(compositeNode->GetLabelVolumeID());
        }
      }
    }
    selectionNode->SetActiveLabelVolumeID(selectedLabelmapID.c_str());

    // Set input volume as background volume, set the original background to foreground with opacity of 0.5
    selectionNode->SetSecondaryVolumeID(selectionNode->GetActiveVolumeID());
    selectionNode->SetActiveVolumeID(volumeNode->GetID());
    qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();

    for (int i=0; i<numberOfCompositeNodes; i++)
    {
      compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast ( volumeNode->GetScene()->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
      if (compositeNode && compositeNode->GetForegroundOpacity() == 0.0)
      {
        compositeNode->SetForegroundOpacity(0.5);
      }
    }
  }
}
