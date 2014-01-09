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

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSubjectHierarchyConstants.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLDisplayableNode.h"
#include "vtkMRMLDisplayNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
const std::string vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR = std::string(":");
const std::string vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR = std::string("; ");

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSubjectHierarchyNode);

//----------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::vtkMRMLSubjectHierarchyNode()
  : Level(NULL)
  , OwnerPluginName(NULL)
  , OwnerPluginAutoSearch(true)
{
  this->SetLevel("Other");

  this->UIDs.clear();
}

//----------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::~vtkMRMLSubjectHierarchyNode()
{
  this->UIDs.clear();
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " Level=\""
    << (this->Level ? this->Level : "NULL" ) << "\n";

  os << indent << " OwnerPluginName=\""
    << (this->OwnerPluginName ? this->OwnerPluginName : "NULL" ) << "\n";

  os << indent << " OwnerPluginAutoSearch=\""
    << (this->OwnerPluginAutoSearch ? "true" : "false") << "\n";

  os << indent << " UIDs=\"";
  for (std::map<std::string, std::string>::iterator uidsIt = this->UIDs.begin(); uidsIt != this->UIDs.end(); ++uidsIt)
    {
      os << uidsIt->first << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR.c_str()
        << uidsIt->second << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR.c_str();
    }
  os << "\"";
}

//----------------------------------------------------------------------------
const char* vtkMRMLSubjectHierarchyNode::GetNodeTagName()
{
  return "SubjectHierarchy";
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::ReadXMLAttributes( const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "Level")) 
      {
      this->SetLevel(attValue);
      }
    else if (!strcmp(attName, "OwnerPluginName")) 
      {
      this->SetOwnerPluginName(attValue);
      }
    else if (!strcmp(attName, "OwnerPluginAutoSearch")) 
    {
      this->OwnerPluginAutoSearch = 
        (strcmp(attValue,"true") ? false : true);
    }
    else if (!strcmp(attName, "UIDs"))
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();

      this->UIDs.clear();
      size_t itemSeparatorPosition = valueStr.find(vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR);
      while (itemSeparatorPosition != std::string::npos)
        {
        std::string itemStr = valueStr.substr(0, itemSeparatorPosition);
        size_t nameValueSeparatorPosition = itemStr.find(vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR);

        std::string name = itemStr.substr(0, nameValueSeparatorPosition);
        std::string value = itemStr.substr(nameValueSeparatorPosition + vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR.size());
        this->AddUID(name, value);

        valueStr = valueStr.substr(itemSeparatorPosition + vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR.size());
        itemSeparatorPosition = valueStr.find(vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR);
        }
      if (! valueStr.empty() )
        {
        std::string itemStr = valueStr.substr(0, itemSeparatorPosition);
        size_t tagLevelSeparatorPosition = itemStr.find(vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR);

        std::string name = itemStr.substr(0, tagLevelSeparatorPosition);
        std::string value = itemStr.substr(tagLevelSeparatorPosition + vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR.size());
        this->AddUID(name, value);
        }
      }
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);

  vtkIndent indent(nIndent);

  of << indent << " Level=\""
    << (this->Level ? this->Level : "NULL" ) << "\"";

  of << indent << " OwnerPluginName=\""
    << (this->OwnerPluginName ? this->OwnerPluginName : "NULL" ) << "\"";

  of << indent << " OwnerPluginAutoSearch=\""
    << (this->OwnerPluginAutoSearch ? "true" : "false") << "\"";

  of << indent << " UIDs=\"";
  for (std::map<std::string, std::string>::iterator uidsIt = this->UIDs.begin(); uidsIt != this->UIDs.end(); ++uidsIt)
    {
    of << uidsIt->first << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR
      << uidsIt->second << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR;
    }
  of << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLSubjectHierarchyNode *node = (vtkMRMLSubjectHierarchyNode*) anode;

  this->SetLevel(node->Level);
  this->SetOwnerPluginName(node->OwnerPluginName);
  this->SetOwnerPluginAutoSearch(node->GetOwnerPluginAutoSearch());

  this->UIDs = node->GetUIDs();

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::AddUID(const char* uidName, const char* uidValue)
{
  std::string uidNameStr(uidName);
  std::string uidValueStr(uidValue);
  this->AddUID(uidNameStr, uidValueStr);
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::AddUID(std::string uidName, std::string uidValue)
{
  // Use the find function to prevent adding an empty UID to the list
  if (this->UIDs.find(uidName) != this->UIDs.end())
  {
    // Log warning if the new UID value is different than the one already set
    if (this->UIDs[uidName].compare(uidValue))
    {
      vtkWarningMacro( "AddUID: UID with name '" << uidName
        << "' already exists in subject hierarchy node '"
        << (this->Name ? this->Name : "Unnamed") << "' with value '"
        << this->UIDs[uidName] << "'. Replacing it with value '"
        << uidValue << "'!" );
    }
    else
    {
      return; // Do nothing if the UID values match
    }
  }
  this->UIDs[uidName] = uidValue;
  this->Modified();
}

//----------------------------------------------------------------------------
std::string vtkMRMLSubjectHierarchyNode::GetUID(std::string uidName)
{
  // Use the find function to prevent adding an empty UID to the list
  if (this->UIDs.find(uidName) == this->UIDs.end())
  {
    return std::string();
  }
  return this->UIDs[uidName];
}

//----------------------------------------------------------------------------
std::map<std::string, std::string> vtkMRMLSubjectHierarchyNode::GetUIDs()
{
  return this->UIDs;
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(vtkMRMLScene* scene, const char* uidName, const char* uidValue)
{
  if (!scene || !uidName || !uidValue)
  {
    std::cerr << "vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID: Invalid scene or searched UID!" << std::endl;
    return NULL;
  }

  std::vector<vtkMRMLNode*> subjectHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLSubjectHierarchyNode", subjectHierarchyNodes);
  for (unsigned int shNodeIndex=0; shNodeIndex<numberOfNodes; shNodeIndex++)
  {
    vtkMRMLSubjectHierarchyNode* node = vtkMRMLSubjectHierarchyNode::SafeDownCast(subjectHierarchyNodes[shNodeIndex]);
    if (node)
    {
      std::string nodeUidValueStr = node->GetUID(uidName);
      const char* nodeUidValue = nodeUidValueStr.c_str();
      if (nodeUidValue && !strcmp(uidValue, nodeUidValue))
      {
        return node;
      }
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSubjectHierarchyNode::GetAssociatedDataNode()
{
  vtkMRMLNode* firstAssociatedNode = this->GetAssociatedNode();
  if (!firstAssociatedNode)
  {
    return NULL;
  }
  else if (firstAssociatedNode->IsA("vtkMRMLSubjectHierarchyNode"))
  {
    vtkErrorMacro("GetAssociatedDataNode: Subject hierarchy node '" << this->Name << "' is associated to another subject hierarchy node! This is not permitted.");
    return NULL;
  }
  else if (firstAssociatedNode->IsA("vtkMRMLHierarchyNode"))
  {
    vtkMRMLHierarchyNode* firstAssociatedHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(firstAssociatedNode);
    vtkMRMLNode* secondAssociatedNode = firstAssociatedHierarchyNode->GetAssociatedNode();
    if (secondAssociatedNode->IsA("vtkMRMLHierarchyNode"))
    {
      vtkErrorMacro("GetAssociatedDataNode: Subject hierarchy node '" << this->Name << "' has douuble-nested association! This is not permitted.");
      return NULL;
    }
    else
    {
      return secondAssociatedNode;
    }
  }
  else
  {
    return firstAssociatedNode;
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::GetAssociatedChildrenNodes(vtkCollection *children, const char* childClass)
{
  if (children == NULL)
  {
    vtkErrorMacro("GetAssociatedChildrenNodes: Argument collection must be created before calling the method");
    return;
  }
  if (this->Scene == NULL)
  {
    vtkErrorMacro("GetAssociatedChildrenNodes: Unable to find children for node " << this->Name << ", because it has no MRML scene set");
    return;
  }

  std::string nodeClass("vtkMRMLNode");
  if (childClass)
  {
    nodeClass = childClass;
  }

  int numNodes = this->Scene->GetNumberOfNodesByClass(nodeClass.c_str());
  for (int n=0; n < numNodes; n++) 
  {
    vtkMRMLNode* currentNode = this->Scene->GetNthNodeByClass(n, nodeClass.c_str());

    // Check for a hierarchy node for this node
    vtkMRMLHierarchyNode* hierarchyNode = this->GetAssociatedHierarchyNode(this->Scene, currentNode->GetID());

    // See if there is a nested association (only check here because nesting is only allowed for leaves)
    if (hierarchyNode)
    {
      vtkMRMLHierarchyNode* secondHierarchyNode = this->GetAssociatedHierarchyNode(this->Scene, hierarchyNode->GetID());
      if (secondHierarchyNode)
      {
        hierarchyNode = secondHierarchyNode;
      }
    }

    while (hierarchyNode)
    {
      if (hierarchyNode == this) 
      {
        children->AddItem(currentNode);
        break;
      }

      // The hierarchy node for this node may not be the one we're checking against, go up the tree
      hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(hierarchyNode->GetParentNode());
    }
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::SetDisplayVisibilityForBranch(int visible)
{
  if (visible != 0 && visible != 1)
  {
    vtkDebugMacro("SetDisplayVisibilityForBranch: Invalid visibility value to set: " << visible);
    return;
  }
  if (this->Scene->IsBatchProcessing())
  {
    vtkDebugMacro("SetDisplayVisibilityForBranch: Batch processing is on, returning");
    return;
  }

  vtkSmartPointer<vtkCollection> childDisplayableNodes = vtkSmartPointer<vtkCollection>::New();
  this->GetAssociatedChildrenNodes(childDisplayableNodes, "vtkMRMLDisplayableNode");
  childDisplayableNodes->InitTraversal();
  std::set<vtkMRMLSubjectHierarchyNode*> parentNodes;
  for (int childNodeIndex=0; childNodeIndex<childDisplayableNodes->GetNumberOfItems(); ++childNodeIndex)
  {
    vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(childDisplayableNodes->GetItemAsObject(childNodeIndex));
    if (displayableNode)
    {
      displayableNode->SetDisplayVisibility(visible);

      vtkMRMLDisplayNode* displayNode = displayableNode->GetDisplayNode();
      if (displayNode)
      {
        displayNode->SetSliceIntersectionVisibility(visible);
      }

      displayableNode->Modified();
      this->Modified();

      // Collect all parents
      vtkMRMLSubjectHierarchyNode* parentNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode( displayableNode );
      do 
      {
        parentNodes.insert(parentNode);
      }
      while ( (parentNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(parentNode->GetParentNode()) ) != NULL); // The double parentheses avoids a Linux build warning
    }
  }

  // Set Modified flag for all parent nodes so that their icons are refreshed in the tree view
  for (std::set<vtkMRMLSubjectHierarchyNode*>::iterator parentsIt = parentNodes.begin(); parentsIt != parentNodes.end(); ++ parentsIt)
  {
    (*parentsIt)->Modified();
  }
}

//---------------------------------------------------------------------------
int vtkMRMLSubjectHierarchyNode::GetDisplayVisibilityForBranch()
{
  int visible = -1;
  vtkSmartPointer<vtkCollection> childDisplayableNodes = vtkSmartPointer<vtkCollection>::New();
  this->GetAssociatedChildrenNodes(childDisplayableNodes, "vtkMRMLDisplayableNode");
  childDisplayableNodes->InitTraversal();

  for (int childNodeIndex=0; childNodeIndex<childDisplayableNodes->GetNumberOfItems(); ++childNodeIndex)
  {
    vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(
      childDisplayableNodes->GetItemAsObject(childNodeIndex) );
    // Omit volume nodes from the process (they are displayed differently than every other type)
    if (displayableNode && !displayableNode->IsA("vtkMRMLScalarVolumeNode"))
    {
      // If we set visibility
      if (visible == -1)
      {
        visible = displayableNode->GetDisplayVisibility();

        // We expect only 0 or 1 from leaf nodes
        if (visible == 2)
        {
          vtkWarningMacro("GetDisplayVisibilityForBranch: Unexpected visibility value for node " << displayableNode->GetName());
        }
      }
      // If the current node visibility does not match the found visibility, then set partial visibility
      else if (displayableNode->GetDisplayVisibility() != visible)
      {
        return 2;
      }
    }
  }

  return visible;
}

//---------------------------------------------------------------------------
bool vtkMRMLSubjectHierarchyNode::IsLevel(const char* level)
{
  if (!level)
  {
    vtkErrorMacro("IsLevel: Invalid input argument!");
    return false;
  }

  if (this->Level && !strcmp(this->Level, level))
  {
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(vtkMRMLNode* associatedNode, vtkMRMLScene* scene/*=NULL*/)
{
  if (!associatedNode)
  {
    std::cerr << "vtkSlicerSubjectHierarchyModuleLogic::GetAssociatedSubjectHierarchyNode: associated node is null" << std::endl;
    return NULL;
  }
  if (!scene)
  {
    scene = associatedNode->GetScene();
    if (!scene)
    {
      vtkErrorWithObjectMacro(associatedNode, "GetAssociatedSubjectHierarchyNode: No MRML scene available (not given as argument, and the associated node has no scene)!");
      return NULL;
    }
  }

  if (associatedNode->IsA("vtkMRMLSubjectHierarchyNode"))
  {
    return vtkMRMLSubjectHierarchyNode::SafeDownCast(associatedNode);
  }

  vtkSmartPointer<vtkCollection> hierarchyNodes = vtkSmartPointer<vtkCollection>::Take( scene->GetNodesByClass("vtkMRMLSubjectHierarchyNode") );
  vtkObject* nextObject = NULL;
  for (hierarchyNodes->InitTraversal(); (nextObject = hierarchyNodes->GetNextItemAsObject()); )
  {
    vtkMRMLSubjectHierarchyNode* hierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nextObject);
    if (hierarchyNode)
    {
      vtkMRMLNode* currentAssociatedNode = hierarchyNode->GetAssociatedDataNode();
      if ( currentAssociatedNode && !strcmp(currentAssociatedNode->GetID(), associatedNode->GetID()) )
      {
        return hierarchyNode;
      }
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
const char* vtkMRMLSubjectHierarchyNode::GetAttributeFromAncestor(const char* attributeName, const char* level/*=NULL*/)
{
  if (!attributeName)
  {
    vtkErrorMacro("GetAttributeFromAncestor: Empty attribute name!");
    return NULL;
  }

  const char* attributeValue = NULL;
  vtkMRMLSubjectHierarchyNode* hierarchyNode = this;
  while (hierarchyNode && hierarchyNode->GetParentNodeID())
  {
    hierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(hierarchyNode->GetParentNode());
    if (!hierarchyNode)
    {
      break;
    }
    else if (level && !hierarchyNode->IsLevel(level))
    {
      continue;
    }

    attributeValue = hierarchyNode->GetAttribute(attributeName);
    if (attributeValue)
    {
      return attributeValue;
    }
  }

  return attributeValue;
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* vtkMRMLSubjectHierarchyNode::GetAncestorAtLevel(const char* level)
{
  if (!level)
  {
    vtkErrorMacro("GetAttributeFromAncestor: Empty subject hierarchy level!");
    return NULL;
  }

  vtkMRMLSubjectHierarchyNode* hierarchyNode = this;
  while (hierarchyNode && hierarchyNode->GetParentNodeID()) // We do not return source node even if it is at the requested level, we only look in the ancestors
  {
    hierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(hierarchyNode->GetParentNode());
    if (hierarchyNode && hierarchyNode->IsLevel(level))
    {
      // Level found
      return hierarchyNode;
    }
  }

  vtkWarningMacro("GetAttributeFromAncestor: No ancestor found for node '" << this->Name << "' at level '" << level << "'!");
  return NULL;
}

//---------------------------------------------------------------------------
std::string vtkMRMLSubjectHierarchyNode::GetNameWithoutPostfix()
{
  std::string nameStr(this->Name);
  size_t postfixStart = nameStr.find(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX);
  std::string strippedNameStr = nameStr.substr(0, postfixStart);
  return strippedNameStr;
}
