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

#ifndef __vtkMRMLSegmentEditorEffectNode_h
#define __vtkMRMLSegmentEditorEffectNode_h

// MRML includes
#include <vtkMRMLNode.h>

#include "vtkSlicerSegmentationsModuleMRMLExport.h"

class vtkMRMLScene;

/// \ingroup Segmentations
class VTK_SLICER_SEGMENTATIONS_MODULE_MRML_EXPORT vtkMRMLSegmentEditorEffectNode : public vtkMRMLNode
{
public:
  static vtkMRMLSegmentEditorEffectNode *New();
  vtkTypeMacro(vtkMRMLSegmentEditorEffectNode, vtkMRMLNode);
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
  virtual const char* GetNodeTagName() { return "SegmentEditorEffect"; };

public:
  /// Get segment editor effect name that this parameter set node belongs to
  vtkGetStringMacro(EffectName);
  /// Set segment editor effect name that this parameter set node belongs to
  vtkSetStringMacro(EffectName);

protected:
  vtkMRMLSegmentEditorEffectNode();
  ~vtkMRMLSegmentEditorEffectNode();
  vtkMRMLSegmentEditorEffectNode(const vtkMRMLSegmentEditorEffectNode&);
  void operator=(const vtkMRMLSegmentEditorEffectNode&);

  /// Segment editor effect name that this parameter set node belongs to
  char* EffectName;
};

#endif // __vtkMRMLSegmentEditorEffectNode_h
