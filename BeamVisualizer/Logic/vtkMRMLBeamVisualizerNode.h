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

#ifndef __vtkMRMLBeamVisualizerNode_h
#define __vtkMRMLBeamVisualizerNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerBeamVisualizerModuleLogicExport.h"

class VTK_SLICER_BEAMVISUALIZER_LOGIC_EXPORT vtkMRMLBeamVisualizerNode : public vtkMRMLNode
{
public:
  static vtkMRMLBeamVisualizerNode *New();
  vtkTypeMacro(vtkMRMLBeamVisualizerNode,vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "BeamVisualizer";};

  /// Get isocenter fiducial MRML Id 
  vtkGetStringMacro(IsocenterFiducialNodeId);

  /// Get source fiducial MRML Id 
  vtkGetStringMacro(SourceFiducialNodeId);

  /// Set and observe isocenter fiducial MRML Id 
  void SetAndObserveIsocenterFiducialNodeId(const char* id);

  /// Set and observe souce fiducial MRML Id 
  void SetAndObserveSourceFiducialNodeId(const char* id);

  /// Get output beam model MRML Id 
  vtkGetStringMacro(BeamModelNodeId);

  /// Set and observe output beam model MRML Id 
  void SetAndObserveBeamModelNodeId(const char* id);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

protected:
  /// Set isocenter fiducial MRML Id 
  vtkSetStringMacro(IsocenterFiducialNodeId);

  /// Set source fiducial MRML Id 
  vtkSetStringMacro(SourceFiducialNodeId);

  /// Set output beam model MRML Id 
  vtkSetStringMacro(BeamModelNodeId);

protected:
  vtkMRMLBeamVisualizerNode();
  ~vtkMRMLBeamVisualizerNode();
  vtkMRMLBeamVisualizerNode(const vtkMRMLBeamVisualizerNode&);
  void operator=(const vtkMRMLBeamVisualizerNode&);

protected:
  /// ID of the input isocenter fiducial node
  char* IsocenterFiducialNodeId;

  /// ID of the input source fiducial node
  char* SourceFiducialNodeId;

  /// ID of the output beam model node
  char* BeamModelNodeId;
};

#endif
