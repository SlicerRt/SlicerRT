/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QCheckBox>
#include <QDebug>

// SlicerQt includes
#include "qSlicerIsodoseModuleWidget.h"
#include "ui_qSlicerIsodoseModule.h"
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"

// Slicer includes
#include "vtkSlicerColorLogic.h"

// SlicerRtCommon includes
#include "vtkSlicerRtCommon.h"

// qMRMLWidget includes
#include "qMRMLThreeDView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"

// Isodose includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLIsodoseNode.h"

// MRML includes
#include <vtkMRMLApplicationLogic.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLColorLegendDisplayNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSubjectHierarchyNode.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Isodose
class qSlicerIsodoseModuleWidgetPrivate: public Ui_qSlicerIsodoseModule
{
  Q_DECLARE_PUBLIC(qSlicerIsodoseModuleWidget);
protected:
  qSlicerIsodoseModuleWidget* const q_ptr;
public:
  qSlicerIsodoseModuleWidgetPrivate(qSlicerIsodoseModuleWidget &object);
  ~qSlicerIsodoseModuleWidgetPrivate();

  vtkSlicerIsodoseModuleLogic* logic() const;
  vtkSlicerColorLogic* colorLogic() const;

  vtkWeakPointer<vtkMRMLScalarVolumeNode> DoseVolumeNode;
  vtkWeakPointer<vtkMRMLIsodoseNode> IsodoseNode;
  vtkWeakPointer<vtkMRMLColorTableNode> IsodoseColorTableNode;
  vtkWeakPointer<vtkMRMLColorLegendDisplayNode> ColorLegendNode;
};

//-----------------------------------------------------------------------------
// qSlicerIsodoseModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidgetPrivate::qSlicerIsodoseModuleWidgetPrivate(qSlicerIsodoseModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidgetPrivate::~qSlicerIsodoseModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic* qSlicerIsodoseModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerIsodoseModuleWidget);
  return vtkSlicerIsodoseModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
vtkSlicerColorLogic* qSlicerIsodoseModuleWidgetPrivate::colorLogic() const
{
  Q_Q(const qSlicerIsodoseModuleWidget);
  return vtkSlicerColorLogic::SafeDownCast(this->logic()->GetMRMLApplicationLogic()->GetModuleLogic("Colors"));
}

//-----------------------------------------------------------------------------
// qSlicerIsodoseModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidget::qSlicerIsodoseModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerIsodoseModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidget::~qSlicerIsodoseModuleWidget() = default;

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerIsodoseModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );
  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndCloseEvent, this, SLOT(onSceneClosedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene && d->MRMLNodeComboBox_ParameterSet->currentNode() == nullptr)
  {
    vtkMRMLNode* node = scene->GetFirstNodeByClass("vtkMRMLIsodoseNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkMRMLNode* newNode = this->mrmlScene()->AddNewNodeByClass("vtkMRMLIsodoseNode");
      this->setParameterNode(newNode);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onSceneClosedEvent()
{
  //TODO: Hide colorbars if they are shown on slices and 3D.
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  Q_D(qSlicerIsodoseModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic";
    return;
  }
  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  // If we have a parameter node select it
  if (!paramNode)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetFirstNodeByClass("vtkMRMLIsodoseNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkMRMLNode* newNode = this->mrmlScene()->AddNewNodeByClass("vtkMRMLIsodoseNode");
      if (newNode)
      {
        this->setParameterNode(newNode);
      }
    }
  }
  else
  {
    this->updateWidgetFromMRML();
  }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (d->IsodoseNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->IsodoseNode);

    if (d->IsodoseNode->GetDoseVolumeNode())
    {
      d->MRMLNodeComboBox_DoseVolume->setCurrentNode(d->IsodoseNode->GetDoseVolumeNode());
    }
    else
    {
      this->setDoseVolumeNode(d->MRMLNodeComboBox_DoseVolume->currentNode());
    }

    d->groupBox_RelativeIsolevels->setChecked(d->IsodoseNode->GetRelativeRepresentationFlag());

    //TODO: It causes a crash when switch from Volumes module to Isodose module
//    this->updateScalarBarsFromSelectedColorTable();

    d->checkBox_Isoline->setChecked(d->IsodoseNode->GetShowIsodoseLines());
    d->checkBox_Isosurface->setChecked(d->IsodoseNode->GetShowIsodoseSurfaces());
    d->checkBox_ShowDoseVolumesOnly->setChecked(d->IsodoseNode->GetShowDoseVolumesOnly());

    if (d->IsodoseNode->GetIsosurfacesModelNode())
    {
      vtkMRMLColorLegendDisplayNode* clNode = d->colorLogic()->GetColorLegendDisplayNode(d->IsodoseNode->GetIsosurfacesModelNode());
      if (clNode)
      {
        qvtkReconnect( clNode, d->ColorLegendNode, vtkCommand::ModifiedEvent,
          this, SLOT(updateColorLegendFromMRML()));
        d->ColorLegendNode = clNode;
        // Update color legend display widget 
        this->updateColorLegendFromMRML();
      }
      else
      {
        d->ColorLegendNode = nullptr;
        d->ColorLegendDisplayNodeWidget->setMRMLColorLegendDisplayNode(clNode);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setup()
{
  Q_D(qSlicerIsodoseModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Show only dose volumes in the dose volume combobox by default
  d->MRMLNodeComboBox_DoseVolume->addAttribute( QString("vtkMRMLScalarVolumeNode"), vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());

  // Make connections
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setParameterNode(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( setDoseVolumeNode(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_IsodoseModel, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( setIsodoseModelNode(vtkMRMLNode*) ) );
  connect( d->spinBox_NumberOfLevels, SIGNAL(valueChanged(int)), this, SLOT(setNumberOfLevels(int)));

  connect( d->checkBox_ShowDoseVolumesOnly, SIGNAL( stateChanged(int) ), this, SLOT( showDoseVolumesOnlyCheckboxChanged(int) ) );
  connect( d->checkBox_Isoline, SIGNAL(toggled(bool)), this, SLOT( setIsolineVisibility(bool) ) );
  connect( d->checkBox_Isosurface, SIGNAL(toggled(bool)), this, SLOT( setIsosurfaceVisibility(bool) ) );
  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );
  connect( d->groupBox_RelativeIsolevels, SIGNAL(toggled(bool)), this, SLOT(setRelativeIsolevelsFlag(bool)));
  connect( d->sliderWidget_ReferenceDose, SIGNAL(valueChanged(double)), this, SLOT(setReferenceDoseValue(double)));

  d->pushButton_Apply->setMinimumSize(d->pushButton_Apply->sizeHint().width() + 50, d->pushButton_Apply->sizeHint().height() + 20);

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  // Select the default color node
  this->updateScalarBarsFromSelectedColorTable();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->IsodoseNode, paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );
  d->IsodoseNode = paramNode;

  this->updateWidgetFromMRML();
  this->updateColorLegendFromMRML();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setDoseVolumeNode(vtkMRMLNode* node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  if (!d->IsodoseNode || !node)
  {
    return;
  }

  // Unobserve previous color node
  vtkMRMLColorTableNode* previousColorNode = d->IsodoseNode->GetColorTableNode();
  if (previousColorNode)
  {
    qvtkDisconnect(previousColorNode, vtkCommand::ModifiedEvent,
      this, SLOT(updateScalarBarsFromSelectedColorTable()));
    d->IsodoseColorTableNode = nullptr;
  }

  d->IsodoseNode->DisableModifiedEventOn();
  d->IsodoseNode->SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  d->IsodoseNode->DisableModifiedEventOff();

  d->DoseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);

  if (d->DoseVolumeNode && vtkSlicerRtCommon::IsDoseVolumeNode(d->DoseVolumeNode))
  {
    d->label_NotDoseVolumeWarning->setText("");

    vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
    if (!shNode)
    {
      qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
      return;
    }

    std::string doseUnitName("");
    vtkIdType doseShItemID = shNode->GetItemByDataNode(d->DoseVolumeNode);
    if (doseShItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
      doseUnitName = shNode->GetAttributeFromItemAncestor(
        doseShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    }

    double valueRange[2];
    vtkImageData* image = d->DoseVolumeNode->GetImageData();
    image->GetScalarRange(valueRange);
    if (!doseUnitName.compare("RELATIVE"))
    {
      d->IsodoseNode->SetDoseUnits(vtkMRMLIsodoseNode::Relative);
      d->IsodoseNode->SetReferenceDoseValue(-1.);
      d->sliderWidget_ReferenceDose->setEnabled(false);
      d->groupBox_RelativeIsolevels->setEnabled(false);
      d->sliderWidget_ReferenceDose->setSuffix("");
    }
    else if (!doseUnitName.compare("GY"))
    {
      d->IsodoseNode->SetDoseUnits(vtkMRMLIsodoseNode::Gy);
      d->IsodoseNode->SetReferenceDoseValue(valueRange[1]);
      d->groupBox_RelativeIsolevels->setEnabled(true);
      d->sliderWidget_ReferenceDose->setEnabled(true);
      d->sliderWidget_ReferenceDose->setMinimum(valueRange[0]);
      d->sliderWidget_ReferenceDose->setMaximum(2. * valueRange[1]);
      d->sliderWidget_ReferenceDose->setValue(0.87 * valueRange[1]);
      d->sliderWidget_ReferenceDose->setSuffix(tr(" Gy"));
    }
    else
    {
      d->IsodoseNode->SetDoseUnits(vtkMRMLIsodoseNode::Unknown);
      d->IsodoseNode->SetReferenceDoseValue(valueRange[1]);
      d->groupBox_RelativeIsolevels->setEnabled(true);
      d->sliderWidget_ReferenceDose->setEnabled(true);
      d->sliderWidget_ReferenceDose->setMinimum(valueRange[0]);
      d->sliderWidget_ReferenceDose->setMaximum(2. * valueRange[1]);
      d->sliderWidget_ReferenceDose->setValue(0.87 * valueRange[1]);
      d->sliderWidget_ReferenceDose->setSuffix("");
      d->label_NotDoseVolumeWarning->setText(tr("Neither \"GY\" nor \"RELATIVE\""));
    }
  }
  else
  {
    d->label_NotDoseVolumeWarning->setText(tr(" Selected volume is not a dose"));
  }

  // Make sure the dose volume has an associated isodose color table node
  d->logic()->SetupColorTableNodeForDoseVolumeNode(d->DoseVolumeNode);
  // Show color table node associated to the dose volume
  vtkMRMLColorTableNode* selectedColorNode = d->IsodoseNode->GetColorTableNode();
  d->tableView_IsodoseLevels->setMRMLColorNode(selectedColorNode);
  // Set current number of isodose levels
  bool wasBlocked = d->spinBox_NumberOfLevels->blockSignals(true);
  if (selectedColorNode)
  {
    d->spinBox_NumberOfLevels->setValue(selectedColorNode->GetNumberOfColors());

    qvtkConnect(selectedColorNode, vtkCommand::ModifiedEvent,
      this, SLOT(updateScalarBarsFromSelectedColorTable()));
    d->IsodoseColorTableNode = selectedColorNode;
  }
  else
  {
    d->spinBox_NumberOfLevels->setValue(0);
  }
  d->spinBox_NumberOfLevels->blockSignals(wasBlocked);
  // Update scalar bars
  this->updateScalarBarsFromSelectedColorTable();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsodoseModelNode(vtkMRMLNode* node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  if (!d->IsodoseNode || !node)
  {
    return;
  }

  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
  if (!modelNode->GetDisplayNode())
  {
    modelNode->CreateDefaultDisplayNodes();
  }
  d->IsodoseNode->SetAndObserveIsosurfacesModelNode(modelNode);
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setNumberOfLevels(int newNumber)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!d->IsodoseNode)
  {
    return;
  }

  d->logic()->SetNumberOfIsodoseLevels(d->IsodoseNode, newNumber);

  if (!d->IsodoseColorTableNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid color table node";
    return;
  }

  this->updateScalarBarsFromSelectedColorTable();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::showDoseVolumesOnlyCheckboxChanged(int aState)
{
  Q_D(qSlicerIsodoseModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  if (!d->IsodoseNode)
  {
    return;
  }

  d->IsodoseNode->DisableModifiedEventOn();
  d->IsodoseNode->SetShowDoseVolumesOnly(aState);
  d->IsodoseNode->DisableModifiedEventOff();

  if (aState)
  {
    d->MRMLNodeComboBox_DoseVolume->addAttribute("vtkMRMLScalarVolumeNode", vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
  }
  else
  {
    d->MRMLNodeComboBox_DoseVolume->removeAttribute("vtkMRMLScalarVolumeNode", vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
  }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setRelativeIsolevelsFlag(bool useRelativeIsolevels)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  if (!d->IsodoseNode)
  {
    return;
  }

  d->IsodoseNode->DisableModifiedEventOn();
  d->IsodoseNode->SetRelativeRepresentationFlag(useRelativeIsolevels);
  d->IsodoseNode->DisableModifiedEventOff();

  vtkMRMLIsodoseNode::DoseUnitsType doseUnits = d->IsodoseNode->GetDoseUnits();
  
  // Get dose unit name and assemble scalar bar title
  QString labelHeaderTitle = QObject::tr("Label");
  switch (doseUnits)
  {
  case vtkMRMLIsodoseNode::Gy:
  {
    QString msg = useRelativeIsolevels ? QObject::tr("Relative Dose (%)") : QObject::tr("Dose (Gy)");
    labelHeaderTitle = msg;
  }
    break;
  case vtkMRMLIsodoseNode::Unknown:
  {
    QString msg = useRelativeIsolevels ? QObject::tr("Relative Units (%)") : QObject::tr("Units (MU)");
    labelHeaderTitle = msg;
  }
    break;
  case vtkMRMLIsodoseNode::Relative:
    labelHeaderTitle = QObject::tr("Relative Dose (%)");
    break;
  default:
    break;
  }
  d->tableView_IsodoseLevels->model()->setHeaderData( 1, Qt::Horizontal, labelHeaderTitle);

  // Make sure the dose volume has an associated isodose color table node
  vtkMRMLColorTableNode* selectedColorNode = (useRelativeIsolevels) ? 
    d->logic()->GetRelativeIsodoseColorTable(this->mrmlScene())
    :
    d->logic()->GetDefaultIsodoseColorTable(this->mrmlScene());

  // Show color table node associated to the dose volume
  d->IsodoseNode->DisableModifiedEventOn();
  d->IsodoseNode->SetAndObserveColorTableNode(selectedColorNode);
  d->IsodoseNode->DisableModifiedEventOff();

  d->tableView_IsodoseLevels->setMRMLColorNode(selectedColorNode);
  // Set current number of isodose levels
  bool wasBlocked = d->spinBox_NumberOfLevels->blockSignals(true);
  if (selectedColorNode)
  {
    d->spinBox_NumberOfLevels->setValue(selectedColorNode->GetNumberOfColors());

    qvtkConnect(selectedColorNode, vtkCommand::ModifiedEvent,
      this, SLOT(updateScalarBarsFromSelectedColorTable()));
    d->IsodoseColorTableNode = selectedColorNode;
  }
  else
  {
    d->spinBox_NumberOfLevels->setValue(0);
  }
  d->spinBox_NumberOfLevels->blockSignals(wasBlocked);
  // Update scalar bars
  this->updateScalarBarsFromSelectedColorTable();
  // Update dose volume palette
  d->logic()->UpdateDoseColorTableFromIsodose(d->IsodoseNode);
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setReferenceDoseValue(double value)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!d->IsodoseNode)
  {
    return;
  }

  if (d->sliderWidget_ReferenceDose->maximum() > 0.)
  {
    double percentage = 200. * value / d->sliderWidget_ReferenceDose->maximum();
    d->label_PercentageOfMaxVolumeDose->setText(tr("%1 %").arg( percentage, 0, 'g', 4));
  }
  else
  {
    d->label_PercentageOfMaxVolumeDose->setText("");
  }

  d->IsodoseNode->DisableModifiedEventOn();
  switch (d->IsodoseNode->GetDoseUnits())
  {
  case vtkMRMLIsodoseNode::Gy:
  case vtkMRMLIsodoseNode::Unknown:
    d->IsodoseNode->SetReferenceDoseValue(value);
    break;
  case vtkMRMLIsodoseNode::Relative:
  default:
    d->IsodoseNode->SetReferenceDoseValue(-1.);
    break;
  }
  d->IsodoseNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
QString qSlicerIsodoseModuleWidget::generateNewIsodoseLevel() const
{
  QString newIsodoseLevelBase("New level");
  QString newIsodoseLevel(newIsodoseLevelBase);
  return newIsodoseLevel;
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsolineVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  if (!d->IsodoseNode)
  {
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  d->IsodoseNode->DisableModifiedEventOn();
  d->IsodoseNode->SetShowIsodoseLines(visible);
  d->IsodoseNode->DisableModifiedEventOff();

  if (d->IsodoseNode->GetIsosurfacesModelNode())
  {
    vtkMRMLModelNode* modelNode = d->IsodoseNode->GetIsosurfacesModelNode();
    modelNode->GetDisplayNode()->SetVisibility2D(visible);
  }
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsosurfaceVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  if (!d->IsodoseNode)
  {
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  d->IsodoseNode->DisableModifiedEventOn();
  d->IsodoseNode->SetShowIsodoseSurfaces(visible);
  d->IsodoseNode->DisableModifiedEventOff();

  if (d->IsodoseNode->GetIsosurfacesModelNode())
  {
    vtkMRMLModelNode* modelNode = d->IsodoseNode->GetIsosurfacesModelNode();
    modelNode->GetDisplayNode()->SetVisibility(visible);
  }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::applyClicked()
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  if (!d->IsodoseNode)
  {
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the isodose surface for the selected dose volume
  // and create color legend node if isosurfaces model node has been calculated
  // successfully
  bool res = d->logic()->CreateIsodoseSurfaces(d->IsodoseNode);
  if (res && d->IsodoseNode->GetIsosurfacesModelNode())
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(d->MRMLNodeComboBox_IsodoseModel->currentNode());
    if (!modelNode)
    {
      d->MRMLNodeComboBox_IsodoseModel->setCurrentNode(d->IsodoseNode->GetIsosurfacesModelNode());
    }

    vtkMRMLColorLegendDisplayNode* clNode = d->colorLogic()->GetColorLegendDisplayNode(d->IsodoseNode->GetIsosurfacesModelNode());
    if (!clNode)
    {
      clNode = d->colorLogic()->AddDefaultColorLegendDisplayNode(d->IsodoseNode->GetIsosurfacesModelNode());
      d->logic()->SetColorLegendDefaults(d->IsodoseNode);
    }

    qvtkReconnect( clNode, d->ColorLegendNode, vtkCommand::ModifiedEvent,
      this, SLOT(updateColorLegendFromMRML()));
    d->ColorLegendNode = clNode;
    // Update color legend display widget 
    this->updateColorLegendFromMRML();

  }

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateButtonsState()
{
  Q_D(qSlicerIsodoseModuleWidget);

  bool applyEnabled = d->IsodoseNode && d->IsodoseColorTableNode
                   && d->IsodoseNode->GetDoseVolumeNode()
                   && d->IsodoseColorTableNode->GetNumberOfColors() > 0;
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------
bool qSlicerIsodoseModuleWidget::setEditedNode(
  vtkMRMLNode* node, QString role /* = QString()*/, QString context /* = QString() */)
{
  Q_D(qSlicerIsodoseModuleWidget);
  Q_UNUSED(role);
  Q_UNUSED(context);

  if (!vtkSlicerRtCommon::IsDoseVolumeNode(node))
    {
    return false;
    }

  d->MRMLNodeComboBox_DoseVolume->setCurrentNode(node);
  return true;
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateScalarBarsFromSelectedColorTable()
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene() || !d->IsodoseNode)
  {
    return;
  }

  if (!d->IsodoseColorTableNode)
  {
    qDebug() << Q_FUNC_INFO << ": No color table node is selected";
    
    return;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = d->IsodoseNode->GetDoseVolumeNode();
  if (!doseVolumeNode)
  {
    qWarning() << Q_FUNC_INFO << ": No dose volume node is selected";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  vtkMRMLIsodoseNode::DoseUnitsType doseUnits = d->IsodoseNode->GetDoseUnits();
  bool relativeRepresentation = d->IsodoseNode->GetRelativeRepresentationFlag();
  
  // Get dose unit name and assemble scalar bar title
  std::string scalarBarTitle("Isolevels");
  switch (doseUnits)
  {
  case vtkMRMLIsodoseNode::Gy:
  {
    const char* str = relativeRepresentation ? " (%)" : " (Gy)";
    scalarBarTitle += std::string(str);
  }
    break;
  case vtkMRMLIsodoseNode::Relative:
    scalarBarTitle += std::string(" (%)");
    break;
  case vtkMRMLIsodoseNode::Unknown:
  default:
  {
    const char* str = relativeRepresentation ? " (%)" : " (MU)";
    scalarBarTitle += std::string(str);
  }
    break;
  }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateColorLegendFromMRML()
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLDisplayNode* displayNode = nullptr;
  if (d->IsodoseNode && d->IsodoseNode->GetIsosurfacesModelNode())
  {
    displayNode = d->IsodoseNode->GetIsosurfacesModelNode()->GetDisplayNode();
  }

  d->ColorLegendNode = vtkSlicerColorLogic::GetColorLegendDisplayNode(displayNode);
  d->ColorLegendDisplayNodeWidget->setMRMLColorLegendDisplayNode(d->ColorLegendNode);
}
