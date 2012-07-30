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

#ifndef __vtkMRMLIsodoseNode_h
#define __vtkMRMLIsodoseNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <vector>

#include "vtkSlicerIsodoseModuleLogicExport.h"

typedef struct 
{
  std::string DoseLevelName;
  double DoseLevelValue;
} DoseLevelStruct ;

class VTK_SLICER_ISODOSE_LOGIC_EXPORT vtkMRMLIsodoseNode : public vtkMRMLNode
{
public:
  static vtkMRMLIsodoseNode *New();
  vtkTypeMacro(vtkMRMLIsodoseNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "Isodose";};

  /// Get volumes node IDs to weights map
  std::vector<DoseLevelStruct>* GetIsodoseLevelVector()
  {
    return &this->IsodoseLevelVector;
  }

  /// Get/Set dose volume node ID
  vtkGetStringMacro(DoseVolumeNodeId);
  vtkSetStringMacro(DoseVolumeNodeId);

  /// Get/Set output hierarchy node ID
  vtkGetStringMacro(OutputHierarchyNodeId);
  vtkSetStringMacro(OutputHierarchyNodeId);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
 
protected:
  vtkMRMLIsodoseNode();
  ~vtkMRMLIsodoseNode();
  vtkMRMLIsodoseNode(const vtkMRMLIsodoseNode&);
  void operator=(const vtkMRMLIsodoseNode&);

protected:
  /// Map assigning dose levels to the available input volume nodes
  /// (as the user set it on the module GUI)
  std::vector<DoseLevelStruct> IsodoseLevelVector;

  /// Selected dose volume MRML node object ID
  char* DoseVolumeNodeId;

  /// Selected dose volume MRML node object ID
  char* OutputHierarchyNodeId;
};

#endif
