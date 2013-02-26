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

// .NAME vtkMRMLPatientHierarchyNode - MRML node to represent patient hierarchy of DICOM objects
// .SECTION Description
// n/a
//

#ifndef __vtkMRMLPatientHierarchyNode_h
#define __vtkMRMLPatientHierarchyNode_h

#include "vtkMRMLHierarchyNode.h"

#include "vtkSlicerPatientHierarchyModuleMRMLExport.h"

/// \brief MRML node to represent the patient hierarchy of DICOM objects
///        (Patient / Study / Series)
class VTK_SLICER_PATIENTHIERARCHY_MODULE_MRML_EXPORT vtkMRMLPatientHierarchyNode : public vtkMRMLHierarchyNode
{
public:
  class VTK_SLICER_PATIENTHIERARCHY_MODULE_MRML_EXPORT HierarchyTag
  {
  public:
    // Define separators used at serialization
    static const std::string PATIENTHIERARCHY_HIERARCHYTAG_NAME_LEVEL_SEPARATOR;
    static const std::string PATIENTHIERARCHY_HIERARCHYTAG_ITEM_SEPARATOR;
    static const std::string PATIENTHIERARCHY_HIERARCHYTAG_INVALID_NAME;

  public:
    HierarchyTag();
    HierarchyTag(const char* tag, int level=-1);
    virtual ~HierarchyTag();
    HierarchyTag(const HierarchyTag& src);
    HierarchyTag &operator=(const HierarchyTag &src);

    /// Check if the hierarchy tag name is valid (must not contain double colon or double semicolon)
    static bool IsNameValid(const char* name);
    /// Replaces tag name with "Invalid" if invalid, leaves as is otherwise
    static const char* ValidateName(const char* name);

    /// Hierarchy tag name string
    const char* Name;

    /// Level of this tag. -1 if it doesn't represent a level (default)
    int Level;
  };

  // Default patient hierarchy level tags
  static const HierarchyTag PATIENTHIERARCHY_LEVEL_PATIENT;
  static const HierarchyTag PATIENTHIERARCHY_LEVEL_STUDY;
  static const HierarchyTag PATIENTHIERARCHY_LEVEL_SERIES;
  static const HierarchyTag PATIENTHIERARCHY_LEVEL_SUBSERIES;

public:
  static vtkMRMLPatientHierarchyNode *New();
  vtkTypeMacro(vtkMRMLPatientHierarchyNode,vtkMRMLHierarchyNode);
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
  /// Set UID
  vtkSetStringMacro(Uid);
  /// Get UID
  vtkGetStringMacro(Uid);

  /// Set DICOM database file name
  vtkSetStringMacro(DicomDatabaseFileName);
  /// Get DICOM database file name
  vtkGetStringMacro(DicomDatabaseFileName);

  /// Get the level attribute from the first tag that has a valid one (not -1)
  int GetLevel();
  /// Add tag by name and level
  void AddTag(const char* name, int level);
  /// Add tag by tag object
  void AddTag(HierarchyTag tag);
  /// Get tag
  HierarchyTag* GetTag(int index);

protected:
  /// The UID of the corresponding DICOM entity in the database
  char* Uid;

  /// Reference to the corresponding DICOM database
  /// The file name is used because the database pointer gets invalid
  /// (only one database object is present at a time in Slicer)
  char* DicomDatabaseFileName;

  /// List of tags for this hierarchy instance. One tag should always indicate
  /// the level of this item in the hierarchy tree
  std::vector<HierarchyTag> Tags;

protected:
  vtkMRMLPatientHierarchyNode();
  ~vtkMRMLPatientHierarchyNode();
  vtkMRMLPatientHierarchyNode(const vtkMRMLPatientHierarchyNode&);
  void operator=(const vtkMRMLPatientHierarchyNode&);

};

#endif
