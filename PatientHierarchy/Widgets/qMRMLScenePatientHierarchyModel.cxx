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

#include "qMRMLScenePatientHierarchyModel.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// Patient Hierarchy includes
#include "qMRMLScenePatientHierarchyModel_p.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"
#include "vtkSlicerPatientHierarchyPluginHandler.h"
#include "vtkSlicerPatientHierarchyPlugin.h"

// MRML includes
#include <vtkMRMLDisplayableHierarchyNode.h>
#include <vtkMRMLDisplayNode.h>

// Qt includes
#include <QMimeData>

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModelPrivate::qMRMLScenePatientHierarchyModelPrivate(qMRMLScenePatientHierarchyModel& object)
: Superclass(object)
{
  this->NodeTypeColumn = -1;

  this->BeamIcon = QIcon(":Icons/Beam.png");
  this->ColorTableIcon = QIcon(":Icons/ColorTable.png");
  this->ContourIcon = QIcon(":Icons/Contour.png");
  this->DoseVolumeIcon = QIcon(":Icons/DoseVolume.png");
  this->IsocenterIcon = QIcon(":Icons/Isocenter.png");
  this->IsodoseLinesIcon = QIcon(":Icons/IsodoseLines.png");
  this->PatientIcon = QIcon(":Icons/Patient.png");
  this->PlanIcon = QIcon(":Icons/Plan.png");
  this->ShowInViewersIcon = QIcon(":Icons/ShowInViewers.png");
  this->StructureSetIcon = QIcon(":Icons/StructureSet.png");
  this->StudyIcon = QIcon(":Icons/Study.png");
  this->VolumeIcon = QIcon(":Icons/Volume.png");
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModelPrivate::init()
{
  Q_Q(qMRMLScenePatientHierarchyModel);
  this->Superclass::init();

  q->setNameColumn(0);
  q->setNodeTypeColumn(q->nameColumn());
  q->setVisibilityColumn(1);
  q->setIDColumn(2);

  q->setHorizontalHeaderLabels(
    QStringList() << "Nodes" << "Vis" << "IDs");

  q->horizontalHeaderItem(0)->setToolTip(QObject::tr("Node name and type"));
  q->horizontalHeaderItem(1)->setToolTip(QObject::tr("Show/hide branch or node"));
  q->horizontalHeaderItem(2)->setToolTip(QObject::tr("Node ID"));
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModel::qMRMLScenePatientHierarchyModel(QObject *vparent)
: Superclass(new qMRMLScenePatientHierarchyModelPrivate(*this), vparent)
{
  Q_D(qMRMLScenePatientHierarchyModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModel::~qMRMLScenePatientHierarchyModel()
{
}

//------------------------------------------------------------------------------
QStringList qMRMLScenePatientHierarchyModel::mimeTypes()const
{
  QStringList types;
  types << "application/vnd.text.list";
  return types;
}

//------------------------------------------------------------------------------
QMimeData* qMRMLScenePatientHierarchyModel::mimeData(const QModelIndexList &indexes) const
{
  Q_D(const qMRMLScenePatientHierarchyModel);

  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  foreach (const QModelIndex &index, indexes)
  {
    // Only add one pointer per row
    if (index.isValid() && index.column() == 0)
    {
      d->DraggedNodes << this->mrmlNodeFromIndex(index);
      QString text = data(index, PointerRole).toString();
      stream << text;
    }
  }

  mimeData->setData("application/vnd.text.list", encodedData);
  return mimeData;
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::canBeAChild(vtkMRMLNode* node)const
{
  vtkMRMLHierarchyNode* hnode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if ( hnode && SlicerRtCommon::IsPatientHierarchyNode(hnode) )
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::canBeAParent(vtkMRMLNode* node)const
{
  vtkMRMLHierarchyNode* hnode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if ( hnode && SlicerRtCommon::IsPatientHierarchyNode(hnode) )
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int qMRMLScenePatientHierarchyModel::nodeTypeColumn()const
{
  Q_D(const qMRMLScenePatientHierarchyModel);
  return d->NodeTypeColumn;
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModel::setNodeTypeColumn(int column)
{
  Q_D(qMRMLScenePatientHierarchyModel);
  d->NodeTypeColumn = column;
  this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLScenePatientHierarchyModel::maxColumnId()const
{
  Q_D(const qMRMLScenePatientHierarchyModel);
  int maxId = this->Superclass::maxColumnId();
  maxId = qMax(maxId, d->VisibilityColumn);
  maxId = qMax(maxId, d->NodeTypeColumn);
  maxId = qMax(maxId, d->NameColumn);
  maxId = qMax(maxId, d->IDColumn);
  return maxId;
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModel::updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  Q_D(qMRMLScenePatientHierarchyModel);

  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if (!hierarchyNode || !SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode))
  {
    return;
  }
  vtkMRMLNode* associatedNode = hierarchyNode->GetAssociatedNode();

  if (column == this->nameColumn())
  {
    QString nodeText(hierarchyNode->GetName());
    if (nodeText.endsWith(QString(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX.c_str())))
    {
      nodeText = nodeText.left( nodeText.size() - SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX.size() );
    }
    else if (!node->IsA("vtkMRMLModelHierarchyNode") && !node->IsA("vtkMRMLAnnotationHierarchyNode")) // Do not expect postfix for "multi-functional" hierarchies (that are to be used also in other modules)
    {
      vtkWarningWithObjectMacro(this->mrmlScene(),"qMRMLScenePatientHierarchyModel::updateItemDataFromNode: Patient hierarchy node name should end with '" << SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX << "'. It has the name '" << hierarchyNode->GetName() << "'");
    }
    item->setText(nodeText);
    item->setToolTip(vtkSlicerPatientHierarchyModuleLogic::GetTooltipForPatientHierarchyNode(hierarchyNode).c_str());
  }
  if (column == this->idColumn())
  {
    item->setText(QString(hierarchyNode->GetID()));
  }
  if (column == this->visibilityColumn())
  {
    int visible = vtkSlicerPatientHierarchyModuleLogic::GetBranchVisibility(hierarchyNode);

    // Always set a different icon to volumes. If not a volume then set the appropriate eye icon
    if (associatedNode && associatedNode->IsA("vtkMRMLVolumeNode"))
    {
      item->setIcon(d->ShowInViewersIcon);
    }
    // It should be fine to set the icon even if it is the same, but due
    // to a bug in Qt (http://bugreports.qt.nokia.com/browse/QTBUG-20248),
    // it would fire a superflous itemChanged() signal.
    else if (item->data(VisibilityRole).isNull() || item->data(VisibilityRole).toInt() != visible)
    {
      item->setData(visible, VisibilityRole);
      switch (visible)
      {
      case 0:
        item->setIcon(d->HiddenIcon);
        break;
      case 1:
        item->setIcon(d->VisibleIcon);
        break;
      case 2:
        item->setIcon(d->PartiallyVisibleIcon);
        break;
      default:
        break;
      }
    }
  }
  if (column == this->nodeTypeColumn())
  {
    if ( vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel( hierarchyNode,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_PATIENT) )
    {
      item->setIcon(d->PatientIcon);
    }
    else if ( vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel( hierarchyNode,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_STUDY) )
    {
      item->setIcon(d->StudyIcon);
    }
    else if ( vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel( hierarchyNode,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES) )
    {
      if (hierarchyNode->IsA("vtkMRMLDisplayableHierarchyNode") && hierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()))
      {
        item->setIcon(d->StructureSetIcon);
      }
      else if (associatedNode && associatedNode->IsA("vtkMRMLVolumeNode"))
      {
        if (SlicerRtCommon::IsDoseVolumeNode(associatedNode))
        {
          item->setIcon(d->DoseVolumeIcon);
        }
        else
        {
          item->setIcon(d->VolumeIcon);
        }
      }
      //TODO: Set icon for plan
    }
    else if ( vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel( hierarchyNode,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES) )
    {
      if (associatedNode)
      {
        if (associatedNode->IsA("vtkMRMLColorTableNode"))
        {
          item->setIcon(d->ColorTableIcon);
        }
        else if (associatedNode->IsA("vtkMRMLContourNode"))
        {
          item->setIcon(d->ContourIcon);
        }
        else if (associatedNode->IsA("vtkMRMLAnnotationFiducialNode"))
        {
          QString parentHierarchyNodeName(hierarchyNode->GetParentNode()->GetName());
          if (parentHierarchyNodeName.contains(SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX.c_str()))
          {
            item->setIcon(d->IsocenterIcon);
          }
          else
          {
            vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::updateItemDataFromNode: Unrecognized annotation object '" << associatedNode->GetName() << "'");
          }
        }
        else if (associatedNode->IsA("vtkMRMLModelNode"))
        {
          QString hierarchyNodeName(hierarchyNode->GetName());
          QString parentHierarchyNodeName(hierarchyNode->GetParentNode()->GetName());
          if ( parentHierarchyNodeName.contains(SlicerRtCommon::DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX.c_str())
            || hierarchyNodeName.startsWith(SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX.c_str()) )
          {
            item->setIcon(d->BeamIcon);
          }
          else if (parentHierarchyNodeName.contains(SlicerRtCommon::ISODOSE_ISODOSE_SURFACES_HIERARCHY_NODE_NAME_POSTFIX.c_str()))
          {
            item->setIcon(d->IsodoseLinesIcon);
          }
          else
          {
            vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::updateItemDataFromNode: Unrecognized model object '" << associatedNode->GetName() << "'");
          }
        }
      }
    }
    else
    {
      vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::updateItemDataFromNode: Invalid DICOM level found for node '" << node->GetName() << "'");
    }
  }
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModel::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if (!hierarchyNode || !SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode))
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::updateNodeFromItemData: Invalid node in patient hierarchy tree! Nodes must all be patient hierarchy nodes.");
    return;
  }
  vtkMRMLNode* associatedNode = hierarchyNode->GetAssociatedNode();

  if ( item->column() == this->nameColumn() )
  {
    hierarchyNode->SetName(item->text().append(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX.c_str()).toLatin1().constData());
  }
  if ( item->column() == this->visibilityColumn()
    && !item->data(VisibilityRole).isNull() )
  {
    int visible = item->data(VisibilityRole).toInt();
    if (visible > -1)
    {
      vtkSlicerPatientHierarchyModuleLogic::SetBranchVisibility(hierarchyNode, visible);
    }
  }
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
  Q_D(const qMRMLScenePatientHierarchyModel);
  Q_UNUSED(row);
  Q_UNUSED(column);

  if (action == Qt::IgnoreAction)
  {
    return true;
  }
  if (!this->mrmlScene())
  {
    std::cerr << "qMRMLScenePatientHierarchyModel::dropMimeData: Invalid MRML scene!" << std::endl;
    return false;
  }
  if (!data->hasFormat("application/vnd.text.list"))
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Plain text MIME type is expected");
    return false;
  }

  // Nothing can be dropped to the top level (patients can only be loaded at the moment from the DICOM browser)
  if (!parent.isValid())
  {
    vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Items cannot be dropped on top level!");
    return false;
  }
  vtkMRMLNode* parentNode = this->mrmlNodeFromIndex(parent);
  if (!parentNode)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Unable to get parent node!");
    return false;
  }

  // Decode MIME data
  QByteArray encodedData = data->data("application/vnd.text.list");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  QStringList streamItems;
  int rows = 0;

  while (!stream.atEnd())
  {
    QString text;
    stream >> text;
    streamItems << text;
    ++rows;
  }

  if (rows == 0)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Unable to decode dropped MIME data!");
    return false;
  }
  if (rows > 1)
  {
    vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: More than one data item decoded from dropped MIME data! Only the first one will be used.");
  }

  QString nodePointerString = streamItems[0];

  vtkMRMLNode* droppedNode = vtkMRMLNode::SafeDownCast(reinterpret_cast<vtkObject*>(nodePointerString.toULongLong()));
  if (!droppedNode)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Unable to get MRML node from dropped MIME text (" << nodePointerString.toLatin1().constData() << ")!");
    return false;
  }

  d->DraggedNodes.clear();

  // Reparent the node
  return this->reparent(droppedNode, parentNode);
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::reparent(vtkMRMLNode* node, vtkMRMLNode* newParent)
{
  if (!node || this->parentNode(node) == newParent)
  {
    return false;
  }
  Q_ASSERT(newParent != node);

  if (!this->mrmlScene())
  {
    std::cerr << "qMRMLScenePatientHierarchyModel::reparent: Invalid MRML scene!" << std::endl;
    return false;
  }

  vtkMRMLHierarchyNode* parentPatientHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(newParent);
  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if (hierarchyNode && !SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode))
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::reparent: Invalid node in patient hierarchy tree! Hierarchy nodes must all be patient hierarchy nodes.");
    return false;
  }

  if (!this->canBeAParent(newParent))
  {
    vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::reparent: Target parent node (" << newParent->GetName() << ") is not a valid patient hierarchy parent node!");
  }

  // Prevent collapse of the patient hierarchy tree view (TODO: This is a workaround)
  emit saveTreeExpandState();

  // If dropped from within the patient hierarchy tree
  if (hierarchyNode)
  {
    bool successfullyReadByPlugin = false;
    vtkSlicerPatientHierarchyPlugin* foundPlugin
      = vtkSlicerPatientHierarchyPluginHandler::GetInstance()->GetPluginForReparentInsidePatientHierarchyForNode(hierarchyNode);
    if (foundPlugin)
    {
      successfullyReadByPlugin = foundPlugin->ReparentNodeInsidePatientHierarchy(hierarchyNode, parentPatientHierarchyNode);
      if (!successfullyReadByPlugin)
      {
        // Put back to its original place
        hierarchyNode->SetParentNodeID( hierarchyNode->GetParentNodeID() );

        vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::reparent: Failed to reparent node "
          << hierarchyNode->GetName() << " through plugin " << (foundPlugin->GetName()?foundPlugin->GetName():"Unnamed") << "!");
      }
    }
    else
    {
      hierarchyNode->SetParentNodeID(parentPatientHierarchyNode->GetID());
    }
  }
  // If dropped from the potential patient hierarchy nodes list
  else
  {
    // Get possible associated non-patient hierarchy node for reparented node
    vtkMRMLHierarchyNode* associatedNonPatientHierarchyNode
      = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedNonPatientHierarchyNode(node);

    // Delete associated hierarchy node if it's not a patient hierarchy node. Should not occur unless it is an annotation hierarchy node
    // (they are an exception as they are needed for the Annotations module; TODO: review when Annotations module ha been improved)
    if (associatedNonPatientHierarchyNode && !associatedNonPatientHierarchyNode->IsA("vtkMRMLAnnotationHierarchyNode"))
    {
      vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::reparent: Reparented node had a non-patient hierarchy node associated!");
      this->mrmlScene()->RemoveNode(associatedNonPatientHierarchyNode);
    }

    // If there is a plugin that can handle the dropped node then let it take care of it
    bool successfullyReadByPlugin = false;
    vtkSlicerPatientHierarchyPlugin* foundPlugin = vtkSlicerPatientHierarchyPluginHandler::GetInstance()->GetPluginForAddToPatientHierarchyForNode(node);
    if (foundPlugin)
    {
      successfullyReadByPlugin = foundPlugin->AddNodeToPatientHierarchy(node, parentPatientHierarchyNode);
      if (!successfullyReadByPlugin)
      {
        vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::reparent: Failed to add node "
          << node->GetName() << " through plugin " << (foundPlugin->GetName()?foundPlugin->GetName():"Unnamed") << "!");
      }
    }

    if (!foundPlugin || !successfullyReadByPlugin)
    {
      // Associate to a new hierarchy node and put it in the tree under the parent
      vtkSmartPointer<vtkMRMLHierarchyNode> associatedPatientHierarchyNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
      associatedPatientHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(this->mrmlScene()->AddNode(associatedPatientHierarchyNode));
      associatedPatientHierarchyNode->SetAssociatedNodeID(node->GetID());
      std::string phNodeName;
      phNodeName = std::string(node->GetName()) + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
      phNodeName = this->mrmlScene()->GenerateUniqueName(phNodeName);
      associatedPatientHierarchyNode->SetName(phNodeName.c_str());
      associatedPatientHierarchyNode->HideFromEditorsOff();
      associatedPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
        SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
      associatedPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
        vtkSlicerPatientHierarchyModuleLogic::GetChildDicomLevel(parentPatientHierarchyNode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME)));
      associatedPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
        "");
      associatedPatientHierarchyNode->SetParentNodeID(parentPatientHierarchyNode->GetID());
    }
  }


  // Force updating the whole scene (TODO: this should not be needed)
  this->updateScene();

  // Prevent collapse of the patient hierarchy tree view (TODO: This is a workaround)
  emit loadTreeExpandState();

  return true;
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModel::forceUpdateScene()
{
  // Force updating the whole scene (TODO: this should not be needed)
  this->updateScene();
}
