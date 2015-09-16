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

// ExternalBeamPlanning includes
#include "qSlicerSubjectHierarchyRTBeamPlugin.h"

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
#include <QStandardItem>
#include <QAction>

// SlicerQt includes
//#include "qSlicerAbstractModuleWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerSubjectHierarchyRTBeamPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyRTBeamPlugin);
protected:
  qSlicerSubjectHierarchyRTBeamPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyRTBeamPluginPrivate(qSlicerSubjectHierarchyRTBeamPlugin& object);
  ~qSlicerSubjectHierarchyRTBeamPluginPrivate();
public:
  QIcon BeamIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRTBeamPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTBeamPluginPrivate::qSlicerSubjectHierarchyRTBeamPluginPrivate(qSlicerSubjectHierarchyRTBeamPlugin& object)
 : q_ptr(&object)
{
  this->BeamIcon = QIcon(":Icons/Beam.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTBeamPluginPrivate::~qSlicerSubjectHierarchyRTBeamPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTBeamPlugin::qSlicerSubjectHierarchyRTBeamPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRTBeamPluginPrivate(*this) )
{
  this->m_Name = QString("RTBeam");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRTBeamPlugin::~qSlicerSubjectHierarchyRTBeamPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRTBeamPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRTBeamPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedNode();

  // RT beam
  if ( associatedNode && associatedNode->IsA("vtkMRMLRTBeamNode") )
  {
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyRTBeamPlugin::roleForPlugin()const
{
  return "RT beam";
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRTBeamPlugin::icon(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRTBeamPlugin::icon: NULL node given!";
    return QIcon();
  }

  Q_D(qSlicerSubjectHierarchyRTBeamPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    return d->BeamIcon;
  }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRTBeamPlugin::visibilityIcon(int visible)
{
  // Have the default plugin (which is not registered) take care of this
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->visibilityIcon(visible);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRTBeamPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  Q_UNUSED(node);

  // Switch to External Beam Planning module
  qSlicerSubjectHierarchyAbstractPlugin::switchToModule("ExternalBeamPlanning");
}
