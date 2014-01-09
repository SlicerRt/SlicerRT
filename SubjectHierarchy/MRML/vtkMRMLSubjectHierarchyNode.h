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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkMRMLSubjectHierarchyNode - MRML node to represent subject hierarchy of DICOM objects
// .SECTION Description
// N/A
//

#ifndef __vtkMRMLSubjectHierarchyNode_h
#define __vtkMRMLSubjectHierarchyNode_h

#include "vtkSlicerSubjectHierarchyModuleMRMLExport.h"

// MRML includes
#include <vtkMRMLHierarchyNode.h>

// STD includes
#include <map>

/// \ingroup Slicer_QtModules_SubjectHierarchy
/// \brief MRML node to represent a subject hierarchy object
///   Separate node type has the advantage of identifying itself faster (type vs string comparison)
///   and providing utility functions within the class
class VTK_SLICER_SUBJECTHIERARCHY_MODULE_MRML_EXPORT vtkMRMLSubjectHierarchyNode : public vtkMRMLHierarchyNode
{
public:
  // Separator characters for (de)serializing the UID map
  static const std::string SUBJECTHIERARCHY_UID_ITEM_SEPARATOR;
  static const std::string SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR;

public:
  static vtkMRMLSubjectHierarchyNode *New();
  vtkTypeMacro(vtkMRMLSubjectHierarchyNode,vtkMRMLHierarchyNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Read node attributes from XML file
  virtual void ReadXMLAttributes(const char** atts);

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// Get node XML tag name (like Volume, Contour)
  virtual const char* GetNodeTagName();

public:
  /// Find subject hierarchy node according to a UID
  static vtkMRMLSubjectHierarchyNode* GetSubjectHierarchyNodeByUID(vtkMRMLScene* scene, const char* uidName, const char* uidValue);

  /// Get associated subject hierarchy node for a MRML node
  /// \param associatedNode The node for which we want the associated hierarchy node
  /// \param scene MRML scene pointer (in case the associated node is not in the scene any more). If not specified, then the scene of the argument node is used.
  /// \return If associatedNode is a subject hierarchy node, then return that. Otherwise the first hierarchy node found in the scene that fulfills the conditions.
  static vtkMRMLSubjectHierarchyNode* GetAssociatedSubjectHierarchyNode(vtkMRMLNode *associatedNode, vtkMRMLScene* scene=NULL);

public:
  //TODO: Make this an override of vtkMRMLHierarchyNode::GetAssociatedNode. For that it has to be virtual.
  /// Get node associated with this hierarchy node.
  /// This must be used instead of vtkMRMLHierarchyNode::GetAssociatedNode, because there are nested associations to avoid conflicts.
  /// E.g. a data node is associated to both a ModelHierarchy and a SubjectHierarchy node. In that case the first associated hierarchy
  /// node is returned by the utility function, which is a non-deterministic behavior. To avoid this we use nested associations. In the
  /// example case the associations are as follows: SubjectHierarchy -> ModelHierarchy -> DataNode
  vtkMRMLNode* GetAssociatedDataNode();

  //TODO: Make this an override of vtkMRMLHierarchyNode::GetAssociatedChildrendNodes. For that it has to be virtual. It has a typo in its name to be fixed too.
  /// Find all associated children nodes of a specified class in the hierarchy.
  /// Re-implemented to handle nested associations \sa GetAssociatedDataNode
  /// \param childClass Name of the class we are looking for. NULL returns all associated children nodes.
  void GetAssociatedChildrenNodes(vtkCollection *children, const char* childClass=NULL);

  /// Set subject hierarchy branch visibility
  void SetDisplayVisibilityForBranch(int visible);

  /// Get subject hierarchy branch visibility
  /// \return Visibility value (0:Hidden, 1:Visible, 2:PartiallyVisible)
  int GetDisplayVisibilityForBranch();

  /// Determine if a node is of a certain level
  /// \param level Level to check
  /// \return True if the node is of the specified level, false otherwise
  bool IsLevel(const char* level);

  /// Get attribute value for a node from an upper level in the subject hierarchy
  /// \attributeName Name of the requested attribute
  /// \level Level of the ancestor node we look for the attribute in (e.g. SubjectHierarchy_LEVEL_STUDY). If NULL, then look all the way up to the subject
  /// \return Attribute value from the lowest level ancestor where the attribute can be found
  const char* GetAttributeFromAncestor(const char* attributeName, const char* level=NULL);

  /// Get ancestor subject hierarchy node at a certain level
  /// \param sourceNode Node where we start searching. Can be subject hierarchy or associated node
  vtkMRMLSubjectHierarchyNode* GetAncestorAtLevel(const char* level);

  /// Get node name without the subject hierarchy postfix
  std::string GetNameWithoutPostfix();

public:
  /// Set level
  vtkSetStringMacro(Level);
  /// Get level
  vtkGetStringMacro(Level);

  /// Set owner plugin name
  vtkSetStringMacro(OwnerPluginName);
  /// Get owner plugin name
  vtkGetStringMacro(OwnerPluginName);

  /// Set plugin auto search flag
  vtkBooleanMacro(OwnerPluginAutoSearch, bool);
  /// Set plugin auto search flag
  vtkSetMacro(OwnerPluginAutoSearch, bool);
  /// Get plugin auto search flag
  vtkGetMacro(OwnerPluginAutoSearch, bool);

  /// Add UID to the subject hierarchy node
  void AddUID(const char* uidName, const char* uidValue);
  /// Add UID to the subject hierarchy node
  void AddUID(std::string uidName, std::string uidValue);

  /// Get a UID with a known name
  /// \return The UID value if exists, empty string if does not
  std::string GetUID(std::string uidName);

  /// Get UID map for this subject hierarchy node
  std::map<std::string, std::string> GetUIDs();

protected:
  /// Level identifier (default levels are Subject and Study)
  char* Level;

  /// Name of the owner plugin that claimed this node
  char* OwnerPluginName;

  /// Flag indicating whether a plugin automatic search needs to be performed when the node is modified
  /// By default it is true. It is usually only set to false when the user has manually overridden the
  /// automatic choice. In that case the manual selection is not automatically overridden.
  bool OwnerPluginAutoSearch;

  /// List of UIDs of this subject hierarchy node
  /// UIDs can be DICOM UIDs, MIDAS urls, etc.
  std::map<std::string, std::string> UIDs;

protected:
  vtkMRMLSubjectHierarchyNode();
  ~vtkMRMLSubjectHierarchyNode();
  vtkMRMLSubjectHierarchyNode(const vtkMRMLSubjectHierarchyNode&);
  void operator=(const vtkMRMLSubjectHierarchyNode&);
};

#endif
