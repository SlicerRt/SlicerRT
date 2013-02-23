// MRML includes
#include "vtkMRMLContourHierarchyNode.h"
#include "vtkMRMLContourNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourHierarchyNode);

//----------------------------------------------------------------------------
vtkMRMLContourHierarchyNode::vtkMRMLContourHierarchyNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLContourHierarchyNode::~vtkMRMLContourHierarchyNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLContourHierarchyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
const char* vtkMRMLContourHierarchyNode::GetNodeTagName()
{
  return "ContourHierarchy";
}
//----------------------------------------------------------------------------
void vtkMRMLContourHierarchyNode::ReadXMLAttributes( const char** atts)
{
  Superclass::ReadXMLAttributes(atts);
}

//----------------------------------------------------------------------------
void vtkMRMLContourHierarchyNode::WriteXML(ostream& of, int indent)
{
  Superclass::WriteXML(of,indent);
}

//---------------------------------------------------------------------------
void vtkMRMLContourHierarchyNode::GetChildrenContourNodes(vtkCollection *contours)
{
  vtkMRMLScene *scene = this->GetScene();
  if (!contours || !scene)
    {
    return;
    }

  vtkMRMLNode *mnode = NULL;
  vtkMRMLContourHierarchyNode *hnode = NULL;
  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
    {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLContourNode"))
      {
      hnode = vtkMRMLContourHierarchyNode::SafeDownCast(
        vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, mnode->GetID()));

      while (hnode)
        {
        if (hnode == this) 
          {
          contours->AddItem(mnode);
          break;
          }
        hnode = vtkMRMLContourHierarchyNode::SafeDownCast(hnode->GetParentNode());
        } // end while
      } // end if
    } // end for
}
