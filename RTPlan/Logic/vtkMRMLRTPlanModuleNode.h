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

#ifndef __vtkMRMLRTPlanModuleNode_h
#define __vtkMRMLRTPlanModuleNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <vector>

#include "vtkSlicerRTPlanModuleLogicExport.h"

class VTK_SLICER_RTPLAN_MODULE_LOGIC_EXPORT vtkMRMLRTPlanModuleNode : public vtkMRMLNode
{
public:
  static vtkMRMLRTPlanModuleNode *New();
  vtkTypeMacro(vtkMRMLRTPlanModuleNode, vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "RTPlanModule";};

public:
  /// Get dose volume node ID
  vtkGetStringMacro(ReferenceVolumeNodeID);

  /// Set and observe dose volume node ID
  void SetAndObserveReferenceVolumeNodeID(const char* id);

  /// Get dose volume node ID
  vtkGetStringMacro(RTPlanNodeID);

  /// Set and observe dose volume node ID
  void SetAndObserveRTPlanNodeID(const char* id);

  /// Get dose volume node ID
  vtkGetStringMacro(ISOCenterNodeID);

  /// Set and observe dose volume node ID
  void SetAndObserveISOCenterNodeID(const char* id);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

protected:
  /// Set dose volume node ID
  vtkSetStringMacro(ReferenceVolumeNodeID);

  /// Set dose volume node ID
  vtkSetStringMacro(RTPlanNodeID);
 
  /// Set dose volume node ID
  vtkSetStringMacro(ISOCenterNodeID);
 
protected:
  vtkMRMLRTPlanModuleNode();
  ~vtkMRMLRTPlanModuleNode();
  vtkMRMLRTPlanModuleNode(const vtkMRMLRTPlanModuleNode&);
  void operator=(const vtkMRMLRTPlanModuleNode&);

  /// Selected dose volume MRML node object ID
  char* ReferenceVolumeNodeID;

  /// Selected dose volume MRML node object ID
  char* RTPlanNodeID;

  /// Selected dose volume MRML node object ID
  char* ISOCenterNodeID;

};

#endif
