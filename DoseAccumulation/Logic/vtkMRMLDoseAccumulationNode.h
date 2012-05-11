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

#ifndef __vtkMRMLDoseAccumulationNode_h
#define __vtkMRMLDoseAccumulationNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkStringArray.h>
#include <vtkDoubleArray.h>

#include "vtkSlicerDoseAccumulationModuleLogicExport.h"

class VTK_SLICER_DOSEACCUMULATION_MODULE_LOGIC_EXPORT vtkMRMLDoseAccumulationNode : public vtkMRMLNode
{
public:
  static vtkMRMLDoseAccumulationNode *New();
  vtkTypeMacro(vtkMRMLDoseAccumulationNode,vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "DoseAccumulation";};

  /// Enable/Disable show dose volumes only
  vtkBooleanMacro(ShowDoseVolumesOnly, bool);
  vtkGetMacro(ShowDoseVolumesOnly, bool);
  vtkSetMacro(ShowDoseVolumesOnly, bool);

  /// Get/Set selected input volumes MRML Ids
  vtkGetObjectMacro(SelectedInputVolumeIds, vtkStringArray);
  vtkSetObjectMacro(SelectedInputVolumeIds, vtkStringArray);

  /// Get/Set selected input volumes weights
  vtkGetObjectMacro(SelectedInputVolumeWeights, vtkDoubleArray);
  vtkSetObjectMacro(SelectedInputVolumeWeights, vtkDoubleArray);

  /// Get/Set output accumulated dose volume MRML Id 
  vtkGetStringMacro(AccumulatedDoseVolumeId);
  vtkSetStringMacro(AccumulatedDoseVolumeId);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
 
protected:
  vtkMRMLDoseAccumulationNode();
  ~vtkMRMLDoseAccumulationNode();
  vtkMRMLDoseAccumulationNode(const vtkMRMLDoseAccumulationNode&);
  void operator=(const vtkMRMLDoseAccumulationNode&);

protected:
  bool ShowDoseVolumesOnly;
  vtkStringArray* SelectedInputVolumeIds;
  vtkDoubleArray* SelectedInputVolumeWeights;
  char* AccumulatedDoseVolumeId;
};

#endif
