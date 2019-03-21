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

// .NAME vtkSlicerSegmentComparisonModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerSegmentComparisonModuleLogic_h
#define __vtkSlicerSegmentComparisonModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerSegmentComparisonModuleLogicExport.h"

class vtkMRMLSegmentComparisonNode;
class vtkSlicerSegmentComparisonModuleLogicPrivate;

/// \ingroup SlicerRt_QtModules_SegmentComparison
class VTK_SLICER_SEGMENTCOMPARISON_MODULE_LOGIC_EXPORT vtkSlicerSegmentComparisonModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerSegmentComparisonModuleLogic *New();
  vtkTypeMacro(vtkSlicerSegmentComparisonModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

public:
  /// Compute Dice statistics from the selected input segment labelmaps
  /// \return Error message, empty string if no error
  std::string ComputeDiceStatistics(vtkMRMLSegmentComparisonNode* parameterNode);

  /// Compute Hausdorff distances from the selected input segment labelmaps
  /// \return Error message, empty string if no error
  std::string ComputeHausdorffDistances(vtkMRMLSegmentComparisonNode* parameterNode);

public:
  vtkGetMacro(LogSpeedMeasurements, bool);
  vtkSetMacro(LogSpeedMeasurements, bool);
  vtkBooleanMacro(LogSpeedMeasurements, bool);

protected:
  /// Set private logic implementation
  void SetLogicPrivate(vtkSlicerSegmentComparisonModuleLogicPrivate* logicPrivate);

protected:
  vtkSlicerSegmentComparisonModuleLogic();
  ~vtkSlicerSegmentComparisonModuleLogic() override;

  void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;

  void UpdateFromMRMLScene() override;
  void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) override;
  void OnMRMLSceneEndClose() override;

private:
  vtkSlicerSegmentComparisonModuleLogic(const vtkSlicerSegmentComparisonModuleLogic&) = delete;
  void operator=(const vtkSlicerSegmentComparisonModuleLogic&) = delete;

protected:
  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;

  /// Private implementation class for the logic
  vtkSlicerSegmentComparisonModuleLogicPrivate* LogicPrivate;
};

#endif
