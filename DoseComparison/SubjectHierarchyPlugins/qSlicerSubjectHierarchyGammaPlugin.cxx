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
#include "qSlicerSubjectHierarchyGammaPlugin.h"

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
#include "qSlicerAbstractModuleWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerSubjectHierarchyGammaPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyGammaPlugin);
protected:
  qSlicerSubjectHierarchyGammaPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyGammaPluginPrivate(qSlicerSubjectHierarchyGammaPlugin& object);
  ~qSlicerSubjectHierarchyGammaPluginPrivate();
public:
  QIcon GammaIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyGammaPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyGammaPluginPrivate::qSlicerSubjectHierarchyGammaPluginPrivate(qSlicerSubjectHierarchyGammaPlugin& object)
 : q_ptr(&object)
{
  this->GammaIcon = QIcon(":Icons/Gamma.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyGammaPluginPrivate::~qSlicerSubjectHierarchyGammaPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyGammaPlugin::qSlicerSubjectHierarchyGammaPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyGammaPluginPrivate(*this) )
{
  this->m_Name = QString("Gamma");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyGammaPlugin::~qSlicerSubjectHierarchyGammaPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyGammaPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyGammaPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedNode();

  // Gamma volume
  if ( associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode")
    && associatedNode->GetAttribute(SlicerRtCommon::DOSECOMPARISON_GAMMA_VOLUME_IDENTIFIER_ATTRIBUTE_NAME) )
  {
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyGammaPlugin::roleForPlugin()const
{
  return "Gamma dose comparison volume";
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyGammaPlugin::icon(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyGammaPlugin::icon: NULL node given!";
    return QIcon();
  }

  Q_D(qSlicerSubjectHierarchyGammaPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    return d->GammaIcon;
  }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyGammaPlugin::visibilityIcon(int visible)
{
  return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->visibilityIcon(visible);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyGammaPlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyGammaPlugin::setDisplayVisibility: NULL node!";
    return;
  }

  if (this->canOwnSubjectHierarchyNode(node))
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->setDisplayVisibility(node, visible);
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(node, visible);
  }
}

//---------------------------------------------------------------------------
int qSlicerSubjectHierarchyGammaPlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyGammaPlugin::getDisplayVisibility: NULL node!";
    return -1;
  }

  if (this->canOwnSubjectHierarchyNode(node))
  {
    return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->getDisplayVisibility(node);
  }

  // Default
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->getDisplayVisibility(node);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyGammaPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  // Switch to dose comparison module
  qSlicerAbstractModuleWidget* moduleWidget = qSlicerSubjectHierarchyAbstractPlugin::switchToModule("DoseComparison");
}
