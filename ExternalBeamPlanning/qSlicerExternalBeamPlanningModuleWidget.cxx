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

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// QSlicer includes
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h> 
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <qSlicerAbstractCoreModule.h>

// ExtensionTemplate Logic includes
#include <vtkSlicerCLIModuleLogic.h>

// SlicerRt includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLRTProtonBeamNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLLayoutNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QTime>

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

  int CurrentBeamRow;
  int NumberOfBeamRows;
};

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
  CurrentBeamRow = -1;
  NumberOfBeamRows = 0;
}

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::~qSlicerExternalBeamPlanningModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic*
qSlicerExternalBeamPlanningModuleWidgetPrivate::logic() const
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
qSlicerExternalBeamPlanningModuleWidget::~qSlicerExternalBeamPlanningModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find a plan node and select it if there is one in the scene
  if (scene && d->logic()->GetRTPlanNode())
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
    if (node)
    {
      this->rtPlanNodeChanged(node);
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
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (d->logic() == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic!";
    return;
  }

  // Select RT plan node
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (rtPlanNode == NULL)
  {
    // If not, try to find one in the scene
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
    if (node)
    {
      rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(node);
    }
    // Otherwise, create a new one
    //TODO: Commented out, as in Slicer new nodes should not be created simply by switching to a module
    //else
    //{
    //  vtkSmartPointer<vtkMRMLRTPlanNode> newNode = vtkSmartPointer<vtkMRMLRTPlanNode>::New();
    //  rtPlanNode = newNode;
    //  this->mrmlScene()->AddNode(newNode);
    //}
  }

  // Update UI from plan node
  this->updateWidgetFromMRML();

  // This is not used?
  d->ModuleWindowInitialized = true;
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
  //  qWarning() << Q_FUNC_INFO << ": MatlabDoseCalculation module is not found!";
  //}
  
  // Make connections
  this->connect( d->MRMLNodeComboBox_RtPlan, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(rtPlanNodeChanged(vtkMRMLNode*)) );

  // RT plan page
  this->connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_PlanSegmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(planSegmentationNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_PlanPOIs, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(planPOIsNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_DoseROI, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseROINodeChanged(vtkMRMLNode*)) );
  this->connect( d->lineEdit_DoseGridSpacing, SIGNAL(textChanged(const QString &)), this, SLOT(doseGridSpacingChanged(const QString &)) );
  this->connect( d->comboBox_DoseEngineType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(doseEngineTypeChanged(const QString &)) );

  // RT Beams page
  this->connect( d->tableWidget_Beams, SIGNAL(cellClicked(int, int)), this, SLOT(tableWidgetCellClicked(int, int)) );
  this->connect( d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeamClicked()) );
  this->connect( d->pushButton_RemoveBeam, SIGNAL(clicked()), this, SLOT(removeBeamClicked()) );

  // Beam global parameters
  this->connect( d->lineEdit_BeamName, SIGNAL(textChanged(const QString &)), this, SLOT(beamNameChanged(const QString &)) );
  this->connect( d->comboBox_RadiationType, SIGNAL(currentIndexChanged(int)), this, SLOT(radiationTypeChanged(int)) );

  // Prescription page
  this->connect( d->comboBox_BeamType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(beamTypeChanged(const QString &)) );
  this->connect( d->MRMLSegmentSelectorWidget_TargetVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(targetSegmentationNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLSegmentSelectorWidget_TargetVolume, SIGNAL(currentSegmentChanged(QString)), this, SLOT(targetSegmentChanged(const QString&)) );
  this->connect( d->doubleSpinBox_RxDose, SIGNAL(valueChanged(double)), this, SLOT(rxDoseChanged(double)) );
  this->connect( d->comboBox_IsocenterSpec, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(isocenterSpecChanged(const QString &)));
  this->connect( d->MRMLCoordinatesWidget_IsocenterCoordinates, SIGNAL(coordinatesChanged(double*)), this, SLOT(isocenterCoordinatesChanged(double *)));

  // Energy page
  this->connect( d->doubleSpinBox_ProximalMargin, SIGNAL(valueChanged(double)), this, SLOT(proximalMarginChanged(double)) );
  this->connect( d->doubleSpinBox_DistalMargin, SIGNAL(valueChanged(double)), this, SLOT(distalMarginChanged(double)) );

  // Proton widgets
  this->connect( d->comboBox_BeamLineType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(beamLineTypeChanged(const QString &)) );
  this->connect( d->checkBox_EnergyPrescription, SIGNAL(clicked(bool)), this, SLOT(manualEnergyPrescriptionChanged(bool)) );
  this->connect( d->doubleSpinBox_MinimumEnergy, SIGNAL(valueChanged(double)), this, SLOT(minimumEnergyChanged(double)) );
  this->connect( d->doubleSpinBox_MaximumEnergy, SIGNAL(valueChanged(double)), this, SLOT(maximumEnergyChanged(double)) );

  // Geometry page
  this->connect( d->MRMLNodeComboBox_MLCPositionDoubleArray, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(mlcPositionDoubleArrayNodeChanged(vtkMRMLNode*)) );
  this->connect( d->doubleSpinBox_SAD, SIGNAL(valueChanged(double)), this, SLOT(sourceDistanceChanged(double)) );
  this->connect( d->RangeWidget_XJawsPosition, SIGNAL(valuesChanged(double, double)), this, SLOT(xJawsPositionValuesChanged(double, double)) );
  this->connect( d->RangeWidget_YJawsPosition, SIGNAL(valuesChanged(double, double)), this, SLOT(yJawsPositionValuesChanged(double, double)) );
  this->connect( d->SliderWidget_CollimatorAngle, SIGNAL(valueChanged(double)), this, SLOT(collimatorAngleChanged(double)) );
  this->connect( d->SliderWidget_GantryAngle, SIGNAL(valueChanged(double)), this, SLOT(gantryAngleChanged(double)) );
  this->connect( d->SliderWidget_CouchAngle, SIGNAL(valueChanged(double)), this, SLOT(couchAngleChanged(double)) );
  this->connect( d->doubleSpinBox_Smearing, SIGNAL(valueChanged(double)), this, SLOT(smearingChanged(double)) );
  this->connect( d->doubleSpinBox_BeamWeight, SIGNAL(valueChanged(double)), this, SLOT(beamWeightChanged(double)) );

  // Proton beam model
  this->connect( d->doubleSpinBox_ApertureDistance, SIGNAL(valueChanged(double)), this, SLOT(apertureDistanceChanged(double)) );
  this->connect( d->comboBox_Algorithm, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(algorithmChanged(const QString &)) );
  this->connect( d->doubleSpinBox_PencilBeamSpacing, SIGNAL(valueChanged(double)), this, SLOT(pencilBeamSpacingChanged(double)) );
  this->connect( d->doubleSpinBox_SourceSize, SIGNAL(valueChanged(double)), this, SLOT(sourceSizeChanged(double)) );
  this->connect( d->doubleSpinBox_EnergyResolution, SIGNAL(valueChanged(double)), this, SLOT(energyResolutionChanged(double)) );
  this->connect( d->doubleSpinBox_EnergySpread, SIGNAL(valueChanged(double)), this, SLOT(energySpreadChanged(double)) );
  this->connect( d->doubleSpinBox_StepLength, SIGNAL(valueChanged(double)), this, SLOT(stepLengthChanged(double)) );
  this->connect( d->checkBox_WEDApproximation, SIGNAL(clicked(bool)), this, SLOT(wedApproximationChanged(bool)) );
  this->connect( d->checkBox_RangeCompensatorHighland, SIGNAL(clicked(bool)), this, SLOT(rangeCompensatorHighlandChanged(bool)) );

  // Beam visualization
  this->connect( d->pushButton_UpdateDRR, SIGNAL(clicked()), this, SLOT(updateDRRClicked()) );
  this->connect( d->checkBox_BeamEyesView, SIGNAL(clicked(bool)), this, SLOT(beamEyesViewClicked(bool)) );
  this->connect( d->checkBox_ContoursInBEW, SIGNAL(clicked(bool)), this, SLOT(contoursInBEWClicked(bool)) );

  // Calculation buttons
  this->connect( d->pushButton_CalculateDose, SIGNAL(clicked()), this, SLOT(calculateDoseClicked()) );
  this->connect( d->pushButton_CalculateWED, SIGNAL(clicked()), this, SLOT(calculateWEDClicked()) );
  this->connect( d->pushButton_ClearDose, SIGNAL(clicked()), this, SLOT(clearDoseClicked()) );

  // Hide non-functional items //TODO:
  d->label_DoseROI->setVisible(false);
  d->MRMLNodeComboBox_DoseROI->setVisible(false);
  d->label_DoseGridSpacing->setVisible(false);
  d->lineEdit_DoseGridSpacing->setVisible(false);

  // Remove all tabs in Beam TabWidget
  d->tabWidget->clear();

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

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  d->MRMLNodeComboBox_RtPlan->setCurrentNode(rtPlanNode);

  // Enable GUI only if a plan node is selected
  d->CollapsibleButton_RTPlan->setEnabled(rtPlanNode);
  d->CollapsibleButton_BeamParameters->setEnabled(rtPlanNode);

  if (!rtPlanNode)
  {
    // GCS FIX TODO: Still need to wipe the GUI in this case
    return;
  }

  if (rtPlanNode->GetReferenceVolumeNode())
  {
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(rtPlanNode->GetReferenceVolumeNode());
  }
  if (rtPlanNode->GetSegmentationNode())
  {
    d->MRMLNodeComboBox_PlanSegmentation->setCurrentNode(rtPlanNode->GetSegmentationNode());
  }
  if (rtPlanNode->GetDoseVolumeNode())
  {
    d->MRMLNodeComboBox_DoseVolume->setCurrentNode(rtPlanNode->GetDoseVolumeNode());
  }
  if (rtPlanNode->GetMarkupsFiducialNode())
  {
    d->MRMLNodeComboBox_PlanPOIs->setCurrentNode(rtPlanNode->GetMarkupsFiducialNode());
  }

  // Set beam-specific UI items
  this->updateWidgetFromRTBeam(this->currentBeamNode());

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateWidgetFromRTBeam(vtkMRMLRTBeamNode* beamNode)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // If node is empty, remove all tabs
  if (!beamNode)
  {
    d->tabWidget->clear();
    d->lineEdit_BeamName->setText("");
    // GCS FIX TODO How do I disconnect ?
    return;
  }

  // TODO: This should be where the beam node is selected, not on update
  qvtkConnect(beamNode, vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()));

  // Enable appropriate tabs and widgets for this beam type and set 
  // widget values from MRML node
  vtkMRMLRTBeamNode::RTRadiationType radType = beamNode->GetRadiationType();
  if (radType == vtkMRMLRTBeamNode::Photon)
  {
    // Enable or disable widgets on the prescription tab 
    d->label_BeamType->setEnabled(true);
    d->comboBox_BeamType->setEnabled(true);
    
    // Disable unneeded fields
    // Just in case there is a common index between photons and protons
    d->label_BeamLineType->setEnabled(false);
    d->comboBox_BeamLineType->setEnabled(false);
    d->label_EnergyPrescription->setEnabled(false);
    d->checkBox_EnergyPrescription->setEnabled(false);
    d->label_MinimumEnergy->setEnabled(false);
    d->doubleSpinBox_MinimumEnergy->setEnabled(false);
    d->label_MaximumEnergy->setEnabled(false);
    d->doubleSpinBox_MaximumEnergy->setEnabled(false);

    // Disable unneeded tabs
    int index;
    index = d->tabWidget->indexOf(d->tabWidgetPageProtonBeamModel);
    if (index >= 0)
    {
      d->tabWidget->removeTab(index);
    }

    // Enable needed tabs and set widget values
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageGeometry, "Geometry");
    }
  }
  else if (radType == vtkMRMLRTBeamNode::Proton)
  {
    // Enable needed fields
    // Just in case there is a common index between photons and protons
    d->label_BeamLineType->setEnabled(true);
    d->comboBox_BeamLineType->setEnabled(true);
    d->label_EnergyPrescription->setEnabled(true);
    d->checkBox_EnergyPrescription->setEnabled(true);

    // no energy prescription, so must be disabled for the moment
    d->label_MinimumEnergy->setEnabled(false);
    d->doubleSpinBox_MinimumEnergy->setEnabled(false);
    d->label_MaximumEnergy->setEnabled(false);
    d->doubleSpinBox_MaximumEnergy->setEnabled(false);

    // Disable unneeded tabs
    int index;

    // Enable needed tabs and set widget values
    index = d->tabWidget->indexOf(d->tabWidgetPageRx);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageRx, "Prescription");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageProtonEnergy);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageProtonEnergy, "Energy");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageGeometry, "Geometry");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageProtonBeamModel);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageProtonBeamModel, "Beam Model");
    }
  }
  else if (radType == vtkMRMLRTBeamNode::Electron)
  {
    // Not implemented
    qWarning() << Q_FUNC_INFO << ": Electron beam not yet supported";
    d->tabWidget->clear();
  }

  // Set values into beam parameters tab
  d->lineEdit_BeamName->setText(QString::fromStdString(beamNode->GetName()));
  switch (beamNode->GetRadiationType())
  {
  case vtkMRMLRTBeamNode::Photon:
    d->comboBox_RadiationType->setCurrentIndex(1);
    break;
  case vtkMRMLRTBeamNode::Electron:
    d->comboBox_RadiationType->setCurrentIndex(2);
    break;
  default:
    d->comboBox_RadiationType->setCurrentIndex(0); // Proton
    break;
  }

  // Set values into prescription tab
  switch (beamNode->GetBeamType())
  {
  case vtkMRMLRTProtonBeamNode::Dynamic:
    d->comboBox_RadiationType->setCurrentIndex(1);
    break;
  default:
    d->comboBox_RadiationType->setCurrentIndex(0); // Static
    break;
  }

  //d->doubleSpinBox_RxDose->setValue(beamNode->GetRxDose()); The RxDose doesn't need to be reset, it is the same for the plan

  double iso[3];
  beamNode->GetIsocenterPosition (iso);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(true);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->setCoordinates(iso);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(false);

  double rdp[3] = {beamNode->GetReferenceDosePointPosition(0), beamNode->GetReferenceDosePointPosition(1), beamNode->GetReferenceDosePointPosition(2)};
  d->MRMLCoordinatesWidget_DosePointCoordinates->setCoordinates(rdp);

  // Set values into energy tab
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  d->doubleSpinBox_ProximalMargin->setValue(protonNode->GetProximalMargin());
  d->doubleSpinBox_DistalMargin->setValue(protonNode->GetDistalMargin());
  if (protonNode->GetBeamLineType() == true)
  {
    d->comboBox_BeamLineType->setCurrentIndex(0);
  }
  else
  {
    d->comboBox_BeamLineType->setCurrentIndex(1);
  }
  d->checkBox_EnergyPrescription->setChecked(protonNode->GetManualEnergyLimits());
  d->doubleSpinBox_MinimumEnergy->setValue(protonNode->GetMinimumEnergy());
  d->doubleSpinBox_MaximumEnergy->setValue(protonNode->GetMaximumEnergy());

  // Set values into geometry tab
  d->doubleSpinBox_SAD->setValue(beamNode->GetSAD());
  d->RangeWidget_XJawsPosition->setValues(beamNode->GetX1Jaw(), beamNode->GetX2Jaw());
  d->RangeWidget_YJawsPosition->setValues(beamNode->GetY1Jaw(), beamNode->GetY2Jaw());
  d->SliderWidget_CollimatorAngle->setValue(beamNode->GetCollimatorAngle());
  d->SliderWidget_GantryAngle->blockSignals(true);
  d->SliderWidget_GantryAngle->setValue(beamNode->GetGantryAngle());
  d->SliderWidget_GantryAngle->blockSignals(false);
  d->SliderWidget_CouchAngle->setValue(beamNode->GetCouchAngle());
  d->doubleSpinBox_Smearing->setValue(beamNode->GetSmearing());
  d->doubleSpinBox_BeamWeight->setValue(beamNode->GetBeamWeight());

  // Set values into proton beam model
  if (beamNode->GetRadiationType() == vtkMRMLRTBeamNode::Proton)
  {
    d->doubleSpinBox_ApertureDistance->setValue(protonNode->GetApertureOffset());
    switch (protonNode->GetAlgorithm())
    {
    case vtkMRMLRTProtonBeamNode::CGS:
      d->comboBox_Algorithm->setCurrentIndex(1);
      break;
    case vtkMRMLRTProtonBeamNode::DGS:
      d->comboBox_Algorithm->setCurrentIndex(2);
      break;
    case vtkMRMLRTProtonBeamNode::HGS:
      d->comboBox_Algorithm->setCurrentIndex(3);
      break;
    default: // Ray Tracer or any other mistake
      d->comboBox_Algorithm->setCurrentIndex(0);
      break;
    }
    d->doubleSpinBox_PencilBeamSpacing->setValue(protonNode->GetPBResolution());
    d->doubleSpinBox_SourceSize->setValue(protonNode->GetSourceSize());
    d->doubleSpinBox_EnergyResolution->setValue(protonNode->GetEnergyResolution());
    d->doubleSpinBox_EnergySpread->setValue(protonNode->GetEnergySpread());
    d->doubleSpinBox_StepLength->setValue(protonNode->GetStepLength());
    d->checkBox_WEDApproximation->setChecked(protonNode->GetLateralSpreadHomoApprox());
    d->checkBox_RangeCompensatorHighland->setChecked(protonNode->GetRangeCompensatorHighland());
  }
}

//-----------------------------------------------------------------------------
vtkMRMLRTBeamNode* qSlicerExternalBeamPlanningModuleWidget::currentBeamNode()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return NULL;
  }

  QTableWidgetItem *item = NULL;
  item = d->tableWidget_Beams->item(d->CurrentBeamRow, 0);
  if (!item)
  {
    return NULL;
  }
  int beamNumber = item->text().toInt();
  vtkMRMLRTBeamNode* beamNode = rtPlanNode->GetBeamByNumber(beamNumber);

  return beamNode;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rtPlanNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = vtkMRMLRTPlanNode::SafeDownCast(node);
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect(d->logic()->GetRTPlanNode(), rtPlanNode, vtkCommand::ModifiedEvent, this, SLOT(onRTPlanNodeModified()));

  d->logic()->SetAndObserveRTPlanNode(rtPlanNode);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onRTPlanNodeModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onLogicModified()
{
  this->updateRTBeamTableWidget();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onRTBeamNodeModifiedEvent()
{
  //qDebug() << "Got onRTBeamNodeModifiedEvent"; //TODO:
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Clear the beam table
  d->tableWidget_Beams->setRowCount(0);
  d->tableWidget_Beams->setColumnCount(0);
  d->tableWidget_Beams->clearContents();

  // Get RT plan node
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  { 
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // Get beam nodes for the plan
  std::vector<vtkMRMLRTBeamNode*> beams;
  rtPlanNode->GetBeams(beams);

  // Set up the table
  d->tableWidget_Beams->setColumnCount(4);
  QStringList headerLabels;
  headerLabels << "Beam#" << "Name" << "Type" << "Radiation";

  d->tableWidget_Beams->blockSignals(true);
  d->tableWidget_Beams->setColumnWidth(0, 24);
  d->tableWidget_Beams->setHorizontalHeaderLabels(headerLabels);
  d->tableWidget_Beams->setRowCount(beams.size());

  // Fill the table
  int i=0;
  for (std::vector<vtkMRMLRTBeamNode*>::iterator it=beams.begin(); it!=beams.end(); ++it, ++i)
  {
    vtkMRMLRTBeamNode* beamNode = *it;
    if (beamNode)
    {
      d->tableWidget_Beams->setItem(i, 0, new QTableWidgetItem( QString::number(beamNode->GetBeamNumber()) ) );
      d->tableWidget_Beams->setItem(i, 1, new QTableWidgetItem( QString(beamNode->GetName()) ) );
      d->tableWidget_Beams->setItem(i, 2, new QTableWidgetItem( QString::number(beamNode->GetGantryAngle()) ) );
    }
  }

  d->tableWidget_Beams->blockSignals(false);
  if (d->CurrentBeamRow >= 0)
  {
    d->tableWidget_Beams->selectRow(d->CurrentBeamRow);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::planSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::planPOIsNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();

  // GCS FIX TODO: Update beams isocenters
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseROINodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  qWarning() << Q_FUNC_INFO << ": Not implemented!";

  // rtPlanNode->DisableModifiedEventOn();
  // rtPlanNode->SetAndObserveDoseROINode(vtkMRMLAnnotationsROINode::SafeDownCast(node));
  // rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseGridSpacingChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  qWarning() << Q_FUNC_INFO << ": Not implemented!";

  // TODO: to be implemented
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseEngineTypeChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  if (text.compare("Plastimatch") == 0)
  {
    rtPlanNode->SetDoseEngine(vtkMRMLRTPlanNode::Plastimatch);
  }
  else if (text.compare("PMH") == 0)
  {
    rtPlanNode->SetDoseEngine(vtkMRMLRTPlanNode::PMH);
  }
  else if (text.compare("Matlab") == 0)
  {
    rtPlanNode->SetDoseEngine(vtkMRMLRTPlanNode::Matlab);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::addBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // Create new beam node by replicating currently selected beam
  vtkMRMLRTBeamNode* beamNode = NULL;
  if (this->currentBeamNode())
  {
    beamNode = rtPlanNode->CopyAndAddBeam(this->currentBeamNode());
  }
  else
  {
    //TODO: Create the type of beam specified by the selected dose engine plugin
    beamNode = vtkMRMLRTProtonBeamNode::New();
    beamNode->SetName(rtPlanNode->GenerateNewBeamName().c_str());
    this->mrmlScene()->AddNode(beamNode);
    beamNode->CreateDefaultBeamModel();
    beamNode->Delete(); // Return ownership to scene only
    rtPlanNode->AddBeam(beamNode);
  }
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to add beam!";
    return;
  }

  // Disconnect events for currently selected row
  qvtkDisconnect(beamNode, vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()));
  
  // Make new beam current in the table
  d->CurrentBeamRow = d->NumberOfBeamRows++;
  d->tableWidget_Beams->selectRow(d->CurrentBeamRow);

  // Clear instruction text
  d->label_CalculateDoseStatus->setText("");

  // GCS TODO FIX -- this should be called when logic is modified, maybe it gets called twice?
  this->updateRTBeamTableWidget();

  // Update UI
  this->updateWidgetFromRTBeam(beamNode);

  // Update beam visualization
  this->updateCurrentBeamTransform();

  // Update beam visualization
  this->updateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::removeBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    return;
  }

  // Disconnect events for currently selected row
  qvtkDisconnect( beamNode, vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()) );
  
  // Remove beam
  rtPlanNode->RemoveBeam(beamNode);

  // Select a different beam as current
  d->NumberOfBeamRows--;
  if (d->CurrentBeamRow >= d->NumberOfBeamRows)
  {
    d->CurrentBeamRow = d->NumberOfBeamRows-1;
  }

  // Update UI table
  d->tableWidget_Beams->selectRow(d->CurrentBeamRow);
  this->updateRTBeamTableWidget();

  // Update lower UI tabs
  beamNode = this->currentBeamNode();
  this->updateWidgetFromRTBeam(beamNode);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::tableWidgetCellClicked(int row, int column)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(column);

  // Don't do anything when clicking on already selected beam
  if (row == d->CurrentBeamRow)
  {
    return;
  }

  // Disconnect events for currently selected row
  qvtkDisconnect (this->currentBeamNode(), vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()));

  // Make sure inputs are initialized
  d->CurrentBeamRow = row;
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    qCritical() << "tableWidgetItemClicked: Inputs are not initialized!";
    return;
  }

  // Copy data from beam into tabs UI
  this->updateWidgetFromRTBeam(beamNode);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamNameChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    return;
  }
  beamNode->SetName(text.toStdString().c_str());

  // Update in table
  this->updateRTBeamTableWidget();
  
  // Update beam visualization
  //this->updateCurrentBeamTransform();

  // Update beam visualization
  //this->updateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::radiationTypeChanged(int index)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (index == -1)
  {
    return;
  }

  // GCS FIX TODO: This needs to make changes to the node, then set 
  // values from node into UI

#if defined (commentout)
  QString text = d->comboBox_RadiationType->currentText();

  if (text == "Photon")
  {
    // Make Photon pages visible and others invisible
    int index =-1;
    index = d->tabWidget->indexOf(d->tabWidgetPageBeams);
    if (index >= 0)
    {
      d->tabWidget->removeTab(index);
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageRx);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageRx, "Prescription");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageGeometry, "Geometry");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageBeams);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageBeams, "Beam Model");
    }
  }
  else if (text == "Proton")
  {
    // Make Photon pages visible and others invisible
    int index =-1;
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index >=0)
    {
      d->tabWidget->removeTab(index);
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageBeams);
    if (index >= 0)
    {
      d->tabWidget->removeTab(index);
    }
    index = 0;
    index = d->tabWidget->indexOf(d->tabWidgetPageRx);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageRx, "Prescription");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageGeometry, "Geometry");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageBeams);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageBeams, "Beam Model");
    }

  }
  else if (text == "Electron")
  {
  }
#endif
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamTypeChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL) {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }

  if (text.compare("Dynamic") == 0)
  {
    beamNode->SetBeamType(vtkMRMLRTBeamNode::Dynamic);
  }
  else
  {
    beamNode->SetBeamType(vtkMRMLRTBeamNode::Static);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::targetSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  //TODO: The segmentation node should be fixed for the plan
  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    return;
  }
  beamNode->SetAndObserveTargetSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::targetSegmentChanged(const QString& segment)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    return;
  }

  // Set target segment ID
  beamNode->SetTargetSegmentID(segment.toLatin1().constData());

  if (beamNode->GetIsocenterSpec() == vtkMRMLRTBeamNode::CenterOfTarget)
  {
    d->logic()->SetBeamIsocenterToTargetCenter(beamNode);

    // GCS FIX TODO: Testing...I copied this from above.  I should be getting
    // a markup moved event, why didn't I?
    double iso[3];
    beamNode->GetIsocenterPosition(iso);
    d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(true);
    d->MRMLCoordinatesWidget_IsocenterCoordinates->setCoordinates(iso);
    d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(false);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rxDoseChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  rtPlanNode->SetRxDose(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterSpecChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current beam node!";
    return;
  }

  if (text.compare("Center of target") == 0)
  {
    qDebug() << "Setting isocenter spec to center of target";
    d->logic()->SetBeamIsocenterToTargetCenter(beamNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterCoordinatesChanged(double *coords)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node!";
    return;
  }

  beamNode->SetIsocenterPosition(coords);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterFiducialNodeChangedfromCoordinates(double* coordinates)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(coordinates);

  //TODO: Implement
  qWarning() << Q_FUNC_INFO << ": Not implemented!";
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::dosePointFiducialNodeChangedfromCoordinates(double* coordinates)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(coordinates);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // TODO: to be implemented
  qWarning() << Q_FUNC_INFO << ": Not implemented!";
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::proximalMarginChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetProximalMargin(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::distalMarginChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetDistalMargin(value);
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamLineTypeChanged(const QString &text)
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  
  if (text.compare("Active scanning") == 0)
  {
    protonNode->SetBeamLineType("active");
  }
  else
  {
    protonNode->SetBeamLineType("passive");
  }
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::manualEnergyPrescriptionChanged(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }

  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetHavePrescription(checked);

  if (protonNode->GetHavePrescription())
  {
    d->doubleSpinBox_MinimumEnergy->setEnabled(true);
    d->doubleSpinBox_MaximumEnergy->setEnabled(true);
    d->label_MinimumEnergy->setEnabled(true);
    d->label_MaximumEnergy->setEnabled(true);
  }
  else
  {
    d->doubleSpinBox_MinimumEnergy->setEnabled(false);
    d->doubleSpinBox_MaximumEnergy->setEnabled(false);
    d->label_MinimumEnergy->setEnabled(false);
    d->label_MaximumEnergy->setEnabled(false);
  }
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::minimumEnergyChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }

  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetMinimumEnergy(value);
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::maximumEnergyChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }

  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
  protonNode->SetMaximumEnergy(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::mlcPositionDoubleArrayNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // GCS FIX TODO *** Come back to this later ***
#if defined (commentout)

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  // Get rt plan node for ExternalBeamPlanning node
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetExternalBeamPlanningNode()->GetRtPlanNode();
  if (!rtPlanNode)
  { 
    qCritical() << Q_FUNC_INFO << ": Invalid rtplan node!";
    return;
  }

  paramNode->SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Inputs are not initialized!";
    return;
  }

  beamNode->SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode::SafeDownCast(node));
#endif
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::xJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  beamNode->SetX1Jaw(minVal);
  beamNode->SetX2Jaw(maxVal);

  // Update beam visualization
  this->updateCurrentBeamTransform();
  this->updateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::yJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  beamNode->SetY1Jaw(minVal);
  beamNode->SetY2Jaw(maxVal);

  // Update beam visualization
  this->updateCurrentBeamTransform();
  this->updateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::gantryAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam selected!";
    return;
  }
  beamNode->SetGantryAngle(value);

  // Update beam visualization
  this->updateCurrentBeamTransform();

  // Update the table
  this->updateRTBeamTableWidget();

  // Update beam visualization
  this->updateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::collimatorAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode) {
    qCritical() << Q_FUNC_INFO << ": No current beam selected!";
    return;
  }
  beamNode->SetCollimatorAngle(value);

  // Update beam visualization
  this->updateCurrentBeamTransform();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::couchAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam selected!";
    return;
  }
  beamNode->SetCouchAngle(value);

  // Update beam visualization

  this->updateCurrentBeamTransform();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::smearingChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL) {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  
  beamNode->SetSmearing(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamWeightChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  beamNode->SetBeamWeight(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::sourceDistanceChanged(double value)
{
   Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  beamNode->SetSAD(value);

  // Update beam visualization
  this->updateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::apertureDistanceChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetApertureOffset(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::algorithmChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);

  if (text == "Ray tracer algorithm")
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::RayTracer);
    d->label_CalculateDoseStatus->setText("Ray Tracer Algorithm");
  }
  else if (text == "Cartesian geometry dose calculation")
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::CGS);
    d->label_CalculateDoseStatus->setText("Cartesian Geometry Algorithm");
  }
  else if (text == "Divergent geometry dose calculation")
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::DGS);
    d->label_CalculateDoseStatus->setText("Divergent Geometry Algorithm");
  }
  else if (text == "Hong dose calculation")
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::HGS);
    d->label_CalculateDoseStatus->setText("Hong Algorithm");
  }
  else
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::RayTracer);
    d->label_CalculateDoseStatus->setText("Algorithm error, Ray Tracer chosen by default");
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::pencilBeamSpacingChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  // GCS FIX TODO: Why are the function names different?
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetPBResolution(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::sourceSizeChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetSourceSize(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::energyResolutionChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
  protonNode->SetEnergyResolution(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::energySpreadChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
  protonNode->SetEnergySpread(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::stepLengthChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetStepLength(value);
}


//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::wedApproximationChanged(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetLateralSpreadHomoApprox(checked);
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rangeCompensatorHighlandChanged(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetRangeCompensatorHighland(checked);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamEyesViewClicked(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  if (checked)
  {
    qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutTwoOverTwoView);
  }
  else
  {
    qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutFourUpView);
  }

  //TODO: Set camera
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::contoursInBEWClicked(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // TODO: add the logic to check if contours should be included in the DRR view
  // right now the contours are included always. 
  if (checked)
  {
  }
  else
  {
  }
  qWarning() << Q_FUNC_INFO << ": Not implemented!";
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateDRRClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No beam node is selected!";
    return;
  }

  d->logic()->UpdateDRR(beamNode->GetName());
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateWEDClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting WED calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
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
void qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting dose calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    QString errorString("No RT plan node selected");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }
  // Make sure inputs were specified
  if (rtPlanNode->GetDoseEngine() != vtkMRMLRTPlanNode::Plastimatch)
  {
    QString errorString("Dose calculation is not available for this dose calculation engine");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }
  vtkMRMLScalarVolumeNode* referenceVolume = rtPlanNode->GetReferenceVolumeNode();
  if (!referenceVolume)
  {
    QString errorString("No reference anatomical volume is selected");
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = rtPlanNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    QString errorString("No plan segmentation node is selected"); // MD Fix TODO -> dose could be computed without target
    d->label_CalculateDoseStatus->setText(errorString);
    qCritical() << Q_FUNC_INFO << ": " << errorString;
    return;
  }

  // Start timer
  QTime time;
  time.start();
  
  // The last verifications were fine so we can compute the dose
  // Dose Calculation - loop on all the beam and sum in a global dose matrix

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetBeams(beams);
  if (!beams) 
  {
    d->label_CalculateDoseStatus->setText("No beam found in the plan");
    return;
  }
  vtkMRMLRTProtonBeamNode* beamNode = NULL;

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  std::string errorMessage = d->logic()->InitializeAccumulatedDose();
  if (!errorMessage.empty())
  {
    d->label_CalculateDoseStatus->setText(errorMessage.c_str());
    QApplication::restoreOverrideCursor();
    return;
  }

  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      QString progressMessage = QString("Dose calculation in progress: %1").arg(beamNode->GetName());
      d->label_CalculateDoseStatus->setText(progressMessage);

      errorMessage = d->logic()->ComputeDose(beamNode);
      if (!errorMessage.empty())
      {
        d->label_CalculateDoseStatus->setText(errorMessage.c_str());
        break;
      }

      //TODO: sum in the final dose matrix with weights externalbeam for RxDose, and beamNode for beam weight
    }
    else
    {
      QString message = QString("Beam %1 not found").arg(beamNode->GetName());
      d->label_CalculateDoseStatus->setText(message);
    }
  }
  
  errorMessage = d->logic()->FinalizeAccumulatedDose();
  if (!errorMessage.empty())
  {
    d->label_CalculateDoseStatus->setText(errorMessage.c_str());
  }
  else
  {
    d->label_CalculateDoseStatus->setText("Dose calculation done");
  }
  QApplication::restoreOverrideCursor();

  qDebug() << Q_FUNC_INFO << ": Dose calculated in " << time.elapsed() << " ms";

  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::clearDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->logic()->RemoveDoseNodes();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateBeamParameters()
{
  //Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // TODO:
  std::string errorMessage;
  //d->logic()->UpdateBeam();
  qWarning() << Q_FUNC_INFO << ": Not implemented!";
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateCurrentBeamTransform()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTBeamNode* beamNode = this->currentBeamNode();
  if (beamNode)
  {
    beamNode->UpdateBeamTransform();
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateBeamGeometryModel()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  QTableWidgetItem *item = NULL;
  item = d->tableWidget_Beams->item(d->CurrentBeamRow, 1);
  if (!item)
  {
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": " << "No RT plan node selected";
    return;
  }

  // Get beam node by name
  std::string beamName(item->text().toLatin1().constData());
  vtkMRMLRTBeamNode* beamNode = rtPlanNode->GetBeamByName(beamName.c_str());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << "Unable to access beam node with name " << beamName.c_str();
    return;
  }

  beamNode->UpdateBeamGeometry();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::collimatorTypeChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);
  
  //TODO:
  qWarning() << Q_FUNC_INFO << ": Not implemented!";
}
