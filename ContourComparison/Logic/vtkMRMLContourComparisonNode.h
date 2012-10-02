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

#ifndef __vtkMRMLContourComparisonNode_h
#define __vtkMRMLContourComparisonNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <vector>
#include <set>

#include "vtkSlicerContourComparisonModuleLogicExport.h"

class VTK_SLICER_CONTOURCOMPARISON_LOGIC_EXPORT vtkMRMLContourComparisonNode : public vtkMRMLNode
{
public:
  static vtkMRMLContourComparisonNode *New();
  vtkTypeMacro(vtkMRMLContourComparisonNode,vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "ContourComparison";};

public:
  /// Get reference contour labelmap volume node ID
  vtkGetStringMacro(ReferenceContourLabelmapVolumeNodeId);

  /// Set and observe reference contour labelmap volume node ID
  void SetAndObserveReferenceContourLabelmapVolumeNodeId(const char* id);

  /// Get compare contour labelmap volume node ID
  vtkGetStringMacro(CompareContourLabelmapVolumeNodeId);

  /// Set and observe reference contour labelmap volume node ID
  void SetAndObserveCompareContourLabelmapVolumeNodeId(const char* id);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

protected:
  /// Set reference contour labelmap volume node ID
  vtkSetStringMacro(ReferenceContourLabelmapVolumeNodeId);

  /// Set compare contour labelmap volume node ID
  vtkSetStringMacro(CompareContourLabelmapVolumeNodeId);

protected:
  vtkMRMLContourComparisonNode();
  ~vtkMRMLContourComparisonNode();
  vtkMRMLContourComparisonNode(const vtkMRMLContourComparisonNode&);
  void operator=(const vtkMRMLContourComparisonNode&);

protected:
  /// Selected reference contour labelmap volume MRML node object ID
  char* ReferenceContourLabelmapVolumeNodeId;

  /// Selected compare contour labelmap volume MRML node object ID
  char* CompareContourLabelmapVolumeNodeId;
};

#endif
