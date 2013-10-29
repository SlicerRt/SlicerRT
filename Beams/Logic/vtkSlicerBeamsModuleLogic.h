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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerBeamsModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerBeamsModuleLogic_h
#define __vtkSlicerBeamsModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include "vtkSlicerBeamsModuleLogicExport.h"

class vtkTransform;
class vtkMRMLBeamsNode;

/// \ingroup Slicer_QtModules_Beams
class VTK_SLICER_BEAMS_LOGIC_EXPORT vtkSlicerBeamsModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerBeamsModuleLogic *New();
  vtkTypeMacro(vtkSlicerBeamsModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Compute and set source fiducial position from isocenter position and beam parameters
  void ComputeSourceFiducialPosition(std::string &errorMessage, vtkTransform* sourceToIsocenterTransform=NULL);

  /// Create beam geometry model from isocenter and source fiducials
  /// \param errorMessage Error message if any. If empty, then there have been no errors in course of the computation
  void CreateBeamModel(std::string &errorMessage);

public:
  /// Set and observe dose accumulation parameter node 
  void SetAndObserveBeamsNode(vtkMRMLBeamsNode* node);

  /// Get dose accumulation parameter node 
  vtkGetObjectMacro(BeamsNode, vtkMRMLBeamsNode);

protected:
  vtkSlicerBeamsModuleLogic();
  virtual ~vtkSlicerBeamsModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerBeamsModuleLogic(const vtkSlicerBeamsModuleLogic&); // Not implemented
  void operator=(const vtkSlicerBeamsModuleLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLBeamsNode* BeamsNode;
};

#endif

