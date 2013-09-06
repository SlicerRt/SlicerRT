/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// ExternalBeamPlanning includes
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"
#include "vtkMRMLExternalBeamPlanningNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanHierarchyNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"

// Plastimatch includes
#include "ion_beam.h"
#include "ion_plan.h"
#include "itk_image_save.h"
#include "rpl_volume.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkConeSource.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerExternalBeamPlanningModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkSlicerExternalBeamPlanningModuleLogic()
{
  this->ExternalBeamPlanningNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::~vtkSlicerExternalBeamPlanningModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetAndObserveExternalBeamPlanningNode(vtkMRMLExternalBeamPlanningNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLExternalBeamPlanningNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTBeamNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanHierarchyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->ExternalBeamPlanningNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLExternalBeamPlanningNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->ExternalBeamPlanningNode)
    {
    return;
    }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLExternalBeamPlanningNode"))
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLExternalBeamPlanningNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLExternalBeamPlanningNode");
  if (node)
    {
    paramNode = vtkMRMLExternalBeamPlanningNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, paramNode);
    }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::CreateBeamPolyData()
{
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateBeam(char *beamname, double gantryAngle)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    return;
  }
  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetRTPlanNodeID()));
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetReferenceVolumeNodeID()));
  vtkMRMLAnnotationFiducialNode* IsocenterFiducialNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetIsocenterNodeID()));
  // Make sure inputs are initialized
  if (!RTPlanNode || !IsocenterFiducialNode )
  {
    vtkErrorMacro("RTPlan: inputs are not initialized!")
    return ;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  RTPlanNode->GetRTBeamNodes(beams);
  // Fill the table
  if (!beams) return;
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      if ( strcmp(beamNode->GetBeamName(), beamname) == 0)
      {
        //RTPlanNode->RemoveRTBeamNode(beamNode);
        break;
      }
    }
  }

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(gantryAngle);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  IsocenterFiducialNode->GetFiducialCoordinates(isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();

  vtkMRMLLinearTransformNode *transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
             this->GetMRMLScene()->GetNodeByID(beamModelNode->GetTransformNodeID()));
  if (transformNode)
  {
    transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::AddBeam()
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    return;
  }

  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetRTPlanNodeID()));
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetReferenceVolumeNodeID()));
  vtkMRMLAnnotationFiducialNode* IsocenterFiducialNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetIsocenterNodeID()));
  // Make sure inputs are initialized
  if (!RTPlanNode || !IsocenterFiducialNode )
  {
    vtkErrorMacro("RTPlan: inputs are not initialized!")
    return ;
  }

  // Get rtplan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
  double baseRadius = 2;
  coneSource->SetRadius(baseRadius*2.0*sqrt(2.0));
  coneSource->SetHeight(100*2.0);
  coneSource->SetResolution(4);
  coneSource->SetDirection(0,1,0);
  coneSource->Update();

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(0);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  IsocenterFiducialNode->GetFiducialCoordinates(isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  transformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->AddNode(transformNode));
  transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  
  // Create rtbeam model node
  vtkSmartPointer<vtkMRMLModelNode> RTBeamModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  RTBeamModelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelNode));
  //RTBeamNode->SetBeamName("BeamName");
  RTBeamModelNode->SetAndObservePolyData(coneSource->GetOutput());
  RTBeamModelNode->SetAndObserveTransformNodeID(transformNode->GetID());
  RTBeamModelNode->HideFromEditorsOff();

  // create model node for beam
  vtkSmartPointer<vtkMRMLModelDisplayNode> RTBeamModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  RTBeamModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelDisplayNode));
  RTBeamModelDisplayNode->SetColor(0.0, 1.0, 0.0);
  RTBeamModelDisplayNode->SetOpacity(0.3);
  // Disable backface culling to make the back side of the contour visible as well
  RTBeamModelDisplayNode->SetBackfaceCulling(0);
  RTBeamModelDisplayNode->HideFromEditorsOff();
  RTBeamModelDisplayNode->VisibilityOn(); 
  RTBeamModelNode->SetAndObserveDisplayNodeID(RTBeamModelDisplayNode->GetID());
  RTBeamModelDisplayNode->SliceIntersectionVisibilityOn();  

  // Create rtbeam node
  vtkSmartPointer<vtkMRMLRTBeamNode> RTBeamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  RTBeamNode = vtkMRMLRTBeamNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamNode));
  RTBeamNode->SetBeamName("BeamName");
  RTBeamNode->SetAndObserveBeamModelNodeId(RTBeamModelNode->GetID());
  RTBeamNode->HideFromEditorsOff();

  // Put the RTBeam node in the hierarchy
  RTPlanNode->AddRTBeamNode(RTBeamNode);

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RemoveBeam(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    return;
  }

  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetRTPlanNodeID()));
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetReferenceVolumeNodeID()));
  // Make sure inputs are initialized
  if (!RTPlanNode)
  {
    vtkErrorMacro("RTPlan: inputs are not initialized!")
    return ;
  }

  // Get rtplan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  // Find rtbeam node in rtplan hierarchy node
  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  RTPlanNode->GetRTBeamNodes(beams);

  beams->InitTraversal();
  if (beams->GetNumberOfItems() < 1)
  {
    std::cerr << "Warning: Selected RTPlan node has no children contour nodes!" << std::endl;
    return;
  }

  // Fill the table
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      if ( strcmp(beamNode->GetBeamName(), beamname) == 0)
      {
        vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();
        vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelDisplayNode = beamModelNode->GetModelDisplayNode();
        RTPlanNode->RemoveRTBeamNode(beamNode);
        this->GetMRMLScene()->RemoveNode(beamModelNode);
        this->GetMRMLScene()->RemoveNode(beamModelDisplayNode);
        break;
      }
    }
  }

  this->Modified();
}

 //---------------------------------------------------------------------------
template<class T> 
static void 
itk_rectify_volume_hack (T image)
{
  typename T::ObjectType::RegionType rg = image->GetLargestPossibleRegion ();
  typename T::ObjectType::PointType og = image->GetOrigin();
  typename T::ObjectType::SpacingType sp = image->GetSpacing();
  typename T::ObjectType::SizeType sz = rg.GetSize();
  typename T::ObjectType::DirectionType dc = image->GetDirection();

  og[0] = og[0] - (sz[0] - 1) * sp[0];
  og[1] = og[1] - (sz[1] - 1) * sp[1];
  dc[0][0] = 1.;
  dc[1][1] = 1.;

  image->SetOrigin(og);
  image->SetDirection(dc);
}

//---------------------------------------------------------------------------
template<class T>
static vtkSmartPointer<vtkImageData> 
itk_to_vtk (T itkImage, int vtkType)
{
  typedef typename T::ObjectType::InternalPixelType TPixel;
  typename T::ObjectType::SizeType imageSize = 
    itkImage->GetBufferedRegion().GetSize();
  int extent[6]={0, (int) imageSize[0]-1, 
                 0, (int) imageSize[1]-1, 
                 0, (int) imageSize[2]-1};

  vtkSmartPointer<vtkImageData> vtkVolume 
    = vtkSmartPointer<vtkImageData>::New();
  vtkVolume->SetExtent(extent);
  vtkVolume->SetScalarType(vtkType);
  vtkVolume->SetNumberOfScalarComponents(1);
  vtkVolume->AllocateScalars();

  TPixel* pixPtr = (TPixel*) vtkVolume->GetScalarPointer();

  itk::ImageRegionIteratorWithIndex< itk::Image<TPixel, 3> > 
    itPix (itkImage, 
           itkImage->GetLargestPossibleRegion());
  for (itPix.GoToBegin(); !itPix.IsAtEnd(); ++itPix)
  {
    typename itk::Image<TPixel, 3>::IndexType i = itPix.GetIndex();
    (*pixPtr) = itkImage->GetPixel(i);
    pixPtr++;
  }

  return vtkVolume;
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeDose()
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    return;
  }

  // Convert input images to ITK format for Plastimatch
  vtkMRMLVolumeNode* referenceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetReferenceVolumeNodeID()));
  itk::Image<short, 3>::Pointer referenceVolumeItk = itk::Image<short, 3>::New();

  vtkMRMLContourNode* targetContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetProtonTargetVolumeNodeID()));
  vtkMRMLScalarVolumeNode* targetVolumeNode = targetContourNode->GetIndexedLabelmapVolumeNode();
  itk::Image<unsigned char, 3>::Pointer targetVolumeItk = itk::Image<unsigned char, 3>::New();

  SlicerRtCommon::ConvertVolumeNodeToItkImage<short>(referenceVolumeNode, referenceVolumeItk);
  SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(targetVolumeNode, targetVolumeItk);

  // Ray tracing code expects identity direction cosines.  This is a hack.
  itk_rectify_volume_hack (referenceVolumeItk);
  itk_rectify_volume_hack (targetVolumeItk);

  Ion_plan ion_plan;

  try {
    // Assign inputs to dose calc logic
    printf ("Setting reference volume\n");
    ion_plan.set_patient (referenceVolumeItk);
    printf ("Setting target volume\n");
    ion_plan.set_target (targetVolumeItk);
    printf ("Done.\n");

    printf ("Gantry angle is: %g\n",
            this->ExternalBeamPlanningNode->GetGantryAngle());

    float src_dist = 2000;
    float src[3];
    float isocenter[3] = { 0, 0, 0 };

    /* Adjust src according to gantry angle */
    float ga_radians = 
      this->ExternalBeamPlanningNode->GetGantryAngle() * M_PI / 180.;
    src[0] = - src_dist * sin(ga_radians);
    src[1] = src_dist * cos(ga_radians);
    src[2] = 0.f;

    ion_plan.beam->set_source_position (src);
    ion_plan.beam->set_isocenter_position (isocenter);

    float ap_offset = 1500;
    int ap_dim[2] = { 30, 30 };
//    float ap_origin[2] = { -19, -19 };
    float ap_spacing[2] = { 2, 2 };
    ion_plan.get_aperture()->set_distance (ap_offset);
    ion_plan.get_aperture()->set_dim (ap_dim);
//    ion_plan.get_aperture()->set_origin (ap_origin);
    ion_plan.get_aperture()->set_spacing (ap_spacing);
    ion_plan.set_step_length (1);
    ion_plan.set_smearing (this->ExternalBeamPlanningNode->GetSmearing());
    if (!ion_plan.init ()) {
      /* Failure.  How to notify the user?? */
      std::cerr << "Sorry, ion_plan.init() failed.\n";
      return;
    }

    /* A little warm fuzzy for the developers */
    ion_plan.debug ();
    printf ("Working...\n");
    fflush(stdout);

    /* Compute the aperture and range compensator */
    vtkWarningMacro ("Computing beam modifier\n");
    ion_plan.compute_beam_modifiers ();
    vtkWarningMacro ("Computing beam modifier done!\n");

  } catch (std::exception& ex) {
    vtkWarningMacro ("Plastimatch exception: " << ex.what());
    return;
  }

  /* Get aperture as itk image */
  Rpl_volume *rpl_vol = ion_plan.rpl_vol;
  Plm_image::Pointer& ap 
    = rpl_vol->get_aperture()->get_aperture_image();
  itk::Image<unsigned char, 3>::Pointer apertureVolumeItk 
    = ap->itk_uchar();

  /* Get range compensator as itk image */
  Plm_image::Pointer& rc 
    = rpl_vol->get_aperture()->get_range_compensator_image();
  itk::Image<float, 3>::Pointer rcVolumeItk 
    = rc->itk_float();

  /* Convert range compensator image to vtk */
  vtkSmartPointer<vtkImageData> rcVolume 
    = itk_to_vtk (rcVolumeItk, VTK_FLOAT);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> rcVolumeNode 
    = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  rcVolumeNode->SetAndObserveImageData (rcVolume);
  rcVolumeNode->SetSpacing (
    rcVolumeItk->GetSpacing()[0],
    rcVolumeItk->GetSpacing()[1],
    rcVolumeItk->GetSpacing()[2]);
  rcVolumeNode->SetOrigin (
    rcVolumeItk->GetOrigin()[0],
    rcVolumeItk->GetOrigin()[1],
    rcVolumeItk->GetOrigin()[2]);

  std::string rcVolumeNodeName = this->GetMRMLScene()
    ->GenerateUniqueName(std::string ("range_compensator_"));
  rcVolumeNode->SetName(rcVolumeNodeName.c_str());

  rcVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(rcVolumeNode);

  /* Convert aperture image to vtk */
  vtkSmartPointer<vtkImageData> apertureVolume 
    = itk_to_vtk (apertureVolumeItk, VTK_UNSIGNED_CHAR);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> apertureVolumeNode 
    = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  apertureVolumeNode->SetAndObserveImageData (apertureVolume);
  apertureVolumeNode->SetSpacing (
    apertureVolumeItk->GetSpacing()[0],
    apertureVolumeItk->GetSpacing()[1],
    apertureVolumeItk->GetSpacing()[2]);
  apertureVolumeNode->SetOrigin (
    apertureVolumeItk->GetOrigin()[0],
    apertureVolumeItk->GetOrigin()[1],
    apertureVolumeItk->GetOrigin()[2]);

  std::string apertureVolumeNodeName = this->GetMRMLScene()->GenerateUniqueName(std::string ("aperture_"));
  apertureVolumeNode->SetName(apertureVolumeNodeName.c_str());

  apertureVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(apertureVolumeNode);

  /* Compute the dose */
  try {
    vtkWarningMacro ("Applying beam modifiers...\n");
    ion_plan.apply_beam_modifiers ();

    vtkWarningMacro ("Optimizing SOBP\n");
    ion_plan.beam->set_proximal_margin (
      this->ExternalBeamPlanningNode->GetProximalMargin());
    ion_plan.beam->set_distal_margin (
      this->ExternalBeamPlanningNode->GetDistalMargin());
    ion_plan.beam->set_sobp_prescription_min_max (
      rpl_vol->get_min_wed(), rpl_vol->get_max_wed());
    ion_plan.beam->optimize_sobp ();

    vtkWarningMacro ("Computing dose\n");
    ion_plan.compute_dose ();
    vtkWarningMacro ("Computing dose -- complete.\n");
  } catch (std::exception& ex) {
    vtkWarningMacro ("Plastimatch exception: " << ex.what());
    return;
  }

  /* Get dose as itk image */
  itk::Image<float, 3>::Pointer doseVolumeItk 
    = ion_plan.get_dose_itk();

  /* Convert dose image to vtk */
  vtkSmartPointer<vtkImageData> doseVolume 
    = itk_to_vtk (doseVolumeItk, VTK_FLOAT);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> doseVolumeNode 
    = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  doseVolumeNode->SetAndObserveImageData (doseVolume);
  doseVolumeNode->SetSpacing (
    doseVolumeItk->GetSpacing()[0],
    doseVolumeItk->GetSpacing()[1],
    doseVolumeItk->GetSpacing()[2]);
  doseVolumeNode->SetOrigin (
    doseVolumeItk->GetOrigin()[0],
    doseVolumeItk->GetOrigin()[1],
    doseVolumeItk->GetOrigin()[2]);

  std::string doseVolumeNodeName = this->GetMRMLScene()
    ->GenerateUniqueName(std::string ("proton_dose_"));
  doseVolumeNode->SetName(doseVolumeNodeName.c_str());

  doseVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(doseVolumeNode);

  /* Testing .. */
  doseVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  /* More testing .. */
  if (doseVolumeNode->GetVolumeDisplayNode() == NULL)
  {
    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    displayNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(displayNode);
    doseVolumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  }
  if (doseVolumeNode->GetVolumeDisplayNode())
  {
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(doseVolumeNode->GetVolumeDisplayNode());
    doseScalarVolumeDisplayNode->SetAutoWindowLevel(0);
    doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, 16.0);

#if defined (commentout)
    if (this->DefaultGammaColorTableNodeId)
    {
      gammaScalarVolumeDisplayNode->SetAndObserveColorNodeID(this->DefaultGammaColorTableNodeId);
    }
    else
    {
      vtkWarningMacro("ComputeGammaDoseDifference: Loading gamma color table failed, stock color table is used!");
      gammaScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    }
#endif

    /* Just do this... */
    doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    doseScalarVolumeDisplayNode->SetLowerThreshold (1.0);
    doseScalarVolumeDisplayNode->ApplyThresholdOn ();
  }
  else
  {
    vtkWarningMacro("ComputeGammaDoseDifference: Display node is not available for gamma volume node. The default color table will be used.");
  }

#if defined (commentout)
#endif
}

void vtkSlicerExternalBeamPlanningModuleLogic::ComputeWED()
{



  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    return;
  }

  // Convert input images to ITK format for Plastimatch
  vtkMRMLVolumeNode* referenceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetReferenceVolumeNodeID()));
  //  itk::Image<short, 3>::Pointer referenceVolumeItk = itk::Image<short, 3>::New();
  itk::Image<short, 3>::Pointer referenceVolumeItk = itk::Image<short, 3>::New();

  SlicerRtCommon::ConvertVolumeNodeToItkImage<short>(referenceVolumeNode, referenceVolumeItk);

  // Ray tracing code expects identity direction cosines.  This is a hack.
  itk_rectify_volume_hack (referenceVolumeItk);
 

  Ion_plan ion_plan;


  try {
    // Assign inputs to dose calc logic
    printf ("Setting reference volume\n");
    ion_plan.set_patient (referenceVolumeItk);
    printf ("Done.\n");

    printf ("Gantry angle is: %g\n",
            this->ExternalBeamPlanningNode->GetGantryAngle());

    float src_dist = 2000;
    float src[3];
    float isocenter[3] = { 0, 0, 0 };

    /* Adjust src according to gantry angle */
    float ga_radians = 
      this->ExternalBeamPlanningNode->GetGantryAngle() * M_PI / 180.;
    src[0] = - src_dist * sin(ga_radians);
    src[1] = src_dist * cos(ga_radians);
    src[2] = 0.f;

    ion_plan.beam->set_source_position (src);
    ion_plan.beam->set_isocenter_position (isocenter);

    float ap_offset = 1500;
    int ap_dim[2] = { 30, 30 };
//    float ap_origin[2] = { -19, -19 };
    float ap_spacing[2] = { 2, 2 };
    ion_plan.get_aperture()->set_distance (ap_offset);
    ion_plan.get_aperture()->set_dim (ap_dim);
//    ion_plan.get_aperture()->set_origin (ap_origin);
    ion_plan.get_aperture()->set_spacing (ap_spacing);
    ion_plan.set_step_length (1);
    if (!ion_plan.init ()) {
      /* Failure.  How to notify the user?? */
      std::cerr << "Sorry, ion_plan.init() failed.\n";
      return;
    }

    /* A little warm fuzzy for the developers */
    ion_plan.debug ();
    printf ("Working...\n");
    fflush(stdout);

  } catch (std::exception& ex) {
    vtkWarningMacro ("Plastimatch exception: " << ex.what());
    return;
  }

  // Get wed as itk image 
  Rpl_volume *rpl_vol = ion_plan.rpl_vol;

  Plm_image::Pointer patient = Plm_image::New();
  patient->set_itk (referenceVolumeItk);
  Volume* patient_vol = patient->get_vol_float();
  // Volume* wed = rpl_vol->create_wed_volume (&ion_plan); //TODO: this line broke the build, needs to be fixed
  Volume* wed = NULL; // Creating dummy variable to ensure compilation
  return; // TODO: remove this return statement once the 

  // Volume* wed = create_wed_volume (0,&ion_plan);

  // Feed in reference volume, as wed output is the warped reference image.
  rpl_vol->compute_wed_volume(wed,patient_vol,-1000);

  Plm_image::Pointer wed_image = Plm_image::New (new Plm_image (wed));

  itk::Image<float, 3>::Pointer wedVolumeItk 
    = wed_image->itk_float();

  /* Convert aperture image to vtk */
  vtkSmartPointer<vtkImageData> wedVolume 
    = itk_to_vtk (wedVolumeItk, VTK_FLOAT);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> wedVolumeNode 
    = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  wedVolumeNode->SetAndObserveImageData (wedVolume);
  wedVolumeNode->SetSpacing (
    wedVolumeItk->GetSpacing()[0],
    wedVolumeItk->GetSpacing()[1],
    wedVolumeItk->GetSpacing()[2]);
  wedVolumeNode->SetOrigin (
    wedVolumeItk->GetOrigin()[0],
    wedVolumeItk->GetOrigin()[1],
    wedVolumeItk->GetOrigin()[2]);

  std::string wedVolumeNodeName = this->GetMRMLScene()->GenerateUniqueName(std::string ("wed_"));
  wedVolumeNode->SetName(wedVolumeNodeName.c_str());

  wedVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(wedVolumeNode);

#if defined (commentout)
#endif
}
