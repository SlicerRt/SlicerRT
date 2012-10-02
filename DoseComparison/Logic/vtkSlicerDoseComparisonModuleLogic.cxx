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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// DoseComparison includes
#include "vtkSlicerDoseComparisonModuleLogic.h"
#include "vtkMRMLDoseComparisonNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// Plastimatch includes
#include "gamma_dose_comparison.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageExport.h>
#include <vtkTimerLog.h>
#include <vtkTransform.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseComparisonModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic::vtkSlicerDoseComparisonModuleLogic()
{
  this->DoseComparisonNode = NULL;

  this->LogSpeedMeasurementsOff();
}

//----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic::~vtkSlicerDoseComparisonModuleLogic()
{
  SetAndObserveDoseComparisonNode(NULL); // release the node object to avoid memory leaks
}

//----------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::SetAndObserveDoseComparisonNode(vtkMRMLDoseComparisonNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseComparisonNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerDoseComparisonModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseComparisonNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLDoseComparisonNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLDoseComparisonNode");
  if (node)
  {
    paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->DoseComparisonNode, paramNode);
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseComparisonModuleLogic::DoseVolumeContainsDose(vtkMRMLNode* node)
{
  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(node);
  const char* doseUnitName = doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());

  if (doseUnitName != NULL)
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic
::ConvertVolumeNodeToItkImage(vtkMRMLVolumeNode* inVolumeNode, itk::Image<float, 3>::Pointer outItkVolume)
{
  if ( inVolumeNode == NULL )
  {
    vtkErrorMacro("Failed to convert vtk image to itk image - input MRML volume node is NULL!"); 
    return; 
  }

  vtkImageData* inVolume = inVolumeNode->GetImageData();
  if ( inVolume == NULL )
  {
    vtkErrorMacro("Failed to convert vtk image to itk image - image in input MRML volume node is NULL!"); 
    return; 
  }

  if ( outItkVolume.IsNull() )
  {
    vtkErrorMacro("Failed to convert vtk image to itk image - output image is NULL!"); 
    return; 
  }

  // convert vtkImageData to itkImage 
  vtkSmartPointer<vtkImageExport> imageExport = vtkSmartPointer<vtkImageExport>::New(); 
  imageExport->SetInput(inVolume);
  imageExport->Update(); 

  // Determine input volume to world transform
  vtkSmartPointer<vtkMatrix4x4> rasToWorldTransformMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  vtkMRMLTransformNode* inTransformNode=inVolumeNode->GetParentTransformNode();
  if (inTransformNode!=NULL)
  {
    if (inTransformNode->IsTransformToWorldLinear() == 0)
    {
      vtkErrorMacro("There is a non-linear transform assigned to an input dose volume. Only linear transforms are supported!");
      return;
    }
    inTransformNode->GetMatrixTransformToWorld(rasToWorldTransformMatrix);
  }

  vtkSmartPointer<vtkMatrix4x4> inVolumeToRasTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inVolumeNode->GetIJKToRASMatrix(inVolumeToRasTransformMatrix);

  vtkSmartPointer<vtkTransform> inVolumeToWorldTransform = vtkSmartPointer<vtkTransform>::New();
  inVolumeToWorldTransform->Identity();
  inVolumeToWorldTransform->PostMultiply();
  inVolumeToWorldTransform->Concatenate(inVolumeToRasTransformMatrix);
  inVolumeToWorldTransform->Concatenate(rasToWorldTransformMatrix);

  // Set ITK image properties
  double outputSpacing[3];
  inVolumeToWorldTransform->GetScale(outputSpacing);
  outItkVolume->SetSpacing(outputSpacing);

  double outputOrigin[3];
  inVolumeToWorldTransform->GetPosition(outputOrigin);
  outItkVolume->SetOrigin(outputOrigin);

  double outputOrienationAngles[3];
  inVolumeToWorldTransform->GetOrientation(outputOrienationAngles);
  vtkSmartPointer<vtkTransform> inVolumeToWorldOrientationTransform = vtkSmartPointer<vtkTransform>::New();
  inVolumeToWorldOrientationTransform->Identity();
  inVolumeToWorldOrientationTransform->RotateX(outputOrienationAngles[0]);
  inVolumeToWorldOrientationTransform->RotateY(outputOrienationAngles[1]);
  inVolumeToWorldOrientationTransform->RotateZ(outputOrienationAngles[2]);
  vtkSmartPointer<vtkMatrix4x4> inVolumeToWorldOrientationTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inVolumeToWorldOrientationTransform->GetMatrix(inVolumeToWorldOrientationTransformMatrix);
  itk::Matrix<double,3,3> outputDirectionMatrix;
  for(int i=0; i<3; i++)
  {
    for(int j=0; j<3; j++)
    {
      outputDirectionMatrix[i][j] = inVolumeToWorldOrientationTransformMatrix->GetElement(i,j);
    }
  }
  outItkVolume->SetDirection(outputDirectionMatrix);

  int inputExtent[6]={0,0,0,0,0,0}; 
  inVolume->GetExtent(inputExtent); 
  itk::Image<float, 3>::SizeType inputSize;
  inputSize[0] = inputExtent[1] - inputExtent[0] + 1;
  inputSize[1] = inputExtent[3] - inputExtent[2] + 1;
  inputSize[2] = inputExtent[5] - inputExtent[4] + 1;

  itk::Image<float, 3>::IndexType start;
  start[0]=start[1]=start[2]=0.0;

  itk::Image<float, 3>::RegionType region;
  region.SetSize(inputSize);
  region.SetIndex(start);
  outItkVolume->SetRegions(region);

  try
  {
    outItkVolume->Allocate();
  }
  catch(itk::ExceptionObject & err)
  {
    vtkErrorMacro("Failed to allocate memory for the image conversion: " << err.GetDescription() );
    return;
  }

  imageExport->Export( outItkVolume->GetBufferPointer() );
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::ComputeGammaDoseDifference()
{
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();

  // Convert input images to the format Plastimatch can use
  vtkMRMLVolumeNode* referenceDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseComparisonNode->GetReferenceDoseVolumeNodeId()));
  itk::Image<float, 3>::Pointer referenceDoseVolumeItk = itk::Image<float, 3>::New();

  vtkMRMLVolumeNode* compareDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseComparisonNode->GetCompareDoseVolumeNodeId()));
  itk::Image<float, 3>::Pointer compareDoseVolumeItk = itk::Image<float, 3>::New();

  // Convert inputs to ITK images
  double checkpointItkConvertStart = timer->GetUniversalTime();
  ConvertVolumeNodeToItkImage(referenceDoseVolumeNode, referenceDoseVolumeItk);
  ConvertVolumeNodeToItkImage(compareDoseVolumeNode, compareDoseVolumeItk);

  // Compute gamma dose volume
  double checkpointGammaStart = timer->GetUniversalTime();
  Gamma_dose_comparison gamma;
  gamma.set_reference_image(referenceDoseVolumeItk);
  gamma.set_compare_image(compareDoseVolumeItk);
  gamma.set_spatial_tolerance(this->DoseComparisonNode->GetDtaDistanceToleranceMm());
  gamma.set_dose_difference_tolerance(this->DoseComparisonNode->GetDoseDifferenceTolerancePercent());
  if (!this->DoseComparisonNode->GetUseMaximumDose())
  {
    gamma.set_reference_dose(this->DoseComparisonNode->GetReferenceDoseGy());
  }
  //gamma.set_analysis_threshold(this->DoseComparisonNode->GetAnalysisThresholdPercent()); //TODO: uncomment when Plastimatch supports it
  gamma.set_gamma_max(this->DoseComparisonNode->GetMaximumGamma());

  gamma.run();

  itk::Image<float, 3>::Pointer gammaVolumeItk = gamma.get_gamma_image_itk();

  // Convert output to VTK
  double checkpointVtkConvertStart = timer->GetUniversalTime();
  vtkSmartPointer<vtkImageData> gammaVolume = vtkSmartPointer<vtkImageData>::New();
  itk::Image<float, 3>::RegionType region = gammaVolumeItk->GetBufferedRegion();
  itk::Image<float, 3>::SizeType imageSize = region.GetSize();
  int extent[6]={0, imageSize[0]-1, 0, imageSize[1]-1, 0, imageSize[2]-1};
  gammaVolume->SetExtent(extent);
  gammaVolume->SetScalarType(VTK_FLOAT);
  gammaVolume->SetNumberOfScalarComponents(1);
  gammaVolume->AllocateScalars();

  float* gammaPtr = (float*)gammaVolume->GetScalarPointer();
  itk::ImageRegionIteratorWithIndex< itk::Image<float, 3> > itGammaItk(
    gammaVolumeItk, gammaVolumeItk->GetLargestPossibleRegion() );
  for ( itGammaItk.GoToBegin(); !itGammaItk.IsAtEnd(); ++itGammaItk )
  {
    itk::Image<float, 3>::IndexType i = itGammaItk.GetIndex();
    (*gammaPtr) = gammaVolumeItk->GetPixel(i);
    gammaPtr++;
  }

  // Set properties of output volume node
  vtkMRMLVolumeNode* gammaVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseComparisonNode->GetGammaVolumeNodeId()));

  if (gammaVolumeNode==NULL)
  {
    vtkErrorMacro("gammaVolumeNode is invalid");
    return;
  }

  gammaVolumeNode->CopyOrientation(referenceDoseVolumeNode);
  gammaVolumeNode->SetAndObserveImageData(gammaVolume);

  // Assign gamma volume under the transform node of the reference volume node
  if (referenceDoseVolumeNode->GetParentTransformNode())
  {
    gammaVolumeNode->SetAndObserveTransformNodeID(referenceDoseVolumeNode->GetParentTransformNode()->GetID());
  }

  // Set default colormap to red
  if (gammaVolumeNode->GetVolumeDisplayNode()==NULL)
  {
    // gammaVolumeNode->CreateDefaultDisplayNodes(); unfortunately this is not implemented for Scalar nodes
    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> sdisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    sdisplayNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(sdisplayNode);
    gammaVolumeNode->SetAndObserveDisplayNodeID(sdisplayNode->GetID());
  }
  if (gammaVolumeNode->GetVolumeDisplayNode()!=NULL)
  {
    gammaVolumeNode->GetVolumeDisplayNode()->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
  }
  else
  {
    vtkWarningMacro("Display node is not available for gamma volume node. The default color table will be used.");
  }

  // Select as active volume
  if (this->GetApplicationLogic()!=NULL)
  {
    if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(gammaVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection();
    }
  }

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    std::cout << "Total gamma computation time: " << checkpointEnd-checkpointStart << " s" << std::endl
      << "\tApplying transforms: " << checkpointItkConvertStart-checkpointStart << " s" << std::endl
      << "\tConverting from VTK to ITK: " << checkpointGammaStart-checkpointItkConvertStart << " s" << std::endl
      << "\tGamma computation: " << checkpointVtkConvertStart-checkpointGammaStart << " s" << std::endl
      << "\tConverting back from ITK to VTK: " << checkpointEnd-checkpointVtkConvertStart << " s" << std::endl;
  }
}
