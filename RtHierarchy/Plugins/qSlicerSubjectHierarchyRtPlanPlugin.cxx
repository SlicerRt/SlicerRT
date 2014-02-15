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
#include "qSlicerSubjectHierarchyRtPlanPlugin.h"

// SubjectHierarchy MRML includes
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>

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
class qSlicerSubjectHierarchyRtPlanPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyRtPlanPlugin);
protected:
  qSlicerSubjectHierarchyRtPlanPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyRtPlanPluginPrivate(qSlicerSubjectHierarchyRtPlanPlugin& object);
  ~qSlicerSubjectHierarchyRtPlanPluginPrivate();
public:
  QIcon IsocenterIcon;
  QIcon PlanIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRtPlanPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtPlanPluginPrivate::qSlicerSubjectHierarchyRtPlanPluginPrivate(qSlicerSubjectHierarchyRtPlanPlugin& object)
 : q_ptr(&object)
{
  this->IsocenterIcon = QIcon(":Icons/Isocenter.png");
  this->PlanIcon = QIcon(":Icons/Plan.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtPlanPluginPrivate::~qSlicerSubjectHierarchyRtPlanPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtPlanPlugin::qSlicerSubjectHierarchyRtPlanPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRtPlanPluginPrivate(*this) )
{
  this->m_Name = QString("RtPlan");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtPlanPlugin::~qSlicerSubjectHierarchyRtPlanPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRtPlanPlugin::canOwnSubjectHierarchyNode(
  vtkMRMLSubjectHierarchyNode* node, QString &role/*=QString()*/)
{
  role = QString();
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtPlanPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

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
    role = QString("RT plan");
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyRtPlanPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyRtPlanPlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyRtPlanPlugin);

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  QString hierarchyNodeName(node->GetName());
  QString parentHierarchyNodeName("");
  if (node->GetParentNode())
  {
    parentHierarchyNodeName = QString(node->GetParentNode()->GetName());
  }

  // Isocenter for RT Plan
  if ( associatedNode && associatedNode->IsA("vtkMRMLMarkupsFiducialNode")
    && parentHierarchyNodeName.contains(SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX.c_str()) )
  {
    item->setIcon(d->IsocenterIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtPlanPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  // Have the default plugin (which is not registered) take care of this
  qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setVisibilityIcon(node, item);
}

