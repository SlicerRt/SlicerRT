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
#include "vtkSlicerRTScalarBarActor.h"

// MRML includes
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkScalarBarWidget.h>
#include <vtkVersion.h>

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

  vtkScalarBarWidget* ScalarBarWidget;
  vtkScalarBarWidget* ScalarBarWidget2DRed;
  vtkScalarBarWidget* ScalarBarWidget2DYellow;
  vtkScalarBarWidget* ScalarBarWidget2DGreen;

  vtkSlicerRTScalarBarActor* ScalarBarActor;
  vtkSlicerRTScalarBarActor* ScalarBarActor2DRed;
  vtkSlicerRTScalarBarActor* ScalarBarActor2DYellow;
  vtkSlicerRTScalarBarActor* ScalarBarActor2DGreen;

  std::vector<vtkScalarBarWidget*> ScalarBarWidgets;
};

//-----------------------------------------------------------------------------
// qSlicerIsodoseModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidgetPrivate::qSlicerIsodoseModuleWidgetPrivate(qSlicerIsodoseModuleWidget& object)
  : q_ptr(&object)
{
  // 3D view scalar bar
  this->ScalarBarActor = vtkSlicerRTScalarBarActor::New();
  this->ScalarBarWidget = vtkScalarBarWidget::New();
  this->ScalarBarWidget->SetScalarBarActor(this->ScalarBarActor);
  this->ScalarBarWidgets.push_back(this->ScalarBarWidget);
  
  // 2D views scalar bar
  this->ScalarBarActor2DRed = vtkSlicerRTScalarBarActor::New();
  this->ScalarBarWidget2DRed = vtkScalarBarWidget::New();
  this->ScalarBarWidget2DRed->SetScalarBarActor(this->ScalarBarActor2DRed);
  this->ScalarBarWidgets.push_back(this->ScalarBarWidget2DRed);

  this->ScalarBarActor2DYellow = vtkSlicerRTScalarBarActor::New();
  this->ScalarBarWidget2DYellow = vtkScalarBarWidget::New();
  this->ScalarBarWidget2DYellow->SetScalarBarActor(this->ScalarBarActor2DYellow);
  this->ScalarBarWidgets.push_back(this->ScalarBarWidget2DYellow);

  this->ScalarBarActor2DGreen = vtkSlicerRTScalarBarActor::New();
  this->ScalarBarWidget2DGreen = vtkScalarBarWidget::New();
  this->ScalarBarWidget2DGreen->SetScalarBarActor(this->ScalarBarActor2DGreen);
  this->ScalarBarWidgets.push_back(this->ScalarBarWidget2DGreen);

  for (vtkScalarBarWidget* scalarBarWidget : ScalarBarWidgets)
  {
    vtkSlicerRTScalarBarActor* actor = vtkSlicerRTScalarBarActor::SafeDownCast( scalarBarWidget->GetScalarBarActor() );
    actor->SetOrientationToVertical();
    actor->SetNumberOfLabels(0);
    actor->SetMaximumNumberOfColors(0);
    actor->SetTitle("Dose");
    actor->SetLabelFormat(" %s");
    actor->UseAnnotationAsLabelOn();
    // It's a 2d actor, position it in screen space by percentages
    actor->SetPosition(0.1, 0.1);
    actor->SetWidth(0.1);
    actor->SetHeight(0.8);
  }
}

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidgetPrivate::~qSlicerIsodoseModuleWidgetPrivate()
{
  if (this->ScalarBarWidget)
  {
    this->ScalarBarWidget->Delete();
    this->ScalarBarWidget = 0;
  }
  if (this->ScalarBarActor)
  {
    this->ScalarBarActor->Delete();
    this->ScalarBarActor = 0;
  }
  if (this->ScalarBarWidget2DRed)
  {
    this->ScalarBarWidget2DRed->Delete();
    this->ScalarBarWidget2DRed = 0;
  }
  if (this->ScalarBarActor2DRed)
  {
    this->ScalarBarActor2DRed->Delete();
    this->ScalarBarActor2DRed = 0;
  }
  if (this->ScalarBarWidget2DYellow)
  {
    this->ScalarBarWidget2DYellow->Delete();
    this->ScalarBarWidget2DYellow = 0;
  }
  if (this->ScalarBarActor2DYellow)
  {
    this->ScalarBarActor2DYellow->Delete();
    this->ScalarBarActor2DYellow = 0;
  }
  if (this->ScalarBarWidget2DGreen)
  {
    this->ScalarBarWidget2DGreen->Delete();
    this->ScalarBarWidget2DGreen = 0;
  }
  if (this->ScalarBarActor2DGreen)
  {
    this->ScalarBarActor2DGreen->Delete();
    this->ScalarBarActor2DGreen = 0;
  }
}

//-----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic* qSlicerIsodoseModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerIsodoseModuleWidget);
  return vtkSlicerIsodoseModuleLogic::SafeDownCast(q->logic());
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
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkNew<vtkMRMLIsodoseNode> newNode;
      this->mrmlScene()->AddNode(newNode);
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
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkSmartPointer<vtkMRMLIsodoseNode> newNode = vtkSmartPointer<vtkMRMLIsodoseNode>::New();
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
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

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

    if (paramNode->GetDoseVolumeNode())
    {
      d->MRMLNodeComboBox_DoseVolume->setCurrentNode(paramNode->GetDoseVolumeNode());
    }
    else
    {
      this->setDoseVolumeNode(d->MRMLNodeComboBox_DoseVolume->currentNode());
    }

    d->groupBox_RelativeIsolevels->setChecked(paramNode->GetRelativeRepresentationFlag());

    //TODO: It causes a crash when switch from Volumes module to Isodose module
//    this->updateScalarBarsFromSelectedColorTable();

    d->checkBox_Isoline->setChecked(paramNode->GetShowIsodoseLines());
    d->checkBox_Isosurface->setChecked(paramNode->GetShowIsodoseSurfaces());

    d->checkBox_ScalarBar->setChecked(paramNode->GetShowScalarBar());
    d->checkBox_ScalarBar2D->setChecked(paramNode->GetShowScalarBar2D());

    d->checkBox_ShowDoseVolumesOnly->setChecked(paramNode->GetShowDoseVolumesOnly());
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
  connect( d->spinBox_NumberOfLevels, SIGNAL(valueChanged(int)), this, SLOT(setNumberOfLevels(int)));

  connect( d->checkBox_ShowDoseVolumesOnly, SIGNAL( stateChanged(int) ), this, SLOT( showDoseVolumesOnlyCheckboxChanged(int) ) );
  connect( d->checkBox_Isoline, SIGNAL(toggled(bool)), this, SLOT( setIsolineVisibility(bool) ) );
  connect( d->checkBox_Isosurface, SIGNAL(toggled(bool)), this, SLOT( setIsosurfaceVisibility(bool) ) );
  connect( d->checkBox_ScalarBar, SIGNAL(toggled(bool)), this, SLOT( setScalarBarVisibility(bool) ) );
  connect( d->checkBox_ScalarBar2D, SIGNAL(toggled(bool)), this, SLOT( setScalarBar2DVisibility(bool) ) );

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );
  connect( d->groupBox_RelativeIsolevels, SIGNAL(toggled(bool)), this, SLOT(setRelativeIsolevelsFlag(bool)));
  connect( d->sliderWidget_ReferenceDose, SIGNAL(valueChanged(double)), this, SLOT(setReferenceDoseValue(double)));

  d->pushButton_Apply->setMinimumSize(d->pushButton_Apply->sizeHint().width() + 8, d->pushButton_Apply->sizeHint().height() + 4);

  qSlicerApplication * app = qSlicerApplication::application();
  if (app && app->layoutManager())
  {
    qMRMLThreeDView* threeDView = app->layoutManager()->threeDWidget(0)->threeDView();
    vtkRenderer* activeRenderer = app->layoutManager()->activeThreeDRenderer();
    if (activeRenderer)
    {
      d->ScalarBarWidget->SetInteractor(activeRenderer->GetRenderWindow()->GetInteractor());
    }
    connect(d->checkBox_ScalarBar, SIGNAL(stateChanged(int)), threeDView, SLOT(scheduleRender()));

    QStringList sliceViewerNames = app->layoutManager()->sliceViewNames();
    qMRMLSliceWidget* sliceViewerWidgetRed = app->layoutManager()->sliceWidget(sliceViewerNames[0]);
    const qMRMLSliceView* sliceViewRed = sliceViewerWidgetRed->sliceView();
    d->ScalarBarWidget2DRed->SetInteractor(sliceViewerWidgetRed->interactorStyle()->GetInteractor());
    qMRMLSliceWidget* sliceViewerWidgetYellow = app->layoutManager()->sliceWidget(sliceViewerNames[1]);
    const qMRMLSliceView* sliceViewYellow = sliceViewerWidgetYellow->sliceView();
    d->ScalarBarWidget2DYellow->SetInteractor(sliceViewerWidgetYellow->interactorStyle()->GetInteractor());
    qMRMLSliceWidget* sliceViewerWidgetGreen = app->layoutManager()->sliceWidget(sliceViewerNames[2]);
    const qMRMLSliceView* sliceViewGreen = sliceViewerWidgetGreen->sliceView();
    d->ScalarBarWidget2DGreen->SetInteractor(sliceViewerWidgetGreen->interactorStyle()->GetInteractor());

    connect(d->checkBox_ScalarBar2D, SIGNAL(stateChanged(int)), sliceViewRed, SLOT(scheduleRender()));
    connect(d->checkBox_ScalarBar2D, SIGNAL(stateChanged(int)), sliceViewYellow, SLOT(scheduleRender()));
    connect(d->checkBox_ScalarBar2D, SIGNAL(stateChanged(int)), sliceViewGreen, SLOT(scheduleRender()));
  }

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
  qvtkReconnect( paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  this->updateWidgetFromMRML();
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

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !node)
  {
    return;
  }

  // Unobserve previous color node
  vtkMRMLColorTableNode* previousColorNode = paramNode->GetColorTableNode();
  if (previousColorNode)
  {
    qvtkDisconnect(previousColorNode, vtkCommand::ModifiedEvent,
      this, SLOT(updateScalarBarsFromSelectedColorTable()));
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  if (paramNode->GetDoseVolumeNode() && vtkSlicerRtCommon::IsDoseVolumeNode(paramNode->GetDoseVolumeNode()))
  {
    d->label_NotDoseVolumeWarning->setText("");

    vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
    if (!shNode)
    {
      qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
      return;
    }

    std::string doseUnitName("");
    vtkIdType doseShItemID = shNode->GetItemByDataNode(paramNode->GetDoseVolumeNode());
    if (doseShItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
      doseUnitName = shNode->GetAttributeFromItemAncestor(
        doseShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    }

    double valueRange[2];
    vtkImageData* image = paramNode->GetDoseVolumeNode()->GetImageData();
    image->GetScalarRange(valueRange);
    if (!doseUnitName.compare("RELATIVE"))
    {
      paramNode->SetDoseUnits(vtkMRMLIsodoseNode::Relative);
      paramNode->SetReferenceDoseValue(-1.);
      d->sliderWidget_ReferenceDose->setEnabled(false);
      d->groupBox_RelativeIsolevels->setEnabled(false);
      d->sliderWidget_ReferenceDose->setSuffix("");
    }
    else if (!doseUnitName.compare("GY"))
    {
      paramNode->SetDoseUnits(vtkMRMLIsodoseNode::Gy);
      paramNode->SetReferenceDoseValue(valueRange[1]);
      d->groupBox_RelativeIsolevels->setEnabled(true);
      d->sliderWidget_ReferenceDose->setEnabled(true);
      d->sliderWidget_ReferenceDose->setMinimum(valueRange[0]);
      d->sliderWidget_ReferenceDose->setMaximum(2. * valueRange[1]);
      d->sliderWidget_ReferenceDose->setValue(0.87 * valueRange[1]);
      d->sliderWidget_ReferenceDose->setSuffix(tr(" Gy"));
    }
    else
    {
      paramNode->SetDoseUnits(vtkMRMLIsodoseNode::Unknown);
      paramNode->SetReferenceDoseValue(valueRange[1]);
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
  d->logic()->SetupColorTableNodeForDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  // Show color table node associated to the dose volume
  vtkMRMLColorTableNode* selectedColorNode = paramNode->GetColorTableNode();
  d->tableView_IsodoseLevels->setMRMLColorNode(selectedColorNode);
  // Set current number of isodose levels
  bool wasBlocked = d->spinBox_NumberOfLevels->blockSignals(true);
  if (selectedColorNode)
  {
    d->spinBox_NumberOfLevels->setValue(selectedColorNode->GetNumberOfColors());

    qvtkConnect(selectedColorNode, vtkCommand::ModifiedEvent,
      this, SLOT(updateScalarBarsFromSelectedColorTable()));
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
void qSlicerIsodoseModuleWidget::setNumberOfLevels(int newNumber)
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }

  d->logic()->SetNumberOfIsodoseLevels(paramNode, newNumber);
  vtkMRMLColorTableNode* selectedColorNode = paramNode->GetColorTableNode();
  if (!selectedColorNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid color table node";
    return;
  }

  int numberOfColors = selectedColorNode->GetNumberOfColors();
  d->ScalarBarActor->SetMaximumNumberOfColors(numberOfColors);
  d->ScalarBarActor->SetNumberOfLabels(numberOfColors);

  d->ScalarBarActor2DRed->SetMaximumNumberOfColors(numberOfColors);
  d->ScalarBarActor2DRed->SetNumberOfLabels(numberOfColors);
  d->ScalarBarActor2DYellow->SetMaximumNumberOfColors(numberOfColors);
  d->ScalarBarActor2DYellow->SetNumberOfLabels(numberOfColors);
  d->ScalarBarActor2DGreen->SetMaximumNumberOfColors(numberOfColors);
  d->ScalarBarActor2DGreen->SetNumberOfLabels(numberOfColors);

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
  
  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowDoseVolumesOnly(aState);
  paramNode->DisableModifiedEventOff();

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

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetRelativeRepresentationFlag(useRelativeIsolevels);
  paramNode->DisableModifiedEventOff();

  vtkMRMLIsodoseNode::DoseUnitsType doseUnits = paramNode->GetDoseUnits();
  
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
  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveColorTableNode(selectedColorNode);
  paramNode->DisableModifiedEventOff();

  d->tableView_IsodoseLevels->setMRMLColorNode(selectedColorNode);
  // Set current number of isodose levels
  bool wasBlocked = d->spinBox_NumberOfLevels->blockSignals(true);
  if (selectedColorNode)
  {
    d->spinBox_NumberOfLevels->setValue(selectedColorNode->GetNumberOfColors());

    qvtkConnect(selectedColorNode, vtkCommand::ModifiedEvent,
      this, SLOT(updateScalarBarsFromSelectedColorTable()));
  }
  else
  {
    d->spinBox_NumberOfLevels->setValue(0);
  }
  d->spinBox_NumberOfLevels->blockSignals(wasBlocked);
  // Update scalar bars
  this->updateScalarBarsFromSelectedColorTable();
  // Update dose volume palette
  d->logic()->UpdateDoseColorTableFromIsodose(paramNode);
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setReferenceDoseValue(double value)
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
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

  paramNode->DisableModifiedEventOn();
  switch (paramNode->GetDoseUnits())
  {
  case vtkMRMLIsodoseNode::Gy:
  case vtkMRMLIsodoseNode::Unknown:
    paramNode->SetReferenceDoseValue(value);
    break;
  case vtkMRMLIsodoseNode::Relative:
  default:
    paramNode->SetReferenceDoseValue(-1.);
    break;
  }
  paramNode->DisableModifiedEventOff();
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

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowIsodoseLines(visible);
  paramNode->DisableModifiedEventOff();

  vtkIdType isdoseFolderItemID = d->logic()->GetIsodoseFolderItemID(paramNode);
  if (!isdoseFolderItemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid isodose surface models folder";
    return;
  }

  std::vector<vtkIdType> childItemIDs;
  shNode->GetItemChildren(isdoseFolderItemID, childItemIDs, false);
  for (vtkIdType childItemID : childItemIDs)
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(shNode->GetItemDataNode(childItemID));
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

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowIsodoseSurfaces(visible);
  paramNode->DisableModifiedEventOff();

  vtkIdType isdoseFolderItemID = d->logic()->GetIsodoseFolderItemID(paramNode);
  if (!isdoseFolderItemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid isodose surface models folder";
    return;
  }

  std::vector<vtkIdType> childItemIDs;
  shNode->GetItemChildren(isdoseFolderItemID, childItemIDs, false);
  for (vtkIdType childItemID : childItemIDs)
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(shNode->GetItemDataNode(childItemID));
    modelNode->GetDisplayNode()->SetVisibility(visible);
  }
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setScalarBarVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }
  if (!d->ScalarBarWidget)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scalar bar widget";
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowScalarBar(visible);
  paramNode->DisableModifiedEventOff();

  if (visible)
  {
    d->ScalarBarActor->UseAnnotationAsLabelOn();
  }

  vtkMRMLColorTableNode* selectedColorNode = paramNode->GetColorTableNode();
  if (!selectedColorNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid color table node";
    return;
  }

  this->updateScalarBarsFromSelectedColorTable();

  d->ScalarBarWidget->SetEnabled(visible);
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setScalarBar2DVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }
  if (!d->ScalarBarWidget2DRed || !d->ScalarBarWidget2DYellow || !d->ScalarBarWidget2DGreen)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scalar bar widget";
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowScalarBar2D(visible);
  paramNode->DisableModifiedEventOff();

  if (visible)
  {
    d->ScalarBarActor2DRed->UseAnnotationAsLabelOn();
    d->ScalarBarActor2DYellow->UseAnnotationAsLabelOn();
    d->ScalarBarActor2DGreen->UseAnnotationAsLabelOn();
  }

  vtkMRMLColorTableNode* selectedColorNode = paramNode->GetColorTableNode();
  if (!selectedColorNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid color table node";
    return;
  }

  this->updateScalarBarsFromSelectedColorTable();

  d->ScalarBarWidget2DRed->SetEnabled(visible);
  d->ScalarBarWidget2DYellow->SetEnabled(visible);
  d->ScalarBarWidget2DGreen->SetEnabled(visible);
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

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the isodose surface for the selected dose volume
  d->logic()->CreateIsodoseSurfaces(paramNode);

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateButtonsState()
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  bool applyEnabled = paramNode
                   && paramNode->GetDoseVolumeNode()
                   && paramNode->GetColorTableNode()
                   && paramNode->GetColorTableNode()->GetNumberOfColors() > 0;
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

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!this->mrmlScene() || !paramNode)
  {
    return;
  }

  vtkMRMLColorTableNode* selectedColorNode = paramNode->GetColorTableNode();
  if (!selectedColorNode)
  {
    qDebug() << Q_FUNC_INFO << ": No color table node is selected";
    return;
  }
  vtkMRMLScalarVolumeNode* doseVolumeNode = paramNode->GetDoseVolumeNode();
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

  int newNumberOfColors = selectedColorNode->GetNumberOfColors();

  vtkMRMLIsodoseNode::DoseUnitsType doseUnits = paramNode->GetDoseUnits();
  bool relativeRepresentation = paramNode->GetRelativeRepresentationFlag();
  
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

  // Update all scalar bar actors
  for (vtkScalarBarWidget* scalarBarWidget : d->ScalarBarWidgets)
  {
    vtkSlicerRTScalarBarActor* actor = vtkSlicerRTScalarBarActor::SafeDownCast( scalarBarWidget->GetScalarBarActor() );

    // Update actor
    actor->UseAnnotationAsLabelOn(); // Needed each time
    actor->SetLookupTable(selectedColorNode->GetLookupTable());
    actor->SetNumberOfLabels(newNumberOfColors);
    actor->SetMaximumNumberOfColors(newNumberOfColors);
    actor->GetLookupTable()->ResetAnnotations();
    for (int colorIndex=0; colorIndex<newNumberOfColors; ++colorIndex)
    {
      actor->GetLookupTable()->SetAnnotation(colorIndex, vtkStdString(selectedColorNode->GetColorName(colorIndex)));
    }
    actor->SetTitle(scalarBarTitle.c_str());
    scalarBarWidget->Render();
  }
}
