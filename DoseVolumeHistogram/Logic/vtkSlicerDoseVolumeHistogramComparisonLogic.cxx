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
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
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
double vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables(vtkMRMLTableNode* dvh1TableNode, vtkMRMLTableNode* dvh2TableNode,
                                                                     vtkMRMLScalarVolumeNode* doseVolumeNode, 
                                                                     double volumeDifferenceCriterion, double doseToAgreementCriterion, double doseMax/*=0.0*/ )
{
  if (!dvh1TableNode || !dvh2TableNode)
  {
    vtkGenericWarningMacro("vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables: Invalid input DVH nodes!");
    return 0.0;
  }

  vtkTable* dvh1Table = dvh1TableNode->GetTable();
  unsigned int dvh1Size = dvh1Table->GetNumberOfRows();

  vtkTable* dvh2Table = dvh2TableNode->GetTable();
  unsigned int dvh2Size = dvh2Table->GetNumberOfRows();

  vtkTable* baselineDoubleArray = NULL;
  unsigned int baselineSize = 0;

  vtkTable* currentDoubleArray = NULL;
  //unsigned int currentSize = 0; //This variable might be used for sanity checks later

  // Determine total volume from the attribute of the current double array node
  std::ostringstream attributeNameStream;
  attributeNameStream << vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC;
  const char* totalVolumeChar;

  // The vtkDoubleArray with the smallest number of tuples is the baseline
  if (dvh1Size < dvh2Size)
  {
    baselineDoubleArray = dvh1Table;
    baselineSize = dvh1Size;

    currentDoubleArray = dvh2Table;
    //currentSize = dvh2Size;
  
    totalVolumeChar = dvh2TableNode->GetAttribute(attributeNameStream.str().c_str());
  }
  else
  {
    baselineDoubleArray = dvh2Table;
    baselineSize = dvh2Size;

    currentDoubleArray = dvh1Table;
    //currentSize = dvh1Size;

    totalVolumeChar = dvh1TableNode->GetAttribute(attributeNameStream.str().c_str());
  }

  // Read the total volume from the current node attribute
  double totalVolumeCCs = 0;
  if (totalVolumeChar != NULL)
  {
    totalVolumeCCs = vtkVariant(totalVolumeChar).ToDouble();
  }
  
  if (totalVolumeCCs == 0)
  {
    vtkErrorWithObjectMacro(dvh1TableNode, "vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables: Invalid volume for structure!");
  }

  // Determine maximum dose
  if (doseVolumeNode)
  {
    vtkDebugWithObjectMacro(dvh1TableNode, "vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables: Calculating maximum dose from the given dose volume");
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
      vtkErrorWithObjectMacro(dvh1TableNode, "vtkSlicerDoseVolumeHistogramComparisonLogic::CompareDvhTables: Invalid agreement, skipped!");
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
double vtkSlicerDoseVolumeHistogramComparisonLogic::GetAgreementForDvhPlotPoint( vtkTable* referenceDvhPlot, vtkTable* compareDvhPlot,
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

  unsigned int compareSize = compareDvhPlot->GetNumberOfRows();
  if (compareIndex >= compareSize)
  {
    vtkGenericWarningMacro("Invalid bin index for compare plot! (" << compareIndex << ">=" << compareSize << ")");
    return -1.0;
  }

  double gamma = VTK_DOUBLE_MAX;

  // Get the dose and volume values from the current bin in the 'compare' double array.
  double di = compareDvhPlot->GetValue(compareIndex, 0).ToDouble();
  double vi = compareDvhPlot->GetValue(compareIndex, 1).ToDouble();

  for (int referenceIndex = 0; referenceIndex != referenceDvhPlot->GetNumberOfRows(); ++referenceIndex)
  {
    // Get the dose and volume values from the current bin in the 'reference' double array.
    double dr = referenceDvhPlot->GetValue(referenceIndex, 0).ToDouble();
    double vr = referenceDvhPlot->GetValue(referenceIndex, 1).ToDouble();

    double currentGamma = sqrt(   pow( ( 100.0*(vr-vi) ) / ( volumeDifferenceCriterion * totalVolumeCCs ),2)
                                + pow( ( 100.0*(dr-di) ) / ( doseToAgreementCriterion * doseMax ),2) );

    if (currentGamma < gamma)
    {
      gamma = currentGamma;     
    }
  }

  return gamma;
}
