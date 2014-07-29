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
#include "qSlicerSubjectHierarchyRtDoseVolumePlugin.h"

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QIcon>
#include <QStandardItem>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerSubjectHierarchyRtDoseVolumePluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyRtDoseVolumePlugin);
protected:
  qSlicerSubjectHierarchyRtDoseVolumePlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyRtDoseVolumePluginPrivate(qSlicerSubjectHierarchyRtDoseVolumePlugin& object);
  ~qSlicerSubjectHierarchyRtDoseVolumePluginPrivate();
  void init();
public:
  QIcon DoseVolumeIcon;

  QAction* ConvertToRtDoseVolumeAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRtDoseVolumePluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePluginPrivate::qSlicerSubjectHierarchyRtDoseVolumePluginPrivate(qSlicerSubjectHierarchyRtDoseVolumePlugin& object)
 : q_ptr(&object)
{
  this->DoseVolumeIcon = QIcon(":Icons/DoseVolume.png");

  this->ConvertToRtDoseVolumeAction = NULL;
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePluginPrivate::~qSlicerSubjectHierarchyRtDoseVolumePluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePlugin::qSlicerSubjectHierarchyRtDoseVolumePlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRtDoseVolumePluginPrivate(*this) )
{
  this->m_Name = QString("RtDoseVolume");

  Q_D(qSlicerSubjectHierarchyRtDoseVolumePlugin);
  d->init();
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyRtDoseVolumePlugin);

  this->ConvertToRtDoseVolumeAction = new QAction("Convert to RT dose volume...",q);
  QObject::connect(this->ConvertToRtDoseVolumeAction, SIGNAL(triggered()), q, SLOT(convertCurrentNodeToRtDoseVolume()));
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePlugin::~qSlicerSubjectHierarchyRtDoseVolumePlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRtDoseVolumePlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedNode();

  // RT Dose
  if ( associatedNode && SlicerRtCommon::IsDoseVolumeNode(associatedNode) )
  {
    return 1.0; // Only this plugin can handle this node
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyRtDoseVolumePlugin::roleForPlugin()const
{
  return "RT dose volume";
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRtDoseVolumePlugin::icon(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::icon: NULL node given!";
    return QIcon();
  }

  Q_D(qSlicerSubjectHierarchyRtDoseVolumePlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    return d->DoseVolumeIcon;
  }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRtDoseVolumePlugin::visibilityIcon(int visible)
{
  return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->visibilityIcon(visible);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::setDisplayVisibility: NULL node!";
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
int qSlicerSubjectHierarchyRtDoseVolumePlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::getDisplayVisibility: NULL node!";
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
void qSlicerSubjectHierarchyRtDoseVolumePlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->editProperties(node);
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyRtDoseVolumePlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyRtDoseVolumePlugin);

  QList<QAction*> actions;
  actions << d->ConvertToRtDoseVolumeAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePlugin::showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyRtDoseVolumePlugin);
  this->hideAllContextMenuActions();

  if (!node)
  {
    // There are no scene actions in this plugin
    return;
  }

  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();

  // Volume but not RT dose or labelmap
  if ( qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->canOwnSubjectHierarchyNode(node)
    && !SlicerRtCommon::IsLabelmapVolumeNode(node->GetAssociatedNode()) && m_Name.compare(node->GetOwnerPluginName()) )
  {
    d->ConvertToRtDoseVolumeAction->setVisible(true);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePlugin::convertCurrentNodeToRtDoseVolume()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!currentNode)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::convertCurrentNodeToRtDoseVolume: Invalid current node!";
    return;
  }

  // Get associated volume node
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    currentNode->GetAssociatedNode() );
  if (!volumeNode)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::convertCurrentNodeToRtDoseVolume: Data node associated to current node '" << currentNode->GetNameWithoutPostfix().c_str() << "' is not a volume!";
    return;
  }

  // Get study node
  vtkMRMLSubjectHierarchyNode* studyNode = currentNode->GetAncestorAtLevel(vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY);
  if (!studyNode)
  {
    qCritical() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::convertCurrentNodeToRtDoseVolume: Failed to find study node among the ancestors of current node '" << currentNode->GetNameWithoutPostfix().c_str() << "'!";
    return;
  }
  const char* doseUnitNameInStudy = studyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());
  const char* doseUnitValueInStudy = studyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str());

  // Show dialogs asking about dose unit name and value
  bool ok;
  QString defaultDoseUnitName = (doseUnitNameInStudy ? QString(doseUnitNameInStudy) : "GY");
  QString doseUnitName = QInputDialog::getText(NULL, tr("RT dose volume properties (1/2)"),
    tr("Dose unit name:"), QLineEdit::Normal, defaultDoseUnitName, &ok);
  if (!ok || doseUnitName.isEmpty())
  {
    qWarning() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::convertCurrentNodeToRtDoseVolume(): Failed to get valid dose unit name from dialog. Check study node attributes.";
  }

  double defaultDoseUnitValue = (doseUnitValueInStudy ? QString(doseUnitValueInStudy).toDouble() : 1.0);
  double doseUnitValue = QInputDialog::getDouble(NULL, tr("RT dose volume properties (2/2)"),
    tr("Dose unit value (scaling):"), defaultDoseUnitValue, EPSILON*EPSILON, 1000.0, 16, &ok);
  if (!ok)
  {
    qWarning() << "qSlicerSubjectHierarchyRtDoseVolumePlugin::convertCurrentNodeToRtDoseVolume(): Failed to get valid dose unit value from dialog. Check study node attributes.";
  }

  // Set RT dose volume properties to study node
  bool setDoseUnitName = true;
  if (doseUnitNameInStudy && doseUnitName.compare(doseUnitNameInStudy))
  {
    QMessageBox::StandardButton answer =
      QMessageBox::question(NULL, tr("Dose unit name changed"),
      tr("Entered dose unit name is different from the already set dose unit name in study.\n\n"
      "Do you wish to overwrite it? Might result in unwanted results."),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::No)
    {
      setDoseUnitName = false;
    }
  }
  if (setDoseUnitName)
  {
    studyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), doseUnitName.toLatin1().constData());
  }

  bool setDoseUnitValue = true;
  if ( doseUnitValueInStudy
    && fabs(doseUnitValue - QString(doseUnitValueInStudy).toDouble()) > EPSILON*EPSILON )
  {
    QMessageBox::StandardButton answer =
      QMessageBox::question(NULL, tr("Dose unit name changed"),
      tr("Entered dose unit value is different from the already set dose unit value in study.\n\n"
      "Do you wish to overwrite it? Might result in unwanted results."),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::No)
    {
      setDoseUnitValue = false;
    }
  }
  if (setDoseUnitValue)
  {
    QString doseUnitValueString = QString::number(doseUnitValue);
    studyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str(), doseUnitValueString.toLatin1().constData());
  }

  // Set RT dose identifier attribute to data node
  volumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  currentNode->Modified();
}
