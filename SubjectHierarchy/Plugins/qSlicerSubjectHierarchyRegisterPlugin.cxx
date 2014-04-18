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
#include "vtkSubjectHierarchyConstants.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyRegisterPlugin.h"
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
#include <vtkMRMLSliceNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QStandardItem>
#include <QAction>
#include <QMenu>

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// MRML widgets includes
#include "qMRMLNodeComboBox.h"

// STD includes
#include <set>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class qSlicerSubjectHierarchyRegisterPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyRegisterPlugin);
protected:
  qSlicerSubjectHierarchyRegisterPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyRegisterPluginPrivate(qSlicerSubjectHierarchyRegisterPlugin& object);
  ~qSlicerSubjectHierarchyRegisterPluginPrivate();
  void init();
public:
  QAction* RegisterThisAction;
  QAction* RegisterToAction;

  QAction* RigidImageBasedAction;
  QAction* BSplineImageBasedAction;
  QAction* InteractiveLandmarkAction;
  QAction* FiducialAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRegisterPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRegisterPluginPrivate::qSlicerSubjectHierarchyRegisterPluginPrivate(qSlicerSubjectHierarchyRegisterPlugin& object)
: q_ptr(&object)
{
  this->RegisterThisAction = NULL;
  this->RegisterToAction = NULL;

  this->RigidImageBasedAction = NULL;
  this->BSplineImageBasedAction = NULL;
  this->InteractiveLandmarkAction = NULL;
  this->FiducialAction = NULL;
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRegisterPluginPrivate::~qSlicerSubjectHierarchyRegisterPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRegisterPlugin::qSlicerSubjectHierarchyRegisterPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRegisterPluginPrivate(*this) )
 , m_RegisterFromNode(NULL)
{
  this->m_Name = QString("Register");

  Q_D(qSlicerSubjectHierarchyRegisterPlugin);
  d->init();
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyRegisterPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyRegisterPlugin);

  this->RegisterThisAction = new QAction("Register this...",q);
  QObject::connect(this->RegisterThisAction, SIGNAL(triggered()), q, SLOT(registerCurrentNodeTo()));

  this->RegisterToAction = new QAction("Register * to this using...",q);
  QObject::connect(this->RegisterToAction, SIGNAL(triggered()), q, SLOT(registerSelectedToCurrentNode()));

  // Actions for the registration methods
  QMenu* registrationMethodsSubMenu = new QMenu();
  this->RegisterToAction->setMenu(registrationMethodsSubMenu);

  this->RigidImageBasedAction = new QAction("Rigid image-based registration",q);
  QObject::connect(this->RigidImageBasedAction, SIGNAL(triggered()), q, SLOT(())); //TODO:
  registrationMethodsSubMenu->addAction(this->RigidImageBasedAction);

  this->BSplineImageBasedAction = new QAction("BSpline image-based registration",q);
  QObject::connect(this->BSplineImageBasedAction, SIGNAL(triggered()), q, SLOT(())); //TODO:
  registrationMethodsSubMenu->addAction(this->BSplineImageBasedAction);

  this->InteractiveLandmarkAction = new QAction("Interactive landmark registration",q);
  QObject::connect(this->InteractiveLandmarkAction, SIGNAL(triggered()), q, SLOT(())); //TODO:
  registrationMethodsSubMenu->addAction(this->InteractiveLandmarkAction);

  this->FiducialAction = new QAction("Fiducial registration",q);
  QObject::connect(this->FiducialAction, SIGNAL(triggered()), q, SLOT(())); //TODO:
  registrationMethodsSubMenu->addAction(this->FiducialAction);
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRegisterPlugin::~qSlicerSubjectHierarchyRegisterPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRegisterPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_UNUSED(node);

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyRegisterPlugin::roleForPlugin()const
{
  return "N/A";
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyRegisterPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setIcon(node, item);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRegisterPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setVisibilityIcon(node, item);
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyRegisterPlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyRegisterPlugin);

  QList<QAction*> actions;
  actions << d->RegisterThisAction << d->RegisterToAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRegisterPlugin::showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyRegisterPlugin);

  d->RegisterThisAction->setVisible(false);
  d->RegisterToAction->setVisible(false);

  if (!node)
  {
    // There are no scene actions in this plugin
    return;
  }

  // Volume
  if (qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->canOwnSubjectHierarchyNode(node))
  {
    // Get current node
    vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
    if (!currentNode)
    {
      qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::showContextMenuActionsForNode: Invalid current node!";
      return;
    }
    
    // Show 'Register this' action if there is no 'from' node saved
    if (!this->m_RegisterFromNode)
    {
        d->RegisterThisAction->setVisible(true);
    }
    // Show 'Register to' action if 'from' node is saved, and another one is selected
    // (does not make sense to register a volume to itself)
    else if (currentNode != this->m_RegisterFromNode)
    {
      d->RegisterToAction->setVisible(true);
    }
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRegisterPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  // No role, no edit properties
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRegisterPlugin::registerCurrentNodeTo()
{
  Q_D(qSlicerSubjectHierarchyRegisterPlugin);

  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  if (!currentNode || !scene)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::createChildContourSetForCurrentNode: Invalid current node or MRML scene!";
    return;
  }

  // Save selected node as 'from' node
  this->m_RegisterFromNode = currentNode;

  d->RegisterToAction->setText( QString("Register %1 to this using...").arg(
    currentNode->GetAssociatedDataNode()->GetName() ) );
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRegisterPlugin::registerSelectedToCurrentNode()
{
  /// Switch to registration module corresponding to selected method, set chosen
  /// input nodes, offer a best guess parameter set based on modalities etc.
  /// The parameter sets are stored in files and loaded in a separate scene at
  /// startup.

  // Reset saved 'from' node
  this->m_RegisterFromNode = NULL;
}
