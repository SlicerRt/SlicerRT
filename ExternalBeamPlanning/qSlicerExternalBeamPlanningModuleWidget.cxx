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

// SlicerQt includes
#include "qSlicerExternalBeamPlanningModuleWidget.h"
#include "ui_qSlicerExternalBeamPlanningModule.h"

// Slicer includes
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <qSlicerAbstractCoreModule.h>

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// Beams includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkSlicerBeamsModuleLogic.h"

// ExternalBeamPlanning MRML and logic includes
#include "vtkMRMLRTObjectiveNode.h"
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"

// ExternalBeamPlanning Widgets includes
#include "qSlicerAbstractDoseEngine.h"
#include "qSlicerAbstractObjective.h"
#include "qSlicerAbstractPlanOptimizer.h"
#include "qSlicerDoseEngineLogic.h"
#include "qSlicerDoseEnginePluginHandler.h"
#include "qSlicerMockDoseEngine.h"
#include "qSlicerMockPlanOptimizer.h"
#include "qSlicerObjectiveLogic.h"
#include "qSlicerObjectivePluginHandler.h"
#include "qSlicerPlanOptimizerLogic.h"
#include "qSlicerPlanOptimizerPluginHandler.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QTime>
#include <QItemSelection>
#include <QMessageBox>
#include <QDir>

// PythonQt includes
#include "PythonQt.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerExternalBeamPlanningModuleWidgetPrivate: public Ui_qSlicerExternalBeamPlanningModule
{
  Q_DECLARE_PUBLIC(qSlicerExternalBeamPlanningModuleWidget);
protected:
  qSlicerExternalBeamPlanningModuleWidget* const q_ptr;
public:
  qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object);
  ~qSlicerExternalBeamPlanningModuleWidgetPrivate();
  vtkSlicerExternalBeamPlanningModuleLogic* logic() const;
public:
  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
  /// Dose engine logic for dose calculation related functions
  qSlicerDoseEngineLogic* DoseEngineLogic;
  /// Optimization engine logic for optimization related functions
  qSlicerPlanOptimizerLogic* PlanOptimizerLogic;
};

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
  , DoseEngineLogic(nullptr)
  , PlanOptimizerLogic(nullptr)
{
  this->DoseEngineLogic = new qSlicerDoseEngineLogic(&object);
  this->PlanOptimizerLogic = new qSlicerPlanOptimizerLogic(&object);
}

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::~qSlicerExternalBeamPlanningModuleWidgetPrivate()
{
  if (this->DoseEngineLogic)
  {
    delete this->DoseEngineLogic;
    this->DoseEngineLogic = nullptr;
  }
  if (this->PlanOptimizerLogic)
  {
    delete this->PlanOptimizerLogic;
    this->PlanOptimizerLogic = nullptr;
  }
}

//-----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic* qSlicerExternalBeamPlanningModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerExternalBeamPlanningModuleWidget);
  return vtkSlicerExternalBeamPlanningModuleLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidget::qSlicerExternalBeamPlanningModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerExternalBeamPlanningModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidget::~qSlicerExternalBeamPlanningModuleWidget() = default;

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Set scene to dose engine logic
  d->DoseEngineLogic->setMRMLScene(scene);

  // Set scene to optimization engine logic
  d->PlanOptimizerLogic->setMRMLScene(scene);

  // Find a plan node and select it if there is one in the scene
  if (scene && d->MRMLNodeComboBox_RtPlan->currentNode())
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
    if (node)
    {
      this->setPlanNode(node);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();

  //
  // Register dose engines and optimizers if not already done
  //

  // Inline function for determining if a dose engine is already registered by class name
  auto isDoseEngineRegistered = [](const QString& className) -> bool
    {
      QList<qSlicerAbstractDoseEngine*> registeredEngines = qSlicerDoseEnginePluginHandler::instance()->registeredDoseEngines();
      for (qSlicerAbstractDoseEngine* engine : registeredEngines)
      {
        if (engine->metaObject()->className() == className)
        {
          return true;
        }
      }
      return false;
    };
  // Register built-in C++ dose engines
  if (!isDoseEngineRegistered("qSlicerMockDoseEngine"))
  {
    qSlicerDoseEnginePluginHandler::instance()->registerDoseEngine(new qSlicerMockDoseEngine());
  }

  // Python engines
  // (otherwise it would be the responsibility of the module that embeds the dose engine)
  PythonQt::init();
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  context.evalScript( QString(
    "from DoseEngines import * \n"
    "import qSlicerExternalBeamPlanningModuleWidgetsPythonQt as engines \n"
    "import traceback \n"
    "import logging \n"
    "try: \n"
    "  slicer.modules.doseEngineNames \n"
    "except AttributeError: \n"
    "  slicer.modules.doseEngineNames = [] \n"
    "for engineName in slicer.modules.doseEngineNames: \n"
    "  try: \n"
    "    exec(\"{0}Instance = engines.qSlicerScriptedDoseEngine(None); {0}Instance.setPythonSource({0}.__file__.replace('\\\\\\\\','/')); {0}Instance.self().register()\".format(engineName)) \n"
    "  except Exception: \n"
    "    logging.error(traceback.format_exc()) \n") );

  // Register the Beams module's beam parameters tab widget to the dose engines
  qSlicerAbstractDoseEngine::registerBeamParametersTabWidget(qSlicerAbstractDoseEngine::beamParametersTabWidgetFromBeamsModule());

  // Register optimizers
  qSlicerPlanOptimizerPluginHandler::instance()->registerPlanOptimizer(new qSlicerMockPlanOptimizer());

  // Python optimizers
  // (otherwise it would be the responsibility of the module that embeds the plan optimizer)
  context.evalScript( QString(
    "from PlanOptimizers import * \n"
    "import qSlicerExternalBeamPlanningModuleWidgetsPythonQt as optimizers \n"
    "import traceback \n"
    "import logging \n"
    "try: \n"
    "  slicer.modules.planOptimizerNames \n"
    "except AttributeError: \n"
    "  slicer.modules.planOptimizerNames=[] \n"
    "for optimizerName in slicer.modules.planOptimizerNames: \n"
    "  try: \n"
    "    exec(\"{0}Instance = optimizers.qSlicerScriptedPlanOptimizer(None); {0}Instance.setPythonSource({0}.__file__.replace('\\\\\\\\','/')); {0}Instance.self().register()\".format(optimizerName)) \n"
    "  except Exception: \n"
    "    logging.error(traceback.format_exc()) \n") );
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (d->logic() == nullptr)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic";
    return;
  }

  // Select RT plan node
  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (planNode == nullptr)
  {
    // Try to find one in the scene
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
    if (node)
    {
      planNode = vtkMRMLRTPlanNode::SafeDownCast(node);
    }
  }

  // Update UI from plan node
  this->updateWidgetFromMRML();

  // This is not used?
  d->ModuleWindowInitialized = true;

  // Run Python script to check and install pyRadPlan if needed
  PythonQt::init();
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  context.evalScript(QString(
    "try:\n"
    " import pyRadPlan\n"
    " assert pyRadPlan.__version__ == '0.2.8'\n"
    "except (ImportError, AttributeError, AssertionError):\n"
    " if slicer.util.confirmOkCancelDisplay("
    "   'This module requires pyRadPlan.\\nClick OK to install it now. Slicer will automatically restart.'\n"
    " ):\n"
    "   slicer.util.pip_install('pyRadPlan==0.2.8')\n"
    "   slicer.app.restart()\n"));
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setup()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Check for matlab dose calculation module //TODO: Re-enable when matlab dose adaptor is created
  //vtkSlicerExternalBeamPlanningModuleLogic* externalBeamPlanningModuleLogic =
  //  vtkSlicerExternalBeamPlanningModuleLogic::SafeDownCast(this->logic());
  //qSlicerAbstractCoreModule* matlabDoseCalculationModule =
  //  qSlicerCoreApplication::application()->moduleManager()->module("MatlabDoseCalculation");
  //if (matlabDoseCalculationModule)
  //{
  //  vtkSlicerCLIModuleLogic* matlabDoseCalculationModuleLogic =
  //    vtkSlicerCLIModuleLogic::SafeDownCast(matlabDoseCalculationModule->logic());
  //  externalBeamPlanningModuleLogic->SetMatlabDoseCalculationModuleLogic(matlabDoseCalculationModuleLogic);
  //}
  //else
  //{
  //  qWarning() << Q_FUNC_INFO << ": MatlabDoseCalculation module is not found";
  //}

  // Make connections
  connect( d->MRMLNodeComboBox_RtPlan, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setPlanNode(vtkMRMLNode*)) );

  // Plan parameters section
  connect(d->checkBox_IonPlanFlag, SIGNAL(stateChanged(int)), this, SLOT(ionPlanFlagCheckboxStateChanged(int)));

  connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_PlanSegmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(segmentationNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_PlanPOIs, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(poisMarkupsNodeChanged(vtkMRMLNode*)) );

  connect( d->MRMLCoordinatesWidget_IsocenterCoordinates, SIGNAL(coordinatesChanged(double*)), this, SLOT(isocenterCoordinatesChanged(double *)));
  connect( d->pushButton_CenterViewToIsocenter, SIGNAL(clicked()), this, SLOT(centerViewToIsocenterClicked()) );

  connect( d->MRMLSegmentSelectorWidget_TargetStructure, SIGNAL(currentSegmentChanged(QString)), this, SLOT(targetSegmentChanged(const QString&)) );
  connect( d->checkBox_IsocenterAtTargetCenter, SIGNAL(stateChanged(int)), this, SLOT(isocenterAtTargetCenterCheckboxStateChanged(int)));

  connect( d->checkBox_InversePlanning, SIGNAL(stateChanged(int)), this, SLOT(inversePlanningCheckboxStateChanged(int)));

  connect( d->comboBox_DoseEngine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(doseEngineChanged(const QString&)) );
  connect( d->doubleSpinBox_RxDose, SIGNAL(valueChanged(double)), this, SLOT(rxDoseChanged(double)) );

  // Output section
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_DoseROI, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseROINodeChanged(vtkMRMLNode*)) );
  connect(d->doubleSpinBox_DoseGridSpacingX, SIGNAL(valueChanged(double)), this, SLOT(doseGridSpacingXComponentChanged(double)));
  connect(d->doubleSpinBox_DoseGridSpacingY, SIGNAL(valueChanged(double)), this, SLOT(doseGridSpacingYComponentChanged(double)));
  connect(d->doubleSpinBox_DoseGridSpacingZ, SIGNAL(valueChanged(double)), this, SLOT(doseGridSpacingZComponentChanged(double)));
  connect(d->pushButton_UseCTGridForDoseGridSpacing, SIGNAL(clicked()), this, SLOT(useCTGridForDoseGridSpacingClicked()));

  // Beams section
  //connect( d->BeamsTableView, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(beamSelectionChanged(QItemSelection,QItemSelection) ) );
  connect( d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeamClicked()) );
  connect( d->pushButton_RemoveBeam, SIGNAL(clicked()), this, SLOT(removeBeamClicked()) );

  // Calculation buttons
  connect( d->pushButton_CalculateDose, SIGNAL(clicked()), this, SLOT(calculateDoseClicked()) );
  connect( d->pushButton_CalculateWED, SIGNAL(clicked()), this, SLOT(calculateWEDClicked()) );
  connect( d->pushButton_ClearDose, SIGNAL(clicked()), this, SLOT(clearDoseClicked()) );

  // Plan Optimization
  connect( d->comboBox_PlanOptimizer, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(PlanOptimizerChanged(const QString&)));
  connect( d->pushButton_OptimizePlan, SIGNAL(clicked()), this, SLOT(optimizePlanClicked()));

  // Objective Table
  connect(d->pushButton_AddObjective, SIGNAL(clicked()), this, SLOT(addObjectiveClicked()));
  connect(d->pushButton_RemoveObjective, SIGNAL(clicked()), this, SLOT(removeObjectiveClicked()));

  // Connect to progress event
  connect( d->DoseEngineLogic, SIGNAL(progressUpdated(double)), this, SLOT(onProgressUpdated(double)) );
  connect( d->PlanOptimizerLogic, SIGNAL(progressInfoUpdated(QString)), this, SLOT(onOptimizerProgressInfoUpdated(QString)) );

  // Hide non-functional items //TODO:
  d->label_DoseROI->setVisible(false);
  d->MRMLNodeComboBox_DoseROI->setVisible(false);

  // Set status text to initial instruction
  d->label_CalculateDoseStatus->setText("Add plan and beam to start planning");

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());

  // Enable GUI only if a plan node is selected
  d->CollapsibleButton_PlanParameters->setEnabled(planNode);
  d->CollapsibleButton_Beams->setEnabled(planNode);

  if (!planNode)
  {
    return;
  }

  // Plan parameters section
  d->checkBox_IonPlanFlag->setChecked(planNode->GetIonPlanFlag());

  // None is enabled for the reference volume and segmentation comboboxes, and invalid selection
  // in plan node is set to GUI so that the user needs to select nodes that are then set to the beams.
  d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(planNode->GetReferenceVolumeNode());
  d->MRMLNodeComboBox_PlanSegmentation->setCurrentNode(planNode->GetSegmentationNode());

  if (planNode->GetPoisMarkupsFiducialNode() && d->MRMLNodeComboBox_PlanPOIs->currentNode())
  {
    d->MRMLNodeComboBox_PlanPOIs->setCurrentNode(planNode->GetPoisMarkupsFiducialNode());
  }
  if (planNode->GetOutputTotalDoseVolumeNode() && d->MRMLNodeComboBox_DoseVolume->currentNode())
  {
    d->MRMLNodeComboBox_DoseVolume->setCurrentNode(planNode->GetOutputTotalDoseVolumeNode());
  }

  // Set target segment
  d->MRMLSegmentSelectorWidget_TargetStructure->setCurrentNode(planNode->GetSegmentationNode());
  d->MRMLSegmentSelectorWidget_TargetStructure->setCurrentSegmentID(planNode->GetTargetSegmentID());

  // Update isocenter specification
  d->checkBox_IsocenterAtTargetCenter->setChecked(planNode->GetIsocenterSpecification() == vtkMRMLRTPlanNode::CenterOfTarget);
  // Update isocenter controls based on plan isocenter position
  this->updateIsocenterPosition();

  // Update Dose Grid Spacing
  d->doubleSpinBox_DoseGridSpacingX->setValue(planNode->GetDoseGridSpacing()[0]);
  d->doubleSpinBox_DoseGridSpacingY->setValue(planNode->GetDoseGridSpacing()[1]);
  d->doubleSpinBox_DoseGridSpacingZ->setValue(planNode->GetDoseGridSpacing()[2]);

  // Populate dose engines combobox and make selection
  this->updateDoseEngines();

  // Populate optimiization engines combobox
  this->updatePlanOptimizers();

  // Set prescription
  d->doubleSpinBox_RxDose->setValue(planNode->GetRxDose());

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setPlanNode(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(node);

  // Make sure the plan node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_RtPlan->setCurrentNode(planNode);

  // Set plan node in beams table
  d->BeamsTableView->setPlanNode(planNode);

  // Set plan node in objectives table
  d->ObjectivesTableWidget->setPlanNode(planNode);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect(planNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
  qvtkReconnect(planNode, vtkMRMLRTPlanNode::IsocenterModifiedEvent, this, SLOT(updateIsocenterPosition()));

  if (planNode)
  {
    // for a newly created plan add ion plan flag manually
    if (d->checkBox_IonPlanFlag->isChecked() && !planNode->GetIonPlanFlag())
    {
      planNode->SetIonPlanFlag(true);
    }

    // Set input segmentation and reference volume if specified by DICOM
    vtkIdType planShItemID = shNode->GetItemByDataNode(planNode);
    if (!planShItemID)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy item for plan " << planNode->GetName();
      return;
    }
    vtkIdType referencedSegmentationShItemID = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    std::vector<vtkIdType> referencedShItemsFromPlan = shNode->GetItemsReferencedFromItemByDICOM(planShItemID);
    for (std::vector<vtkIdType>::iterator refIt=referencedShItemsFromPlan.begin(); refIt!=referencedShItemsFromPlan.end(); ++refIt)
    {
      vtkIdType referencedShItemID = (*refIt);
      vtkMRMLSegmentationNode* referencedSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(
        shNode->GetItemDataNode(referencedShItemID) );
      if (referencedSegmentationNode)
      {
        // Referenced structure set segmentation node found
        referencedSegmentationShItemID = referencedShItemID;
        planNode->SetAndObserveSegmentationNode(referencedSegmentationNode);
        break;
      }
    }

    // Look for the reference anatomical volume too if referenced structure set was found
    if (referencedSegmentationShItemID)
    {
      std::vector<vtkIdType> referencedShItemsFromStructureSet = shNode->GetItemsReferencedFromItemByDICOM(referencedSegmentationShItemID);
      for (std::vector<vtkIdType>::iterator refIt=referencedShItemsFromStructureSet.begin(); refIt!=referencedShItemsFromStructureSet.end(); ++refIt)
      {
        vtkIdType referencedShItemID = (*refIt);
        vtkMRMLScalarVolumeNode* referencedVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
          shNode->GetItemDataNode(referencedShItemID) );
        if (referencedVolumeNode)
        {
          // Referenced volume found, set it as referenced anatomical volume
          planNode->SetAndObserveReferenceVolumeNode(referencedVolumeNode);
          break;
        }
      }
    }

    // Set dose engine from UI if not specified in plan
    if (!planNode->GetDoseEngineName())
    {
      planNode->SetDoseEngineName(d->comboBox_DoseEngine->currentText().toUtf8().constData());
    }


    // Set optimizer if not specified in plan
    if (!planNode->GetPlanOptimizerName() )
    {
      planNode->SetPlanOptimizerName(d->comboBox_PlanOptimizer->currentText().toUtf8().constData());
    }


    // Trigger update of IEC logic based on the first beam
    if (planNode->GetNumberOfBeams() > 0)
    {
      vtkMRMLRTBeamNode* firstBeamNode = planNode->GetBeamByNumber(1);
      if (firstBeamNode)
      {
        firstBeamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);
      }
    }

    // Clear instructions text
    d->label_CalculateDoseStatus->setText("");
    d->label_OptimizationStatus->setText("");
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onLogicModified()
{
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    }
    return;
  }

  planNode->DisableModifiedEventOn();
  planNode->SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  planNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::segmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    }
    return;
  }

  planNode->DisableModifiedEventOn();
  planNode->SetAndObserveSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  planNode->DisableModifiedEventOff();

  // Set segmentation node to target selector
  d->MRMLSegmentSelectorWidget_TargetStructure->setCurrentNode(node);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::poisMarkupsNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    }
    return;
  }

  planNode->DisableModifiedEventOn();
  planNode->SetAndObservePoisMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode::SafeDownCast(node));
  planNode->DisableModifiedEventOff();

  if (!node)
  {
    // Do not update beams or isocenter if no markups node was given
    return;
  }

  // Update beam transforms based on new isocenter
  std::vector<vtkMRMLRTBeamNode*> beams;
  planNode->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
  {
    // Calculate transform from beam parameters and isocenter from plan
    vtkMRMLRTBeamNode* beamNode = (*beamIt);
    beamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rxDoseChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  planNode->DisableModifiedEventOn();
  planNode->SetRxDose(value);
  planNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    }
    return;
  }

  planNode->DisableModifiedEventOn();
  planNode->SetAndObserveOutputTotalDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  planNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseROINodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    if (node)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    }
    return;
  }

  qWarning() << Q_FUNC_INFO << ": Not implemented";

  //TODO:
  // planNode->DisableModifiedEventOn();
  // planNode->SetAndObserveDoseROINode(vtkMRMLMarkupsROINode::SafeDownCast(node));
  // planNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseGridSpacingComponentChanged(int index, double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  planNode->DisableModifiedEventOn();
  planNode->SetDoseGridSpacingComponent(index, value);
  planNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseGridSpacingXComponentChanged(double value)
{
  this->doseGridSpacingComponentChanged(0, value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseGridSpacingYComponentChanged(double value)
{
  this->doseGridSpacingComponentChanged(1, value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseGridSpacingZComponentChanged(double value)
{
  this->doseGridSpacingComponentChanged(2, value);
}

//----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::useCTGridForDoseGridSpacingClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }


  planNode->DisableModifiedEventOn();
  planNode->SetDoseGridSpacingToCTGridSpacing();
  planNode->DisableModifiedEventOff();

  // Update GUI
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::targetSegmentChanged(const QString& segment)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  // Set target segment ID
  planNode->DisableModifiedEventOn();
  planNode->SetTargetSegmentID(segment.toUtf8().constData());
  planNode->DisableModifiedEventOff();

  if (planNode->GetIsocenterSpecification() == vtkMRMLRTPlanNode::CenterOfTarget)
  {
    planNode->SetIsocenterToTargetCenter();
    this->centerViewToIsocenterClicked();
  }

  // Trigger update of IEC logic based on the first beam
  if (planNode->GetNumberOfBeams() > 0)
  {
    vtkMRMLRTBeamNode* firstBeamNode = planNode->GetBeamByNumber(1);
    if (firstBeamNode)
    {
      firstBeamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterAtTargetCenterCheckboxStateChanged(int state)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  if (state > 0)
  {
    planNode->SetIsocenterSpecification(vtkMRMLRTPlanNode::CenterOfTarget);
  }
  else
  {
    planNode->SetIsocenterSpecification(vtkMRMLRTPlanNode::ArbitraryPoint);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterCoordinatesChanged(double* coords)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  // If isocenter specification is CenterOfTarget, then reset it to previous isocenter
  if (planNode->GetIsocenterSpecification() == vtkMRMLRTPlanNode::CenterOfTarget)
  {
    this->updateIsocenterPosition();
  }
  else // Otherwise (if ArbitraryPoint) set coordinates as isocenter position
  {
    planNode->DisableModifiedEventOn();
    planNode->SetIsocenterPosition(coords);
    planNode->DisableModifiedEventOff();
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::centerViewToIsocenterClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  // Get isocenter position
  double isocenter[3] = {0.0,0.0,0.0};
  if (!planNode->GetIsocenterPosition(isocenter))
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get plan isocenter for plan " << planNode->GetName();
  }

  // Navigate slice views to position
  std::vector<vtkMRMLNode*> nodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLSliceNode", nodes);
  for (std::vector<vtkMRMLNode*>::iterator nodeIt=nodes.begin(); nodeIt!=nodes.end(); ++nodeIt)
  {
    vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(*nodeIt);
    sliceNode->JumpSlice(isocenter[0], isocenter[1], isocenter[2]);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateIsocenterPosition()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  // Get isocenter position
  double isocenter[3] = {0.0,0.0,0.0};
  if (!planNode->GetIsocenterPosition(isocenter))
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get plan isocenter for plan " << planNode->GetName();
  }

  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(true);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->setCoordinates(isocenter);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(false);
}

//------------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::ionPlanFlagCheckboxStateChanged(int state)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  // delete all beams
  planNode->RemoveAllBeams();

  // update beam parameters for ion plan
  qSlicerDoseEnginePluginHandler::DoseEngineListType engines =
    qSlicerDoseEnginePluginHandler::instance()->registeredDoseEngines();

  for (qSlicerDoseEnginePluginHandler::DoseEngineListType::iterator engineIt = engines.begin();
    engineIt != engines.end(); ++engineIt)
  {
    if ((*engineIt)->canDoIonPlan())
    {
      (*engineIt)->updateBeamParametersForIonPlan(state);
    }
  }

  this->updateDoseEngines();

  // set ion plan flag in plan
  //TODO: Throws error if any beam in the plan is not fully initialized (e.g., missing ScanSpotTableNode).
  //      Calling SetIonPlanFlag(state) triggers beam visualization (CreateBeamPolyData), which fails if setup is incomplete.
  //      Ensure all beams are fully configured before setting this flag to true.
  planNode->SetIonPlanFlag(state);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::inversePlanningCheckboxStateChanged(int state)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  //TODO: should we write the inverse planning flag to the plan node?

  // Update dose engines
  this->updateDoseEngines();

  // Update Optimization engines
  this->updatePlanOptimizers();

  if (d->checkBox_InversePlanning->isChecked())
  {
      d->pushButton_OptimizePlan->setEnabled(true);
   d->CollapsibleButton_Objectives->setEnabled(true);
  }
  else
  {
      d->pushButton_OptimizePlan->setEnabled(false);
   d->CollapsibleButton_Objectives->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateDoseEngines()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  //Check if Ion Plan is selected
  bool ionPlan = d->checkBox_IonPlanFlag->isChecked();

  //Check if Inverse Planning is selected
  bool inversePlanning = d->checkBox_InversePlanning->isChecked();

  //Skip update if inverse planning is not active and all registered engines are already in the combobox
  qSlicerDoseEnginePluginHandler::DoseEngineListType engines =
    qSlicerDoseEnginePluginHandler::instance()->registeredDoseEngines();
  if (!ionPlan && !inversePlanning && engines.size() == d->comboBox_DoseEngine->count())
  {
    return;
  }

  d->comboBox_DoseEngine->blockSignals(true);
  d->comboBox_DoseEngine->clear();

  for (qSlicerDoseEnginePluginHandler::DoseEngineListType::iterator engineIt = engines.begin();
    engineIt != engines.end(); ++engineIt)
  {
    // Check if ion plan or inverse planning is active and if yes skip engines that can't do it
    if ((ionPlan && !(*engineIt)->canDoIonPlan()) || (inversePlanning && !(*engineIt)->isInverse()))
    {
        continue;
    }
    d->comboBox_DoseEngine->addItem((*engineIt)->name());
  }

  //TODO: Refine sanity handling of the case when no engines are available?
  if (d->comboBox_DoseEngine->count() == 0)
  {
      //qCritical() << Q_FUNC_INFO << ": No dose engines available";
      d->comboBox_DoseEngine->addItem("No dose engines available");
      d->comboBox_DoseEngine->setCurrentIndex(0);
      d->comboBox_DoseEngine->setDisabled(true);
      return;
  }
  else
      d->comboBox_DoseEngine->setDisabled(false);

  // Select previously selected engine
  QString selectedEngineName(planNode->GetDoseEngineName() ? planNode->GetDoseEngineName() : "");
  int index = d->comboBox_DoseEngine->findText(selectedEngineName);
  if (index != -1)
  {
    d->comboBox_DoseEngine->setCurrentIndex(index);
  }
  // If previous selection not found (e.g. no selection has been made yet), then select first engine
  else
  {
    d->comboBox_DoseEngine->setCurrentIndex(0);
  }
  // Apply engine selection (signals are blocked, plus if first index has been selected and it has
  // not been applied, then it needs to be done now)
  this->doseEngineChanged(d->comboBox_DoseEngine->currentText());

  // Update beam parameter tab visibility
  d->DoseEngineLogic->applyDoseEngineInPlan(planNode);

  d->comboBox_DoseEngine->blockSignals(false);
}

void qSlicerExternalBeamPlanningModuleWidget::updatePlanOptimizers()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  //Check if Inverse Planning is selected
  bool inversePlanning = d->checkBox_InversePlanning->isChecked();

  //Skip update if inverse planning is not active and all registered engines are already in the combobox
  qSlicerPlanOptimizerPluginHandler::PlanOptimizerListType engines =
    qSlicerPlanOptimizerPluginHandler::instance()->registeredPlanOptimizers();
  if (!inversePlanning && engines.size() == d->comboBox_PlanOptimizer->count())
  {
    return;
  }

  d->comboBox_PlanOptimizer->blockSignals(true);
  d->comboBox_PlanOptimizer->clear();

  for (qSlicerPlanOptimizerPluginHandler::PlanOptimizerListType::iterator engineIt = engines.begin();
    engineIt != engines.end(); ++engineIt)
  {
    d->comboBox_PlanOptimizer->addItem((*engineIt)->name());
  }

  //TODO: Refine sanity handling of the case when no engines are available?
  if (d->comboBox_PlanOptimizer->count() == 0)
  {
    //qCritical() << Q_FUNC_INFO << ": No dose engines available";
    d->comboBox_PlanOptimizer->addItem("No optimizers available");
    d->comboBox_PlanOptimizer->setCurrentIndex(0);
    d->comboBox_PlanOptimizer->setDisabled(true);
    return;
  }
  else
    d->comboBox_PlanOptimizer->setDisabled(false);

  // Select previously selected engine
  QString selectedEngineName(planNode->GetPlanOptimizerName() ? planNode->GetPlanOptimizerName() : "");
  int index = d->comboBox_PlanOptimizer->findText(selectedEngineName);
  if (index != -1)
  {
    d->comboBox_PlanOptimizer->setCurrentIndex(index);
  }
  // If previous selection not found (e.g. no selection has been made yet), then select first engine
  else
  {
    d->comboBox_PlanOptimizer->setCurrentIndex(0);
  }
  // Apply engine selection (signals are blocked, plus if first index has been selected and it has
  // not been applied, then it needs to be done now)
  this->PlanOptimizerChanged(d->comboBox_PlanOptimizer->currentText());

  // Update beam parameter tab visibility
  //d->PlanOptimizerLogic->applyDoseEngineInPlan(planNode);

  d->comboBox_PlanOptimizer->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseEngineChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  if (planNode->GetDoseEngineName() && !text.compare(planNode->GetDoseEngineName()))
  {
    return;
  }

  // Get newly selected dose engine
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(text.toUtf8().constData());
  if (!selectedEngine)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access dose engine with name" << text;
    return;
  }

  qDebug() << "Dose engine selection changed to " << text;
  planNode->DisableModifiedEventOn();
  planNode->SetDoseEngineName(selectedEngine->name().toUtf8().constData());
  planNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
vtkMRMLRTBeamNode* qSlicerExternalBeamPlanningModuleWidget::currentBeamNode()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return nullptr;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return nullptr;
  }

  QStringList beamNodeIDs = d->BeamsTableView->selectedBeamNodeIDs();
  if (beamNodeIDs.isEmpty())
  {
    return nullptr;
  }
  else if (beamNodeIDs.count() > 1)
  {
    qWarning() << Q_FUNC_INFO << ": Multiple beams selected, the first one is used for the current operation";
  }

  return vtkMRMLRTBeamNode::SafeDownCast( this->mrmlScene()->GetNodeByID(beamNodeIDs[0].toUtf8().constData()) );
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::addBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  // Create new beam node by replicating currently selected beam
  vtkMRMLRTBeamNode* beamNode = d->DoseEngineLogic->createBeamInPlan(planNode);
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to add beam";
    return;
  }

  // Add engine-specific beam parameters to newly created beam node
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(planNode->GetDoseEngineName());
  if (!selectedEngine)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access dose engine with name" << planNode->GetDoseEngineName();
    return;
  }
  selectedEngine->addBeamParameterAttributesToBeamNode(beamNode);

  // Clear instruction text
  d->label_CalculateDoseStatus->setText("");
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::removeBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  QStringList beamNodeIDs = d->BeamsTableView->selectedBeamNodeIDs();
  foreach (QString beamNodeID, beamNodeIDs)
  {
    // Remove beam
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(
      this->mrmlScene()->GetNodeByID(beamNodeID.toUtf8().constData()) );
    planNode->RemoveBeam(beamNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting dose calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    QString errorString("No RT plan node selected");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }

  // Create and select output dose volume if missing
  if (!d->checkBox_InversePlanning->isChecked() && !planNode->GetOutputTotalDoseVolumeNode())
  {
    vtkIdType planShItemID = shNode->GetItemByDataNode(planNode);
    if (!planShItemID)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy item for plan " << planNode->GetName();
      return;
    }
    vtkSmartPointer<vtkMRMLScalarVolumeNode> newDoseVolume = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    std::string newDoseVolumeName = std::string(planNode->GetName()) + "_TotalDose";
    newDoseVolume->SetName(newDoseVolumeName.c_str());
    this->mrmlScene()->AddNode(newDoseVolume);

    // Move total dose volume under study (same branch as plan)
    shNode->SetItemParent(shNode->GetItemByDataNode(newDoseVolume), shNode->GetItemParent(planShItemID));

    // Set volume to plan
    planNode->SetAndObserveOutputTotalDoseVolumeNode(newDoseVolume);

    // Set also on UI
    bool wasBlocked = d->MRMLNodeComboBox_DoseVolume->blockSignals(true);
    d->MRMLNodeComboBox_DoseVolume->setCurrentNode(newDoseVolume);
    d->MRMLNodeComboBox_DoseVolume->blockSignals(wasBlocked);
  }

  // Start timer
  QTime time;
  time.start();
  // Set busy cursor
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Get selected dose engine
  qSlicerAbstractDoseEngine* selectedEngine =
    qSlicerDoseEnginePluginHandler::instance()->doseEngineByName(planNode->GetDoseEngineName());
  if (!selectedEngine)
  {
    QString errorString = QString("Unable to access dose engine with name %1").arg(planNode->GetDoseEngineName() ? planNode->GetDoseEngineName() : "nullptr");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }

  // If inverse planning is selected, we do a sanity check for the dose engine capabilities
  if (d->checkBox_InversePlanning->isChecked() && !selectedEngine->isInverse())
  {
    QString errorString = QString("Selected Dose Engine %1 can't do dose influence matrix calculation!").arg(planNode->GetDoseEngineName() ? planNode->GetDoseEngineName() : "nullptr");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }

  // Calculate dose
  QString errorMessage;
  if (d->checkBox_InversePlanning->isChecked())
  {
    QString message = QString("Starting dose influence matrix calculation...");
    qDebug() << Q_FUNC_INFO << ": " << message;
    errorMessage = d->DoseEngineLogic->calculateDoseInfluenceMatrix(planNode);
  }
  else
  {
    QString message = QString("Starting forward dose calculation...");
    qDebug() << Q_FUNC_INFO << ": " << message;
    errorMessage = d->DoseEngineLogic->calculateDose(planNode);
  }

  if (errorMessage.isEmpty())
  {
    QString message = QString("Dose calculated successfully in %1 s").arg(time.elapsed()/1000.0);
    qDebug() << Q_FUNC_INFO << ": " << message;
    d->label_CalculateDoseStatus->setText(message);
  }
  else
  {
    QString message = QString("ERROR: %1").arg(errorMessage);
    qCritical() << Q_FUNC_INFO << ": " << message;
    d->label_CalculateDoseStatus->setText(message);
  }

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::optimizePlanClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_OptimizationStatus->setText("Starting optimization...");

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    QString errorString("No RT plan node selected");
    d->label_OptimizationStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }

  // Start timer
  QTime time;
  time.start();
  // Set busy cursor
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Get selected optimizer engine
  qSlicerAbstractPlanOptimizer* selectedEngine =
    qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
  if (!selectedEngine)
  {
    QString errorString = QString("Unable to access plan optimizer with name %1").arg(planNode->GetPlanOptimizerName() ? planNode->GetPlanOptimizerName() : "nullptr");
    d->label_OptimizationStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }

  // Optimize
  QString errorMessage;

  if (d->checkBox_InversePlanning->isChecked())
  {
    QString message = QString("Starting optimization...");
    qDebug() << Q_FUNC_INFO << ": " << message;
    errorMessage = d->PlanOptimizerLogic->optimizePlan(planNode);
  }

  if (errorMessage.isEmpty())
  {
    QString message = QString("Optimization calculated successfully in %1 s").arg(time.elapsed() / 1000.0);
    qDebug() << Q_FUNC_INFO << ": " << message;
    d->label_OptimizationStatus->setText(message);
  }
  else
  {
    QString message = QString("ERROR: %1").arg(errorMessage);
    qCritical() << Q_FUNC_INFO << ": " << message;
    d->label_OptimizationStatus->setText(message);
  }

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::PlanOptimizerChanged(const QString& text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node";
    return;
  }

  if (planNode->GetPlanOptimizerName() && !text.compare(planNode->GetPlanOptimizerName()))
  {
    return;
  }

  // Get newly selected Plan Optimzer
  qSlicerAbstractPlanOptimizer* selectedEngine =
    qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(text.toUtf8().constData());
  if (!selectedEngine)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access optimization engine with name" << text;
    return;
  }

  qDebug() << "Optimization engine selection changed to " << text;
  planNode->DisableModifiedEventOn();
  planNode->SetPlanOptimizerName(selectedEngine->name().toUtf8().constData());
  planNode->DisableModifiedEventOff();

  selectedEngine->setAvailableObjectives();
  d->ObjectivesTableWidget->deleteObjectivesTable();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onProgressUpdated(double progress)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  int progressPercent = (int)(progress * 100.0);
  QString progressMessage;
  if (d->checkBox_InversePlanning->isChecked())
    progressMessage = QString("Dose influence matrix calculation in progress: %1 %").arg(progressPercent);
  else
    progressMessage = QString("Dose calculation in progress: %1 %").arg(progressPercent);
  d->label_CalculateDoseStatus->setText(progressMessage);
  QApplication::processEvents();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::clearDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());
  d->DoseEngineLogic->removeIntermediateResults(planNode);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onOptimizerProgressInfoUpdated(QString info)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  QString progressMessage;
  progressMessage = QString("Optimization in progress: ") + info;
  d->label_OptimizationStatus->setText(progressMessage);
  QApplication::processEvents();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateWEDClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting WED calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  /// GCS FIX TODO *** Implementation needs work ***

#if defined (commentout)
  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  // Make sure inputs were specified - only CT needed
  vtkMRMLScalarVolumeNode* referenceVolume = planNode->GetReferenceVolumeNode();
  if (!referenceVolume)
  {
    d->label_CalculateDoseStatus->setText("No reference image");
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Copy pertinent variable values from GUI to logic
  // Is this the right place for this?
  d->logic()->GetExternalBeamPlanningNode()->SetGantryAngle(d->SliderWidget_GantryAngle->value());

  // OK, we're good to go (well, not really, but let's pretend).
  // Do the actual computation in the logic object
  d->logic()->ComputeWED();

  d->label_CalculateDoseStatus->setText("WED calculation done.");
  QApplication::restoreOverrideCursor();
#endif
}

//-----------------------------------------------------------------------------
bool qSlicerExternalBeamPlanningModuleWidget::setEditedNode(vtkMRMLNode* node, QString role/*=QString()*/, QString context/*=QString()*/)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  Q_UNUSED(role);
  Q_UNUSED(context);
  if (vtkMRMLRTPlanNode::SafeDownCast(node))
  {
    d->MRMLNodeComboBox_RtPlan->setCurrentNode(node);
    return true;
  }
  return false;
}


//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::addObjectiveClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->ObjectivesTableWidget->onObjectiveAdded();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::removeObjectiveClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->ObjectivesTableWidget->onObjectiveRemoved();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::saveAvailableObjectives()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // get planNode
  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(d->MRMLNodeComboBox_RtPlan->currentNode());

  // get selected optimizer
  qSlicerAbstractPlanOptimizer* selectedEngine =
    qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
}
