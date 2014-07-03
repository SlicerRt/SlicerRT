/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kyle Sunderland and Csaba Pinter (PerkLab, Queen's University).

==============================================================================*/

#ifndef __vtkSlicerDoseVolumeHistogramComparisonLogic_h
#define __vtkSlicerDoseVolumeHistogramComparisonLogic_h

#include <vtkSlicerDoseVolumeHistogramModuleLogicExport.h>

// VTK includes
#include "vtkObject.h"
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLScalarVolumeNode.h>

class VTK_SLICER_DOSEVOLUMEHISTOGRAM_LOGIC_EXPORT  vtkSlicerDoseVolumeHistogramComparisonLogic : public vtkObject
{

public:
  static vtkSlicerDoseVolumeHistogramComparisonLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramComparisonLogic, vtkObject);

public:

  // Returns the percent of agreeing bins for two vtkMRMLDoubleArrayNodes
  //
  // In order to operate properly, the vtkMRMLDoubleArrayNodes need to be
  // set before use, along with the vtkMRMLScalarVolumeNode (or DoseMax)
  // and VolumeDifferenceCriterion and DoseToAgreementCriterion.
  double CompareDvhTables();

protected:

  // Formula is (based on the article Ebert2010):
  //   gamma(i) = min{ Gamma[(di, vi), (dr, vr)] } for all {r=1..P}, where
  //   compareIndexth Dvh point has dose di and volume vi
  //   P is the number of bins in the reference Dvh, each rth bin having absolute dose dr and volume vr
  //   Gamma[(di, vi), (dr, vr)] = [ ( (100*(vr-vi)) / (volumeDifferenceCriterion * totalVolume) )^2 + ( (100*(dr-di)) / (doseToAgreementCriterion * maxDose) )^2 ] ^ 1/2
  //   volumeDifferenceCriterion is the volume-difference criterion (% of the total structure volume, totalVolume)
  //   doseToAgreementCriterion is the dose-to-agreement criterion (% of the maximum dose, maxDose)
  // A return value of < 1 indicates agreement for the Dvh bin
  double GetAgreementForDvhPlotPoint(vtkDoubleArray *referenceDvhPlot, vtkDoubleArray *compareDvhPlot, unsigned int compareIndex, double totalVolume); 

public:

  vtkSetObjectMacro(Dvh1DoubleArrayNode, vtkMRMLDoubleArrayNode);
  vtkGetObjectMacro(Dvh1DoubleArrayNode, vtkMRMLDoubleArrayNode);

  vtkSetObjectMacro(Dvh2DoubleArrayNode, vtkMRMLDoubleArrayNode);
  vtkGetObjectMacro(Dvh2DoubleArrayNode, vtkMRMLDoubleArrayNode);
  
  vtkSetObjectMacro(DoseVolumeNode, vtkMRMLScalarVolumeNode);
  vtkGetObjectMacro(DoseVolumeNode, vtkMRMLScalarVolumeNode);

  vtkSetMacro(VolumeDifferenceCriterion, double);
  vtkGetMacro(VolumeDifferenceCriterion, double);

  vtkSetMacro(DoseToAgreementCriterion, double);
  vtkGetMacro(DoseToAgreementCriterion, double);

  vtkSetMacro(DoseMax, double);
  vtkGetMacro(DoseMax, double); 

protected:
  vtkMRMLDoubleArrayNode *Dvh1DoubleArrayNode;
  vtkMRMLDoubleArrayNode *Dvh2DoubleArrayNode;
  vtkMRMLScalarVolumeNode *DoseVolumeNode;
  double VolumeDifferenceCriterion;
  double DoseToAgreementCriterion;
  double DoseMax;

protected:
  vtkSlicerDoseVolumeHistogramComparisonLogic();
  virtual ~vtkSlicerDoseVolumeHistogramComparisonLogic();

};

#endif