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

#ifndef __vtkMRMLContourMorphologyNode_h
#define __vtkMRMLContourMorphologyNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerContourMorphologyModuleLogicExport.h"

class VTK_SLICER_CONTOURMORPHOLOGY_MODULE_LOGIC_EXPORT vtkMRMLContourMorphologyNode : public vtkMRMLNode
{
public:
  enum ContourMorphologyOperationType
  {
    None = -1,
    Expand,
    Shrink,
    Union,
    Intersect,
    Subtract
  };

public:
  static vtkMRMLContourMorphologyNode *New();
  vtkTypeMacro(vtkMRMLContourMorphologyNode, vtkMRMLNode);
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
  virtual const char* GetNodeTagName() { return "ContourMorphology"; };

public:
  /// Get contour A node ID
  vtkGetStringMacro(ContourANodeId);

  /// Set and observe contour A node ID
  void SetAndObserveContourANodeId(const char* id);

  /// Get dose volume node ID
  vtkGetStringMacro(ContourBNodeId);

  /// Set and observe contour B node ID
  void SetAndObserveContourBNodeId(const char* id);

  /// Get reference volume node ID
  vtkGetStringMacro(ReferenceVolumeNodeId);

  /// Set and observe reference volume node ID
  void SetAndObserveReferenceVolumeNodeId(const char* id);

  /// Get output contour node ID
  vtkGetStringMacro(OutputContourNodeId);

  /// Set and observe output contour node ID
  void SetAndObserveOutputContourNodeId(const char* id);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
 
  /// Get the operation type
  ContourMorphologyOperationType GetOperation() { return this->Operation; };
  void SetOperation(ContourMorphologyOperationType operation) { this->Operation = operation; };

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(XSize, double);
  vtkSetMacro(XSize, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(YSize, double);
  vtkSetMacro(YSize, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(ZSize, double);
  vtkSetMacro(ZSize, double);

protected:
  /// Set contour A node ID
  vtkSetStringMacro(ContourANodeId);

  /// Set contour B node ID
  vtkSetStringMacro(ContourBNodeId);

  /// Set reference volume node ID
  vtkSetStringMacro(ReferenceVolumeNodeId);

  /// Set output hierarchy node ID
  vtkSetStringMacro(OutputContourNodeId);

protected:
  vtkMRMLContourMorphologyNode();
  ~vtkMRMLContourMorphologyNode();
  vtkMRMLContourMorphologyNode(const vtkMRMLContourMorphologyNode&);
  void operator=(const vtkMRMLContourMorphologyNode&);

protected:
  /// Selected contour A MRML node object ID
  char* ContourANodeId;

  /// Selected contour B MRML node object ID
  char* ContourBNodeId;

  /// Selected reference volume MRML node object ID
  char* ReferenceVolumeNodeId;

  /// Selected output contour MRML node object ID
  char* OutputContourNodeId;

  /// TODO:
  ContourMorphologyOperationType Operation;

  /// TODO:
  double XSize;

  /// TODO:
  double YSize;

  /// TODO:
  double ZSize;
};

#endif
