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

// .NAME vtkSlicerDoseComparisonModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDoseComparisonModuleLogic_h
#define __vtkSlicerDoseComparisonModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerDoseComparisonModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLDoseComparisonNode;

/// \ingroup Slicer_QtModules_DoseComparison
class VTK_SLICER_DOSECOMPARISON_LOGIC_EXPORT vtkSlicerDoseComparisonModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDoseComparisonModuleLogic *New();
  vtkTypeMacro(vtkSlicerDoseComparisonModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Compute gamma metric according to the selected input volumes and parameters (DoseComparison parameter set node content)
  void ComputeGammaDoseDifference();

protected:
  /// Creates default gamma color table. Should never be called, except when updating the default gamma color table file!
  void CreateDefaultGammaColorTable();

  /// Loads default gamma color table file specified in the user settings
  void LoadDefaultGammaColorTable();

public:
  void SetAndObserveDoseComparisonNode(vtkMRMLDoseComparisonNode* node);
  vtkGetObjectMacro(DoseComparisonNode, vtkMRMLDoseComparisonNode);

  vtkGetMacro(LogSpeedMeasurements, bool);
  vtkSetMacro(LogSpeedMeasurements, bool);
  vtkBooleanMacro(LogSpeedMeasurements, bool);

  vtkSetStringMacro(GammaColorTableNodeId);
  vtkGetStringMacro(GammaColorTableNodeId);

protected:
  vtkSlicerDoseComparisonModuleLogic();
  virtual ~vtkSlicerDoseComparisonModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerDoseComparisonModuleLogic(const vtkSlicerDoseComparisonModuleLogic&); // Not implemented
  void operator=(const vtkSlicerDoseComparisonModuleLogic&);               // Not implemented
protected:
  /// Parameter set MRML node
  vtkMRMLDoseComparisonNode* DoseComparisonNode;

  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;

  /// Gamma color table ID. Loaded on Slicer startup.
  char* GammaColorTableNodeId;
};

#endif
