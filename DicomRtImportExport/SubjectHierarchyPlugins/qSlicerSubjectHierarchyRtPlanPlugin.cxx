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
#include "SlicerRtCommon.h"

// RTHierarchy Plugins includes
#include "qSlicerSubjectHierarchyRtPlanPlugin.h"

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
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
double qSlicerSubjectHierarchyRtPlanPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtPlanPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedNode();

  // Isocenter for RT Plan
  QString parentHierarchyNodeName("");
  if (node->GetParentNode())
  {
    parentHierarchyNodeName = QString(node->GetParentNode()->GetName());
  }
  if ( node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries())
    && associatedNode && associatedNode->IsA("vtkMRMLMarkupsFiducialNode")
    && parentHierarchyNodeName.contains(SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX.c_str()) )
  {
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyRtPlanPlugin::roleForPlugin()const
{
  return "RT plan";
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRtPlanPlugin::icon(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtPlanPlugin::icon: NULL node given!";
    return QIcon();
  }

  Q_D(qSlicerSubjectHierarchyRtPlanPlugin);

  vtkMRMLNode* associatedNode = node->GetAssociatedNode();

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
    return d->IsocenterIcon;
  }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRtPlanPlugin::visibilityIcon(int visible)
{
  // Have the default plugin (which is not registered) take care of this
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->visibilityIcon(visible);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtPlanPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  //TODO: Switch to Beams (or RT Plans) module when created

  // For the time being switch to the Markups module if the node is an isocenter markup fiducial
  vtkMRMLNode* associatedNode = node->GetAssociatedNode();
  if (associatedNode && associatedNode->IsA("vtkMRMLMarkupsFiducialNode"))
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Markups")->editProperties(node);
  }
}
