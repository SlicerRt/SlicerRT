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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerIsodoseModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerIsodoseModuleLogic_h
#define __vtkSlicerIsodoseModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerIsodoseModuleLogicExport.h"

// MRML includes
class vtkMRMLIsodoseNode;
class vtkMRMLColorTableNode;

/// \ingroup Slicer_QtModules_Isodose
class VTK_SLICER_ISODOSE_LOGIC_EXPORT vtkSlicerIsodoseModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerIsodoseModuleLogic *New();
  vtkTypeMacro(vtkSlicerIsodoseModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Return a default color node id for a label map
  const char * GetDefaultLabelMapColorTableNodeId();

  /// Set number of isodose levels
  void SetNumberOfIsodoseLevels(int number);

  /// Accumulates dose volumes with the given IDs and corresponding weights
  int ComputeIsodose();

  /// Return false if the dose volume contains a volume that is really a dose volume
  bool DoseVolumeContainsDose();

  ///
  void SetAndObserveIsodoseNode(vtkMRMLIsodoseNode* node);

  ///
  vtkGetObjectMacro(IsodoseNode, vtkMRMLIsodoseNode);

protected:
  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

  void AddDefaultIsodoseColorNode();

  vtkMRMLColorTableNode* CreateIsodoseColorNode();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();
protected:
  vtkSlicerIsodoseModuleLogic();
  virtual ~vtkSlicerIsodoseModuleLogic();

private:

  vtkSlicerIsodoseModuleLogic(const vtkSlicerIsodoseModuleLogic&); // Not implemented
  void operator=(const vtkSlicerIsodoseModuleLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLIsodoseNode* IsodoseNode;
};

#endif

