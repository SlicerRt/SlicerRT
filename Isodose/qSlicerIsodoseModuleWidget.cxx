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
#include "SlicerRtCommon.h"

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
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLProceduralColorNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>

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
  void updateScalarBarsFromSelectedColorTable();

  vtkScalarBarWidget* ScalarBarWidget;
  vtkScalarBarWidget* ScalarBarWidget2DRed;
  vtkScalarBarWidget* ScalarBarWidget2DYellow;
  vtkScalarBarWidget* ScalarBarWidget2DGreen;

  vtkSlicerRTScalarBarActor* ScalarBarActor;
  vtkSlicerRTScalarBarActor* ScalarBarActor2DRed;
  vtkSlicerRTScalarBarActor* ScalarBarActor2DYellow;
  vtkSlicerRTScalarBarActor* ScalarBarActor2DGreen;
};

//-----------------------------------------------------------------------------
// qSlicerIsodoseModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidgetPrivate::qSlicerIsodoseModuleWidgetPrivate(qSlicerIsodoseModuleWidget& object)
  : q_ptr(&object)
{
  this->ScalarBarWidget = vtkScalarBarWidget::New();
  this->ScalarBarActor = vtkSlicerRTScalarBarActor::New();
  this->ScalarBarWidget->SetScalarBarActor(this->ScalarBarActor);
  this->ScalarBarWidget->GetScalarBarActor()->SetOrientationToVertical();
  this->ScalarBarWidget->GetScalarBarActor()->SetNumberOfLabels(6);
  this->ScalarBarWidget->GetScalarBarActor()->SetMaximumNumberOfColors(6);
  this->ScalarBarWidget->GetScalarBarActor()->SetTitle("Dose(Gy)");
  this->ScalarBarWidget->GetScalarBarActor()->SetLabelFormat(" %s");
  
  // it's a 2d actor, position it in screen space by percentages
  this->ScalarBarWidget->GetScalarBarActor()->SetPosition(0.1, 0.1);
  this->ScalarBarWidget->GetScalarBarActor()->SetWidth(0.1);
  this->ScalarBarWidget->GetScalarBarActor()->SetHeight(0.8);

  this->ScalarBarWidget2DRed = vtkScalarBarWidget::New();
  this->ScalarBarActor2DRed = vtkSlicerRTScalarBarActor::New();
  this->ScalarBarWidget2DRed->SetScalarBarActor(this->ScalarBarActor2DRed);
  this->ScalarBarWidget2DRed->GetScalarBarActor()->SetOrientationToVertical();
  this->ScalarBarWidget2DRed->GetScalarBarActor()->SetNumberOfLabels(6);
  this->ScalarBarWidget2DRed->GetScalarBarActor()->SetMaximumNumberOfColors(6);
  this->ScalarBarWidget2DRed->GetScalarBarActor()->SetTitle("Dose(Gy)");
  this->ScalarBarWidget2DRed->GetScalarBarActor()->SetLabelFormat(" %s");
  
  // it's a 2d actor, position it in screen space by percentages
  this->ScalarBarWidget2DRed->GetScalarBarActor()->SetPosition(0.1, 0.1);
  this->ScalarBarWidget2DRed->GetScalarBarActor()->SetWidth(0.1);
  this->ScalarBarWidget2DRed->GetScalarBarActor()->SetHeight(0.8);

  this->ScalarBarWidget2DYellow = vtkScalarBarWidget::New();
  this->ScalarBarActor2DYellow = vtkSlicerRTScalarBarActor::New();
  this->ScalarBarWidget2DYellow->SetScalarBarActor(this->ScalarBarActor2DYellow);
  this->ScalarBarWidget2DYellow->GetScalarBarActor()->SetOrientationToVertical();
  this->ScalarBarWidget2DYellow->GetScalarBarActor()->SetNumberOfLabels(6);
  this->ScalarBarWidget2DYellow->GetScalarBarActor()->SetMaximumNumberOfColors(6);
  this->ScalarBarWidget2DYellow->GetScalarBarActor()->SetTitle("Dose(Gy)");
  this->ScalarBarWidget2DYellow->GetScalarBarActor()->SetLabelFormat(" %s");
  
  // it's a 2d actor, position it in screen space by percentages
  this->ScalarBarWidget2DYellow->GetScalarBarActor()->SetPosition(0.1, 0.1);
  this->ScalarBarWidget2DYellow->GetScalarBarActor()->SetWidth(0.1);
  this->ScalarBarWidget2DYellow->GetScalarBarActor()->SetHeight(0.8);

  this->ScalarBarWidget2DGreen = vtkScalarBarWidget::New();
  this->ScalarBarActor2DGreen = vtkSlicerRTScalarBarActor::New();
  this->ScalarBarWidget2DGreen->SetScalarBarActor(this->ScalarBarActor2DGreen);
  this->ScalarBarWidget2DGreen->GetScalarBarActor()->SetOrientationToVertical();
  this->ScalarBarWidget2DGreen->GetScalarBarActor()->SetNumberOfLabels(6);
  this->ScalarBarWidget2DGreen->GetScalarBarActor()->SetMaximumNumberOfColors(6);
  this->ScalarBarWidget2DGreen->GetScalarBarActor()->SetTitle("Dose(Gy)");
  this->ScalarBarWidget2DGreen->GetScalarBarActor()->SetLabelFormat(" %s");
  
  // it's a 2d actor, position it in screen space by percentages
  this->ScalarBarWidget2DGreen->GetScalarBarActor()->SetPosition(0.1, 0.1);
  this->ScalarBarWidget2DGreen->GetScalarBarActor()->SetWidth(0.1);
  this->ScalarBarWidget2DGreen->GetScalarBarActor()->SetHeight(0.8);
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
vtkSlicerIsodoseModuleLogic*
qSlicerIsodoseModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerIsodoseModuleWidget);
  return vtkSlicerIsodoseModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidgetPrivate::updateScalarBarsFromSelectedColorTable()
{
  Q_Q(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = this->logic()->GetIsodoseNode();
  if (!q->mrmlScene() || !paramNode)
  {
    return;
  }

  vtkMRMLColorTableNode* selectedColorNode = paramNode->GetColorTableNode();
  if (!selectedColorNode)
  {
    qDebug() << "qSlicerIsodoseModuleWidgetPrivate::updateScalarBarsFromSelectedColorTable: No color table node is selected";
    return;
  }

  this->tableView_IsodoseLevels->setMRMLColorNode(selectedColorNode);

  // 3D scalar bar
  int numberOfColors = selectedColorNode->GetNumberOfColors();
  this->ScalarBarWidget->GetScalarBarActor()->SetLookupTable(selectedColorNode->GetLookupTable());
  for (int colorIndex=0; colorIndex<numberOfColors; ++colorIndex)
  {
#if (VTK_MAJOR_VERSION <= 5)
    this->ScalarBarActor->SetColorName(colorIndex, selectedColorNode->GetColorName(colorIndex));
#else
    this->ScalarBarActor->GetLookupTable()->SetAnnotation(colorIndex, vtkStdString(selectedColorNode->GetColorName(colorIndex)));
#endif
  }
  // 2D scalar bar
  this->ScalarBarActor2DRed->SetLookupTable(selectedColorNode->GetLookupTable());
  this->ScalarBarActor2DYellow->SetLookupTable(selectedColorNode->GetLookupTable());
  this->ScalarBarActor2DGreen->SetLookupTable(selectedColorNode->GetLookupTable());

  for (int colorIndex=0; colorIndex<numberOfColors; ++colorIndex)
  {
#if (VTK_MAJOR_VERSION <= 5)
    this->ScalarBarActor2DRed->SetColorName(colorIndex, selectedColorNode->GetColorName(colorIndex));
    this->ScalarBarActor2DYellow->SetColorName(colorIndex, selectedColorNode->GetColorName(colorIndex));
    this->ScalarBarActor2DGreen->SetColorName(colorIndex, selectedColorNode->GetColorName(colorIndex));
#else
    this->ScalarBarActor2DRed->GetLookupTable()->SetAnnotation(colorIndex, vtkStdString(selectedColorNode->GetColorName(colorIndex)));
    this->ScalarBarActor2DYellow->GetLookupTable()->SetAnnotation(colorIndex, vtkStdString(selectedColorNode->GetColorName(colorIndex)));
    this->ScalarBarActor2DGreen->GetLookupTable()->SetAnnotation(colorIndex, vtkStdString(selectedColorNode->GetColorName(colorIndex)));
#endif
  }
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
qSlicerIsodoseModuleWidget::~qSlicerIsodoseModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerIsodoseModuleWidget);

  this->Superclass::setMRMLScene(scene);

  // Find parameters node or create it if there is no one in the scene
  if (scene && d->logic()->GetIsodoseNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
    if (node)
    {
      this->setIsodoseNode( vtkMRMLIsodoseNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
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
    qCritical() << "qSlicerIsodoseModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerIsodoseModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << "qSlicerIsodoseModuleWidget::onEnter: Invalid logic!";
    return;
  }
  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();

  // If we have a parameter node select it
  if (!paramNode)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
    if (node)
    {
      paramNode = vtkMRMLIsodoseNode::SafeDownCast(node);
      d->logic()->SetAndObserveIsodoseNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLIsodoseNode> newNode = vtkSmartPointer<vtkMRMLIsodoseNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveIsodoseNode(newNode);
    }
  }

  // set up default color node
  d->updateScalarBarsFromSelectedColorTable();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
    if (paramNode->GetDoseVolumeNode())
    {
      d->MRMLNodeComboBox_DoseVolume->setCurrentNode(paramNode->GetDoseVolumeNode());
    }
    else
    {
      this->doseVolumeNodeChanged(d->MRMLNodeComboBox_DoseVolume->currentNode());
    }

    d->updateScalarBarsFromSelectedColorTable();

    vtkMRMLColorTableNode* colorTableNode = paramNode->GetColorTableNode();       
    if (!colorTableNode)
    {
      qCritical() << "qSlicerIsodoseModuleWidget::updateWidgetFromMRML: Invalid color table node!";
      return;
    }
    d->spinBox_NumberOfLevels->setValue(colorTableNode->GetNumberOfColors());
    d->checkBox_Isoline->setChecked(paramNode->GetShowIsodoseLines());
    d->checkBox_Isosurface->setChecked(paramNode->GetShowIsodoseSurfaces());
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
  d->MRMLNodeComboBox_DoseVolume->addAttribute( QString("vtkMRMLScalarVolumeNode"), SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());

  // Make connections
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setIsodoseNode(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->spinBox_NumberOfLevels, SIGNAL(valueChanged(int)), this, SLOT(setNumberOfLevels(int)));

  connect( d->checkBox_ShowDoseVolumesOnly, SIGNAL( stateChanged(int) ), this, SLOT( showDoseVolumesOnlyCheckboxChanged(int) ) );
  connect( d->checkBox_Isoline, SIGNAL(toggled(bool)), this, SLOT( setIsolineVisibility(bool) ) );
  connect( d->checkBox_Isosurface, SIGNAL(toggled(bool)), this, SLOT( setIsosurfaceVisibility(bool) ) );
  connect( d->checkBox_ScalarBar, SIGNAL(toggled(bool)), this, SLOT( setScalarBarVisibility(bool) ) );
  connect( d->checkBox_ScalarBar2D, SIGNAL(toggled(bool)), this, SLOT( setScalarBar2DVisibility(bool) ) );

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

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
  d->updateScalarBarsFromSelectedColorTable();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsodoseNode(vtkMRMLNode *node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerIsodoseModuleWidget::setIsodoseNode: Invalid scene!";
    return;
  }
  
  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetIsodoseNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveIsodoseNode(paramNode);

  if (paramNode)
  {
    // Set default color table ID if none specified yet
    if (!paramNode->GetColorTableNode())
    {
      vtkMRMLColorTableNode* defaultIsodoseColorTableNode = vtkMRMLColorTableNode::SafeDownCast(
        this->mrmlScene()->GetNodeByID( d->logic()->GetDefaultIsodoseColorTableNodeId() ));
      paramNode->SetAndObserveColorTableNode(defaultIsodoseColorTableNode);
    }
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerIsodoseModuleWidget::doseVolumeNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  if (d->logic()->DoseVolumeContainsDose())
  {
    d->label_NotDoseVolumeWarning->setText("");
  }
  else
  {
    d->label_NotDoseVolumeWarning->setText(tr(" Selected volume is not a dose"));
  }

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setNumberOfLevels(int newNumber)
{
  Q_D(qSlicerIsodoseModuleWidget);
  if (!d->spinBox_NumberOfLevels->isEnabled() || !d->logic()->GetIsodoseNode())
  {
    qCritical() << "qSlicerIsodoseModuleWidget::setNumberOfLevels: Invalid parameter set node!";
    return;
  }

  d->logic()->SetNumberOfIsodoseLevels(newNumber);
  vtkMRMLColorTableNode* selectedColorNode = d->logic()->GetIsodoseNode()->GetColorTableNode();
  if (!selectedColorNode)
  {
    qCritical() << "qSlicerIsodoseModuleWidget::setNumberOfLevels: Invalid color table node!";
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
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::outputHierarchyNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerIsodoseModuleWidget::outputHierarchyNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveIsodoseSurfaceModelsParentHierarchyNode(vtkMRMLModelHierarchyNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::showDoseVolumesOnlyCheckboxChanged(int aState)
{
  Q_D(qSlicerIsodoseModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerIsodoseModuleWidget::showDoseVolumesOnlyCheckboxChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowDoseVolumesOnly(aState);
  paramNode->DisableModifiedEventOff();

  if (aState)
  {
    d->MRMLNodeComboBox_DoseVolume->addAttribute("vtkMRMLScalarVolumeNode", SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
  }
  else
  {
    d->MRMLNodeComboBox_DoseVolume->removeAttribute("vtkMRMLScalarVolumeNode", SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
  }
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
    qCritical() << "qSlicerIsodoseModuleWidget::setIsolineVisibility: Invalid scene!";
    return;
  }

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowIsodoseLines(visible);
  paramNode->DisableModifiedEventOff();

  vtkMRMLModelHierarchyNode* modelHierarchyNode = paramNode->GetIsodoseSurfaceModelsParentHierarchyNode();
  if(!modelHierarchyNode)
  {
    qCritical() << "qSlicerIsodoseModuleWidget::setIsolineVisibility: Invalid isodose surface models parent hierarchy node!";
    return;
  }

  vtkSmartPointer<vtkCollection> childModelNodes = vtkSmartPointer<vtkCollection>::New();
  modelHierarchyNode->GetChildrenModelNodes(childModelNodes);
  childModelNodes->InitTraversal();
  for (int i=0; i<childModelNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(childModelNodes->GetItemAsObject(i));
    modelNode->GetDisplayNode()->SetSliceIntersectionVisibility(visible);
  }
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsosurfaceVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerIsodoseModuleWidget::setIsosurfaceVisibility: Invalid scene!";
    return;
  }

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowIsodoseSurfaces(visible);
  paramNode->DisableModifiedEventOff();

  vtkMRMLModelHierarchyNode* modelHierarchyNode = paramNode->GetIsodoseSurfaceModelsParentHierarchyNode();
  if(!modelHierarchyNode)
  {
    return;
  }

  vtkSmartPointer<vtkCollection> childModelNodes = vtkSmartPointer<vtkCollection>::New();
  modelHierarchyNode->GetChildrenModelNodes(childModelNodes);
  childModelNodes->InitTraversal();
  for (int i=0; i<childModelNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(childModelNodes->GetItemAsObject(i));
    modelNode->GetDisplayNode()->SetVisibility(visible);
  }
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setScalarBarVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerIsodoseModuleWidget::setScalarBarVisibility: Invalid scene!";
    return;
  }

  if (d->ScalarBarWidget == 0)
  {
    return;
  }
  if (visible)
  {
#if (VTK_MAJOR_VERSION <= 5)
    d->ScalarBarActor->UseColorNameAsLabelOn();
#else
    d->ScalarBarActor->UseAnnotationAsLabelOn();
#endif
  }
  vtkMRMLColorTableNode* selectedColorNode = d->logic()->GetIsodoseNode()->GetColorTableNode();
  if (!selectedColorNode)
  {
    qCritical() << "qSlicerIsodoseModuleWidget::setScalarBarVisibility: Invalid color table node!";
    return;
  }
  int numberOfColors = selectedColorNode->GetNumberOfColors();
  for (int i=0; i<numberOfColors; i++)
  {
#if (VTK_MAJOR_VERSION <= 5)
    d->ScalarBarActor->SetColorName(i, selectedColorNode->GetColorName(i));
#else
    d->ScalarBarActor->GetLookupTable()->SetAnnotation(i, vtkStdString(selectedColorNode->GetColorName(i)));
#endif
  }

  d->ScalarBarWidget->SetEnabled(visible);
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setScalarBar2DVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerIsodoseModuleWidget::setScalarBar2DVisibility: Invalid scene!";
    return;
  }

  if (d->ScalarBarWidget2DRed == 0 || d->ScalarBarWidget2DYellow == 0 || d->ScalarBarWidget2DGreen == 0)
  {
    return;
  }
  if (visible)
  {
#if (VTK_MAJOR_VERSION <= 5)
    d->ScalarBarActor2DRed->UseColorNameAsLabelOn();
    d->ScalarBarActor2DYellow->UseColorNameAsLabelOn();
    d->ScalarBarActor2DGreen->UseColorNameAsLabelOn();
#else
    d->ScalarBarActor2DRed->UseAnnotationAsLabelOn();
    d->ScalarBarActor2DYellow->UseAnnotationAsLabelOn();
    d->ScalarBarActor2DGreen->UseAnnotationAsLabelOn();
#endif
  }
  vtkMRMLColorTableNode* selectedColorNode = d->logic()->GetIsodoseNode()->GetColorTableNode();
  if (!selectedColorNode)
  {
    qCritical() << "qSlicerIsodoseModuleWidget::setScalarBar2DVisibility: Invalid color table node!";
    return;
  }
  int numberOfColors = selectedColorNode->GetNumberOfColors();
  for (int i=0; i<numberOfColors; i++)
  {
#if (VTK_MAJOR_VERSION <= 5)
    d->ScalarBarActor2DRed->SetColorName(i, selectedColorNode->GetColorName(i));
    d->ScalarBarActor2DYellow->SetColorName(i, selectedColorNode->GetColorName(i));
    d->ScalarBarActor2DGreen->SetColorName(i, selectedColorNode->GetColorName(i));
#else
    d->ScalarBarActor2DRed->GetLookupTable()->SetAnnotation(i, vtkStdString(selectedColorNode->GetColorName(i)));
    d->ScalarBarActor2DYellow->GetLookupTable()->SetAnnotation(i, vtkStdString(selectedColorNode->GetColorName(i)));
    d->ScalarBarActor2DGreen->GetLookupTable()->SetAnnotation(i, vtkStdString(selectedColorNode->GetColorName(i)));
#endif
  }

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
    qCritical() << "qSlicerIsodoseModuleWidget::applyClicked: Invalid scene!";
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the isodose surface for the selected dose volume
  d->logic()->CreateIsodoseSurfaces();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateButtonsState()
{
  Q_D(qSlicerIsodoseModuleWidget);

  bool applyEnabled = d->logic()->GetIsodoseNode()
                   && d->logic()->GetIsodoseNode()->GetDoseVolumeNode()
                   && d->logic()->GetIsodoseNode()->GetColorTableNode()
                   && d->logic()->GetIsodoseNode()->GetColorTableNode()->GetNumberOfColors() > 0;
  d->pushButton_Apply->setEnabled(applyEnabled);
}
