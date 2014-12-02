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
#include "qSlicerSubjectHierarchyRtBeamPlugin.h"

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
class qSlicerSubjectHierarchyRtBeamPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyRtBeamPlugin);
protected:
  qSlicerSubjectHierarchyRtBeamPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyRtBeamPluginPrivate(qSlicerSubjectHierarchyRtBeamPlugin& object);
  ~qSlicerSubjectHierarchyRtBeamPluginPrivate();
public:
  QIcon BeamIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRtBeamPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtBeamPluginPrivate::qSlicerSubjectHierarchyRtBeamPluginPrivate(qSlicerSubjectHierarchyRtBeamPlugin& object)
 : q_ptr(&object)
{
  this->BeamIcon = QIcon(":Icons/Beam.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtBeamPluginPrivate::~qSlicerSubjectHierarchyRtBeamPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtBeamPlugin::qSlicerSubjectHierarchyRtBeamPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRtBeamPluginPrivate(*this) )
{
  this->m_Name = QString("RtBeams");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtBeamPlugin::~qSlicerSubjectHierarchyRtBeamPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRtBeamPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtBeamPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedNode();
  QString parentHierarchyNodeName("");
  if (node->GetParentNode())
  {
    parentHierarchyNodeName = QString(node->GetParentNode()->GetName());
  }

  // Beam model for an RT Plan
  QString hierarchyNodeName(node->GetName());
  if ( node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries())
    && associatedNode && associatedNode->IsA("vtkMRMLModelNode")
    && ( parentHierarchyNodeName.contains(SlicerRtCommon::DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX.c_str())
      || hierarchyNodeName.startsWith(SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX.c_str()) ) )
  {
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyRtBeamPlugin::roleForPlugin()const
{
  return "RT beam";
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRtBeamPlugin::icon(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtBeamPlugin::icon: NULL node given!";
    return QIcon();
  }

  Q_D(qSlicerSubjectHierarchyRtBeamPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    return d->BeamIcon;
  }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRtBeamPlugin::visibilityIcon(int visible)
{
  // Have the default plugin (which is not registered) take care of this
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->visibilityIcon(visible);
}


//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtBeamPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  //TODO: Switch to Beams (or RT Plans) module when created

  // For the time being switch to the Models module
  qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Models")->editProperties(node);
}
