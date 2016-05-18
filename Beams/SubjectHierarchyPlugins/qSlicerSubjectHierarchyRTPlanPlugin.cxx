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

// ExternalBeamPlanning includes
#include "qSlicerSubjectHierarchyRTPlanPlugin.h"

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

// MRML Widgets includes
#include <qMRMLNodeComboBox.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// Qt includes
#include <QDebug>
#include <QIcon>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerSubjectHierarchyRTPlanPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyRTPlanPlugin);
protected:
  qSlicerSubjectHierarchyRTPlanPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyRTPlanPluginPrivate(qSlicerSubjectHierarchyRTPlanPlugin& object);
  ~qSlicerSubjectHierarchyRTPlanPluginPrivate();
public:
  QIcon IsocenterIcon;
  QIcon PlanIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRTPlanPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTPlanPluginPrivate::qSlicerSubjectHierarchyRTPlanPluginPrivate(qSlicerSubjectHierarchyRTPlanPlugin& object)
 : q_ptr(&object)
{
  this->IsocenterIcon = QIcon(":Icons/Isocenter.png");
  this->PlanIcon = QIcon(":Icons/Plan.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTPlanPluginPrivate::~qSlicerSubjectHierarchyRTPlanPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTPlanPlugin::qSlicerSubjectHierarchyRTPlanPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRTPlanPluginPrivate(*this) )
{
  this->m_Name = QString("RTPlan");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTPlanPlugin::~qSlicerSubjectHierarchyRTPlanPlugin()
{
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyRTPlanPlugin::canAddNodeToSubjectHierarchy(vtkMRMLNode* node, vtkMRMLSubjectHierarchyNode* parent/*=NULL*/)const
{
  Q_UNUSED(parent);
  if (!node)
  {
    qCritical() << Q_FUNC_INFO << ": Input node is NULL!";
    return 0.0;
  }
  else if (node->IsA("vtkMRMLRTPlanNode"))
  {
    return 1.0;
  }
  return 0.0;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRTPlanPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << Q_FUNC_INFO << ": Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedNode();

  // RT plan
  if ( associatedNode && associatedNode->IsA("vtkMRMLRTPlanNode") )
  {
    return 1.0;
  }

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
const QString qSlicerSubjectHierarchyRTPlanPlugin::roleForPlugin()const
{
  return "RT plan";
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRTPlanPlugin::icon(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << Q_FUNC_INFO << ": NULL node given!";
    return QIcon();
  }

  Q_D(qSlicerSubjectHierarchyRTPlanPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    return d->PlanIcon;
  }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRTPlanPlugin::visibilityIcon(int visible)
{
  // Have the default plugin (which is not registered) take care of this
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->visibilityIcon(visible);
}
