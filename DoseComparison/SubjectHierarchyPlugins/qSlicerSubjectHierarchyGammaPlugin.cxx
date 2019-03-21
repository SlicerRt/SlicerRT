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

// DoseComparison includes
#include "qSlicerSubjectHierarchyGammaPlugin.h"
#include "vtkSlicerDoseComparisonModuleLogic.h"

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
qSlicerSubjectHierarchyGammaPluginPrivate::~qSlicerSubjectHierarchyGammaPluginPrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyGammaPlugin methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyGammaPlugin::qSlicerSubjectHierarchyGammaPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyGammaPluginPrivate(*this) )
{
  this->m_Name = QString("Gamma");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyGammaPlugin::~qSlicerSubjectHierarchyGammaPlugin() = default;

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyGammaPlugin::canOwnSubjectHierarchyItem(vtkIdType itemID)const
{
  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return 0.0;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return 0.0;
  }

  // Gamma volume
  vtkMRMLNode* associatedNode = shNode->GetItemDataNode(itemID);
  if ( associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode")
    && associatedNode->GetAttribute(vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_GAMMA_VOLUME_IDENTIFIER_ATTRIBUTE_NAME) )
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
QIcon qSlicerSubjectHierarchyGammaPlugin::icon(vtkIdType itemID)
{
  Q_D(qSlicerSubjectHierarchyGammaPlugin);

  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return QIcon();
  }

  if (this->canOwnSubjectHierarchyItem(itemID))
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
void qSlicerSubjectHierarchyGammaPlugin::setDisplayVisibility(vtkIdType itemID, int visible)
{
  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  if (this->canOwnSubjectHierarchyItem(itemID))
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->setDisplayVisibility(itemID, visible);
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(itemID, visible);
  }
}

//---------------------------------------------------------------------------
int qSlicerSubjectHierarchyGammaPlugin::getDisplayVisibility(vtkIdType itemID)const
{
  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return -1;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return -1;
  }

  if (this->canOwnSubjectHierarchyItem(itemID))
  {
    return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->getDisplayVisibility(itemID);
  }

  // Default
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->getDisplayVisibility(itemID);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyGammaPlugin::editProperties(vtkIdType itemID)
{
  Q_UNUSED(itemID);

  // Switch to dose comparison module
  qSlicerSubjectHierarchyAbstractPlugin::switchToModule("DoseComparison");
}
