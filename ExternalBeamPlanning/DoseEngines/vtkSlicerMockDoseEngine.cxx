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
#include "vtkSlicerMockDoseEngine.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// Segmentations includes
#include "vtkOrientedImageData.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMockDoseEngine);

//----------------------------------------------------------------------------
vtkSlicerMockDoseEngine::vtkSlicerMockDoseEngine()
{
  this->SetName("Mock random");
}

//----------------------------------------------------------------------------
vtkSlicerMockDoseEngine::~vtkSlicerMockDoseEngine()
{
}

//----------------------------------------------------------------------------
void vtkSlicerMockDoseEngine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkSlicerMockDoseEngine::CreateBeamForEngine()
{
  return vtkMRMLRTBeamNode::New();
}

//---------------------------------------------------------------------------
std::string vtkSlicerMockDoseEngine::CalculateDoseUsingEngine(vtkMRMLRTBeamNode* beamNode, vtkMRMLScalarVolumeNode* resultDoseVolumeNode)
{
  if (!beamNode)
  {
    std::cerr << "vtkSlicerMockDoseEngine::vtkInternal::CalculateDoseUsingDebugEngine: Invalid beam!";
    return "Invalid beam";
  }
  vtkMRMLRTPlanNode* parentPlanNode = beamNode->GetParentPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = parentPlanNode->GetReferenceVolumeNode();
  if (!parentPlanNode || !referenceVolumeNode || !resultDoseVolumeNode)
  {
    std::string errorMessage("Unable to access reference volume");
    vtkErrorWithObjectMacro(beamNode, "CalculateDoseUsingEngine: " << errorMessage);
    return errorMessage;
  }

  vtkMRMLScene* scene = beamNode->GetScene();

  vtkSmartPointer<vtkClosedSurfaceToBinaryLabelmapConversionRule> converter = 
    vtkSmartPointer<vtkClosedSurfaceToBinaryLabelmapConversionRule>::New();
  converter->SetUseOutputImageDataGeometry(true);
  vtkSmartPointer<vtkSegment> beamSegment = vtkSmartPointer<vtkSegment>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateSegmentFromModelNode(beamNode) );
  vtkPolyData* beamPolyData = vtkPolyData::SafeDownCast(beamSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName()));
  vtkSmartPointer<vtkOrientedImageData> beamImageData = vtkSmartPointer<vtkOrientedImageData>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(referenceVolumeNode) );
  converter->Convert(beamPolyData, beamImageData);

  // Create dose image
  vtkSmartPointer<vtkImageData> protonDoseImageData = vtkSmartPointer<vtkImageData>::New();
  protonDoseImageData->SetExtent(referenceVolumeNode->GetImageData()->GetExtent());
  protonDoseImageData->SetSpacing(referenceVolumeNode->GetImageData()->GetSpacing());
  protonDoseImageData->SetOrigin(referenceVolumeNode->GetImageData()->GetOrigin());
  protonDoseImageData->AllocateScalars(VTK_FLOAT, 1);
  if ( beamImageData->GetNumberOfPoints() != protonDoseImageData->GetNumberOfPoints()
    || beamImageData->GetScalarType() != VTK_UNSIGNED_CHAR )
  {
    std::string errorMessage("Geometrical discrepancy between beam and dose");
    vtkErrorWithObjectMacro(beamNode, "CalculateDoseUsingEngine: " << errorMessage);
    return errorMessage;
  }

  // Paint voxels touched by beam prescription+noise, all others zero
  double rxDose = parentPlanNode->GetRxDose();
  unsigned char* beamPtr = (unsigned char*)beamImageData->GetScalarPointer();
  float* floatPtr = (float*)protonDoseImageData->GetScalarPointer();
  for (long i=0; i<protonDoseImageData->GetNumberOfPoints(); ++i)
  {
    if ((*beamPtr) > 0)
    {
      (*floatPtr) = rxDose*19.0/20.0 + (float)(rand()%100)*rxDose/1000.0;
    }
    else
    {
      (*floatPtr) = 0;
    }
    ++floatPtr;
    ++beamPtr;
  }

  resultDoseVolumeNode->SetAndObserveImageData(protonDoseImageData);
  resultDoseVolumeNode->CopyOrientation(referenceVolumeNode);

  std::string randomDoseNodeName = std::string(beamNode->GetName()) + "_MockDose";
  resultDoseVolumeNode->SetName(randomDoseNodeName.c_str());

  return "";
}
