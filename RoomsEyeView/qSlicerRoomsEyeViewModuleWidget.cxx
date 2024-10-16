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

  The collision detection module was partly supported by Conselleria de
  Educación, Investigación, Cultura y Deporte (Generalitat Valenciana), Spain
  under grant number CDEIGENT/2019/011.

==============================================================================*/

// Room's Eye View includes
#include "qSlicerRoomsEyeViewModuleWidget.h"
#include "ui_qSlicerRoomsEyeViewModule.h"

#include "vtkSlicerRoomsEyeViewModuleLogic.h"
#include "vtkMRMLRoomsEyeViewNode.h"

// Beams includes
#include "vtkSlicerIECTransformLogic.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// Slicer includes
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qSlicerIOManager.h>
#include <qSlicerDataDialog.h>
#include <qSlicerSaveDataDialog.h>
#include <qSlicerSubjectHierarchyFolderPlugin.h>
#include <qSlicerSubjectHierarchyPluginHandler.h>
#include <qMRMLSliceWidget.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLThreeDView.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLCameraNode.h>
#include <vtkMRMLViewNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// Qt includes
#include <QDebug>
#include <QDir>
#include <QFileDialog>

// CTK includes
#include <ctkMessageBox.h>
#include <ctkSliderWidget.h>

// VTK includes
#include <vtkCamera.h>
#include "vtkCollisionDetectionFilter.h"
#include <vtkPolyData.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RoomsEyeView
class qSlicerRoomsEyeViewModuleWidgetPrivate : public Ui_qSlicerRoomsEyeViewModule
{
  Q_DECLARE_PUBLIC(qSlicerRoomsEyeViewModuleWidget);
protected:
  qSlicerRoomsEyeViewModuleWidget* const q_ptr;
public:
  qSlicerRoomsEyeViewModuleWidgetPrivate(qSlicerRoomsEyeViewModuleWidget& object);
  ~qSlicerRoomsEyeViewModuleWidgetPrivate()  = default;
  vtkSmartPointer<vtkSlicerRoomsEyeViewModuleLogic> logic() const;
  vtkMRMLCameraNode* get3DViewCameraNode() const;
  qMRMLLayoutManager* getLayoutManager() const;

  vtkMRMLRTPlanNode* currentPlanNode(vtkMRMLRoomsEyeViewNode* paramNode);

  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerRoomsEyeViewModuleWidgetPrivate::qSlicerRoomsEyeViewModuleWidgetPrivate(qSlicerRoomsEyeViewModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkSlicerRoomsEyeViewModuleLogic> qSlicerRoomsEyeViewModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerRoomsEyeViewModuleWidget);
  return vtkSlicerRoomsEyeViewModuleLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
vtkMRMLRTPlanNode* qSlicerRoomsEyeViewModuleWidgetPrivate::currentPlanNode(vtkMRMLRoomsEyeViewNode* paramNode)
{
  if (!paramNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node given";
    return nullptr;
  }

  vtkMRMLRTBeamNode* beamNode = paramNode->GetBeamNode();
  if (beamNode)
  {
    return beamNode->GetParentPlanNode();
  }

  vtkMRMLSegmentationNode* segmentationNode = paramNode->GetPatientBodySegmentationNode();
  if (segmentationNode)
  {
    // Try to get the plan from the same study
    vtkMRMLScene* scene = segmentationNode->GetScene();
    vtkMRMLSubjectHierarchyNode* shNode = scene->GetSubjectHierarchyNode();
    vtkIdType segmentationItemID = shNode->GetItemByDataNode(segmentationNode);
    vtkIdType studyItemID = shNode->GetItemAncestorAtLevel(segmentationItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    if (studyItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
      qCritical() << Q_FUNC_INFO << ": Failed to find current plan, because there is no beam in the parameter node, and the patient segmentation is not in a study";
      return nullptr;
    }
    std::vector<vtkIdType> studyItemIDs;
    shNode->GetItemChildren(shNode->GetSceneItemID(), studyItemIDs, true);
    std::vector<vtkIdType>::iterator itemIt;
    for (itemIt=studyItemIDs.begin(); itemIt!=studyItemIDs.end(); ++itemIt)
    {
      vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(shNode->GetItemDataNode(*itemIt));
      if (planNode)
      {
        return planNode;
      }
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
vtkMRMLCameraNode* qSlicerRoomsEyeViewModuleWidgetPrivate::get3DViewCameraNode() const
{
  Q_Q(const qSlicerRoomsEyeViewModuleWidget);

  // Get 3D view node
  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  if (!layoutManager->threeDViewCount())
  {
    return nullptr;
  }

  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();

  // Get camera node for view
  vtkCollection* cameras = q->mrmlScene()->GetNodesByClass("vtkMRMLCameraNode");
  vtkMRMLCameraNode* cameraNode = nullptr;
  for (int i = 0; i < cameras->GetNumberOfItems(); i++)
  {
    cameraNode = vtkMRMLCameraNode::SafeDownCast(cameras->GetItemAsObject(i));
    std::string viewUniqueName = std::string(viewNode->GetNodeTagName()) + cameraNode->GetLayoutName();
    if (viewUniqueName == viewNode->GetID())
    {
      break;
    }
  }
  if (!cameraNode)
  {
    qCritical() << Q_FUNC_INFO << "Failed to find camera for view " << (viewNode ? viewNode->GetID() : "(null)");
  }
  cameras->Delete();
  return cameraNode;
}

//-----------------------------------------------------------------------------
qMRMLLayoutManager* qSlicerRoomsEyeViewModuleWidgetPrivate::getLayoutManager() const
{
  Q_Q(const qSlicerRoomsEyeViewModuleWidget);

  // Get 3D view node
  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  return slicerApplication->layoutManager();
}

//-----------------------------------------------------------------------------
// qSlicerRoomsEyeViewModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerRoomsEyeViewModuleWidget::qSlicerRoomsEyeViewModuleWidget(QWidget* _parent)
: Superclass(_parent)
, d_ptr(new qSlicerRoomsEyeViewModuleWidgetPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerRoomsEyeViewModuleWidget::~qSlicerRoomsEyeViewModuleWidget() = default;

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  this->Superclass::setMRMLScene(scene);
  qvtkReconnect(d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()));
  qvtkReconnect(d->logic(), scene, vtkMRMLScene::EndCloseEvent, this, SLOT(onSceneClosedEvent()));

  // Find parameters node or create it if there is none in the scene
  if (scene)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLRoomsEyeViewNode");
    if (d->MRMLNodeComboBox_ParameterSet->currentNode())
    {
      this->setParameterNode(d->MRMLNodeComboBox_ParameterSet->currentNode());
    }
    else if (node)
    {
      this->setParameterNode(node);
    }
    else
    {
      vtkSmartPointer<vtkMRMLRoomsEyeViewNode> newNode = vtkSmartPointer<vtkMRMLRoomsEyeViewNode>::New();
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onSceneClosedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  Q_D(qSlicerRoomsEyeViewModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic";
    return;
  }

  // Select or create parameter node
  this->setMRMLScene(this->mrmlScene());

  d->ModuleWindowInitialized = true;
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

  // Each time the node is modified, the UI widgets are updated
  qvtkReconnect(paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  // Set selected MRML nodes in comboboxes in the parameter set if it was nullptr there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (paramNode)
  {
    if (!paramNode->GetBeamNode())
    {
      paramNode->SetAndObserveBeamNode(vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_Beam->currentNode()));
    }
    if (!paramNode->GetPatientBodySegmentationNode())
    {
      paramNode->SetAndObservePatientBodySegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(d->SegmentSelectorWidget_PatientBody->currentNode()));
    }
    if (!paramNode->GetPatientBodySegmentID() && !d->SegmentSelectorWidget_PatientBody->currentSegmentID().isEmpty())
    {
      paramNode->SetPatientBodySegmentID(d->SegmentSelectorWidget_PatientBody->currentSegmentID().toUtf8().constData());
    }
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  if (paramNode && this->mrmlScene())
  {
    if (paramNode->GetBeamNode())
    {
      d->MRMLNodeComboBox_Beam->setCurrentNode(paramNode->GetBeamNode());
    }
    if (paramNode->GetPatientBodySegmentationNode())
    {
      d->SegmentSelectorWidget_PatientBody->setCurrentNode(paramNode->GetPatientBodySegmentationNode());
    }
    if (paramNode->GetPatientBodySegmentID())
    {
      d->SegmentSelectorWidget_PatientBody->setCurrentSegmentID(paramNode->GetPatientBodySegmentID());
    }

    d->GantryRotationSlider->setValue(paramNode->GetGantryRotationAngle());
    d->CollimatorRotationSlider->setValue(paramNode->GetCollimatorRotationAngle());
    d->PatientSupportRotationSlider->setValue(paramNode->GetPatientSupportRotationAngle());
    d->VerticalTableTopDisplacementSlider->setValue(paramNode->GetVerticalTableTopDisplacement());
    d->LongitudinalTableTopDisplacementSlider->setValue(paramNode->GetLongitudinalTableTopDisplacement());
    d->LateralTableTopDisplacementSlider->setValue(paramNode->GetLateralTableTopDisplacement());
    d->ImagingPanelMovementSlider->setValue(paramNode->GetImagingPanelMovement());
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setup()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  d->setupUi(this);

  this->Superclass::setup();

  this->setMRMLScene(this->mrmlScene());

  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  ioManager->registerDialog(new qSlicerDataDialog(this));
  ioManager->registerDialog(new qSlicerSaveDataDialog(this));

  // Add treatment machine options
  d->TreatmentMachineComboBox->addItem("Varian TrueBeam STx", "VarianTrueBeamSTx");
  d->TreatmentMachineComboBox->addItem("Siemens Artiste", "SiemensArtiste");
  d->TreatmentMachineComboBox->addItem("From file...", "FromFile");

  //
  // Make connections
  connect(d->LoadTreatmentMachineButton, SIGNAL(clicked()), this, SLOT(onLoadTreatmentMachineButtonClicked()));

  // Treatment machine components
  connect(d->CollimatorRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(onCollimatorRotationSliderValueChanged(double)));
  connect(d->GantryRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(onGantryRotationSliderValueChanged(double)));
  connect(d->ImagingPanelMovementSlider, SIGNAL(valueChanged(double)), this, SLOT(onImagingPanelMovementSliderValueChanged(double)));
  connect(d->PatientSupportRotationSlider, SIGNAL(valueChanged(double)), this, SLOT(onPatientSupportRotationSliderValueChanged(double)));
  connect(d->VerticalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(onVerticalTableTopDisplacementSliderValueChanged(double)));
  connect(d->LongitudinalTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(onLongitudinalTableTopDisplacementSliderValueChanged(double)));
  connect(d->LateralTableTopDisplacementSlider, SIGNAL(valueChanged(double)), this, SLOT(onLateralTableTopDisplacementSliderValueChanged(double)));

  connect(d->BeamsEyeViewButton, SIGNAL(clicked()), this, SLOT(onBeamsEyeViewButtonClicked()));

  connect(d->MRMLNodeComboBox_Beam, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onBeamNodeChanged(vtkMRMLNode*)));
  connect(d->SegmentSelectorWidget_PatientBody, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onPatientBodySegmentationNodeChanged(vtkMRMLNode*)));
  connect(d->SegmentSelectorWidget_PatientBody, SIGNAL(currentSegmentChanged(QString)), this, SLOT(onPatientBodySegmentChanged(QString)));

  // 3D camera control
  connect(d->FixedCameraCheckBox, SIGNAL(toggled(bool)), this, SLOT(setFixedReferenceCameraEnabled(bool)));

  // Disable treatment machine geometry controls until a machine is loaded
  d->GantryRotationSlider->setEnabled(false);
  d->CollimatorRotationSlider->setEnabled(false);
  d->PatientSupportRotationSlider->setEnabled(false);
  d->VerticalTableTopDisplacementSlider->setEnabled(false);
  d->LongitudinalTableTopDisplacementSlider->setEnabled(false);
  d->LateralTableTopDisplacementSlider->setEnabled(false);
  d->ImagingPanelMovementSlider->setEnabled(false);

  // Handle scene change event if occurs
  qvtkConnect(d->logic(), vtkCommand::ModifiedEvent, this, SLOT(onLogicModified()));
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onBeamNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy";
    return;
  }

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(node);
  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveBeamNode(beamNode);
  paramNode->DisableModifiedEventOff();

  // Show only selected beam, hide others
  std::vector<vtkMRMLNode*> beamNodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLRTBeamNode", beamNodes);
  for (std::vector<vtkMRMLNode*>::iterator beamIt=beamNodes.begin(); beamIt!=beamNodes.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* currentBeamNode = vtkMRMLRTBeamNode::SafeDownCast(*beamIt);
    shNode->SetItemDisplayVisibility(
      shNode->GetItemByDataNode(currentBeamNode), (currentBeamNode == beamNode ? 1 : 0) );
  }

  if (!beamNode)
  {
    return;
  }

  // Trigger update of transforms based on selected beam
  beamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);

  // Select patient segmentation
  vtkMRMLRTPlanNode* planNode = beamNode->GetParentPlanNode();
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access parent plan of beam " << beamNode->GetName();
    return;
  }
  d->SegmentSelectorWidget_PatientBody->setCurrentNode(planNode->GetSegmentationNode());
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onPatientBodySegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy";
    return;
  }

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObservePatientBodySegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  // Show only selected patient segmentation
  std::vector<vtkMRMLNode*> segmentationNodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLSegmentationNode", segmentationNodes);
  for (std::vector<vtkMRMLNode*>::iterator segIt=segmentationNodes.begin(); segIt!=segmentationNodes.end(); ++segIt)
  {
    vtkMRMLSegmentationNode* currentSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(*segIt);
    shNode->SetItemDisplayVisibility(
      shNode->GetItemByDataNode(currentSegmentationNode), (currentSegmentationNode == node ? 1 : 0) );
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onPatientBodySegmentChanged(QString segmentID)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || segmentID.isEmpty() || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetPatientBodySegmentID(segmentID.toUtf8().constData());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLoadTreatmentMachineButtonClicked()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!d->ModuleWindowInitialized)
  {
    return;
  }

  // Get treatment machine descriptor file path
  QString treatmentMachineType(d->TreatmentMachineComboBox->currentData().toString());
  QString descriptorFilePath;
  if (!treatmentMachineType.compare("FromFile"))
  {
    // Ask user for descriptor JSON file if load from file option is selected
    descriptorFilePath = QFileDialog::getOpenFileName( this, "Select treatment machine descriptor JSON file...",
      QString(), "Json files (*.json);; All files (*)" );
  }
  else //TODO: Currently support two default types in addition to loading file. Need to rethink the module
  {
    QString relativeFilePath = QString("%1/%2.json").arg(treatmentMachineType).arg(treatmentMachineType);
    descriptorFilePath = QDir(d->logic()->GetModuleShareDirectory().c_str()).filePath(relativeFilePath);
  }

  this->loadTreatmentMachineFromFile(descriptorFilePath);
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::loadTreatmentMachineFromFile(QString descriptorFilePath, QString treatmentMachineType/*=""*/)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }

  // Check if there is a machine already loaded and ask user what to do if so
  vtkMRMLSubjectHierarchyNode* shNode = this->mrmlScene()->GetSubjectHierarchyNode();
  std::vector<vtkIdType> allItemIDs;
  shNode->GetItemChildren(shNode->GetSceneItemID(), allItemIDs, true);
  std::vector<vtkIdType> machineFolderItemIDs;
  std::vector<vtkIdType>::iterator itemIt;
  for (itemIt=allItemIDs.begin(); itemIt!=allItemIDs.end(); ++itemIt)
  {
    std::string machineDescriptorFilePath = shNode->GetItemAttribute(*itemIt,
      vtkSlicerRoomsEyeViewModuleLogic::TREATMENT_MACHINE_DESCRIPTOR_FILE_PATH_ATTRIBUTE_NAME);
    if (!machineDescriptorFilePath.compare(descriptorFilePath.toUtf8().constData()))
    {
      QMessageBox::warning(this, tr("Machine already loaded"), tr("This treatment machine is already loaded."));
      return;
    }
    if (!machineDescriptorFilePath.empty())
    {
      machineFolderItemIDs.push_back(*itemIt);
    }
  }

  // Ask user what do to if a machine is already loaded
  if (machineFolderItemIDs.size() > 0)
  {
    ctkMessageBox* existingMachineMsgBox = new ctkMessageBox(this);
    existingMachineMsgBox->setWindowTitle(tr("Other machines loaded"));
    existingMachineMsgBox->setText(tr("There is another treatment machine loaded in the scene. Would you like to hide or delete it?"));

    existingMachineMsgBox->addButton(tr("Hide"), QMessageBox::AcceptRole);
    existingMachineMsgBox->addButton(tr("Delete"), QMessageBox::DestructiveRole);
    existingMachineMsgBox->addButton(tr("No action"), QMessageBox::RejectRole);

    existingMachineMsgBox->setDontShowAgainVisible(true);
    existingMachineMsgBox->setDontShowAgainSettingsKey("SlicerRT/DontAskOnMultipleTreatmentMachines");
    existingMachineMsgBox->setIcon(QMessageBox::Question);
    existingMachineMsgBox->exec();
    int resultCode = existingMachineMsgBox->buttonRole(existingMachineMsgBox->clickedButton());
    if (resultCode == QMessageBox::AcceptRole)
    {
      qSlicerSubjectHierarchyFolderPlugin* folderPlugin = qobject_cast<qSlicerSubjectHierarchyFolderPlugin*>(
        qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Folder") );
      for (itemIt=machineFolderItemIDs.begin(); itemIt!=machineFolderItemIDs.end(); ++itemIt)
      {
        folderPlugin->setDisplayVisibility(*itemIt, false);
      }
    }
    else if (resultCode == QMessageBox::DestructiveRole)
    {
      for (itemIt=machineFolderItemIDs.begin(); itemIt!=machineFolderItemIDs.end(); ++itemIt)
      {
        shNode->RemoveItem(*itemIt);
      }
    }
  }

  // Load and setup models
  paramNode->SetTreatmentMachineDescriptorFilePath(descriptorFilePath.toUtf8().constData());

  std::vector<vtkSlicerRoomsEyeViewModuleLogic::TreatmentMachinePartType> loadedParts =
    d->logic()->LoadTreatmentMachine(paramNode);

  // Warn the user if collision detection is disabled for certain part pairs
  QString disabledCollisionDetectionMessage(
    tr("Collision detection for the following part pairs may take very long due to high triangle numbers:\n\n"));
  bool collisionDetectionDisabled = false;
  if (d->logic()->GetGantryTableTopCollisionDetection()->GetInputData(0) == nullptr)
  {
    disabledCollisionDetectionMessage.append("Gantry-TableTop\n");
    collisionDetectionDisabled = true;
  }
  if (d->logic()->GetGantryPatientSupportCollisionDetection()->GetInputData(0) == nullptr)
  {
    disabledCollisionDetectionMessage.append("Gantry-PatientSupport\n");
    collisionDetectionDisabled = true;
  }
  if (d->logic()->GetCollimatorTableTopCollisionDetection()->GetInputData(0) == nullptr)
  {
    disabledCollisionDetectionMessage.append("Collimator-TableTop\n");
    collisionDetectionDisabled = true;
  }
  disabledCollisionDetectionMessage.append(tr("\nWhat would you like to do?"));
  if (collisionDetectionDisabled)
  {
    ctkMessageBox* existingMachineMsgBox = new ctkMessageBox(this);
    existingMachineMsgBox->setWindowTitle(tr("Collision detection might take too long"));
    existingMachineMsgBox->setText(disabledCollisionDetectionMessage);
    existingMachineMsgBox->addButton(tr("Disable on these part pairs"), QMessageBox::AcceptRole);
    existingMachineMsgBox->addButton(tr("Calculate anyway"), QMessageBox::RejectRole);
    existingMachineMsgBox->setIcon(QMessageBox::Warning);
    existingMachineMsgBox->exec();
    int resultCode = existingMachineMsgBox->buttonRole(existingMachineMsgBox->clickedButton());
    if (resultCode == QMessageBox::RejectRole)
    {
      // Set up treatment machine models again but make sure collision detection is not disabled between any parts
      d->logic()->SetupTreatmentMachineModels(paramNode, true);
    }
  }

  // Set treatment machine dependent properties  //TODO: Use degrees of freedom from JSON
  if (!treatmentMachineType.compare("VarianTrueBeamSTx"))
  {
    d->LateralTableTopDisplacementSlider->setMinimum(-230.0);
    d->LateralTableTopDisplacementSlider->setMaximum(230.0);
  }
  else if (!treatmentMachineType.compare("SiemensArtiste"))
  {
    d->LateralTableTopDisplacementSlider->setMinimum(-250.0);
    d->LateralTableTopDisplacementSlider->setMaximum(250.0);
  }

  // Reset camera
  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  slicerApplication->layoutManager()->resetThreeDViews();

  // Enable treatment machine geometry controls
  d->GantryRotationSlider->setEnabled(true);
  d->CollimatorRotationSlider->setEnabled(true);
  d->PatientSupportRotationSlider->setEnabled(true);
  d->VerticalTableTopDisplacementSlider->setEnabled(true);
  d->LongitudinalTableTopDisplacementSlider->setEnabled(true);
  d->LateralTableTopDisplacementSlider->setEnabled(true);
  d->ImagingPanelMovementSlider->setEnabled(true);

  // Hide controls that do not have corresponding parts loaded
  bool imagingPanelsLoaded = (std::find(loadedParts.begin(), loadedParts.end(), vtkSlicerRoomsEyeViewModuleLogic::ImagingPanelLeft) != loadedParts.end() ||
      std::find(loadedParts.begin(), loadedParts.end(), vtkSlicerRoomsEyeViewModuleLogic::ImagingPanelRight) != loadedParts.end());
  d->labelImagingPanel->setVisible(imagingPanelsLoaded);
  d->ImagingPanelMovementSlider->setVisible(imagingPanelsLoaded);

  // Set orientation marker
  //TODO: Add new option 'Treatment room' to orientation marker choices and merged model with actual colors (surface scalars?)
  //vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();
  //viewNode->SetOrientationMarkerHumanModelNodeID(this->mrmlScene()->GetFirstNodeByName("EBRTOrientationMarkerModel")->GetID());
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onCollimatorRotationSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }
  d->getLayoutManager()->pauseRender();

  paramNode->DisableModifiedEventOn();
  paramNode->SetCollimatorRotationAngle(value);
  paramNode->DisableModifiedEventOff();

  // Update IEC transform
  d->logic()->UpdateCollimatorToGantryTransform(paramNode);

  // Update beam parameter
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(paramNode->GetBeamNode());
  if (beamNode)
  {
    beamNode->SetCollimatorAngle(value);
  }

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
  d->getLayoutManager()->resumeRender();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onGantryRotationSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }
  d->getLayoutManager()->pauseRender();

  paramNode->DisableModifiedEventOn();
  paramNode->SetGantryRotationAngle(value);
  paramNode->DisableModifiedEventOff();

  // Update IEC transform
  d->logic()->UpdateGantryToFixedReferenceTransform(paramNode);

  // Update beam parameter
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(paramNode->GetBeamNode());
  if (beamNode)
  {
    beamNode->SetGantryAngle(value);
  }

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
  d->getLayoutManager()->resumeRender();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onImagingPanelMovementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }
  d->getLayoutManager()->pauseRender();

  paramNode->DisableModifiedEventOn();
  paramNode->SetImagingPanelMovement(value);
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateImagingPanelMovementTransforms(paramNode);

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
  d->getLayoutManager()->resumeRender();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onPatientSupportRotationSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }
  vtkSlicerBeamsModuleLogic* beamsLogic = vtkSlicerBeamsModuleLogic::SafeDownCast(d->logic()->GetModuleLogic("Beams"));
  if (!beamsLogic)
  {
    return;
  }
  d->getLayoutManager()->pauseRender();

  paramNode->DisableModifiedEventOn();
  paramNode->SetPatientSupportRotationAngle(value);
  paramNode->DisableModifiedEventOff();

  // Update IEC transform
  d->logic()->UpdatePatientSupportRotationToFixedReferenceTransform(paramNode);
  beamsLogic->UpdateRASRelatedTransforms(d->logic()->GetIECLogic(), d->currentPlanNode(paramNode));

  // Update beam parameter
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(paramNode->GetBeamNode());
  if (beamNode)
  {
    // since slider range [-180, 180], then negative patient support angles must be
    // transformed into positive one
    if (value < 0.)
    {
      beamNode->SetCouchAngle(360. + value);
    }
    else
    {
      beamNode->SetCouchAngle(value);
    }
  }

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
  d->getLayoutManager()->resumeRender();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onVerticalTableTopDisplacementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }
  vtkSlicerBeamsModuleLogic* beamsLogic = vtkSlicerBeamsModuleLogic::SafeDownCast(d->logic()->GetModuleLogic("Beams"));
  if (!beamsLogic)
  {
    return;
  }
  d->getLayoutManager()->pauseRender();

  paramNode->DisableModifiedEventOn();
  paramNode->SetVerticalTableTopDisplacement(value);
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdatePatientSupportToPatientSupportRotationTransform(paramNode);
  d->logic()->UpdateTableTopToTableTopEccentricRotationTransform(paramNode);
  beamsLogic->UpdateRASRelatedTransforms(d->logic()->GetIECLogic(), d->currentPlanNode(paramNode));

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
  d->getLayoutManager()->resumeRender();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLongitudinalTableTopDisplacementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }
  vtkSlicerBeamsModuleLogic* beamsLogic = vtkSlicerBeamsModuleLogic::SafeDownCast(d->logic()->GetModuleLogic("Beams"));
  if (!beamsLogic)
  {
    return;
  }
  d->getLayoutManager()->pauseRender();

  paramNode->DisableModifiedEventOn();
  paramNode->SetLongitudinalTableTopDisplacement(value);
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateTableTopToTableTopEccentricRotationTransform(paramNode);
  beamsLogic->UpdateRASRelatedTransforms(d->logic()->GetIECLogic(), d->currentPlanNode(paramNode));

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
  d->getLayoutManager()->resumeRender();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onLateralTableTopDisplacementSliderValueChanged(double value)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  Q_UNUSED(value);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }
  vtkSlicerBeamsModuleLogic* beamsLogic = vtkSlicerBeamsModuleLogic::SafeDownCast(d->logic()->GetModuleLogic("Beams"));
  if (!beamsLogic)
  {
    return;
  }
  d->getLayoutManager()->pauseRender();

  paramNode->DisableModifiedEventOn();
  paramNode->SetLateralTableTopDisplacement(d->LateralTableTopDisplacementSlider->value());
  paramNode->DisableModifiedEventOff();

  d->logic()->UpdateTableTopToTableTopEccentricRotationTransform(paramNode);
  beamsLogic->UpdateRASRelatedTransforms(d->logic()->GetIECLogic(), d->currentPlanNode(paramNode));

  this->checkForCollisions();
  this->updateTreatmentOrientationMarker();
  d->getLayoutManager()->resumeRender();
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::onBeamsEyeViewButtonClicked()
{
  //TODO: Move feature to beams module

  Q_D(qSlicerRoomsEyeViewModuleWidget);

  // Get 3D view node camera
  vtkMRMLCameraNode* cameraNode = d->get3DViewCameraNode();
  if (!cameraNode)
  {
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_Beam->currentNode());
  double sourcePosition[3] = {0.0, 0.0, 0.0};
  double isocenter[3] = {0.0, 0.0, 0.0};

  if (beamNode && beamNode->GetSourcePosition(sourcePosition))
  {
    vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
    vtkTransform* beamTransform = nullptr;
    vtkNew<vtkMatrix4x4> mat;
    mat->Identity();

    if (beamTransformNode)
    {
      beamTransform = vtkTransform::SafeDownCast(beamTransformNode->GetTransformToParent());
      beamTransform->GetMatrix(mat);
    }
    else
    {
      qCritical() << Q_FUNC_INFO << "Beam transform node is invalid";
      return;
    }

    double viewUpVector[4] = { -1., 0., 0., 0. }; // beam negative X-axis
    double vup[4];

    mat->MultiplyPoint( viewUpVector, vup);
    //vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetFirstNodeByName("CollimatorModel"));
    //vtkPolyData* collimatorModelPolyData = collimatorModel->GetPolyData();

    //double collimatorCenterOfRotation[3] = {0.0, 0.0, 0.0};
    //double collimatorModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

    //collimatorModelPolyData->GetBounds(collimatorModelBounds);
    //collimatorCenterOfRotation[0] = (collimatorModelBounds[0] + collimatorModelBounds[1]) / 2;
    //collimatorCenterOfRotation[1] = (collimatorModelBounds[2] + collimatorModelBounds[3]) / 2;
    //collimatorCenterOfRotation[2] = collimatorModelBounds[4];

    //cameraNode->GetCamera()->SetPosition(collimatorCenterOfRotation);
    cameraNode->GetCamera()->SetPosition(sourcePosition);
    if (beamNode->GetPlanIsocenterPosition(isocenter))
    {
      cameraNode->GetCamera()->SetFocalPoint(isocenter);
    }
    cameraNode->SetViewUp(vup);
  }

  cameraNode->GetCamera()->Elevation(-(d->GantryRotationSlider->value()));

  //TODO: Oblique slice updating real-time based on beam geometry
  //vtkMRMLSliceNode* redSliceNode = redSliceWidget->mrmlSliceNode();
  //redSliceNode->SetSliceVisible(1);

  //TODO: Camera roll also needs to be set to keep the field of view aligned with the beam's field
  //redSliceNode->SetWidgetNormalLockedToCamera(cameraNode->GetCamera()->GetID);
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::checkForCollisions()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  d->CollisionDetectionStatusLabel->setText(QString::fromStdString("Calculating collisions..."));
  d->CollisionDetectionStatusLabel->setStyleSheet("color: black");
  QApplication::processEvents();

  std::string collisionString = d->logic()->CheckForCollisions(paramNode);

  if (collisionString.length() > 0)
  {
    d->CollisionDetectionStatusLabel->setText(QString::fromStdString(collisionString));
    d->CollisionDetectionStatusLabel->setStyleSheet("color: red");
  }
  else
  {
    QString noCollisionsMessage(tr("No collisions detected"));
    if (d->logic()->GetGantryTableTopCollisionDetection()->GetInputData(0) == nullptr
     || d->logic()->GetGantryPatientSupportCollisionDetection()->GetInputData(0) == nullptr
     || d->logic()->GetCollimatorTableTopCollisionDetection()->GetInputData(0) == nullptr)
    {
      noCollisionsMessage.append(tr(" (excluding certain parts)"));
    }
    d->CollisionDetectionStatusLabel->setText(noCollisionsMessage);
    d->CollisionDetectionStatusLabel->setStyleSheet("color: green");
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::updateTreatmentOrientationMarker()
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);
  vtkMRMLRoomsEyeViewNode* paramNode = vtkMRMLRoomsEyeViewNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  // Get 3D view node
  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();

  // Update orientation marker if shown
  if (viewNode->GetOrientationMarkerType() == vtkMRMLViewNode::OrientationMarkerTypeHuman)
  {
    vtkMRMLModelNode* orientationMarkerModel = d->logic()->UpdateTreatmentOrientationMarker(paramNode);
    if (!orientationMarkerModel)
    {
      qCritical() << Q_FUNC_INFO << ": Failed to create orientation marker model";
      return;
    }

    // Make sure the orientation marker has the right model node
    viewNode->SetOrientationMarkerHumanModelNodeID(orientationMarkerModel->GetID());
    viewNode->Modified();
  }
}

//-----------------------------------------------------------------------------
void qSlicerRoomsEyeViewModuleWidget::setFixedReferenceCameraEnabled(bool toggled)
{
  Q_D(qSlicerRoomsEyeViewModuleWidget);

  // Get 3D view node camera
  vtkMRMLCameraNode* cameraNode = d->get3DViewCameraNode();
  if (!cameraNode)
  {
    return;
  }

  // Get FixedReference -> RAS transform node
  vtkMRMLLinearTransformNode* fixedReferenceToRasTransformNode = d->logic()->GetTransformNodeBetween(
    vtkSlicerIECTransformLogic::FixedReference,
    vtkSlicerIECTransformLogic::RAS);

  vtkNew<vtkMatrix4x4> fixedReferenceToRasTransformMatrix;
  fixedReferenceToRasTransformMatrix->Identity();
  if (toggled && fixedReferenceToRasTransformNode)
  {
    // Get FixedReference -> RAS transform matrix
    fixedReferenceToRasTransformNode->GetMatrixTransformToParent(fixedReferenceToRasTransformMatrix);
    // Apply FixedReference -> RAS transform matrix to the camera node
    cameraNode->SetAppliedTransform(fixedReferenceToRasTransformMatrix);
    // Observe FixedReference -> RAS transform node by the camera node
    cameraNode->SetAndObserveTransformNodeID(fixedReferenceToRasTransformNode->GetID());
    return;
  }
  cameraNode->SetAndObserveTransformNodeID(nullptr);
}
