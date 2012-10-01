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

#ifndef __vtkMRMLIsodoseNode_h
#define __vtkMRMLIsodoseNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <vector>

#include "vtkSlicerIsodoseModuleLogicExport.h"

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

public:
  /// Get dose volume node ID
  vtkGetStringMacro(DoseVolumeNodeId);

  /// Set and observe dose volume node ID
  void SetAndObserveDoseVolumeNodeId(const char* id);

  /// Get output hierarchy node ID
  vtkGetStringMacro(OutputHierarchyNodeId);

  /// Set and observe output hierarchy node ID
  void SetAndObserveOutputHierarchyNodeId(const char* id);

  /// Get color node ID
  vtkGetStringMacro(ColorTableNodeId);

  /// Set and observe chart node ID
  void SetAndObserveColorTableNodeId(const char* id);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
 
  /// Get/Set show Gy for D metrics checkbox state
  vtkGetMacro(ShowIsodoseLines, bool);
  vtkSetMacro(ShowIsodoseLines, bool);
  vtkBooleanMacro(ShowIsodoseLines, bool);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(ShowIsodoseSurfaces, bool);
  vtkSetMacro(ShowIsodoseSurfaces, bool);
  vtkBooleanMacro(ShowIsodoseSurfaces, bool);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(ShowScalarBar, bool);
  vtkSetMacro(ShowScalarBar, bool);
  vtkBooleanMacro(ShowScalarBar, bool);

protected:
  /// Set dose volume node ID
  vtkSetStringMacro(DoseVolumeNodeId);

  /// Set output hierarchy node ID
  vtkSetStringMacro(OutputHierarchyNodeId);

  /// Set color node ID
  vtkSetStringMacro(ColorTableNodeId);

protected:
  vtkMRMLIsodoseNode();
  ~vtkMRMLIsodoseNode();
  vtkMRMLIsodoseNode(const vtkMRMLIsodoseNode&);
  void operator=(const vtkMRMLIsodoseNode&);

protected:
  /// Selected dose volume MRML node object ID
  char* DoseVolumeNodeId;

  /// Selected dose volume MRML node object ID
  char* OutputHierarchyNodeId;

  /// Selected chart MRML node object ID
  char* ColorTableNodeId;

  /// State of Show isodose lines checkbox
  bool ShowIsodoseLines;

  /// State of Show isodose surface checkbox
  bool ShowIsodoseSurfaces;

  /// State of Show scalarbar checkbox
  bool ShowScalarBar;
};

#endif
