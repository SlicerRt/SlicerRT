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


// Dose engines includes
#include "qSlicerPlastimatchProtonDoseEngine.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// Plastimatch includes
#include "itk_image_accumulate.h"
#include "itk_image_create.h"
#include "itk_image_save.h"
#include "itk_image_scale.h"
#include "itk_image_stats.h"
#include "plm_image.h"
#include "plm_image_header.h"
#include "rpl_volume.h"
#include "rt_beam.h"
#include "rt_plan.h"
#include "string_util.h"

// Segmentations includes
#include "vtkOrientedImageData.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "PlmCommon.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>

// Qt includes
#include <QDebug>
#include <QStringList>

//----------------------------------------------------------------------------
qSlicerPlastimatchProtonDoseEngine::qSlicerPlastimatchProtonDoseEngine(QObject* parent)
  : qSlicerAbstractDoseEngine(parent)
{
  this->m_Name = QString("Plastimatch proton");
}

//----------------------------------------------------------------------------
qSlicerPlastimatchProtonDoseEngine::~qSlicerPlastimatchProtonDoseEngine()
{
}

//---------------------------------------------------------------------------
void qSlicerPlastimatchProtonDoseEngine::defineBeamParameters()
{
  //TODO: Tooltips for each parameter

  // Energy tab

  this->addBeamParameterSpinBox(
    "Energy", "ProximalMargin", "Proximal margin (mm):", "",
    0.0, 50.0, 5.0, 1.0, 2 );
  this->addBeamParameterSpinBox(
    "Energy", "DistalMargin", "Distal margin (mm):", "",
    0.0, 50.0, 5.0, 1.0, 2 );

  QStringList beamLineTypeOptions;
  beamLineTypeOptions << "Active scanning" << "Passive scattering";
  this->addBeamParameterComboBox(
    "Energy", "BeamLineTypeActive", "Beam line type:", "",
    beamLineTypeOptions, 0 );

  //TODO: Attribute name not consistent with label (ManualEnergyLimits vs Energy prescription)
  //  (also there is a HavePrescription member in proton beam that seems to be erroneously used in previous code and also a duplicate of ManualEnergyLimits)
  QStringList dependentParameters;
  dependentParameters << "MinimumEnergy" << "MaximumEnergy";
  this->addBeamParameterCheckBox(
    "Energy", "ManualEnergyLimits", "Energy prescription:", "", false, dependentParameters);

  this->addBeamParameterSpinBox(
    "Energy", "MinimumEnergy", "Minimum energy (MeV):", "",
    0.0, 99.99, 0.0, 1.0, 2 );
  this->addBeamParameterSpinBox(
    "Energy", "MaximumEnergy", "Maximum energy (MeV):", "",
    0.0, 99.99, 0.0, 1.0, 2 );

  // Beam model tab

  //TODO: Attribute name not consistent with label (ApertureOffset vs Aperture distance)
  this->addBeamParameterSpinBox(
    "Beam model", "ApertureOffset", "Aperture distance (mm):", "",
    1.0, 10000.0, 1500.0, 100.0, 2 );

  QStringList algorithmOptions;
  algorithmOptions << "Ray tracer" << "Pencil beam";
  this->addBeamParameterComboBox(
    "Beam model", "Algorithm", "Dose calculation algorithm:", "",
    algorithmOptions, 0 );

  this->addBeamParameterSpinBox(
    "Beam model", "PencilBeamResolution", "Pencil beam spacing at isocenter (mm):", "",
    0.1, 99.99, 2.0, 1.0, 2 );
  this->addBeamParameterSpinBox(
    "Beam model", "RangeCompensatorSmearingRadius", "Smearing radius (mm):", "",
    0.0, 99.99, 5.0, 1.0, 2 );
  this->addBeamParameterSpinBox(
    "Beam model", "SourceSize", "Source size (mm):", "",
    0.0, 50.0, 0.0, 1.0, 2 );
  this->addBeamParameterSpinBox(
    "Beam model", "EnergyResolution", "Energy resolution (MeV):", "",
    1.0, 5.0, 5.0, 1.0, 2 );
  this->addBeamParameterSpinBox(
    "Beam model", "EnergySpread", "Energy spread (MeV):", "",
    0.0, 99.99, 1.0, 1.0, 2 );
  this->addBeamParameterSpinBox(
    "Beam model", "StepLength", "Step length:", "",
    0.0, 99.99, 2.0, 1.0, 2 );

  this->addBeamParameterCheckBox(
    "Beam model", "KanematsuGottschalk", "Kanematsu-Gottschalk patient scattering:", "", false);
  this->addBeamParameterCheckBox(
    "Beam model", "RangeCompensatorHighland", "Highland model for range compensator:", "", false);
}

//---------------------------------------------------------------------------
QString qSlicerPlastimatchProtonDoseEngine::calculateDoseUsingEngine(vtkMRMLRTBeamNode* beamNode, vtkMRMLScalarVolumeNode* resultDoseVolumeNode)
{
  vtkMRMLRTPlanNode* parentPlanNode = beamNode->GetParentPlanNode();
  if (!parentPlanNode)
  {
    QString errorMessage = QString("Unable to access parent node for beam %1").arg(beamNode->GetName());
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  vtkMRMLScalarVolumeNode* referenceVolumeNode = parentPlanNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    QString errorMessage("Unable to access reference volume");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  if (!resultDoseVolumeNode)
  {
    QString errorMessage("Invalid result dose volume");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  vtkMRMLScene* scene = beamNode->GetScene();

  // Get target as ITK image
  vtkSmartPointer<vtkOrientedImageData> targetLabelmap = parentPlanNode->GetTargetOrientedImageData();
  if (targetLabelmap.GetPointer() == NULL)
  {
    QString errorMessage("Failed to access target labelmap");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  Plm_image::Pointer targetPlmVolume = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(targetLabelmap);
  if (!targetPlmVolume)
  {
    QString errorMessage("Failed to convert segment labelmap");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  targetPlmVolume->print();
  itk::Image<unsigned char, 3>::Pointer targetVolumeItk = targetPlmVolume->itk_uchar();

  // Reference code for setting the geometry of the segmentation rasterization
  // in case the default one (from DICOM) is not desired
#if defined (commentout)
  vtkSmartPointer<vtkMatrix4x4> doseIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  protonDoseVolumeNode->GetIJKToRASMatrix(doseIjkToRasMatrix);
  std::string doseGeometryString = vtkSegmentationConverter::SerializeImageGeometry(doseIjkToRasMatrix, protonDoseVolumeNode->GetImageData());
  segmentationCopy->SetConversionParameter( vtkSegmentationConverter::GetReferenceImageGeometryParameterName(),
    doseGeometryString );
#endif

  // Get isocenter
  double isocenter[3] = {0.0, 0.0, 0.0};
  if (!beamNode->GetPlanIsocenterPosition(isocenter))
  {
    QString errorMessage("Failed to get isocenter position");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }
  // Convert isocenter position to LPS for Plastimatch
  isocenter[0] = -isocenter[0];
  isocenter[1] = -isocenter[1];

  // Calculate sourcePosition position
  double sourcePosition[3] = {0.0, 0.0, 0.0};
  if (!beamNode->CalculateSourcePosition(sourcePosition))
  {
    QString errorMessage("Failed to calculate source position");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
    return errorMessage;
  }

  // Convert reference volume to Plastimatch image
  //TODO: Cache it so that it does not need to be reconverted every time
  Plm_image::Pointer referenceVolumePlm = PlmCommon::ConvertVolumeNodeToPlmImage(referenceVolumeNode);
  referenceVolumePlm->print();
  // Create ITK output dose volume based on the reference volume
  itk::Image<short, 3>::Pointer referenceVolumeItk = referenceVolumePlm->itk_short();

  // Plastimatch RT plan and beam
  Rt_plan rt_plan;
  Rt_beam* rt_beam = NULL;

  // Connection of the beam parameters to the rt_beam class used to calculate the dose in Plastimatch
  try
  {
    // Create a beam
    rt_beam = rt_plan.append_beam();

    // Assign inputs to dose calculation logic

    // Update plan
    std::cout << "\n ***PLAN PARAMETERS***" << std::endl;
    std::cout << "Setting reference volume" << std::endl;
    rt_plan.set_patient (referenceVolumeItk);
    std::cout << "Setting target volume" << std::endl;
    rt_plan.set_target (targetVolumeItk);
    std::cout << "Setting reference dose point -> ";
    rt_plan.set_ref_dose_point(isocenter); //TODO: MD Fix, for the moment, the reference dose point is the isocenter
    std::cout << "Reference dose position: " << rt_plan.get_ref_dose_point()[0] << " " << rt_plan.get_ref_dose_point()[1] << " " << rt_plan.get_ref_dose_point()[2] << std::endl;
    rt_plan.set_have_ref_dose_point(true);
    rt_plan.set_have_dose_norm(true);
    std::cout << "Setting dose prescription -> ";
    rt_plan.set_normalization_dose(parentPlanNode->GetRxDose());
    std::cout << "Dose prescription = " << rt_plan.get_normalization_dose() << std::endl;

    // Not needed for dose calculation: 
    // Parameter Set, Plan Contour, Dose Volume, Dose Grid

    // Set beam parameters
    std::cout << std::endl << " ***BEAM PARAMETERS***" << std::endl;

    std::cout << "Setting source position -> ";
    rt_beam->set_source_position(sourcePosition);
    std::cout << "Source position: " << rt_beam->get_source_position()[0] << " " << rt_beam->get_source_position()[1] << " " << rt_beam->get_source_position()[2] << std::endl;

    std::cout << "Setting isocenter position -> ";
    rt_beam->set_isocenter_position(isocenter);
    std::cout << "Isocenter position: " << rt_beam->get_isocenter_position()[0] << " " << rt_beam->get_isocenter_position()[1] << " " << rt_beam->get_isocenter_position()[2] << std::endl;

    std::cout << "Setting dose calculation algorithm -> ";
    int algorithm = this->integerParameter(beamNode, "Algorithm");
    switch(algorithm)
    {
    case 1: // Pencil beam
      rt_beam->set_flavor('d');
      break;
    default: // Ray tracer
      rt_beam->set_flavor('b');
      break;
    }
    std::cout << "Algorithm Flavor = " << rt_beam->get_flavor() << std::endl;

    bool kgScattering = this->booleanParameter(beamNode, "KanematsuGottschalk");
    if (kgScattering)
    {
      rt_beam->set_homo_approx('n');
      std::cout << "Homo approximation set to false" << std::endl;
    }
    else
    {
      rt_beam->set_homo_approx('y');
      std::cout << "Homo approximation set to true" << std::endl;
    }

    std::cout << "Setting beam weight -> ";
    rt_beam->set_beam_weight(1.0); // Beam weight is applied centrally by the dose engine logic (qSlicerDoseEngineLogic::createAccumulatedDose)
    std::cout << "Beam weight = " << rt_beam->get_beam_weight() << std::endl;

    std::cout << "Setting smearing -> ";
    double rangeCompensatorSmearingRadius = this->doubleParameter(beamNode, "RangeCompensatorSmearingRadius");
    rt_beam->set_smearing(rangeCompensatorSmearingRadius);
    std::cout << "Smearing = " << rt_beam->get_smearing() << std::endl;

    std::cout << "Setting Highland model for range compensator" << std::endl;
    bool rangeCompensatorHighland = this->booleanParameter(beamNode, "RangeCompensatorHighland");
    if (rangeCompensatorHighland)
    {
      rt_beam->set_rc_MC_model('n');
      std::cout << "Highland model for range compensator set to true" << std::endl;
    }
    else
    {
      rt_beam->set_rc_MC_model('y');
      std::cout << "Highland model for range compensator set to false" << std::endl;
    }

    std::cout << "Setting source size -> ";
    double sourceSize = this->doubleParameter(beamNode, "SourceSize");
    rt_beam->set_source_size(sourceSize);
    std::cout << "Source size = " << rt_beam->get_source_size() << std::endl;

    std::cout << "Setting step length -> ";
    double stepLength = this->doubleParameter(beamNode, "StepLength");
    rt_beam->set_step_length(stepLength);
    std::cout << "Step length = " << rt_beam->get_step_length() << std::endl;

    //TODO: Add in the future: CouchAngle

    // Aperture parameters
    std::cout << "\nAPERTURE PARAMETERS:" << std::endl;

    double apertureOffset = this->doubleParameter(beamNode, "ApertureOffset");
    if (beamNode->GetSAD() < 0 || beamNode->GetSAD() < apertureOffset)
    {
      QString errorMessage = QString("SAD (=%1) must be positive and greater than aperture offset (%2)").arg(beamNode->GetSAD()).arg(apertureOffset);
      qCritical() << Q_FUNC_INFO << ": " << errorMessage;
      return errorMessage;
    }
    double apertureOrigin[2] = {
      beamNode->GetX1Jaw() * apertureOffset / beamNode->GetSAD(),
      beamNode->GetY1Jaw() * apertureOffset / beamNode->GetSAD() };

    double pencilBeamResolution = this->doubleParameter(beamNode, "PencilBeamResolution");
    // Convert from spacing at isocenter to spacing at aperture
    double apertureSpacing[2] = {
      pencilBeamResolution * apertureOffset / beamNode->GetSAD(),
      pencilBeamResolution * apertureOffset / beamNode->GetSAD() };

    int apertureDimensions[2] = {
      (int)((beamNode->GetX2Jaw() - beamNode->GetX1Jaw()) / pencilBeamResolution + 1 ),
      (int)((beamNode->GetY2Jaw() - beamNode->GetY1Jaw()) / pencilBeamResolution + 1 ) };

    std::cout << "Setting aperture distance -> ";
    rt_beam->get_aperture()->set_distance(apertureOffset);
    std::cout << "Aperture distance = " << rt_beam->get_aperture()->get_distance() << std::endl;

    std::cout << "Setting aperture origin -> ";
    rt_beam->get_aperture()->set_origin(apertureOrigin);
    std::cout << "Aperture origin = " << apertureOrigin[0] << " " << apertureOrigin[1] << std::endl;

    std::cout << "Setting aperture spacing -> ";
    rt_beam->get_aperture()->set_spacing(apertureSpacing);
    std::cout << "Aperture Spacing = " << rt_beam->get_aperture()->get_spacing(0) << " " << rt_beam->get_aperture()->get_spacing(1) << std::endl;

    std::cout << "Setting aperture dim -> ";
    rt_beam->get_aperture()->set_dim(apertureDimensions);
    std::cout << "Aperture dim = " << rt_beam->get_aperture()->get_dim(0) << " " << rt_beam->get_aperture()->get_dim(1) << std::endl;

    //TODO: Add in the future: CollimatorAngle

    // Update mebs parameters
    std::cout << "\nENERGY PARAMETERS:" << std::endl;

    std::cout << "Setting beam line type -> ";
    int beamLineTypeActive = this->integerParameter(beamNode, "BeamLineTypeActive");
    if (beamLineTypeActive == 0)
    {
      rt_beam->set_beam_line_type("active");      
      std::cout << "beam line type set to active" << std::endl;
    }
    else
    {
      rt_beam->set_beam_line_type("passive");      
      std::cout << "beam line type set to passive" << std::endl;
    }

    std::cout << "Setting have prescription -> ";
    bool manualEnergyLimits = this->booleanParameter(beamNode, "ManualEnergyLimits");
    rt_beam->get_mebs()->set_have_prescription(manualEnergyLimits);
    std::cout << "Manual energy prescription set to " << rt_beam->get_mebs()->get_have_prescription() << std::endl;

    if (rt_beam->get_mebs()->get_have_prescription() == true)
    {
      double minimumEnergy = this->doubleParameter(beamNode, "MinimumEnergy");
      rt_beam->get_mebs()->set_energy_min(minimumEnergy);
      double maximumEnergy = this->doubleParameter(beamNode, "MaximumEnergy");
      rt_beam->get_mebs()->set_energy_max(maximumEnergy);
      std::cout << "Energy min: " << rt_beam->get_mebs()->get_energy_min() << ", Energy max: " << rt_beam->get_mebs()->get_energy_max() << std::endl;
    }

    std::cout << "Setting proximal margin -> ";
    double proximalMargin = this->doubleParameter(beamNode, "ProximalMargin");
    rt_beam->get_mebs()->set_proximal_margin(proximalMargin);
    std::cout << "Proximal margin = " << rt_beam->get_mebs()->get_proximal_margin() << std::endl;

    std::cout << "Setting distal margin -> ";
    double distalMargin = this->doubleParameter(beamNode, "DistalMargin");
    rt_beam->get_mebs()->set_distal_margin(distalMargin);
    std::cout << "Distal margin = " << rt_beam->get_mebs()->get_distal_margin() << std::endl;

    std::cout << "Setting energy resolution -> ";
    double energyResolution = this->doubleParameter(beamNode, "EnergyResolution");
    rt_beam->get_mebs()->set_energy_resolution(energyResolution);
    std::cout << "Energy resolution = " << rt_beam->get_mebs()->get_energy_resolution() << std::endl;

    std::cout << "Setting energy spread -> ";
    double energySpread = this->doubleParameter(beamNode, "EnergySpread");
    rt_beam->get_mebs()->set_spread(energySpread);
    std::cout << "Energy spread = " << rt_beam->get_mebs()->get_spread() << std::endl;

    // A little warm fuzzy for the developers
    rt_plan.print_verif ();
    std::cout << "Working..." << std::endl;
    fflush(stdout);
  }
  catch (std::exception& ex)
  {
    QString errorMessage("Plastimatch exception happened! See log for details");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage << ": " << ex.what();
    return errorMessage;
  }

  // Compute the dose
  try
  {
    rt_plan.compute_dose(rt_beam);
  }
  catch (std::exception& ex)
  {
    QString errorMessage("Plastimatch exception happened! See log for details");
    qCritical() << Q_FUNC_INFO << ": " << errorMessage << ": " << ex.what();
    return errorMessage;
  }

  // Get per-beam dose image and set it to result node
  itk::Image<float, 3>::Pointer doseVolumeItk = rt_beam->get_dose()->itk_float();

  // Create dose image data to set to the volume node
  vtkSmartPointer<vtkImageData> protonDoseImageData = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(doseVolumeItk, protonDoseImageData, VTK_FLOAT);

  // Set image data to result dose volume node
  resultDoseVolumeNode->SetAndObserveImageData(protonDoseImageData);
  resultDoseVolumeNode->CopyOrientation(referenceVolumeNode);

  std::string protonDoseNodeName = std::string(beamNode->GetName()) + "_ProtonDose";
  resultDoseVolumeNode->SetName(protonDoseNodeName.c_str());

  // Get aperture image, create volume node, and add as intermediate result
  Plm_image::Pointer& ap = rt_beam->get_aperture_image();
  itk::Image<unsigned char, 3>::Pointer apertureVolumeItk = ap->itk_uchar();

  vtkSmartPointer<vtkImageData> apertureImageData = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<unsigned char>(apertureVolumeItk, apertureImageData, VTK_UNSIGNED_CHAR);

  vtkSmartPointer<vtkMRMLScalarVolumeNode> apertureVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  apertureVolumeNode->SetAndObserveImageData(apertureImageData);
  apertureVolumeNode->SetSpacing(apertureVolumeItk->GetSpacing()[0], apertureVolumeItk->GetSpacing()[1], apertureVolumeItk->GetSpacing()[2]);
  apertureVolumeNode->SetOrigin(apertureVolumeItk->GetOrigin()[0], apertureVolumeItk->GetOrigin()[1], apertureVolumeItk->GetOrigin()[2]);

  std::string apertureNodeName = std::string(beamNode->GetName()) + "_Aperture";
  apertureVolumeNode->SetName(apertureNodeName.c_str());
  scene->AddNode(apertureVolumeNode);
  
  this->addIntermediateResult(apertureVolumeNode, beamNode);

  // Get range compensator image, create volume node, and add as intermediate result
  Plm_image::Pointer& rc = rt_beam->get_range_compensator_image();
  itk::Image<float, 3>::Pointer rcVolumeItk = rc->itk_float();

  vtkSmartPointer<vtkImageData> rangeCompensatorImageData = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(rcVolumeItk, rangeCompensatorImageData, VTK_FLOAT);

  vtkSmartPointer<vtkMRMLScalarVolumeNode> rangeCompensatorVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  rangeCompensatorVolumeNode->SetAndObserveImageData(rangeCompensatorImageData);
  rangeCompensatorVolumeNode->SetSpacing(rcVolumeItk->GetSpacing()[0], rcVolumeItk->GetSpacing()[1], rcVolumeItk->GetSpacing()[2]);
  rangeCompensatorVolumeNode->SetOrigin(rcVolumeItk->GetOrigin()[0], rcVolumeItk->GetOrigin()[1], rcVolumeItk->GetOrigin()[2]);

  std::string rangeCompensatorNodeName = std::string(beamNode->GetName()) + "_RangeCompensator";
  rangeCompensatorVolumeNode->SetName(rangeCompensatorNodeName.c_str());
  scene->AddNode(rangeCompensatorVolumeNode);
  
  this->addIntermediateResult(rangeCompensatorVolumeNode, beamNode);

  return QString();
}
