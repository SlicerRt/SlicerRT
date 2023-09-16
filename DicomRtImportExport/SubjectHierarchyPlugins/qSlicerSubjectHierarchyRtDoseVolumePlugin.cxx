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
#include "vtkSlicerRtCommon.h"

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
#include <vtkMRMLRTPlanNode.h>

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

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerAbstractModule.h"
#include "qSlicerModuleManager.h"
#include "qSlicerAbstractModuleWidget.h"

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
  QAction* CreateIsodoseAction;
  QAction* CalculateDvhAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRtDoseVolumePluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePluginPrivate::qSlicerSubjectHierarchyRtDoseVolumePluginPrivate(qSlicerSubjectHierarchyRtDoseVolumePlugin& object)
 : q_ptr(&object)
{
  this->DoseVolumeIcon = QIcon(":Icons/DoseVolume.png");

  this->ConvertToRtDoseVolumeAction = nullptr;
  this->CreateIsodoseAction = nullptr;
  this->CalculateDvhAction = nullptr;
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyRtDoseVolumePlugin);

  this->ConvertToRtDoseVolumeAction = new QAction("Convert to RT dose volume...",q);
  QObject::connect(this->ConvertToRtDoseVolumeAction, SIGNAL(triggered()), q, SLOT(convertCurrentNodeToRtDoseVolume()));
  this->CreateIsodoseAction = new QAction("Create isodose surfaces...",q);
  QObject::connect(this->CreateIsodoseAction, SIGNAL(triggered()), q, SLOT(createIsodoseForCurrentItem()));
  this->CalculateDvhAction = new QAction("Calculate DVH...",q);
  QObject::connect(this->CalculateDvhAction, SIGNAL(triggered()), q, SLOT(calculateDvhForCurrentItem()));
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePluginPrivate::~qSlicerSubjectHierarchyRtDoseVolumePluginPrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyRtDoseVolumePlugin methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePlugin::qSlicerSubjectHierarchyRtDoseVolumePlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyRtDoseVolumePluginPrivate(*this) )
{
  this->m_Name = QString("RtDoseVolume");

  Q_D(qSlicerSubjectHierarchyRtDoseVolumePlugin);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyRtDoseVolumePlugin::~qSlicerSubjectHierarchyRtDoseVolumePlugin() = default;

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyRtDoseVolumePlugin::canAddNodeToSubjectHierarchy(
  vtkMRMLNode* node, vtkIdType parentItemID/*=vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID*/)const
{
  Q_UNUSED(parentItemID);
  if (!node)
    {
    qCritical() << Q_FUNC_INFO << ": Input node is nullptr";
    return 0.0;
    }
  else if (vtkSlicerRtCommon::IsDoseVolumeNode(node))
    {
    return 1.0; // Only this plugin can handle this node
    }
  return 0.0;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyRtDoseVolumePlugin::canOwnSubjectHierarchyItem(vtkIdType itemID)const
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

  // RT Dose
  vtkMRMLNode* associatedNode = shNode->GetItemDataNode(itemID);
  if ( associatedNode && vtkSlicerRtCommon::IsDoseVolumeNode(associatedNode) )
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

//-----------------------------------------------------------------------------
QString qSlicerSubjectHierarchyRtDoseVolumePlugin::tooltip(vtkIdType itemID)const
{
  return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->tooltip(itemID);
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyRtDoseVolumePlugin::icon(vtkIdType itemID)
{
  Q_D(qSlicerSubjectHierarchyRtDoseVolumePlugin);

  if (!itemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return QIcon();
  }

  if (this->canOwnSubjectHierarchyItem(itemID))
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
void qSlicerSubjectHierarchyRtDoseVolumePlugin::setDisplayVisibility(vtkIdType itemID, int visible)
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
int qSlicerSubjectHierarchyRtDoseVolumePlugin::getDisplayVisibility(vtkIdType itemID)const
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
QList<QAction*> qSlicerSubjectHierarchyRtDoseVolumePlugin::itemContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyRtDoseVolumePlugin);

  QList<QAction*> actions;
  actions << d->ConvertToRtDoseVolumeAction << d->CreateIsodoseAction << d->CalculateDvhAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePlugin::showContextMenuActionsForItem(vtkIdType itemID)
{
  Q_D(qSlicerSubjectHierarchyRtDoseVolumePlugin);

  if (!itemID)
  {
    // There are no scene actions in this plugin
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  // Volume but not RT dose or labelmap
  vtkMRMLNode* associatedNode = shNode->GetItemDataNode(itemID);
  if ( qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Volumes")->canOwnSubjectHierarchyItem(itemID)
    && !associatedNode->IsA("vtkMRMLLabelMapVolumeNode")
    && m_Name.compare(shNode->GetItemOwnerPluginName(itemID).c_str()) )
  {
    d->ConvertToRtDoseVolumeAction->setVisible(true);
  }
  // Dose volume
  else if (this->canOwnSubjectHierarchyItem(itemID))
  {
    d->CreateIsodoseAction->setVisible(true);
    d->CalculateDvhAction->setVisible(true);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePlugin::convertCurrentNodeToRtDoseVolume()
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (!currentItemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current item";
    return;
  }

  // Get associated volume node
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    shNode->GetItemDataNode(currentItemID) );
  if (!volumeNode)
  {
    qCritical() << Q_FUNC_INFO << ": Data node associated to current item '" << shNode->GetItemName(currentItemID).c_str() << "' is not a volume";
    return;
  }

  // Get study item
  vtkIdType studyItemID = shNode->GetItemAncestorAtLevel(currentItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
  if (!studyItemID)
  {
    QString message("The volume must be under a study in order to be converted to dose. Please drag&drop the volume under a study. If there is no study, it can be created under a subject. Consult the help window for more details.");
    qCritical() << Q_FUNC_INFO << ": Failed to find study item among the ancestors of current item '" << shNode->GetItemName(currentItemID).c_str() << "'! " << message;
    QMessageBox::warning(nullptr, tr("Failed to convert volume to dose"), message);
    return;
  }
  std::string doseUnitNameInStudy = shNode->GetItemAttribute(studyItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME);
  std::string doseUnitValueInStudy = shNode->GetItemAttribute(studyItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME);

  bool referencedInstanceIsSet = false;
  std::vector< vtkIdType > studyChildrenIDs;
  shNode->GetItemChildren(studyItemID, studyChildrenIDs);
  // find RTPlan node and get InstanceUID of that plan to use it as ReferencedInstanceUID for converted RTDose
  for (int childItemID : studyChildrenIDs)
  {
    vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(shNode->GetItemDataNode(childItemID));
    if (planNode)
    {
      std::string planInstanceUID = shNode->GetItemUID(childItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName());
      if (!planInstanceUID.empty())
      {
        shNode->SetItemAttribute(currentItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName(), planInstanceUID.c_str());
        referencedInstanceIsSet = true;
      }
    }
  }
  // no RTPlan Instance UID, use Study series instance UID instead
  if (!referencedInstanceIsSet)
  {
    std::string studyInstanceUID = shNode->GetItemAttribute(studyItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMStudyInstanceUIDAttributeName());
    shNode->SetItemAttribute(currentItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName(), studyInstanceUID.c_str());
  }

  // Show dialogs asking about dose unit name and value
  bool ok;
  QString defaultDoseUnitName(!doseUnitNameInStudy.empty() ? doseUnitNameInStudy.c_str() : "Gy");
  QString doseUnitName = QInputDialog::getText(nullptr, tr("RT dose volume properties (1/2)"),
    tr("Dose unit name:"), QLineEdit::Normal, defaultDoseUnitName, &ok);
  if (!ok || doseUnitName.isEmpty())
  {
    qWarning() << Q_FUNC_INFO << ": Failed to get valid dose unit name from dialog. Check study node attributes.";
  }

  double defaultDoseUnitValue = (!doseUnitValueInStudy.empty() ? QString(doseUnitValueInStudy.c_str()).toDouble() : 1.0);
  double doseUnitValue = QInputDialog::getDouble(nullptr, tr("RT dose volume properties (2/2)"),
    tr("Dose unit value (scaling):\n\nNote: Setting the scaling will NOT apply it to the volume voxels"), defaultDoseUnitValue, EPSILON*EPSILON, 1000.0, 16, &ok);
  if (!ok)
  {
    qWarning() << Q_FUNC_INFO << ": Failed to get valid dose unit value from dialog. Check study node attributes.";
  }

  // Set RT dose volume properties to study node
  bool setDoseUnitName = true;
  if (!doseUnitNameInStudy.empty() && doseUnitName.compare(doseUnitNameInStudy.c_str()))
  {
    QMessageBox::StandardButton answer =
      QMessageBox::question(nullptr, tr("Dose unit name changed"),
      tr("Entered dose unit name is different from the already set dose unit name in study.\n\n"
      "Do you wish to overwrite it? It might produce unwanted results."),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::No)
    {
      setDoseUnitName = false;
    }
  }
  if (setDoseUnitName)
  {
    shNode->SetItemAttribute(studyItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), doseUnitName.toUtf8().constData());
  }
  bool setDoseUnitValue = true;
  if (!doseUnitNameInStudy.empty() && fabs(doseUnitValue - defaultDoseUnitValue) > EPSILON*EPSILON )
  {
    QMessageBox::StandardButton answer =
      QMessageBox::question(nullptr, tr("Dose unit name changed"),
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
    shNode->SetItemAttribute(studyItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str(), doseUnitValueString.toUtf8().constData());
  }

  // Set RT dose identifier attribute to data node
  volumeNode->SetAttribute(vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  shNode->RequestOwnerPluginSearch(currentItemID);
  shNode->ItemModified(currentItemID);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePlugin::createIsodoseForCurrentItem()
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (!currentItemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current item";
    return;
  }

  // Get associated volume node
  vtkMRMLNode* node = shNode->GetItemDataNode(currentItemID);
  if (!vtkSlicerRtCommon::IsDoseVolumeNode(node))
  {
    qCritical() << Q_FUNC_INFO << ": Data node associated to current item '" << shNode->GetItemName(currentItemID).c_str() << "' is not a dose volume";
    return;
  }

  // Get Isodose module
  qSlicerAbstractCoreModule* module = qSlicerApplication::application()->moduleManager()->module("Isodose");
  qSlicerAbstractModule* moduleWithAction = qobject_cast<qSlicerAbstractModule*>(module);
  if (!moduleWithAction)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to get Isodose module";
    return;
    }
  // Select dose volume node in Isodose module
  qSlicerAbstractModuleRepresentation* widget = moduleWithAction->widgetRepresentation();
  if (!widget)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to get Isodose module widget";
    return;
    }
  if (!widget->setEditedNode(node))
    {
    qCritical() << Q_FUNC_INFO << ": Failed to select node " << node->GetName() << " in Isodose module widget";
    }
  // Activate module widget
  moduleWithAction->action()->trigger();
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyRtDoseVolumePlugin::calculateDvhForCurrentItem()
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (!currentItemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current item";
    return;
  }

  // Get associated volume node
  vtkMRMLNode* node = shNode->GetItemDataNode(currentItemID);
  if (!vtkSlicerRtCommon::IsDoseVolumeNode(node))
  {
    qCritical() << Q_FUNC_INFO << ": Data node associated to current item '" << shNode->GetItemName(currentItemID).c_str() << "' is not a dose volume";
    return;
  }

  // Get Dose Volume Histogram module
  qSlicerAbstractCoreModule* module = qSlicerApplication::application()->moduleManager()->module("DoseVolumeHistogram");
  qSlicerAbstractModule* moduleWithAction = qobject_cast<qSlicerAbstractModule*>(module);
  if (!moduleWithAction)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to get Dose Volume Histogram module";
    return;
    }
  // Select dose volume node in Dose Volume Histogram module
  qSlicerAbstractModuleRepresentation* widget = moduleWithAction->widgetRepresentation();
  if (!widget)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to get Dose Volume Histogram module widget";
    return;
    }
  if (!widget->setEditedNode(node))
    {
    qCritical() << Q_FUNC_INFO << ": Failed to select node " << node->GetName() << " in Dose Volume Histogram module widget";
    }
  // Activate module widget
  moduleWithAction->action()->trigger();
}
