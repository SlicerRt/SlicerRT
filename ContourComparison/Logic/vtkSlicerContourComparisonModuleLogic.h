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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkSlicerContourComparisonModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerContourComparisonModuleLogic_h
#define __vtkSlicerContourComparisonModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerContourComparisonModuleLogicExport.h"

class vtkMRMLContourComparisonNode;
class vtkSlicerContourComparisonModuleLogicPrivate;

/// \ingroup Slicer_QtModules_ContourComparison
class VTK_SLICER_CONTOURCOMPARISON_MODULE_LOGIC_EXPORT vtkSlicerContourComparisonModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerContourComparisonModuleLogic *New();
  vtkTypeMacro(vtkSlicerContourComparisonModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Compute Dice statistics from the selected input contour labelmaps
  void ComputeDiceStatistics(std::string &errorMessage);

  /// Compute Hausdorff distances from the selected input contour labelmaps
  void ComputeHausdorffDistances(std::string &errorMessage);

public:
  void SetAndObserveContourComparisonNode(vtkMRMLContourComparisonNode* node);
  vtkGetObjectMacro(ContourComparisonNode, vtkMRMLContourComparisonNode);

  vtkGetMacro(LogSpeedMeasurements, bool);
  vtkSetMacro(LogSpeedMeasurements, bool);
  vtkBooleanMacro(LogSpeedMeasurements, bool);

protected:
  /// Set private logic implementation
  void SetLogicPrivate(vtkSlicerContourComparisonModuleLogicPrivate* logicPrivate);

protected:
  vtkSlicerContourComparisonModuleLogic();
  virtual ~vtkSlicerContourComparisonModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerContourComparisonModuleLogic(const vtkSlicerContourComparisonModuleLogic&); // Not implemented
  void operator=(const vtkSlicerContourComparisonModuleLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLContourComparisonNode* ContourComparisonNode;

  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;

  /// Private implementation class for the logic
  vtkSlicerContourComparisonModuleLogicPrivate* LogicPrivate;
};

#endif
