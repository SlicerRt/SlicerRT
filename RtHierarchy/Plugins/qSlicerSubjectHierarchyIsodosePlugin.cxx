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
#include "vtkMRMLIsodoseNode.h"

// RTHierarchy Plugins includes
#include "qSlicerSubjectHierarchyIsodosePlugin.h"

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
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QIcon>
#include <QStandardItem>
#include <QAction>

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerSubjectHierarchyIsodosePluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyIsodosePlugin);
protected:
  qSlicerSubjectHierarchyIsodosePlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyIsodosePluginPrivate(qSlicerSubjectHierarchyIsodosePlugin& object);
  ~qSlicerSubjectHierarchyIsodosePluginPrivate();
public:
  QIcon IsodoseLinesIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyIsodosePluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyIsodosePluginPrivate::qSlicerSubjectHierarchyIsodosePluginPrivate(qSlicerSubjectHierarchyIsodosePlugin& object)
 : q_ptr(&object)
{
  this->IsodoseLinesIcon = QIcon(":Icons/IsodoseLines.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyIsodosePluginPrivate::~qSlicerSubjectHierarchyIsodosePluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyIsodosePlugin::qSlicerSubjectHierarchyIsodosePlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyIsodosePluginPrivate(*this) )
{
  this->m_Name = QString("Isodose");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyIsodosePlugin::~qSlicerSubjectHierarchyIsodosePlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyIsodosePlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyIsodosePlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  QString parentHierarchyNodeName("");
  if (node->GetParentNode())
  {
    parentHierarchyNodeName = QString(node->GetParentNode()->GetName());
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
const QString qSlicerSubjectHierarchyIsodosePlugin::roleForPlugin()const
{
  return "Isodose surface";
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyIsodosePlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyIsodosePlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyIsodosePlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    item->setIcon(d->IsodoseLinesIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyIsodosePlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  // Have the default plugin (which is not registered) take care of this
  qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setVisibilityIcon(node, item);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyIsodosePlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  // Switch to isodose module with parameter set node already selected
  qSlicerAbstractModuleWidget* moduleWidget = qSlicerSubjectHierarchyAbstractPlugin::switchToModule("Isodose");
  if (moduleWidget)
  {
    // Get node selector combobox
    qMRMLNodeComboBox* nodeSelector = moduleWidget->findChild<qMRMLNodeComboBox*>("MRMLNodeComboBox_ParameterSet");

    //TODO: Get parameter set node for isodose model and model hierarchy node
    // and set corresponding dose volume too when changes in #580 are implemented
    vtkMRMLIsodoseNode* isodoseParameterSetNode = NULL; //TODO:

    // Choose current data node
    if (nodeSelector && isodoseParameterSetNode)
    {
      nodeSelector->setCurrentNode(isodoseParameterSetNode);
    }
  }
}
