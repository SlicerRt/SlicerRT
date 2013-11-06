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

#ifndef __vtkMRMLBeamsNode_h
#define __vtkMRMLBeamsNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerBeamsModuleLogicExport.h"

class vtkMRMLMarkupsFiducialNode;
class vtkMRMLModelNode;

class VTK_SLICER_BEAMS_LOGIC_EXPORT vtkMRMLBeamsNode : public vtkMRMLNode
{
public:
  static vtkMRMLBeamsNode *New();
  vtkTypeMacro(vtkMRMLBeamsNode,vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "Beams";};

public:
  /// Get isocenter markups MRML node.
  /// The source fiducial is set as the second fiducial in this markups node.
  vtkMRMLMarkupsFiducialNode* GetIsocenterMarkupsNode();
  /// Set isocenter markups MRML node.
  void SetAndObserveIsocenterMarkupsNode(vtkMRMLMarkupsFiducialNode* node);

  /// Get output beam model MRML node
  vtkMRMLModelNode* GetBeamModelNode();
  /// Set output beam model MRML node ID
  void SetAndObserveBeamModelNode(vtkMRMLModelNode* node);

  /// Set beams model opacity
  vtkSetMacro(BeamModelOpacity, double);
  /// Get beams model opacity
  vtkGetMacro(BeamModelOpacity, double);

protected:
  vtkMRMLBeamsNode();
  ~vtkMRMLBeamsNode();
  vtkMRMLBeamsNode(const vtkMRMLBeamsNode&);
  void operator=(const vtkMRMLBeamsNode&);

protected:
  /// Opacity of the created beam model
  double BeamModelOpacity;
};

#endif
