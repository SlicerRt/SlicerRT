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

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLScene.h>

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

  connect( d->MRMLNodeComboBox_RtBeam, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setBeamNode(vtkMRMLNode*)) );
  connect( d->pushButton_SwitchToParentPlan, SIGNAL(clicked()), this, SLOT(switchToParentPlanButtonClicked()) );

  // Main parameters
  connect( d->lineEdit_BeamName, SIGNAL(textChanged(const QString &)), this, SLOT(beamNameChanged(const QString &)) );
  connect( d->doubleSpinBox_BeamWeight, SIGNAL(valueChanged(double)), this, SLOT(beamWeightChanged(double)) );

//TODO:
  // Proton energy page
  //connect( d->doubleSpinBox_ProximalMargin, SIGNAL(valueChanged(double)), this, SLOT(proximalMarginChanged(double)) );
  //connect( d->doubleSpinBox_DistalMargin, SIGNAL(valueChanged(double)), this, SLOT(distalMarginChanged(double)) );
  //connect( d->comboBox_BeamLineType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(beamLineTypeChanged(const QString &)) );
  //connect( d->checkBox_EnergyPrescription, SIGNAL(clicked(bool)), this, SLOT(manualEnergyPrescriptionChanged(bool)) );
  //connect( d->doubleSpinBox_MinimumEnergy, SIGNAL(valueChanged(double)), this, SLOT(minimumEnergyChanged(double)) );
  //connect( d->doubleSpinBox_MaximumEnergy, SIGNAL(valueChanged(double)), this, SLOT(maximumEnergyChanged(double)) );

  //// Proton beam model page
  //connect( d->doubleSpinBox_ApertureDistance, SIGNAL(valueChanged(double)), this, SLOT(apertureDistanceChanged(double)) );
  //connect( d->comboBox_Algorithm, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(algorithmChanged(const QString &)) );
  //connect( d->doubleSpinBox_PencilBeamSpacing, SIGNAL(valueChanged(double)), this, SLOT(pencilBeamSpacingChanged(double)) );
  //connect( d->doubleSpinBox_Smearing, SIGNAL(valueChanged(double)), this, SLOT(smearingChanged(double)) );
  //connect( d->doubleSpinBox_SourceSize, SIGNAL(valueChanged(double)), this, SLOT(sourceSizeChanged(double)) );
  //connect( d->doubleSpinBox_EnergyResolution, SIGNAL(valueChanged(double)), this, SLOT(energyResolutionChanged(double)) );
  //connect( d->doubleSpinBox_EnergySpread, SIGNAL(valueChanged(double)), this, SLOT(energySpreadChanged(double)) );
  //connect( d->doubleSpinBox_StepLength, SIGNAL(valueChanged(double)), this, SLOT(stepLengthChanged(double)) );
  //connect( d->checkBox_WEDApproximation, SIGNAL(clicked(bool)), this, SLOT(wedApproximationChanged(bool)) );
  //connect( d->checkBox_RangeCompensatorHighland, SIGNAL(clicked(bool)), this, SLOT(rangeCompensatorHighlandChanged(bool)) );

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

  // If node is empty, disable controls
  d->BeamParametersTabWidget->setEnabled(beamNode);
  if (!beamNode)
  {
    d->lineEdit_BeamName->setText("");
    d->doubleSpinBox_BeamWeight->setValue(0.0);
    return;
  }

  // Main parameters
  d->lineEdit_BeamName->setText(QString::fromStdString(beamNode->GetName()));
  d->doubleSpinBox_BeamWeight->setValue(beamNode->GetBeamWeight());

//TODO:
  // Enable appropriate tabs and widgets for this beam type and set 
  // widget values from MRML node
  /*
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
    int index = 0;

    // Enable needed tabs and set widget values
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
  */

  //switch (beamNode->GetRadiationType())
  //{
  //case vtkMRMLRTBeamNode::Photon:
  //  d->comboBox_RadiationType->setCurrentIndex(1);
  //  break;
  //case vtkMRMLRTBeamNode::Electron:
  //  d->comboBox_RadiationType->setCurrentIndex(2);
  //  break;
  //default:
  //  d->comboBox_RadiationType->setCurrentIndex(0); // Proton
  //  break;
  //}

  //// Set values into prescription tab
  //switch (beamNode->GetBeamType())
  //{
  //case vtkMRMLRTProtonBeamNode::Dynamic:
  //  d->comboBox_RadiationType->setCurrentIndex(1);
  //  break;
  //default:
  //  d->comboBox_RadiationType->setCurrentIndex(0); // Static
  //  break;
  //}

//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
  //if (protonBeamNode)
  //{
  //  // Set values into energy tab
  //  d->doubleSpinBox_ProximalMargin->setValue(protonBeamNode->GetProximalMargin());
  //  d->doubleSpinBox_DistalMargin->setValue(protonBeamNode->GetDistalMargin());
  //  if (protonBeamNode->GetBeamLineTypeActive() == true)
  //  {
  //    d->comboBox_BeamLineType->setCurrentIndex(0);
  //  }
  //  else
  //  {
  //    d->comboBox_BeamLineType->setCurrentIndex(1);
  //  }
  //  d->checkBox_EnergyPrescription->setChecked(protonBeamNode->GetManualEnergyLimits());
  //  d->doubleSpinBox_MinimumEnergy->setValue(protonBeamNode->GetMinimumEnergy());
  //  d->doubleSpinBox_MaximumEnergy->setValue(protonBeamNode->GetMaximumEnergy());

  //  // Set values into proton beam model
  //  //if (beamNode->GetRadiationType() == vtkMRMLRTBeamNode::Proton)
  //  {
  //    d->doubleSpinBox_ApertureDistance->setValue(protonBeamNode->GetApertureOffset());
  //    switch (protonBeamNode->GetAlgorithm())
  //    {
  //    case vtkMRMLRTProtonBeamNode::CGS:
  //      d->comboBox_Algorithm->setCurrentIndex(1);
  //      break;
  //    case vtkMRMLRTProtonBeamNode::DGS:
  //      d->comboBox_Algorithm->setCurrentIndex(2);
  //      break;
  //    case vtkMRMLRTProtonBeamNode::HGS:
  //      d->comboBox_Algorithm->setCurrentIndex(3);
  //      break;
  //    default: // Ray Tracer or any other mistake
  //      d->comboBox_Algorithm->setCurrentIndex(0);
  //      break;
  //    }
  //    d->doubleSpinBox_PencilBeamSpacing->setValue(protonBeamNode->GetPencilBeamResolution());
  //    d->doubleSpinBox_Smearing->setValue(protonBeamNode->GetRangeCompensatorSmearingRadius());
  //    d->doubleSpinBox_SourceSize->setValue(protonBeamNode->GetSourceSize());
  //    d->doubleSpinBox_EnergyResolution->setValue(protonBeamNode->GetEnergyResolution());
  //    d->doubleSpinBox_EnergySpread->setValue(protonBeamNode->GetEnergySpread());
  //    d->doubleSpinBox_StepLength->setValue(protonBeamNode->GetStepLength());
  //    d->checkBox_WEDApproximation->setChecked(protonBeamNode->GetLateralSpreadHomoApprox());
  //    d->checkBox_RangeCompensatorHighland->setChecked(protonBeamNode->GetRangeCompensatorHighland());
  //  }
  //}

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
  beamNode->DisableModifiedEventOn();
  beamNode->SetName(text.toStdString().c_str());
  beamNode->DisableModifiedEventOff();
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

  beamNode->DisableModifiedEventOn();
  beamNode->SetBeamWeight(value);
  beamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetProximalMargin(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetDistalMargin(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetBeamLineTypeActive( text.compare("Active scanning") == 0 );
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetHavePrescription(checked);
  //protonBeamNode->DisableModifiedEventOff();

  //if (protonBeamNode->GetHavePrescription())
  //{
  //  d->doubleSpinBox_MinimumEnergy->setEnabled(true);
  //  d->doubleSpinBox_MaximumEnergy->setEnabled(true);
  //  d->label_MinimumEnergy->setEnabled(true);
  //  d->label_MaximumEnergy->setEnabled(true);
  //}
  //else
  //{
  //  d->doubleSpinBox_MinimumEnergy->setEnabled(false);
  //  d->doubleSpinBox_MaximumEnergy->setEnabled(false);
  //  d->label_MinimumEnergy->setEnabled(false);
  //  d->label_MaximumEnergy->setEnabled(false);
  //}
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetMinimumEnergy(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetMaximumEnergy(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL) {
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetRangeCompensatorSmearingRadius(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetApertureOffset(value);
  //protonBeamNode->DisableModifiedEventOff();
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

//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //if (text == "Ray tracer algorithm")
  //{
  //  protonBeamNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::RayTracer);
  //  d->label_CalculateDoseStatus->setText("Ray Tracer Algorithm");
  //}
  //else if (text == "Cartesian geometry dose calculation")
  //{
  //  protonBeamNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::CGS);
  //  d->label_CalculateDoseStatus->setText("Cartesian Geometry Algorithm");
  //}
  //else if (text == "Divergent geometry dose calculation")
  //{
  //  protonBeamNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::DGS);
  //  d->label_CalculateDoseStatus->setText("Divergent Geometry Algorithm");
  //}
  //else if (text == "Hong dose calculation")
  //{
  //  protonBeamNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::HGS);
  //  d->label_CalculateDoseStatus->setText("Hong Algorithm");
  //}
  //else
  //{
  //  protonBeamNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::RayTracer);
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetPencilBeamResolution(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetSourceSize(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetEnergyResolution(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetEnergySpread(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetStepLength(value);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetLateralSpreadHomoApprox(checked);
  //protonBeamNode->DisableModifiedEventOff();
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
//TODO:
  //vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  //if (protonBeamNode == NULL)
  //{
  //  qCritical() << Q_FUNC_INFO << ": No current proton beam node.";
  //  return;
  //}

  //protonBeamNode->DisableModifiedEventOn();
  //protonBeamNode->SetRangeCompensatorHighland(checked);
  //protonBeamNode->DisableModifiedEventOff();
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