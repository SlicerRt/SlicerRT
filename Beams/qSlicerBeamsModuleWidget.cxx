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

// Qt includes
#include <QDebug>
#include <QTableWidgetItem>

// SlicerQt includes
#include "qSlicerBeamsModuleWidget.h"
#include "ui_qSlicerBeamsModule.h"

// Slicer includes
#include <qSlicerApplication.h>
#include <qSlicerSubjectHierarchyAbstractPlugin.h>

// Beams includes
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTProtonBeamNode.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceNode.h>

// SlicerRT includes
#include "SlicerRtCommon.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Beams
class qSlicerBeamsModuleWidgetPrivate: public Ui_qSlicerBeamsModule
{
  Q_DECLARE_PUBLIC(qSlicerBeamsModuleWidget);
protected:
  qSlicerBeamsModuleWidget* const q_ptr;
public:
  qSlicerBeamsModuleWidgetPrivate(qSlicerBeamsModuleWidget& object);
  vtkSlicerBeamsModuleLogic* logic() const;

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerBeamsModuleWidgetPrivate::qSlicerBeamsModuleWidgetPrivate(qSlicerBeamsModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
}

//-----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic* qSlicerBeamsModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerBeamsModuleWidget);
  return vtkSlicerBeamsModuleLogic::SafeDownCast(q->logic());
} 


//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerBeamsModuleWidget::qSlicerBeamsModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerBeamsModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerBeamsModuleWidget::~qSlicerBeamsModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerBeamsModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  Q_D(qSlicerBeamsModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic!";
    return;
  }

  d->ModuleWindowInitialized = true;

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::setup()
{
  Q_D(qSlicerBeamsModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Beam global parameters
  this->connect( d->MRMLNodeComboBox_RtBeam, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setBeamNode(vtkMRMLNode*)) );
  this->connect( d->pushButton_SwitchToParentPlan, SIGNAL(clicked()), this, SLOT(switchToParentPlanButtonClicked()) );
  this->connect( d->lineEdit_BeamName, SIGNAL(textChanged(const QString &)), this, SLOT(beamNameChanged(const QString &)) );
  this->connect( d->comboBox_RadiationType, SIGNAL(currentIndexChanged(int)), this, SLOT(radiationTypeChanged(int)) );

  // Prescription page
  this->connect( d->comboBox_BeamType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(beamTypeChanged(const QString &)) );
  this->connect( d->MRMLSegmentSelectorWidget_TargetVolume, SIGNAL(currentSegmentChanged(QString)), this, SLOT(targetSegmentChanged(const QString&)) );
  this->connect( d->doubleSpinBox_RxDose, SIGNAL(valueChanged(double)), this, SLOT(rxDoseChanged(double)) );
  this->connect( d->comboBox_IsocenterSpec, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(isocenterSpecChanged(const QString &)));
  this->connect( d->MRMLCoordinatesWidget_IsocenterCoordinates, SIGNAL(coordinatesChanged(double*)), this, SLOT(isocenterCoordinatesChanged(double *)));
  this->connect( d->pushButton_CenterViewToIsocenter, SIGNAL(clicked()), this, SLOT(centerViewToIsocenterClicked()) );

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

  // Remove all tabs in Beam TabWidget
  d->tabWidget->clear();

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::updateButtonsState()
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  bool applyEnabled = beamNode
                   && beamNode->GetParentPlanNode();
  d->pushButton_SwitchToParentPlan->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::updateIsocenterPosition()
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    return;
  }

  double iso[3] = {0.0,0.0,0.0};
  beamNode->GetIsocenterPosition(iso);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(true);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->setCoordinates(iso);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  // Get active beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());

  // Enable beam parameters section only if there is a valid beam node
  d->CollapsibleButton_BeamParameters->setEnabled(beamNode);

  // If node is empty, remove all tabs
  if (!beamNode)
  {
    d->tabWidget->clear();
    d->lineEdit_BeamName->setText("");
    // GCS FIX TODO How do I disconnect ?
    return;
  }

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
    int index = d->tabWidget->indexOf(d->tabWidgetPageProtonBeamModel);
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

  // Set segmentation to be the plan segmentation
  d->MRMLSegmentSelectorWidget_TargetVolume->setCurrentNode(beamNode->GetTargetSegmentationNode());

  //TODO: RxDose to plan?
  //d->doubleSpinBox_RxDose->setValue(beamNode->GetRxDose()); The RxDose doesn't need to be reset, it is the same for the plan

  this->updateIsocenterPosition();

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

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::setBeamNode(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLRTBeamNode* rtBeamNode = vtkMRMLRTBeamNode::SafeDownCast(node);
  if (!rtBeamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect(rtBeamNode, vtkCommand::ModifiedEvent, this, SLOT(onBeamNodeModified()));
  qvtkReconnect(rtBeamNode, vtkMRMLRTBeamNode::IsocenterModifiedEvent, this, SLOT(updateIsocenterPosition()));

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::onBeamNodeModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::switchToParentPlanButtonClicked()
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << "Unable to access active beam node";
    return;
  }

  // Open ExternalBeamPlanning module and select parent plan
  qSlicerApplication::application()->openNodeModule(beamNode->GetParentPlanNode());
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::onLogicModified()
{
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::beamNameChanged(const QString &text)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    return;
  }
  beamNode->SetName(text.toStdString().c_str());
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::radiationTypeChanged(int index)
{
  Q_D(qSlicerBeamsModuleWidget);

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
void qSlicerBeamsModuleWidget::beamTypeChanged(const QString &text)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
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
void qSlicerBeamsModuleWidget::targetSegmentChanged(const QString& segment)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    return;
  }

  // Set target segment ID
  beamNode->SetTargetSegmentID(segment.toLatin1().constData());

  if (beamNode->GetIsocenterSpecification() == vtkMRMLRTBeamNode::CenterOfTarget)
  {
    beamNode->SetIsocenterToTargetCenter();

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
void qSlicerBeamsModuleWidget::rxDoseChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current beam node!";
    return;
  }
  vtkMRMLRTPlanNode* rtPlanNode = beamNode->GetParentPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid RT plan node!";
    return;
  }

  //TODO: It clearly belongs to the plan, so the UI should be in EBP
  rtPlanNode->SetRxDose(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::isocenterSpecChanged(const QString &text)
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current beam node!";
    return;
  }

  if (text.compare("Center of target") == 0)
  {
    qDebug() << "Setting isocenter spec to center of target";
    beamNode->SetIsocenterToTargetCenter();
  }
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::isocenterCoordinatesChanged(double *coords)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node!";
    return;
  }

  beamNode->SetIsocenterPosition(coords);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::centerViewToIsocenterClicked()
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid beam node!";
    return;
  }

  // Get isocenter position
  double iso[3] = {0.0,0.0,0.0};
  beamNode->GetIsocenterPosition(iso);

  // Navigate slice views to position
  this->mrmlScene()->InitTraversal();
  vtkMRMLNode *currentNode = this->mrmlScene()->GetNextNodeByClass("vtkMRMLSliceNode");
  while (currentNode)
  {
    vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(currentNode);
    sliceNode->JumpSlice(iso[0], iso[1], iso[2]);
    currentNode = this->mrmlScene()->GetNextNodeByClass("vtkMRMLSliceNode");
  }
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::isocenterFiducialNodeChangedfromCoordinates(double* coordinates)
{
  Q_D(qSlicerBeamsModuleWidget);
  UNUSED_VARIABLE(coordinates);

  //TODO: Implement
  qWarning() << Q_FUNC_INFO << ": Not implemented!";
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::dosePointFiducialNodeChangedfromCoordinates(double* coordinates)
{
  Q_D(qSlicerBeamsModuleWidget);
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
void qSlicerBeamsModuleWidget::proximalMarginChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
  protonNode->SetProximalMargin(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::distalMarginChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetDistalMargin(value);
}

//---------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::beamLineTypeChanged(const QString &text)
{
  Q_D(qSlicerBeamsModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
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
void qSlicerBeamsModuleWidget::manualEnergyPrescriptionChanged(bool checked)
{
  Q_D(qSlicerBeamsModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
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
void qSlicerBeamsModuleWidget::minimumEnergyChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }

  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetMinimumEnergy(value);
}

//---------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::maximumEnergyChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }

  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
  protonNode->SetMaximumEnergy(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::mlcPositionDoubleArrayNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamsModuleWidget);

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
void qSlicerBeamsModuleWidget::xJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  beamNode->SetX1Jaw(minVal);
  beamNode->SetX2Jaw(maxVal);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::yJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  beamNode->SetY1Jaw(minVal);
  beamNode->SetY2Jaw(maxVal);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::gantryAngleChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam selected!";
    return;
  }
  beamNode->SetGantryAngle(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::collimatorAngleChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode) {
    qCritical() << Q_FUNC_INFO << ": No current beam selected!";
    return;
  }
  beamNode->SetCollimatorAngle(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::couchAngleChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam selected!";
    return;
  }
  beamNode->SetCouchAngle(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::smearingChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL) {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }

  beamNode->SetSmearing(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::beamWeightChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  beamNode->SetBeamWeight(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::sourceDistanceChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  beamNode->SetSAD(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::apertureDistanceChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetApertureOffset(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::algorithmChanged(const QString &text)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);

  //if (text == "Ray tracer algorithm")
  //{
  //  protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::RayTracer);
  //  d->label_CalculateDoseStatus->setText("Ray Tracer Algorithm");
  //}
  //else if (text == "Cartesian geometry dose calculation")
  //{
  //  protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::CGS);
  //  d->label_CalculateDoseStatus->setText("Cartesian Geometry Algorithm");
  //}
  //else if (text == "Divergent geometry dose calculation")
  //{
  //  protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::DGS);
  //  d->label_CalculateDoseStatus->setText("Divergent Geometry Algorithm");
  //}
  //else if (text == "Hong dose calculation")
  //{
  //  protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::HGS);
  //  d->label_CalculateDoseStatus->setText("Hong Algorithm");
  //}
  //else
  //{
  //  protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::RayTracer);
  //  d->label_CalculateDoseStatus->setText("Algorithm error, Ray Tracer chosen by default");
  //}
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::pencilBeamSpacingChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  // GCS FIX TODO: Why are the function names different?
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetPBResolution(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::sourceSizeChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetSourceSize(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::energyResolutionChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
  protonNode->SetEnergyResolution(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::energySpreadChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
  protonNode->SetEnergySpread(value);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::stepLengthChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetStepLength(value);
}


//---------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::wedApproximationChanged(bool checked)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetLateralSpreadHomoApprox(checked);
}

//---------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::rangeCompensatorHighlandChanged(bool checked)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetRangeCompensatorHighland(checked);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::beamEyesViewClicked(bool checked)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  //TODO:
  //if (checked)
  //{
  //  qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutTwoOverTwoView);
  //}
  //else
  //{
  //  qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutFourUpView);
  //}

  //TODO: Set camera
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::contoursInBEWClicked(bool checked)
{
  Q_D(qSlicerBeamsModuleWidget);

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
void qSlicerBeamsModuleWidget::updateDRRClicked()
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": No beam node is selected!";
    return;
  }

  //TODO: Fix DRR code (it is in EBP logic and it is commented out)
  //d->logic()->UpdateDRR(beamNode->GetName());
  qCritical() << Q_FUNC_INFO << ": Not implemented!";
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::collimatorTypeChanged(const QString &text)
{
  Q_D(qSlicerBeamsModuleWidget);
  UNUSED_VARIABLE(text);

  //TODO:
  qWarning() << Q_FUNC_INFO << ": Not implemented!";
}

//-----------------------------------------------------------------------------
bool qSlicerBeamsModuleWidget::setEditedNode(vtkMRMLNode* node, QString role/*=QString()*/, QString context/*=QString()*/)
{
  Q_D(qSlicerBeamsModuleWidget);
  if (vtkMRMLRTBeamNode::SafeDownCast(node))
  {
    d->MRMLNodeComboBox_RtBeam->setCurrentNode(node);
    return true;
  }
  return false;
}

//-----------------------------------------------------------
double qSlicerBeamsModuleWidget::nodeEditable(vtkMRMLNode* node)
{
  /// Return a higher confidence value (0.6) for beam nodes to prevent beams to be opened by Models
  if (vtkMRMLRTBeamNode::SafeDownCast(node))
  {
    return 0.6;
  }

  return 0.5;
} 