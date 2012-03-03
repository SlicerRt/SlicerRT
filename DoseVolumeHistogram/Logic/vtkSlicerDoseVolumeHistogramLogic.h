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

==============================================================================*/

// .NAME vtkSlicerDoseVolumeHistogramLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDoseVolumeHistogramLogic_h
#define __vtkSlicerDoseVolumeHistogramLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// VTK includes
#include "vtkImageAccumulate.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLChartNode;
class vtkMRMLScalarVolumeNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DOSEVOLUMEHISTOGRAM_MODULE_LOGIC_EXPORT vtkSlicerDoseVolumeHistogramLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerDoseVolumeHistogramLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Compute statistics for the selected structure set based on the selected dose volume
  void ComputeStatistics(std::vector<std::string> &names, std::vector<double> &counts, std::vector<double> &meanDoses, std::vector<double> &totalVolumeCCs, std::vector<double> &maxDoses, std::vector<double> &minDoses);

  /// Add dose volume histograms for the selected structure set and dose volume to the selected chart
  void AddDvhToSelectedChart();

public:
  void SetDoseVolumeNode( vtkMRMLVolumeNode* );
  void SetStructureSetModelNode( vtkMRMLModelNode* );
  void SetChartNode( vtkMRMLChartNode* );

  void ResetLabelValue() { this->CurrentLabelValue = 2; };

  vtkGetObjectMacro( DoseVolumeNode, vtkMRMLVolumeNode );
  vtkGetObjectMacro( StructureSetModelNode, vtkMRMLModelNode );
  vtkGetObjectMacro( ChartNode, vtkMRMLChartNode );

protected:
  vtkSlicerDoseVolumeHistogramLogic();
  virtual ~vtkSlicerDoseVolumeHistogramLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// Get node of labelmap corresponding to the selected structure set model. If it does not exist, create one
  virtual void GetLabelmapVolumeNodeForSelectedStructureSet(vtkMRMLScalarVolumeNode* structureSetLabelmapVolumeNode);

private:
  vtkSlicerDoseVolumeHistogramLogic(const vtkSlicerDoseVolumeHistogramLogic&); // Not implemented
  void operator=(const vtkSlicerDoseVolumeHistogramLogic&);               // Not implemented

protected:
  vtkMRMLVolumeNode* DoseVolumeNode;
  vtkMRMLModelNode* StructureSetModelNode;
  vtkMRMLChartNode* ChartNode;

  unsigned int CurrentLabelValue;
};

#endif
