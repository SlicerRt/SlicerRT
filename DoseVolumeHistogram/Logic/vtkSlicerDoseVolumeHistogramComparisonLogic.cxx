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

  This file was originally developed by Kyle Sunderland and Csaba Pinter,
  PerkLab, Queen's University and was supported through the Applied Cancer
  Research Unit program of Cancer Care Ontario with funds provided by the
  Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// DVH comparison
#include "vtkSlicerDoseVolumeHistogramComparisonLogic.h"

// DVH includes
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkSmartPointer.h>
#include <vtkVersion.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseVolumeHistogramComparisonLogic);

//-----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramComparisonLogic::vtkSlicerDoseVolumeHistogramComparisonLogic()
{
  this->Dvh1DoubleArrayNode = NULL;
  this->Dvh2DoubleArrayNode = NULL;
  this->DoseVolumeNode = NULL;
}

//-----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramComparisonLogic::~vtkSlicerDoseVolumeHistogramComparisonLogic()
{
  this->SetDvh1DoubleArrayNode(NULL);
  this->SetDvh2DoubleArrayNode(NULL);
  this->SetDoseVolumeNode(NULL);
}

//-----------------------------------------------------------------------------
double vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables()
{

  vtkDoubleArray *dvh1Array = this->Dvh1DoubleArrayNode->GetArray();
  unsigned int dvh1Size = dvh1Array->GetNumberOfTuples();

  vtkDoubleArray *dvh2Array = this->Dvh2DoubleArrayNode->GetArray();
  unsigned int dvh2Size = dvh2Array->GetNumberOfTuples();

  vtkDoubleArray *baselineDoubleArray = NULL;
  unsigned int baselineSize = 0;

  vtkDoubleArray *currentDoubleArray = NULL;
  //unsigned int currentSize = 0; //This variable might be used for sanity checks later

  // Determine total volume from the attribute of the current double array node
  std::ostringstream attributeNameStream;
  attributeNameStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  const char* totalVolumeChar;

  // The vtkDoubleArray with the smallest number of tuples is the baseline
  if (dvh1Size < dvh2Size)
  {
    baselineDoubleArray = dvh1Array;
    baselineSize = dvh1Size;

    currentDoubleArray = dvh2Array;
    //currentSize = dvh2Size;
  
    totalVolumeChar = this->Dvh2DoubleArrayNode->GetAttribute(attributeNameStream.str().c_str());
  }
  else
  {
    baselineDoubleArray = dvh2Array;
    baselineSize = dvh2Size;

    currentDoubleArray = dvh1Array;
    //currentSize = dvh1Size;

    totalVolumeChar = this->Dvh1DoubleArrayNode->GetAttribute(attributeNameStream.str().c_str());
  }

  // Read the total volume from the current node attribute
  double totalVolumeCCs = 0;
  if (totalVolumeChar != NULL)
  {
    std::stringstream ss;
    ss << totalVolumeChar;
    double doubleValue;
    ss >> doubleValue;
    totalVolumeCCs = doubleValue;
  }
  
  if (totalVolumeCCs == 0)
  {
    std::cerr << "Invalid volume for structure!" << std::endl;
  }

  // Determine maximum dose
  if (this->DoseVolumeNode != NULL)
  {
    vtkNew<vtkImageAccumulate> doseStat;
#if (VTK_MAJOR_VERSION <= 5)
    doseStat->SetInput(this->DoseVolumeNode->GetImageData());
#else
    doseStat->SetInputData(this->DoseVolumeNode->GetImageData());
#endif
    doseStat->Update();
    this->SetDoseMax(doseStat->GetMax()[0]);
  }

  // Compare the current DVH to the baseline and determine mean and maximum difference
  double agreementAcceptancePercentage = 0.0;
  int numberOfAcceptedAgreements = 0;

  for (unsigned int baselineIndex=0; baselineIndex < baselineSize; ++baselineIndex)
  {
    // Compute the agreement for the current baseline bin
    double agreement = GetAgreementForDvhPlotPoint(currentDoubleArray, baselineDoubleArray, baselineIndex, totalVolumeCCs);

    if (agreement == -1.0)
    {
      std::cerr << "Invalid agreement, skipped!" << std::endl;
      continue;
    }

    if (agreement <= 1.0)
    {
      numberOfAcceptedAgreements++;
    }
      
  }

  agreementAcceptancePercentage = 100.0 * (double)numberOfAcceptedAgreements / (double)baselineSize;

  return agreementAcceptancePercentage;
}

//-----------------------------------------------------------------------------
double vtkSlicerDoseVolumeHistogramComparisonLogic::GetAgreementForDvhPlotPoint(vtkDoubleArray *referenceDvhPlot, vtkDoubleArray *compareDvhPlot,
                                                                                unsigned int compareIndex, double totalVolumeCCs)
{
  // Formula is (based on the article Ebert2010):
  //   gamma(i) = min{ Gamma[(di, vi), (dr, vr)] } for all {r=1..P}, where
  //   compareIndexth DVH point has dose di and volume vi
  //   P is the number of bins in the reference DVH, each rth bin having absolute dose dr and volume vr
  //   Gamma[(di, vi), (dr, vr)] = [ ( (100*(vr-vi)) / (volumeDifferenceCriterion * totalVolume) )^2 + ( (100*(dr-di)) / (doseToAgreementCriterion * maxDose) )^2 ] ^ 1/2
  //   volumeDifferenceCriterion is the volume-difference criterion (% of the total structure volume, totalVolume)
  //   doseToAgreementCriterion is the dose-to-agreement criterion (% of the maximum dose, maxDose)
  // A value of gamma(i) < 1 indicates agreement for the DVH bin compareIndex

  unsigned int compareSize = compareDvhPlot->GetNumberOfTuples();
  if (compareIndex >= compareSize)
  {
    std::cerr << "Invalid bin index for compare plot! (" << compareIndex << ">=" << compareSize << ")" << std::endl;
    return -1.0;
  }

  double gamma = VTK_DOUBLE_MAX;

  // Get the dose and volume values from the current bin in the 'compare' double array.
  double *comparePlotTuple = compareDvhPlot->GetTuple(compareIndex);
  double di = comparePlotTuple[0];
  double vi = comparePlotTuple[1];

  for (int referenceIndex = 0; referenceIndex != referenceDvhPlot->GetNumberOfTuples(); ++referenceIndex)
  {
    // Get the dose and volume values from the current bin in the 'reference' double array.
    double *referencePlotTuple = referenceDvhPlot->GetTuple(referenceIndex);
    double dr = referencePlotTuple[0];
    double vr = referencePlotTuple[1];

    double currentGamma = sqrt(   pow( ( 100.0*(vr-vi) ) / ( this->VolumeDifferenceCriterion*totalVolumeCCs ),2)
                                + pow( ( 100.0*(dr-di) ) / ( this->DoseToAgreementCriterion*this->DoseMax   ),2) );

    if (currentGamma < gamma)
    {
      gamma = currentGamma;
    }
  }

  return gamma;
}
