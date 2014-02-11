/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

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

// Qt includes
#include <QtPlugin>
#include <QDebug>

// SubjectHierarchy includes
#include "qSlicerSubjectHierarchyModule.h"
#include "qSlicerSubjectHierarchyModuleWidget.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDICOMPlugin.h"
#include "qSlicerSubjectHierarchyVolumesPlugin.h"
#include "qSlicerSubjectHierarchyParseLocalDataPlugin.h"

// MRML includes
#include <vtkMRMLScene.h>

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerSubjectHierarchyModule, qSlicerSubjectHierarchyModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy
class qSlicerSubjectHierarchyModulePrivate
{
public:
  qSlicerSubjectHierarchyModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyModulePrivate::qSlicerSubjectHierarchyModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyModule methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyModule::qSlicerSubjectHierarchyModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSubjectHierarchyModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyModule::~qSlicerSubjectHierarchyModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSubjectHierarchyModule::helpText()const
{
  QString help = 
    "The Subject Hierarchy module manages and displays subject hierarchies. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/SubjectHierarchy\">%1/Documentation/%2.%3/Modules/SubjectHierarchy</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerSubjectHierarchyModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSubjectHierarchyModule::categories() const
{
  return QStringList() << "" << "Informatics";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSubjectHierarchyModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyModule::icon()const
{
  return this->Superclass::icon();
  //return QIcon(":/Icons/SubjectHierarchy.png");
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModule::setup()
{
  this->Superclass::setup();

  // Handle scene change event if occurs
  qvtkConnect( this->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  // Connect scene node added event so that the new subject hierarchy nodes can be claimed by a plugin
  qvtkConnect( this->mrmlScene(), vtkMRMLScene::NodeAddedEvent, this, SLOT( onNodeAdded(vtkObject*,vtkObject*) ) );

  // Register Subject Hierarchy core plugins
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyDICOMPlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyVolumesPlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyParseLocalDataPlugin());
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSubjectHierarchyModule::createLogic()
{
  return vtkSlicerSubjectHierarchyModuleLogic::New();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerSubjectHierarchyModule::createWidgetRepresentation()
{
  return new qSlicerSubjectHierarchyModuleWidget;
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModule::onLogicModified()
{
  vtkMRMLScene* scene = this->mrmlScene();

  // Connect scene node added event so that the new subject hierarchy nodes can be claimed by a plugin
  qvtkReconnect( scene, vtkMRMLScene::NodeAddedEvent, this, SLOT( onNodeAdded(vtkObject*,vtkObject*) ) );

  // Set the new scene to the plugin handler
  qSlicerSubjectHierarchyPluginHandler::instance()->setScene(scene);
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModule::onNodeAdded(vtkObject* scene, vtkObject* nodeObject)
{
  Q_UNUSED(scene); 

  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nodeObject);
  if (subjectHierarchyNode)
  {
    // Keep 'owner plugin changed' connections up-to date (reconnect to the new plugin)
    qvtkConnect( subjectHierarchyNode, vtkMRMLSubjectHierarchyNode::OwnerPluginChangedEvent,
      qSlicerSubjectHierarchyPluginHandler::instance(), SLOT( reconnectOwnerPluginChanged(vtkObject*,void*) ) );

    // Find plugin for current subject hierarchy node and "claim" it
    qSlicerSubjectHierarchyAbstractPlugin* ownerPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->findAndSetOwnerPluginForSubjectHierarchyNode(subjectHierarchyNode);

    // See if owner plugin has to be changed when a note is modified
    qvtkConnect( subjectHierarchyNode, vtkCommand::ModifiedEvent,
      this, SLOT( onSubjectHierarchyNodeModified(vtkObject*) ) );
  }
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModule::onSubjectHierarchyNodeModified(vtkObject* nodeObject)
{
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nodeObject);
  if (subjectHierarchyNode && subjectHierarchyNode->GetOwnerPluginAutoSearch())
  {
    // Find plugin for current subject hierarchy node and "claim" it if the
    // owner plugin is not manually overridden by the user
    QString pluginBefore( subjectHierarchyNode->GetOwnerPluginName() );
    qSlicerSubjectHierarchyPluginHandler::instance()->findAndSetOwnerPluginForSubjectHierarchyNode(subjectHierarchyNode);
    QString pluginAfter( subjectHierarchyNode->GetOwnerPluginName() );
    if (pluginBefore.compare(pluginAfter))
    {
      qDebug() << "qSlicerSubjectHierarchyModule::onSubjectHierarchyNodeModified: Subject hierarchy node '" <<
        subjectHierarchyNode->GetName() << "' has been modified, plugin search performed, and owner plugin changed from '" <<
        pluginBefore << "' to '" << pluginAfter << "'";
    }
  }
}
