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

// STD includes
#include <vector>

#include "vtkSlicerContourMorphologyModuleLogicExport.h"

class VTK_SLICER_CONTOURMORPHOLOGY_MODULE_LOGIC_EXPORT vtkMRMLContourMorphologyNode : public vtkMRMLNode
{
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
  virtual const char* GetNodeTagName() {return "ContourMorphology";};

public:
  /// Get dose volume node ID
  vtkGetStringMacro(ContourNodeID);

  /// Set and observe dose volume node ID
  void SetAndObserveContourNodeID(const char* id);

  /// Get output hierarchy node ID
  vtkGetStringMacro(OutputContourNodeID);

  /// Set and observe output hierarchy node ID
  void SetAndObserveOutputContourNodeID(const char* id);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
 
  /// Get/Set show Gy for D metrics checkbox state
  vtkGetMacro(Expansion, bool);
  vtkSetMacro(Expansion, bool);
  vtkBooleanMacro(Expansion, bool);

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
  /// Set dose volume node ID
  vtkSetStringMacro(ContourNodeID);

  /// Set output hierarchy node ID
  vtkSetStringMacro(OutputContourNodeID);

protected:
  vtkMRMLContourMorphologyNode();
  ~vtkMRMLContourMorphologyNode();
  vtkMRMLContourMorphologyNode(const vtkMRMLContourMorphologyNode&);
  void operator=(const vtkMRMLContourMorphologyNode&);

protected:
  /// Selected dose volume MRML node object ID
  char* ContourNodeID;

  /// Selected dose volume MRML node object ID
  char* OutputContourNodeID;

  /// State of Show isodose lines checkbox
  bool Expansion;

  /// State of Show isodose surface checkbox
  double XSize;

  /// State of Show scalarbar checkbox
  double YSize;

  /// State of Show scalarbar checkbox
  double ZSize;
};

#endif
