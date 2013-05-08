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

// .NAME vtkSlicerPatientHierarchyModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerPatientHierarchyModuleLogic_h
#define __vtkSlicerPatientHierarchyModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerPatientHierarchyModuleLogicExport.h"

class vtkMRMLHierarchyNode;
class vtkStringArray;

/// \ingroup Slicer_QtModules_PatientHierarchy
class VTK_SLICER_PATIENTHIERARCHY_LOGIC_EXPORT vtkSlicerPatientHierarchyModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerPatientHierarchyModuleLogic *New();
  vtkTypeMacro(vtkSlicerPatientHierarchyModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Default patient hierarchy tags
  static const char* PATIENTHIERARCHY_LEVEL_PATIENT;
  static const char* PATIENTHIERARCHY_LEVEL_STUDY;
  static const char* PATIENTHIERARCHY_LEVEL_SERIES;
  static const char* PATIENTHIERARCHY_LEVEL_SUBSERIES;

public:
  /// Find patient hierarchy node according to a UID and database
  static vtkMRMLHierarchyNode* GetPatientHierarchyNodeByUID(vtkMRMLScene* scene, const char* uid);

  /// Place series in patient hierarchy. Create patient and study node if needed
  static void InsertDicomSeriesInHierarchy(
    vtkMRMLScene* scene, const char* patientId, const char* studyInstanceUID, const char* seriesInstanceUID );

  /// Determine if two patient hierarchy nodes are in the same branch (share the same parent)
  /// \param nodeId1 ID of the first node to check. Can be patient hierarchy node or a node
  ///   associated with one
  /// \param nodeId2 ID of the second node to check
  /// \param lowestCommonLevel Lowest level on which they have to share an ancestor
  /// \return True if the two nodes or their associated hierarchy nodes share a parent on the
  ///   specified level, false otherwise
  static bool AreNodesInSameBranch( vtkMRMLScene* scene,
    const char* nodeId1, const char* nodeId2, const char* lowestCommonLevel=NULL );

  /// Set patient hierarchy branch visibility
  static void SetBranchVisibility(vtkMRMLHierarchyNode* node, int visible);

  /// Get patient hierarchy branch visibility
  /// \return Visibility value (0:Hidden, 1:Visible, 2:PartiallyVisible)
  static int GetBranchVisibility(vtkMRMLHierarchyNode* node);

  /// Find all ancestor hierarchy nodes of the input node and set Modified flag on them
  static void SetModifiedToAllAncestors(vtkMRMLNode* node);

  /// Determine if a node is of a certain DICOM level
  /// \param node Node to examine
  /// \param level DICOM level to check (should be one of the logic constants)
  /// \return True if the node is of the specified level, false otherwise
  static bool IsDicomLevel(vtkMRMLNode* node, const char* level);

  /// Get associated patient hierarchy node for a MRML node
  /// \param scene MRML scene to search in
  /// \param associatedNodeId ID of the node for which we want the associated hierarchy node
  /// \param reverseCriterion If set to true, the function returns non patient hierarchy node.
  ///        Function \sa GetAssociatedNonPatientHierarchyNode should be used instead
  /// \return The first hierarchy node found that fulfills the conditions
  static vtkMRMLHierarchyNode* GetAssociatedPatientHierarchyNode(vtkMRMLScene *scene, const char *associatedNodeId, bool reverseCriterion=false);

  /// Utility function to get a non patient hierarchy node associated with a MRML node
  static vtkMRMLHierarchyNode* GetAssociatedNonPatientHierarchyNode(vtkMRMLScene *scene, const char *associatedNodeId);

  /// Assemble tooltip for node according to the patient hierarchy tree
  static std::string GetTooltipForNode(vtkMRMLNode* node);

protected:
  vtkSlicerPatientHierarchyModuleLogic();
  virtual ~vtkSlicerPatientHierarchyModuleLogic();

private:
  vtkSlicerPatientHierarchyModuleLogic(const vtkSlicerPatientHierarchyModuleLogic&); // Not implemented
  void operator=(const vtkSlicerPatientHierarchyModuleLogic&);               // Not implemented
};

#endif
