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

// .NAME vtkSlicerRTPlanModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerRTPlanModuleLogic_h
#define __vtkSlicerRTPlanModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerRTPlanModuleLogicExport.h"

class vtkMRMLRTPlanNode;
class vtkMRMLRTBeamNode;
class vtkMRMLRTPlanModuleNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_RTPLAN_MODULE_LOGIC_EXPORT vtkSlicerRTPlanModuleLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerRTPlanModuleLogic *New();
  vtkTypeMacro(vtkSlicerRTPlanModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  ///
  void SetAndObserveRTPlanModuleNode(vtkMRMLRTPlanModuleNode* node);

  ///
  vtkGetObjectMacro(RTPlanModuleNode, vtkMRMLRTPlanModuleNode);

  ///
  void AddBeam();

  ///
  void RemoveBeam(char*);

  ///
  void UpdateBeam(char*, double);

  ///
  void CreateBeamPolyData();

protected:
  vtkSlicerRTPlanModuleLogic();
  virtual ~vtkSlicerRTPlanModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

  /// Parameter set MRML node
  vtkMRMLRTPlanModuleNode* RTPlanModuleNode;

private:

  vtkSlicerRTPlanModuleLogic(const vtkSlicerRTPlanModuleLogic&); // Not implemented
  void operator=(const vtkSlicerRTPlanModuleLogic&);               // Not implemented
};

#endif
