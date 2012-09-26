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

#ifndef __vtkMRMLProtonDoseNode_h
#define __vtkMRMLProtonDoseNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <vector>

#include "vtkSlicerProtonDoseModuleLogicExport.h"

class VTK_SLICER_PROTONDOSE_LOGIC_EXPORT vtkMRMLProtonDoseNode : public vtkMRMLNode
{
public:
  static vtkMRMLProtonDoseNode *New();
  vtkTypeMacro(vtkMRMLProtonDoseNode, vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "ProtonDose";};

  /// Get dose volume node ID
  vtkGetStringMacro(DoseVolumeNodeId);

  /// Set and observe dose volume node ID
  void SetAndObserveDoseVolumeNodeId(const char* id);

  /// Get output hierarchy node ID
  vtkGetStringMacro(OutputHierarchyNodeId);

  /// Set and observe output hierarchy node ID
  void SetAndObserveOutputHierarchyNodeId(const char* id);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
 
public:
  /// Set dose volume node ID
  vtkSetStringMacro(DoseVolumeNodeId);

  /// Set output hierarchy node ID
  vtkSetStringMacro(OutputHierarchyNodeId);

  /// Get/Set macros for beam parameters
  vtkGetMacro(GantryAngle, double);
  vtkSetMacro(GantryAngle, double);

protected:
  vtkMRMLProtonDoseNode();
  ~vtkMRMLProtonDoseNode();
  vtkMRMLProtonDoseNode(const vtkMRMLProtonDoseNode&);
  void operator=(const vtkMRMLProtonDoseNode&);

protected:
  /// Greg is playing around
  float GantryAngle;
  float CollimatorAngle;

  /// Selected dose volume MRML node object ID
  char* DoseVolumeNodeId;

  /// Selected dose volume MRML node object ID
  char* OutputHierarchyNodeId;
};

#endif
