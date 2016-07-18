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
  //TODO:
}

//---------------------------------------------------------------------------
QString qSlicerPlastimatchProtonDoseEngine::calculateDoseUsingEngine(vtkMRMLRTBeamNode* beamNode, vtkMRMLScalarVolumeNode* resultDoseVolumeNode)
{
//TODO:
//  vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beamNode);
//  if (!protonBeamNode)
//  {
//    QString errorMessage("Invalid input proton beam");
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
//    return errorMessage;
//  }
//
//  vtkMRMLRTPlanNode* parentPlanNode = beamNode->GetParentPlanNode();
//  if (!parentPlanNode)
//  {
//    QString errorMessage = QString("Unable to access parent node for beam %1").arg(beamNode->GetName());
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
//    return errorMessage;
//  }
//  vtkMRMLScalarVolumeNode* referenceVolumeNode = parentPlanNode->GetReferenceVolumeNode();
//  if (!referenceVolumeNode)
//  {
//    QString errorMessage("Unable to access reference volume");
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
//    return errorMessage;
//  }
//  if (!resultDoseVolumeNode)
//  {
//    QString errorMessage("Invalid result dose volume");
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
//    return errorMessage;
//  }
//
//  vtkMRMLScene* scene = beamNode->GetScene();
//
//  // Get target as ITK image
//  vtkSmartPointer<vtkOrientedImageData> targetLabelmap = parentPlanNode->GetTargetOrientedImageData();
//  if (targetLabelmap.GetPointer() == NULL)
//  {
//    QString errorMessage("Failed to access target labelmap");
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
//    return errorMessage;
//  }
//  Plm_image::Pointer targetPlmVolume = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(targetLabelmap);
//  if (!targetPlmVolume)
//  {
//    QString errorMessage("Failed to convert segment labelmap");
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
//    return errorMessage;
//  }
//  targetPlmVolume->print();
//  itk::Image<unsigned char, 3>::Pointer targetVolumeItk = targetPlmVolume->itk_uchar();
//
//  // Reference code for setting the geometry of the segmentation rasterization
//  // in case the default one (from DICOM) is not desired
//#if defined (commentout)
//  vtkSmartPointer<vtkMatrix4x4> doseIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
//  protonDoseVolumeNode->GetIJKToRASMatrix(doseIjkToRasMatrix);
//  std::string doseGeometryString = vtkSegmentationConverter::SerializeImageGeometry(doseIjkToRasMatrix, protonDoseVolumeNode->GetImageData());
//  segmentationCopy->SetConversionParameter( vtkSegmentationConverter::GetReferenceImageGeometryParameterName(),
//    doseGeometryString );
//#endif
//
//  // Get isocenter
//  double isocenter[3] = {0.0, 0.0, 0.0};
//  if (!beamNode->GetPlanIsocenterPosition(isocenter))
//  {
//    QString errorMessage("Failed to get isocenter position");
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
//    return errorMessage;
//  }
//  // Convert isocenter position to LPS for Plastimatch
//  isocenter[0] = -isocenter[0];
//  isocenter[1] = -isocenter[1];
//
//  // Calculate sourcePosition position
//  double sourcePosition[3] = {0.0, 0.0, 0.0};
//  if (!beamNode->CalculateSourcePosition(sourcePosition))
//  {
//    QString errorMessage("Failed to calculate source position");
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage;
//    return errorMessage;
//  }
//
//  // Convert reference volume to Plastimatch image
//  //TODO: Cache it so that it does not need to be reconverted every time
//  Plm_image::Pointer referenceVolumePlm = PlmCommon::ConvertVolumeNodeToPlmImage(referenceVolumeNode);
//  referenceVolumePlm->print();
//  // Create ITK output dose volume based on the reference volume
//  itk::Image<short, 3>::Pointer referenceVolumeItk = referenceVolumePlm->itk_short();
//
//  // Plastimatch RT plan and beam
//  Rt_plan rt_plan;
//  Rt_beam* rt_beam = NULL;
//
//  // Connection of the beam parameters to the rt_beam class used to calculate the dose in Plastimatch
//  try
//  {
//    // Create a beam
//    rt_beam = rt_plan.append_beam();
//
//    // Assign inputs to dose calculation logic
//
//    // Update plan
//    std::cout << "\n ***PLAN PARAMETERS***" << std::endl;
//    std::cout << "Setting reference volume" << std::endl;
//    rt_plan.set_patient (referenceVolumeItk);
//    std::cout << "Setting target volume" << std::endl;
//    rt_plan.set_target (targetVolumeItk);
//    std::cout << "Setting reference dose point -> ";
//    rt_plan.set_ref_dose_point(isocenter); //TODO: MD Fix, for the moment, the reference dose point is the isocenter
//    std::cout << "Reference dose position: " << rt_plan.get_ref_dose_point()[0] << " " << rt_plan.get_ref_dose_point()[1] << " " << rt_plan.get_ref_dose_point()[2] << std::endl;
//    rt_plan.set_have_ref_dose_point(true);
//    rt_plan.set_have_dose_norm(true);
//    std::cout << "Setting dose prescription -> ";
//    rt_plan.set_normalization_dose(parentPlanNode->GetRxDose());
//    std::cout << "Dose prescription = " << rt_plan.get_normalization_dose() << std::endl;
//
//    // Not needed for dose calculation: 
//    // Parameter Set, Plan Contour, Dose Volume, Dose Grid
//
//    // Update beam
//    std::cout << std::endl << " ***BEAM PARAMETERS***" << std::endl;
//    std::cout << "Setting source position -> ";
//    rt_beam->set_source_position(sourcePosition);
//    std::cout << "Source position: " << rt_beam->get_source_position()[0] << " " << rt_beam->get_source_position()[1] << " " << rt_beam->get_source_position()[2] << std::endl;
//    std::cout << "Setting isocenter position -> ";
//    rt_beam->set_isocenter_position(isocenter);
//    std::cout << "Isocenter position: " << rt_beam->get_isocenter_position()[0] << " " << rt_beam->get_isocenter_position()[1] << " " << rt_beam->get_isocenter_position()[2] << std::endl;
//    std::cout << "Setting dose calculation algorithm -> ";
//    switch(protonBeamNode->GetAlgorithm())
//    {
//      case vtkMRMLRTProtonBeamNode::CGS:
//        rt_beam->set_flavor('f');
//        break;
//      case vtkMRMLRTProtonBeamNode::DGS:
//        rt_beam->set_flavor('g');
//        break;
//      case vtkMRMLRTProtonBeamNode::HGS:
//        rt_beam->set_flavor('h');
//        break;
//      default:
//        rt_beam->set_flavor('a');
//        break;
//    }
//    std::cout << "Algorithm Flavor = " << rt_beam->get_flavor() << std::endl;
//    std::cout << "Setting homo approximation -> ";
//    if (protonBeamNode->GetLateralSpreadHomoApprox() == true)
//    {
//      rt_beam->set_homo_approx('y');
//      std::cout << "Homo approximation set to true" << std::endl;
//    }
//    else
//    {
//      rt_beam->set_homo_approx('n');
//      std::cout << "Homo approximation set to false" << std::endl;
//    }
//    std::cout << "Setting beam weight -> ";
//    rt_beam->set_beam_weight(beamNode->GetBeamWeight());
//    std::cout << "Beam weight = " << rt_beam->get_beam_weight() << std::endl;
//    std::cout << "Setting smearing -> ";
//    rt_beam->set_smearing(protonBeamNode->GetRangeCompensatorSmearingRadius());
//    std::cout << "Smearing = " << rt_beam->get_smearing() << std::endl;
//    std::cout << "Setting Highland model for range compensator" << std::endl;
//    if (protonBeamNode->GetRangeCompensatorHighland() == true)
//    {
//      rt_beam->set_rc_MC_model('n');
//      std::cout << "Highland model for range compensator set to true" << std::endl;
//    }
//    else
//    {
//      rt_beam->set_rc_MC_model('y');
//      std::cout << "Highland model for range compensator set to false" << std::endl;
//    }
//    std::cout << "Setting source size -> ";
//    rt_beam->set_source_size(protonBeamNode->GetSourceSize());
//    std::cout << "Source size = " << rt_beam->get_source_size() << std::endl;
//    std::cout << "Setting step length -> ";
//    rt_beam->set_step_length(protonBeamNode->GetStepLength());
//    std::cout << "Step length = " << rt_beam->get_step_length() << std::endl;
//    // Not needed: BeamType, MLC_Array
//    //TODO: Add in the future: CouchAngle
//
//    // Update aperture parameters
//    std::cout << "\nAPERTURE PARAMETERS:" << std::endl;
//    protonBeamNode->UpdateApertureParameters(); // Sanity check before we set the parameters
//
//    std::cout << "Setting aperture distance -> ";
//    rt_beam->get_aperture()->set_distance(protonBeamNode->GetApertureOffset());
//    std::cout << "Aperture distance = " << rt_beam->get_aperture()->get_distance() << std::endl;
//    std::cout << "Setting aperture origin -> ";
//    rt_beam->get_aperture()->set_origin(protonBeamNode->GetApertureOrigin());
//    std::cout << "Aperture origin = " << protonBeamNode->GetApertureOrigin()[0] << " " << protonBeamNode->GetApertureOrigin()[1] << std::endl;
//    std::cout << "Setting aperture spacing -> ";
//    rt_beam->get_aperture()->set_spacing(protonBeamNode->GetApertureSpacing());
//    std::cout << "Aperture Spacing = " << rt_beam->get_aperture()->get_spacing(0) << " " << rt_beam->get_aperture()->get_spacing(1) << std::endl;
//    std::cout << "Setting aperture dim -> ";
//    rt_beam->get_aperture()->set_dim(protonBeamNode->GetApertureDim() );
//    std::cout << "Aperture dim = " << rt_beam->get_aperture()->get_dim(0) << " " << rt_beam->get_aperture()->get_dim(1) << std::endl;
//    //TODO: Add in the future: CollimatorAngle
//
//    // Update mebs parameters
//    std::cout << "\nENERGY PARAMETERS:" << std::endl;
//    std::cout << "Setting beam line type -> ";
//    if (protonBeamNode->GetBeamLineTypeActive() == true)
//    {
//      rt_beam->set_beam_line_type("active");      
//      std::cout << "beam line type set to active" << std::endl;
//    }
//    else
//    {
//      rt_beam->set_beam_line_type("passive");      
//      std::cout << "beam line type set to passive" << std::endl;
//    }
//    std::cout << "Setting have prescription -> ";
//    rt_beam->get_mebs()->set_have_prescription(protonBeamNode->GetManualEnergyLimits());
//    std::cout << "Manual energy prescription set to " << rt_beam->get_mebs()->get_have_prescription() << std::endl;
//    if (rt_beam->get_mebs()->get_have_prescription() == true)
//    {
//      rt_beam->get_mebs()->set_energy_min(protonBeamNode->GetMinimumEnergy());
//      rt_beam->get_mebs()->set_energy_max(protonBeamNode->GetMaximumEnergy());
//      std::cout << "Energy min: " << rt_beam->get_mebs()->get_energy_min() << ", Energy max: " << rt_beam->get_mebs()->get_energy_max() << std::endl;
//    }
//    std::cout << "Setting proximal margin -> ";
//    rt_beam->get_mebs()->set_proximal_margin (protonBeamNode->GetProximalMargin());
//    std::cout << "Proximal margin = " << rt_beam->get_mebs()->get_proximal_margin() << std::endl;
//    std::cout << "Setting distal margin -> ";
//    rt_beam->get_mebs()->set_distal_margin (protonBeamNode->GetDistalMargin());
//    std::cout << "Distal margin = " << rt_beam->get_mebs()->get_distal_margin() << std::endl;
//    std::cout << "Setting energy resolution -> ";
//    rt_beam->get_mebs()->set_energy_resolution(protonBeamNode->GetEnergyResolution());
//    std::cout << "Energy resolution = " << rt_beam->get_mebs()->get_energy_resolution() << std::endl;
//    std::cout << "Setting energy spread -> ";
//    rt_beam->get_mebs()->set_spread (protonBeamNode->GetEnergySpread());
//    std::cout << "Energy spread = " << rt_beam->get_mebs()->get_spread() << std::endl;
//
//    // Distal and proximal margins are updated when the SOBP is created
//    // All the rt_beam parameters are updated to initiate the dose calculation
//    if (!rt_plan.prepare_beam_for_calc (rt_beam))
//    {
//      QString errorMessage("Sorry, rt_plan.prepare_beam_for_calc() failed");
//      qCritical() << Q_FUNC_INFO << ": " << errorMessage;
//      return errorMessage;
//    }
//
//    // A little warm fuzzy for the developers
//    rt_plan.print_verif ();
//    std::cout << "Working..." << std::endl;
//    fflush(stdout);
//  }
//  catch (std::exception& ex)
//  {
//    QString errorMessage("Plastimatch exception happened! See log for details");
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage << ": " << ex.what();
//    return errorMessage;
//  }
//
//  // Get aperture image, create volume node, and add as intermediate result
//  Plm_image::Pointer& ap = rt_beam->rpl_vol->get_aperture()->get_aperture_image();
//  itk::Image<unsigned char, 3>::Pointer apertureVolumeItk = ap->itk_uchar();
//
//  vtkSmartPointer<vtkImageData> apertureImageData = vtkSmartPointer<vtkImageData>::New();
//  SlicerRtCommon::ConvertItkImageToVtkImageData<unsigned char>(apertureVolumeItk, apertureImageData, VTK_UNSIGNED_CHAR);
//
//  vtkSmartPointer<vtkMRMLScalarVolumeNode> apertureVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
//  apertureVolumeNode->SetAndObserveImageData(apertureImageData);
//  apertureVolumeNode->SetSpacing(apertureVolumeItk->GetSpacing()[0], apertureVolumeItk->GetSpacing()[1], apertureVolumeItk->GetSpacing()[2]);
//  apertureVolumeNode->SetOrigin(apertureVolumeItk->GetOrigin()[0], apertureVolumeItk->GetOrigin()[1], apertureVolumeItk->GetOrigin()[2]);
//
//  std::string apertureNodeName = std::string(beamNode->GetName()) + "_Aperture";
//  apertureVolumeNode->SetName(apertureNodeName.c_str());
//  scene->AddNode(apertureVolumeNode);
//  
//  this->addIntermediateResult(apertureVolumeNode, beamNode);
//
//  // Get range compensator image, create volume node, and add as intermediate result
//  Plm_image::Pointer& rc = rt_beam->rpl_vol->get_aperture()->get_range_compensator_image();
//  itk::Image<float, 3>::Pointer rcVolumeItk = rc->itk_float();
//
//  vtkSmartPointer<vtkImageData> rangeCompensatorImageData = vtkSmartPointer<vtkImageData>::New();
//  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(rcVolumeItk, rangeCompensatorImageData, VTK_FLOAT);
//
//  vtkSmartPointer<vtkMRMLScalarVolumeNode> rangeCompensatorVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
//  rangeCompensatorVolumeNode->SetAndObserveImageData(rangeCompensatorImageData);
//  rangeCompensatorVolumeNode->SetSpacing(rcVolumeItk->GetSpacing()[0], rcVolumeItk->GetSpacing()[1], rcVolumeItk->GetSpacing()[2]);
//  rangeCompensatorVolumeNode->SetOrigin(rcVolumeItk->GetOrigin()[0], rcVolumeItk->GetOrigin()[1], rcVolumeItk->GetOrigin()[2]);
//
//  std::string rangeCompensatorNodeName = std::string(beamNode->GetName()) + "_RangeCompensator";
//  rangeCompensatorVolumeNode->SetName(rangeCompensatorNodeName.c_str());
//  scene->AddNode(rangeCompensatorVolumeNode);
//  
//  this->addIntermediateResult(rangeCompensatorVolumeNode, beamNode);
//
//  // Compute the dose
//  try
//  {
//    if (rt_beam->get_beam_line_type() != "passive")
//    {
//      // Active
//      rt_beam->get_mebs()->compute_particle_number_matrix_from_target_active_slicerRt(rt_beam->rpl_vol, targetPlmVolume, rt_beam->get_smearing());
//    }
//    else
//    {
//      // Passive
//      rt_beam->rpl_vol->compute_beam_modifiers_passive_scattering_slicerRt (targetPlmVolume, rt_beam->get_smearing(), rt_beam->get_mebs()->get_proximal_margin(), rt_beam->get_mebs()->get_distal_margin());
//      rt_beam->get_mebs()->set_prescription_depths(rt_beam->rpl_vol->get_min_wed(), rt_beam->rpl_vol->get_max_wed());
//      rt_beam->rpl_vol->apply_beam_modifiers();
//      rt_beam->get_mebs()->optimize_sobp();
//      int ap_dim[2] = {rt_beam->rpl_vol->get_aperture()->get_dim()[0], rt_beam->rpl_vol->get_aperture()->get_dim()[1]};
//      rt_beam->get_mebs()->generate_part_num_from_weight(ap_dim);
//    }
//
//    // We can compute the dose
//    rt_plan.compute_dose(rt_beam);
//  }
//  catch (std::exception& ex)
//  {
//    QString errorMessage("Plastimatch exception happened! See log for details");
//    qCritical() << Q_FUNC_INFO << ": " << errorMessage << ": " << ex.what();
//    return errorMessage;
//  }
//
//  // Get per-beam dose image and set it to result node
//  itk::Image<float, 3>::Pointer doseVolumeItk = rt_beam->get_dose()->itk_float();
//
//  // Create dose image data to set to the volume node
//  vtkSmartPointer<vtkImageData> protonDoseImageData = vtkSmartPointer<vtkImageData>::New();
//  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(doseVolumeItk, protonDoseImageData, VTK_FLOAT);
//
//  // Set image data to result dose volume node
//  resultDoseVolumeNode->SetAndObserveImageData(protonDoseImageData);
//  resultDoseVolumeNode->CopyOrientation(referenceVolumeNode);
//
//  std::string protonDoseNodeName = std::string(beamNode->GetName()) + "_ProtonDose";
//  resultDoseVolumeNode->SetName(protonDoseNodeName.c_str());
//
  return QString();
}
