/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

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

#ifndef __vtkMRMLSegmentMorphologyNode_h
#define __vtkMRMLSegmentMorphologyNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerSegmentMorphologyModuleLogicExport.h"

class vtkMRMLSegmentationNode;

/// \ingroup SlicerRt_QtModules_SegmentMorphology
class VTK_SLICER_SEGMENTMORPHOLOGY_MODULE_LOGIC_EXPORT vtkMRMLSegmentMorphologyNode : public vtkMRMLNode
{
public:
  enum SegmentMorphologyOperationType
  {
    None = -1,
    Expand,
    Shrink,
    Union,
    Intersect,
    Subtract
  };

public:
  static vtkMRMLSegmentMorphologyNode *New();
  vtkTypeMacro(vtkMRMLSegmentMorphologyNode, vtkMRMLNode);
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
  virtual const char* GetNodeTagName() { return "SegmentMorphology"; };

public:
  /// Get segmentation A node
  vtkMRMLSegmentationNode* GetSegmentationANode();
  /// Set segmentation A node
  void SetAndObserveSegmentationANode(vtkMRMLSegmentationNode* node);

  /// Get segmentation B node
  vtkMRMLSegmentationNode* GetSegmentationBNode();
  /// Set segmentation B node
  void SetAndObserveSegmentationBNode(vtkMRMLSegmentationNode* node);

  /// Get output segmentation node
  vtkMRMLSegmentationNode* GetOutputSegmentationNode();
  /// Set output segmentation node
  void SetAndObserveOutputSegmentationNode(vtkMRMLSegmentationNode* node);
 
  /// Get segment A ID
  vtkGetStringMacro(SegmentAID);
  /// Set segment A ID
  vtkSetStringMacro(SegmentAID);

  /// Get segment B ID
  vtkGetStringMacro(SegmentBID);
  /// Set segment B ID
  vtkSetStringMacro(SegmentBID);

  /// Get the operation type
  void SetOperation(int operation);
  vtkGetMacro(Operation, int);

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
  vtkMRMLSegmentMorphologyNode();
  ~vtkMRMLSegmentMorphologyNode();
  vtkMRMLSegmentMorphologyNode(const vtkMRMLSegmentMorphologyNode&);
  void operator=(const vtkMRMLSegmentMorphologyNode&);

protected:
  /// Segment A ID in segmentation A node
  char* SegmentAID;

  /// Segment B ID in segmentation B node
  char* SegmentBID;

  /// Selected segment morphology operation
  int Operation;

  /// Dimension parameter for the X axis (for Expand or Shrink)
  double XSize;

  /// Dimension parameter for the Y axis (for Expand or Shrink)
  double YSize;

  /// Dimension parameter for the Z axis (for Expand or Shrink)
  double ZSize;
};

#endif
