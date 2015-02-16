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

// Contours includes
#include "vtkCalculateOversamplingFactor.h"
#include "vtkMRMLContourNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTimerLog.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCalculateOversamplingFactor);

//----------------------------------------------------------------------------
vtkCalculateOversamplingFactor::vtkCalculateOversamplingFactor()
{
  this->ContourNode = NULL;
  this->OutputOversamplingFactor = 1;
  this->LogSpeedMeasurementsOff();
}

//----------------------------------------------------------------------------
vtkCalculateOversamplingFactor::~vtkCalculateOversamplingFactor()
{
  this->SetContourNode(NULL);
}

//----------------------------------------------------------------------------
void vtkCalculateOversamplingFactor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkCalculateOversamplingFactor::CalculateOversamplingFactor()
{
  // Set a safe value to use even if the return value is not checked
  this->OutputOversamplingFactor = 1;

  if (!this->ContourNode)
  {
    vtkErrorMacro("CalculateOversamplingFactor: Invalid contour node!");
    return false;
  }
  vtkMRMLScene* mrmlScene = this->ContourNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("CalculateOversamplingFactor: Invalid scene!");
    return false;
  }

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed

  // Get reference volume node
  vtkMRMLScalarVolumeNode* selectedReferenceVolumeNode = this->ContourNode->GetRasterizationReferenceVolumeNode();
  if (!selectedReferenceVolumeNode)
  {
    vtkErrorMacro("CalculateOversamplingFactor: No reference volume node set to contour node " << this->ContourNode->GetName());
    return false;
  }

  // Resample to selected reference coordinate system if referenced anatomy was used
  double checkpointResamplingStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointResamplingStart); // Although it is used later, a warning is logged so needs to be suppressed

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed

    //vtkDebugMacro("CalculateOversamplingFactor: Total model-labelmap conversion time for contour " << this->ContourNode->GetName() << ": " << checkpointEnd-checkpointStart << " s\n"
    //  << "\tAccessing associated nodes and transform model: " << checkpointLabelmapConversionStart-checkpointStart << " s\n"
    //  << "\tConverting to labelmap (to referenced series if available): " << checkpointResamplingStart-checkpointLabelmapConversionStart << " s\n"
    //  << "\tResampling referenced series to selected reference: " << checkpointEnd-checkpointResamplingStart << " s");
  }

  return true;
}
