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
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTimerLog.h>
#include <vtkPolyData.h>
#include <vtkPiecewiseFunction.h>

// STD includes
#include <math.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCalculateOversamplingFactor);

//----------------------------------------------------------------------------
vtkCalculateOversamplingFactor::vtkCalculateOversamplingFactor()
{
  this->ContourNode = NULL;
  this->RasterizationReferenceVolumeNode = NULL;
  this->OutputOversamplingFactor = 1;
  this->MassPropertiesAlgorithm = NULL;
  //this->LogSpeedMeasurementsOff();
  this->LogSpeedMeasurementsOn(); //TODO:
}

//----------------------------------------------------------------------------
vtkCalculateOversamplingFactor::~vtkCalculateOversamplingFactor()
{
  this->SetContourNode(NULL);
  this->SetRasterizationReferenceVolumeNode(NULL);
  this->SetMassPropertiesAlgorithm(NULL);
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
  if (!this->RasterizationReferenceVolumeNode)
  {
    vtkErrorMacro("CalculateOversamplingFactor: Invalid rasterization reference volume node!");
    return false;
  }

  // Closed surface model is preferred to determine oversampling factor, because
  // ribbon is open and provides very inaccurate surface area.
  //TODO: When the PlanarContoursToSurface algorithm is integrated, it can be used to create closed surface directly from planar contours
  vtkPolyData* polyDataUsedForOversamplingCalculation = NULL;
  if (this->ContourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel))
  {
    polyDataUsedForOversamplingCalculation = this->ContourNode->GetClosedSurfacePolyData();
  }
  else if (this->ContourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel))
  {
    // Log warning about the drawbacks of using ribbon model
    vtkErrorMacro("CalculateOversamplingFactor: Ribbon model is used to calculate oversampling factor for contour " << this->ContourNode->GetName() << ". Shape measurement may be inaccurate for certain structures!");

    polyDataUsedForOversamplingCalculation = this->ContourNode->GetRibbonModelPolyData();
  }
  else
  {
    vtkErrorMacro("CalculateOversamplingFactor: No suitable representation has been found in contour for oversampling factor calculation!");
    return false;
  }

  // Mark start time
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed

  // Create mass properties algorithm for common use
  vtkSmartPointer<vtkMassProperties> massProperties = vtkSmartPointer<vtkMassProperties>::New();
  this->SetMassPropertiesAlgorithm(massProperties);
#if (VTK_MAJOR_VERSION <= 5)
  massProperties->SetInput(polyDataUsedForOversamplingCalculation);
#else
  massProperties->SetInputData(polyDataUsedForOversamplingCalculation);
#endif
  // Run algorithm so that results can be extracted for relative structure size calculation and complexity measure
  massProperties->Update();

  // Get relative structure size
  double relativeStructureSize = this->CalculateRelativeStructureSize();
  if (relativeStructureSize == -1.0)
  {
    vtkErrorMacro("CalculateOversamplingFactor: Failed to calculate relative structure size");
    return false;
  }

  // Get complexity measure
  double complexityMeasure = this->CalculateComplexityMeasure();
  if (complexityMeasure == -1.0)
  {
    vtkErrorMacro("CalculateOversamplingFactor: Failed to calculate complexity measure");
    return false;
  }

  double checkpointFuzzyStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointFuzzyStart); // Although it is used later, a warning is logged so needs to be suppressed

  // Determine crisp oversampling factor based on crisp inputs using fuzzy rules
  this->OutputOversamplingFactor = this->DetermineOversamplingFactor(relativeStructureSize, complexityMeasure);

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed

    vtkDebugMacro("CalculateOversamplingFactor: Total automatic oversampling calculation time for contour " << this->ContourNode->GetName() << ": " << checkpointEnd-checkpointStart << " s\n"
      << "\tCalcilating relative structure size and complexity measure: " << checkpointFuzzyStart-checkpointStart << " s\n"
      << "\tDetermining oversampling factor using fuzzy rules: " << checkpointEnd-checkpointFuzzyStart << " s");
  }

  // Set calculated oversampling factor to contour node
  this->ContourNode->SetRasterizationOversamplingFactor(this->OutputOversamplingFactor);

  // Clean up (triggers destruction of member)
  this->SetMassPropertiesAlgorithm(NULL);

  return true;
}

//----------------------------------------------------------------------------
double vtkCalculateOversamplingFactor::CalculateRelativeStructureSize()
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("CalculateRelativeStructureSize: Invalid contour node!");
    return -1.0;
  }
  if (!this->RasterizationReferenceVolumeNode)
  {
    vtkErrorMacro("CalculateRelativeStructureSize: Invalid rasterization reference volume node!");
    return -1.0;
  }
  if (!this->MassPropertiesAlgorithm)
  {
    vtkErrorMacro("CalculateRelativeStructureSize: Invalid mass properties algorithm!");
    return -1.0;
  }

  // Get structure volume in mm^3
  double structureVolume = this->MassPropertiesAlgorithm->GetVolume();

  // Sanity check
  double structureProjectedVolume = this->MassPropertiesAlgorithm->GetVolumeProjected();
  double error = (structureVolume - structureProjectedVolume);
  if (error * 10000 > structureVolume)
  {
    vtkWarningMacro("CalculateRelativeStructureSize: Computed structure volume is possibly invalid for contour " << this->ContourNode->GetName());
  }

  // Calculate reference volume in mm^3
  vtkImageData* referenceImageData = this->RasterizationReferenceVolumeNode->GetImageData();
  if (!referenceImageData)
  {
    vtkErrorMacro("CalculateRelativeStructureSize: Rasterization reference volume node contains invalid image data!");
    return -1.0;
  }
  int dimensions[3] = {0,0,0};
  referenceImageData->GetDimensions(dimensions);
  double spacing[3] = {0.0,0.0,0.0};
  this->RasterizationReferenceVolumeNode->GetSpacing(spacing);
  double volumeVolume = dimensions[0]*dimensions[1]*dimensions[2] * spacing[0]*spacing[1]*spacing[2]; // Number of voxels * volume of one voxel

  double relativeStructureSize = structureVolume / volumeVolume;

  // Map raw measurement to the fuzzy input scale
  double sizeMeasure = (-1.0) * log10(relativeStructureSize);
  vtkDebugMacro("CalculateRelativeStructureSize: " << this->ContourNode->GetName() << " relative structure size: " << relativeStructureSize << ", size measure: " << sizeMeasure);

  return sizeMeasure;
}

//----------------------------------------------------------------------------
double vtkCalculateOversamplingFactor::CalculateComplexityMeasure()
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("CalculateComplexityMeasure: Invalid contour node!");
    return -1.0;
  }
  if (!this->MassPropertiesAlgorithm)
  {
    vtkErrorMacro("CalculateComplexityMeasure: Invalid mass properties algorithm!");
    return -1.0;
  }

  // Normalized shape index (NSI) characterizes the deviation of the shape of an object
  // from a sphere (from surface area and volume). A sphere's NSI is one. This number is always >= 1.0
  double normalizedShapeIndex = this->MassPropertiesAlgorithm->GetNormalizedShapeIndex();

  // Map raw measurement to the fuzzy input scale
  double complexityMeasure = (normalizedShapeIndex - 1.0 > 0.0 ? normalizedShapeIndex - 1.0 : 0.0); // If smaller then 0, then return 0
  vtkDebugMacro("CalculateComplexityMeasure: " << this->ContourNode->GetName() << " normalized shape index: " << normalizedShapeIndex << ", complexity measure: " << complexityMeasure);

  return complexityMeasure;
}

//---------------------------------------------------------------------------
// Fuzzy membership functions:
// https://www.assembla.com/spaces/slicerrt/documents/bzADACUi8r5kGMdmr6bg7m/download/bzADACUi8r5kGMdmr6bg7m
//
// Fuzzy rules:
// 1. If RSS is Very small, then Oversampling is High
// 2. If RSS is Small and NSI is High, then Oversampling is High
// 3. If RSS is Small and NSI is Low then Oversampling is Medium
// 4. If RSS is Medium and NSI is High, then Oversampling is Medium
// 5. If RSS is Medium and NSI is Low, then Oversampling is Low
// 6. If RSS is Large, then Oversampling is Low
//---------------------------------------------------------------------------
double vtkCalculateOversamplingFactor::DetermineOversamplingFactor(double relativeStructureSize, double complexityMeasure)
{
  if (relativeStructureSize == -1.0 || complexityMeasure == -1.0)
  {
    vtkErrorMacro("DetermineOversamplingFactor: Invalid input measures! Returning default oversampling of 1");
    return 1.0;
  }

  // Define membership functions
  vtkSmartPointer<vtkPiecewiseFunction> sizeLarge = vtkSmartPointer<vtkPiecewiseFunction>::New();
  sizeLarge->AddPoint(0.5, 1);
  sizeLarge->AddPoint(2, 0);
  vtkSmartPointer<vtkPiecewiseFunction> sizeMedium = vtkSmartPointer<vtkPiecewiseFunction>::New();
  sizeMedium->AddPoint(0.5, 0);
  sizeMedium->AddPoint(2, 1);
  sizeMedium->AddPoint(2.5, 1);
  sizeMedium->AddPoint(3, 0);
  vtkSmartPointer<vtkPiecewiseFunction> sizeSmall = vtkSmartPointer<vtkPiecewiseFunction>::New();
  sizeSmall->AddPoint(2.5, 0);
  sizeSmall->AddPoint(3, 1);
  sizeSmall->AddPoint(3.25, 1);
  sizeSmall->AddPoint(3.75, 0);
  vtkSmartPointer<vtkPiecewiseFunction> sizeVerySmall = vtkSmartPointer<vtkPiecewiseFunction>::New();
  sizeVerySmall->AddPoint(3.25, 0);
  sizeVerySmall->AddPoint(3.75, 1);

  vtkSmartPointer<vtkPiecewiseFunction> complexityLow = vtkSmartPointer<vtkPiecewiseFunction>::New();
  complexityLow->AddPoint(0.2, 1);
  complexityLow->AddPoint(0.6, 0);
  vtkSmartPointer<vtkPiecewiseFunction> complexityHigh = vtkSmartPointer<vtkPiecewiseFunction>::New();
  complexityHigh->AddPoint(0.2, 0);
  complexityHigh->AddPoint(0.6, 1);

  // Fuzzify inputs
  double sizeLargeMembership = sizeLarge->GetValue(relativeStructureSize);
  double sizeMediumMembership = sizeMedium->GetValue(relativeStructureSize);
  double sizeSmallMembership = sizeSmall->GetValue(relativeStructureSize);
  double sizeVerySmallMembership = sizeVerySmall->GetValue(relativeStructureSize);

  double complexityLowMembership = complexityLow->GetValue(relativeStructureSize);
  double complexityHighMembership = complexityHigh->GetValue(relativeStructureSize);

  // Apply rules and determine consequents

  // Apply weights to each consequent (clipping)

  // Calculate center of mass

  // Defuzzify output

  //TODO:
  return 1.0;
}
