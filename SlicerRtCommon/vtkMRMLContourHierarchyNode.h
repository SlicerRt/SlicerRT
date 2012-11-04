// .NAME vtkMRMLContourHierarchyNode - MRML node to represent hierarchy of Contours.
// .SECTION Description
// n/a
//

#ifndef __vtkMRMLContourHierarchyNode_h
#define __vtkMRMLContourHierarchyNode_h

#include "vtkMRMLDisplayableHierarchyNode.h"

/// \ingroup Slicer_QtModules_Contour
class vtkMRMLContourHierarchyNode : public vtkMRMLDisplayableHierarchyNode
{
public:
  static vtkMRMLContourHierarchyNode *New();
  vtkTypeMacro(vtkMRMLContourHierarchyNode,vtkMRMLDisplayableHierarchyNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// Get node XML tag name (like Volume, Contour)
  virtual const char* GetNodeTagName();

  /// Find all child model nodes in the hierarchy
  virtual void GetChildrenContourNodes(vtkCollection *contours);

protected:
  vtkMRMLContourHierarchyNode();
  ~vtkMRMLContourHierarchyNode();
  vtkMRMLContourHierarchyNode(const vtkMRMLContourHierarchyNode&);
  void operator=(const vtkMRMLContourHierarchyNode&);

};

#endif
