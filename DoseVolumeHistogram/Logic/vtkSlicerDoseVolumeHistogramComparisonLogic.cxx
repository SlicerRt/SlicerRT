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
#include "vtkMRMLDoseVolumeHistogramNode.h"

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
}

//-----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramComparisonLogic::~vtkSlicerDoseVolumeHistogramComparisonLogic()
{
}

//-----------------------------------------------------------------------------
double vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables(vtkMRMLDoubleArrayNode* dvh1DoubleArrayNode, vtkMRMLDoubleArrayNode* dvh2DoubleArrayNode,
                                                                     vtkMRMLScalarVolumeNode* doseVolumeNode, 
                                                                     double volumeDifferenceCriterion, double doseToAgreementCriterion, double doseMax/*=0.0*/ )
{
  if (!dvh1DoubleArrayNode || !dvh2DoubleArrayNode)
  {
    vtkGenericWarningMacro("vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables: Invalid input DVH nodes!");
    return 0.0;
  }

  vtkDoubleArray *dvh1Array = dvh1DoubleArrayNode->GetArray();
  unsigned int dvh1Size = dvh1Array->GetNumberOfTuples();

  vtkDoubleArray *dvh2Array = dvh2DoubleArrayNode->GetArray();
  unsigned int dvh2Size = dvh2Array->GetNumberOfTuples();

  vtkDoubleArray *baselineDoubleArray = NULL;
  unsigned int baselineSize = 0;

  vtkDoubleArray *currentDoubleArray = NULL;
  //unsigned int currentSize = 0; //This variable might be used for sanity checks later

  // Determine total volume from the attribute of the current double array node
  std::ostringstream attributeNameStream;
  attributeNameStream << vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC;
  const char* totalVolumeChar;

  // The vtkDoubleArray with the smallest number of tuples is the baseline
  if (dvh1Size < dvh2Size)
  {
    baselineDoubleArray = dvh1Array;
    baselineSize = dvh1Size;

    currentDoubleArray = dvh2Array;
    //currentSize = dvh2Size;
  
    totalVolumeChar = dvh2DoubleArrayNode->GetAttribute(attributeNameStream.str().c_str());
  }
  else
  {
    baselineDoubleArray = dvh2Array;
    baselineSize = dvh2Size;

    currentDoubleArray = dvh1Array;
    //currentSize = dvh1Size;

    totalVolumeChar = dvh1DoubleArrayNode->GetAttribute(attributeNameStream.str().c_str());
  }

  // Read the total volume from the current node attribute
  double totalVolumeCCs = 0;
  if (totalVolumeChar != NULL)
  {
    totalVolumeCCs = vtkVariant(totalVolumeChar).ToDouble();
  }
  
  if (totalVolumeCCs == 0)
  {
    vtkErrorWithObjectMacro(dvh1DoubleArrayNode, "vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables: Invalid volume for structure!");
  }

  // Determine maximum dose
  if (doseVolumeNode)
  {
    vtkDebugWithObjectMacro(dvh1DoubleArrayNode, "vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables: Calculating maximum dose from the given dose volume");
    vtkNew<vtkImageAccumulate> doseStat;
    doseStat->SetInputData(doseVolumeNode->GetImageData());
    doseStat->Update();
    doseMax = doseStat->GetMax()[0];
  }

  // Compare the current DVH to the baseline and determine mean and maximum difference
  double agreementAcceptancePercentage = 0.0;
  int numberOfAcceptedAgreements = 0;

  for (unsigned int baselineIndex=0; baselineIndex < baselineSize; ++baselineIndex)
  {
    // Compute the agreement for the current baseline bin
    double agreement = vtkSlicerDoseVolumeHistogramComparisonLogic::GetAgreementForDvhPlotPoint(
      currentDoubleArray, baselineDoubleArray, baselineIndex, totalVolumeCCs, doseMax, volumeDifferenceCriterion, doseToAgreementCriterion );

    if (agreement == -1.0)
    {
      vtkErrorWithObjectMacro(dvh1DoubleArrayNode, "vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables: Invalid agreement, skipped!");
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
double vtkSlicerDoseVolumeHistogramComparisonLogic::GetAgreementForDvhPlotPoint( vtkDoubleArray *referenceDvhPlot, vtkDoubleArray *compareDvhPlot,
                                                                                 unsigned int compareIndex, double totalVolumeCCs, double doseMax,
                                                                                 double volumeDifferenceCriterion, double doseToAgreementCriterion )
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
    vtkGenericWarningMacro("Invalid bin index for compare plot! (" << compareIndex << ">=" << compareSize << ")");
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

    double currentGamma = sqrt(   pow( ( 100.0*(vr-vi) ) / ( volumeDifferenceCriterion * totalVolumeCCs ),2)
                                + pow( ( 100.0*(dr-di) ) / ( doseToAgreementCriterion * doseMax ),2) );

    if (currentGamma < gamma)
    {
      gamma = currentGamma;
    }
  }

  return gamma;
}
