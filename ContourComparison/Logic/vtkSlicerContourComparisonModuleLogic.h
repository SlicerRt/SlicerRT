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

// ITK includes
#include "itkImage.h"

// STD includes
#include <cstdlib>

#include "vtkSlicerContourComparisonModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLContourComparisonNode;

/// \ingroup Slicer_QtModules_ContourComparison
class VTK_SLICER_CONTOURCOMPARISON_LOGIC_EXPORT vtkSlicerContourComparisonModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerContourComparisonModuleLogic *New();
  vtkTypeMacro(vtkSlicerContourComparisonModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Compute gamma metric according to the selected input volumes and parameters (ContourComparison parameter set node content)
  void ComputeGammaDoseDifference();

  /// Return false if the argument volume contains a volume that is really a dose volume
  bool DoseVolumeContainsDose(vtkMRMLNode* node);

public:
  void SetAndObserveContourComparisonNode(vtkMRMLContourComparisonNode* node);
  vtkGetObjectMacro(ContourComparisonNode, vtkMRMLContourComparisonNode);

  vtkGetMacro(LogSpeedMeasurements, bool);
  vtkSetMacro(LogSpeedMeasurements, bool);
  vtkBooleanMacro(LogSpeedMeasurements, bool);

protected:
  vtkSlicerContourComparisonModuleLogic();
  virtual ~vtkSlicerContourComparisonModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

protected:
//BTX  
  /// Convert VTK image to ITK image
  void ConvertVolumeNodeToItkImage(vtkMRMLVolumeNode* inVolumeNode, itk::Image<float, 3>::Pointer outItkVolume);
//ETX

private:
  vtkSlicerContourComparisonModuleLogic(const vtkSlicerContourComparisonModuleLogic&); // Not implemented
  void operator=(const vtkSlicerContourComparisonModuleLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLContourComparisonNode* ContourComparisonNode;

  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;
};


#endif

