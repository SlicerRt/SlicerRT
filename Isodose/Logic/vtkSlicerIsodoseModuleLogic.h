/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerIsodoseModuleLogic - slicer logic class for isodose creation
// .SECTION Description
// This class manages the logic associated with creating isodose lines and
// surfaces from dose distributions


#ifndef __vtkSlicerIsodoseModuleLogic_h
#define __vtkSlicerIsodoseModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerIsodoseModuleLogicExport.h"

// MRML includes
class vtkMRMLIsodoseNode;
class vtkMRMLModelHierarchyNode;
class vtkMRMLColorTableNode;

/// \ingroup SlicerRt_QtModules_Isodose
class VTK_SLICER_ISODOSE_LOGIC_EXPORT vtkSlicerIsodoseModuleLogic : public vtkSlicerModuleLogic
{
public:
  // Isodose constants
  static const char* DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME;
  static const std::string ISODOSE_MODEL_NODE_NAME_PREFIX;
  static const std::string ISODOSE_PARAMETER_SET_BASE_NAME_PREFIX;
  static const std::string ISODOSE_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string ISODOSE_ROOT_SUBJECT_HIERARCHY_NODE_NAME_POSTFIX;

public:
  static vtkSlicerIsodoseModuleLogic *New();
  vtkTypeMacro(vtkSlicerIsodoseModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Set number of isodose levels
  void SetNumberOfIsodoseLevels(vtkMRMLIsodoseNode* parameterNode, int newNumberOfColors);

  /// Accumulates dose volumes with the given IDs and corresponding weights
  void CreateIsodoseSurfaces(vtkMRMLIsodoseNode* parameterNode);

  /// Get dose volume node
  vtkMRMLModelHierarchyNode* GetRootModelHierarchyNode(vtkMRMLIsodoseNode* parameterNode);

public:
  /// Creates default isodose color table. Gets and returns if already exists
  static vtkMRMLColorTableNode* CreateDefaultIsodoseColorTable(vtkMRMLScene* scene);

  /// Creates default dose color table (which is the default isodose color table stretched)
  static vtkMRMLColorTableNode* CreateDefaultDoseColorTable(vtkMRMLScene *scene);

protected:
  /// Loads default isodose color table from the supplied color table file
  void LoadDefaultIsodoseColorTable();

protected:
  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndClose();

protected:
  vtkSlicerIsodoseModuleLogic();
  virtual ~vtkSlicerIsodoseModuleLogic();

private:
  vtkSlicerIsodoseModuleLogic(const vtkSlicerIsodoseModuleLogic&); // Not implemented
  void operator=(const vtkSlicerIsodoseModuleLogic&);               // Not implemented
};

#endif

