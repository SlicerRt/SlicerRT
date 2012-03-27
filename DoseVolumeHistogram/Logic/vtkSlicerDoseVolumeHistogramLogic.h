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
class vtkMRMLChartViewNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DOSEVOLUMEHISTOGRAM_MODULE_LOGIC_EXPORT vtkSlicerDoseVolumeHistogramLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerDoseVolumeHistogramLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Compute DVH and return statistics for the selected structure set based on the selected dose volume
  void ComputeDvh(std::vector<std::string> &names, std::vector<std::string> &dvhArrayIDs, std::vector<double> &counts, std::vector<double> &meanDoses, std::vector<double> &totalVolumeCCs, std::vector<double> &maxDoses, std::vector<double> &minDoses);

  /// Compute DVH for the selected structure set based on the selected dose volume
  void ComputeDvh();

  /// Add dose volume histogram of a structure set to the selected chart given its name and the corresponding DVH double array node ID
  void AddDvhToSelectedChart(const char* structureSetName, const char* dvhArrayId);

  /// Remove dose volume histogram of a structure from the selected chart
  void RemoveDvhFromSelectedChart(const char* dvhArrayId);

public:
  void SetDoseVolumeNode( vtkMRMLVolumeNode* );
  void SetStructureSetModelNode( vtkMRMLNode* );
  void SetChartNode( vtkMRMLChartNode* );

  void ResetLabelValue() { this->CurrentLabelValue = 2; };

  vtkGetObjectMacro( DoseVolumeNode, vtkMRMLVolumeNode );
  vtkGetObjectMacro( StructureSetModelNode, vtkMRMLNode );
  vtkGetObjectMacro( ChartNode, vtkMRMLChartNode );

protected:
  vtkSlicerDoseVolumeHistogramLogic();
  virtual ~vtkSlicerDoseVolumeHistogramLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Compute DVH and return statistics for the given volume (which is the selected dose volume stenciled with a structure set) with the given structure set name
  void ComputeDvh(vtkMRMLScalarVolumeNode* structureSetStenciledDoseVolumeNode, char* structureSetName, std::vector<std::string> &names, std::vector<std::string> &dvhArrayIDs, std::vector<double> &counts, std::vector<double> &meanDoses, std::vector<double> &totalVolumeCCs, std::vector<double> &maxDoses, std::vector<double> &minDoses);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// Get stenciled dose volume for a structure set
  virtual void GetStenciledDoseVolumeForStructureSet(vtkMRMLScalarVolumeNode* structureSetStenciledDoseVolumeNode, vtkMRMLModelNode* structureSetModel);

  /// Return the chart view node object from the layout
  vtkMRMLChartViewNode* GetChartViewNode();

private:
  vtkSlicerDoseVolumeHistogramLogic(const vtkSlicerDoseVolumeHistogramLogic&); // Not implemented
  void operator=(const vtkSlicerDoseVolumeHistogramLogic&);               // Not implemented

protected:
  vtkMRMLVolumeNode* DoseVolumeNode;
  vtkMRMLNode* StructureSetModelNode;
  vtkMRMLChartNode* ChartNode;

  unsigned int CurrentLabelValue;
};

#endif
