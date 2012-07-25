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

// STD includes
#include <set>
#include <map>

#include "vtkSlicerDoseAccumulationModuleLogicExport.h"

class VTK_SLICER_DOSEACCUMULATION_LOGIC_EXPORT vtkMRMLDoseAccumulationNode : public vtkMRMLNode
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

  /// Get selected input volumes MRML Ids
  std::set<std::string>* GetSelectedInputVolumeIds()
  {
    return &this->SelectedInputVolumeIds;
  }

  /// Get volumes node IDs to weights map
  std::map<std::string,double>* GetVolumeNodeIdsToWeightsMap()
  {
    return &this->VolumeNodeIdsToWeightsMap;
  }

  /// Get/Set output accumulated dose volume MRML Id 
  vtkGetStringMacro(AccumulatedDoseVolumeNodeId);
  vtkSetStringMacro(AccumulatedDoseVolumeNodeId);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
 
protected:
  vtkMRMLDoseAccumulationNode();
  ~vtkMRMLDoseAccumulationNode();
  vtkMRMLDoseAccumulationNode(const vtkMRMLDoseAccumulationNode&);
  void operator=(const vtkMRMLDoseAccumulationNode&);

protected:
  /// State of Show dose volumes only checkbox
  bool ShowDoseVolumesOnly;

  /// List of IDs of the selected input volume nodes
  std::set<std::string> SelectedInputVolumeIds;

  /// Map assigning a weight to the available input volume nodes
  /// (as the user set it on the module GUI)
  std::map<std::string,double> VolumeNodeIdsToWeightsMap;

  /// ID of the output accumulated dose volume node
  char* AccumulatedDoseVolumeNodeId;
};

#endif
