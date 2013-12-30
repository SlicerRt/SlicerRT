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
#include "qSlicerSubjectHierarchyRTPlugin.h"

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
class qSlicerSubjectHierarchyRTPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyRTPlugin);
protected:
  qSlicerSubjectHierarchyRTPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyRTPluginPrivate(qSlicerSubjectHierarchyRTPlugin& object);
  ~qSlicerSubjectHierarchyRTPluginPrivate();
public:
  QIcon BeamIcon;
  QIcon DoseVolumeIcon;
  QIcon IsocenterIcon;
  QIcon IsodoseLinesIcon;
  QIcon PlanIcon;
  QIcon PlanarImageIcon;

  QIcon VisibleIcon;
  QIcon HiddenIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRTPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTPluginPrivate::qSlicerSubjectHierarchyRTPluginPrivate(qSlicerSubjectHierarchyRTPlugin& object)
 : q_ptr(&object)
{
  this->BeamIcon = QIcon(":Icons/Beam.png");
  this->DoseVolumeIcon = QIcon(":Icons/DoseVolume.png");
  this->IsocenterIcon = QIcon(":Icons/Isocenter.png");
  this->IsodoseLinesIcon = QIcon(":Icons/IsodoseLines.png");
  this->PlanIcon = QIcon(":Icons/Plan.png");
  this->PlanarImageIcon = QIcon(":Icons/PlanarImage.png");

  this->VisibleIcon = QIcon(":Icons/VisibleOn.png");
  this->HiddenIcon = QIcon(":Icons/VisibleOff.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTPluginPrivate::~qSlicerSubjectHierarchyRTPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTPlugin::qSlicerSubjectHierarchyRTPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRTPluginPrivate(*this) )
{
  this->m_Name = QString("RT");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTPlugin::~qSlicerSubjectHierarchyRTPlugin()
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerSubjectHierarchyRTPlugin::dependencies()const
{
  return QStringList() << "DICOM" << "Volumes";
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyRTPlugin::canAddNodeToSubjectHierarchy(vtkMRMLNode* node, vtkMRMLSubjectHierarchyNode* parent/*=NULL*/)
{
  // The RT Subject Hierarchy plugin does not perform steps additional
  // to the default when adding nodes to the hierarchy from outside
  return 0.0;
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyRTPlugin::canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* node, vtkMRMLSubjectHierarchyNode* parent)
{
  // The RT Subject Hierarchy plugin does not perform steps additional
  // to the default when reparenting inside the hierarchy
  return 0.0;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRTPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRTPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // RT Dose
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && associatedNode && SlicerRtCommon::IsDoseVolumeNode(associatedNode) )
  {
    return 1.0; // Only this plugin can handle this node
  }

  // RT Image
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode")
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    return 1.0; // Only this plugin can handle this node
  }

  // Isocenter for RT Plan
  QString parentHierarchyNodeName("");
  if (node->GetParentNode())
  {
    parentHierarchyNodeName = QString(node->GetParentNode()->GetName());
  }
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES)
    && associatedNode && associatedNode->IsA("vtkMRMLMarkupsFiducialNode")
    && parentHierarchyNodeName.contains(SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX.c_str()) )
  {
    return 1.0;
  }

  // Beam model for an RT Plan
  QString hierarchyNodeName(node->GetName());
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES)
    && associatedNode && associatedNode->IsA("vtkMRMLModelNode")
    && ( parentHierarchyNodeName.contains(SlicerRtCommon::DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX.c_str())
      || hierarchyNodeName.startsWith(SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX.c_str()) ) )
  {
    return 1.0;
  }

  // Isodose lines
  if ( associatedNode && associatedNode->IsA("vtkMRMLModelNode")
    && parentHierarchyNodeName.contains(SlicerRtCommon::ISODOSE_ISODOSE_SURFACES_HIERARCHY_NODE_NAME_POSTFIX.c_str()) )
  {
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyRTPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyRTPlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyRTPlugin);

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  QString hierarchyNodeName(node->GetName());
  QString parentHierarchyNodeName("");
  if (node->GetParentNode())
  {
    parentHierarchyNodeName = QString(node->GetParentNode()->GetName());
  }

  // RT Dose
  if (SlicerRtCommon::IsDoseVolumeNode(associatedNode))
  {
    item->setIcon(d->DoseVolumeIcon);
    return true;
  }
  // RT Image
  else if ( associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode")
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    item->setIcon(d->PlanarImageIcon);
    return true;
  }
  // Isocenter for RT Plan
  else if ( associatedNode && associatedNode->IsA("vtkMRMLMarkupsFiducialNode")
    && parentHierarchyNodeName.contains(SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX.c_str()) )
  {
    item->setIcon(d->IsocenterIcon);
    return true;
  }
  // Beam model for an RT Plan
  else if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES)
    && associatedNode && associatedNode->IsA("vtkMRMLModelNode")
    && hierarchyNodeName.startsWith(SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX.c_str()) )
  {
    item->setIcon(d->BeamIcon);
    return true;
  }
  // Isodose lines
  else if ( associatedNode && associatedNode->IsA("vtkMRMLModelNode")
    && parentHierarchyNodeName.contains(SlicerRtCommon::ISODOSE_ISODOSE_SURFACES_HIERARCHY_NODE_NAME_POSTFIX.c_str()) )
  {
    item->setIcon(d->IsodoseLinesIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRTPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyRTPlugin::setVisibilityIcon: NULL node or item given!";
    return;
  }

  Q_D(qSlicerSubjectHierarchyRTPlugin);

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (!associatedNode)
  {
    qCritical() << "qSlicerSubjectHierarchyRTPlugin::setVisibilityIcon: Invalid associated node for subject hierarchy node " << node->GetName();
    return;
  }
  
  // RT image (show regular eye icon (because it can be shown and hidden)
  if ( associatedNode->IsA("vtkMRMLScalarVolumeNode")
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
      associatedNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
    if (!modelNode)
    {
      qCritical() << "qSlicerSubjectHierarchyRTPlugin::setVisibilityIcon: No displayed model found for planar image '" << associatedNode->GetName() << "'!";
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
  else if (SlicerRtCommon::IsDoseVolumeNode(associatedNode))
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
void qSlicerSubjectHierarchyRTPlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRTPlugin::setDisplayVisibility: NULL node!";
    return;
  }

  vtkMRMLScalarVolumeNode* associatedVolumeNode =
    vtkMRMLScalarVolumeNode::SafeDownCast(node->GetAssociatedDataNode());
  if (associatedVolumeNode)
  {
    // RT Image: show/hide is available. Not propagated to possible children
    if (node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str()))
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        associatedVolumeNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
      if (!modelNode)
      {
        qCritical() << "qSlicerSubjectHierarchyRTPlugin::setDisplayVisibility: No displayed model found for planar image '" << associatedVolumeNode->GetName() << "'!";
        return;
      }
      modelNode->SetDisplayVisibility(visible);
      node->Modified(); // Triggers icon refresh in subject hierarchy tree
    }
    // RT Dose: show like a regular volume
    else if (SlicerRtCommon::IsDoseVolumeNode(associatedVolumeNode))
    {
      qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->setDisplayVisibility(node, visible);
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
int qSlicerSubjectHierarchyRTPlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRTPlugin::getDisplayVisibility: NULL node!";
    return -1;
  }

  vtkMRMLScalarVolumeNode* associatedVolumeNode =
    vtkMRMLScalarVolumeNode::SafeDownCast(node->GetAssociatedDataNode());
  if (associatedVolumeNode)
  {
    // RT Image: show/hide is available. Not propagated to possible children
    if (node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str()))
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        associatedVolumeNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
      if (!modelNode)
      {
        qCritical() << "qSlicerSubjectHierarchyRTPlugin::setDisplayVisibility: No displayed model found for planar image '" << associatedVolumeNode->GetName() << "'!";
        return -1;
      }
      return modelNode->GetDisplayVisibility();
    }
  }

  // Default
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->getDisplayVisibility(node);
}
