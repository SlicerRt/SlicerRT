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

// MRML includes
#include "vtkMRMLPatientHierarchyNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

// Define separators used at serialization
const std::string vtkMRMLPatientHierarchyNode::HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_NAME_LEVEL_SEPARATOR = "::";
const std::string vtkMRMLPatientHierarchyNode::HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_ITEM_SEPARATOR = ";;";
const std::string vtkMRMLPatientHierarchyNode::HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_INVALID_NAME = "Invalid";

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode::HierarchyTag::HierarchyTag()
: Name("EmptyTagName")
, Level(-1)
{
}

vtkMRMLPatientHierarchyNode::HierarchyTag::HierarchyTag(const char* name, int level/*=-1*/)
: Level(level)
{
  this->Name = this->ValidateName(name);
}

vtkMRMLPatientHierarchyNode::HierarchyTag::~HierarchyTag()
{
}

vtkMRMLPatientHierarchyNode::HierarchyTag::HierarchyTag(const HierarchyTag& src)
{
  Name = this->ValidateName(src.Name);
  Level = src.Level;
}

vtkMRMLPatientHierarchyNode::HierarchyTag& vtkMRMLPatientHierarchyNode::HierarchyTag::operator=(const HierarchyTag &src)
{
  Name = this->ValidateName(src.Name);
  Level = src.Level;
  return (*this);
}

bool vtkMRMLPatientHierarchyNode::HierarchyTag::IsNameValid(const char* name)
{
  std::string nameStr(name);
  if (nameStr.find(PATIENTHIERARCHY_HIERARCHYTAG_NAME_LEVEL_SEPARATOR) != std::string::npos)
    {
    return false;
    }
  if (nameStr.find(PATIENTHIERARCHY_HIERARCHYTAG_ITEM_SEPARATOR) != std::string::npos)
    {
    return false;
    }
  return true;
}

const char* vtkMRMLPatientHierarchyNode::HierarchyTag::ValidateName(const char* name)
{
  if (vtkMRMLPatientHierarchyNode::HierarchyTag::IsNameValid(name))
    {
    return name;
    }
  else
    {
    return PATIENTHIERARCHY_HIERARCHYTAG_INVALID_NAME.c_str();
    }
}

//----------------------------------------------------------------------------
const vtkMRMLPatientHierarchyNode::HierarchyTag
  vtkMRMLPatientHierarchyNode::PATIENTHIERARCHY_LEVEL_PATIENT = vtkMRMLPatientHierarchyNode::HierarchyTag("Patient",1);
const vtkMRMLPatientHierarchyNode::HierarchyTag
  vtkMRMLPatientHierarchyNode::PATIENTHIERARCHY_LEVEL_STUDY = vtkMRMLPatientHierarchyNode::HierarchyTag("Study",2);
const vtkMRMLPatientHierarchyNode::HierarchyTag
  vtkMRMLPatientHierarchyNode::PATIENTHIERARCHY_LEVEL_SERIES = vtkMRMLPatientHierarchyNode::HierarchyTag("Series",3);
const vtkMRMLPatientHierarchyNode::HierarchyTag
  vtkMRMLPatientHierarchyNode::PATIENTHIERARCHY_LEVEL_SUBSERIES = vtkMRMLPatientHierarchyNode::HierarchyTag("Subseries",4);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPatientHierarchyNode);

//----------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode::vtkMRMLPatientHierarchyNode()
{
  this->Uid = NULL;
  this->DicomDatabaseFileName = NULL;
  this->Tags.clear();
}

//----------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode::~vtkMRMLPatientHierarchyNode()
{
  this->Tags.clear();
}

//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " Uid=\""
    << (this->Uid ? this->Uid : "NULL" ) << "\n";

  os << indent << " DicomDatabaseFileName=\""
    << (this->DicomDatabaseFileName ? this->DicomDatabaseFileName : "NULL" ) << "\n";

  os << indent << " Tags=\"";
  for (std::vector<HierarchyTag>::iterator tagsIt = this->Tags.begin(); tagsIt != this->Tags.end(); ++tagsIt)
    {
    os << tagsIt->Name << HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_NAME_LEVEL_SEPARATOR
       << tagsIt->Level << HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_ITEM_SEPARATOR;
    }
  os << "\"";
}

//----------------------------------------------------------------------------
const char* vtkMRMLPatientHierarchyNode::GetNodeTagName()
{
  return "PatientHierarchy";
}

//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::ReadXMLAttributes( const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "Uid")) 
      {
      this->SetUid(attValue);
      }
    else if (!strcmp(attName, "DicomDatabaseFileName"))
      {
      this->SetDicomDatabaseFileName(attValue);
      }
    else if (!strcmp(attName, "Tags")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();

      this->Tags.clear();
      size_t itemSeparatorPosition = valueStr.find( HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_ITEM_SEPARATOR );
      while (itemSeparatorPosition != std::string::npos)
        {
        std::string itemStr = valueStr.substr(0, itemSeparatorPosition);
        size_t tagLevelSeparatorPosition = itemStr.find( HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_NAME_LEVEL_SEPARATOR );

        std::string name = itemStr.substr(0, tagLevelSeparatorPosition);

        std::stringstream ssLevel;
        ssLevel << itemStr.substr( tagLevelSeparatorPosition + HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_NAME_LEVEL_SEPARATOR.size() );
        int level;
        ssLevel >> level;

        this->AddTag(name.c_str(), level);

        valueStr = valueStr.substr( itemSeparatorPosition + HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_ITEM_SEPARATOR.size() );
        itemSeparatorPosition = valueStr.find( HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_ITEM_SEPARATOR );
        }
      if (! valueStr.empty() )
        {
        std::string itemStr = valueStr.substr(0, itemSeparatorPosition);
        size_t tagLevelSeparatorPosition = itemStr.find( HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_NAME_LEVEL_SEPARATOR );

        std::string name = itemStr.substr(0, tagLevelSeparatorPosition);

        std::stringstream ssLevel;
        ssLevel << itemStr.substr( tagLevelSeparatorPosition + HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_NAME_LEVEL_SEPARATOR.size() );
        int level;
        ssLevel >> level;

        this->AddTag(name.c_str(), level);
        }
      }
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);

  vtkIndent indent(nIndent);

  of << indent << " Uid=\""
    << (this->Uid ? this->Uid : "NULL" ) << "\"";

  of << indent << " DicomDatabaseFileName=\""
    << (this->DicomDatabaseFileName ? this->DicomDatabaseFileName : "NULL" ) << "\"";

  of << indent << " Tags=\"";
  for (std::vector<HierarchyTag>::iterator tagsIt = this->Tags.begin(); tagsIt != this->Tags.end(); ++tagsIt)
    {
    of << tagsIt->Name << HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_NAME_LEVEL_SEPARATOR
      << tagsIt->Level << HierarchyTag::PATIENTHIERARCHY_HIERARCHYTAG_ITEM_SEPARATOR;
    }
  of << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLPatientHierarchyNode *node = (vtkMRMLPatientHierarchyNode*) anode;

  this->SetUid(node->Uid);
  this->SetDicomDatabaseFileName(node->DicomDatabaseFileName);

  HierarchyTag* tag = NULL;
  int index = 0;
  while ( tag = node->GetTag(index++) )
    {
    this->AddTag(*tag);
    }

  this->EndModify(disabledModify);
}

//---------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode*
vtkMRMLPatientHierarchyNode::GetPatientHierarchyNodeByUid(
  vtkMRMLScene *scene, const char* dicomDatabaseFileName, const char* uid )
{
  if (!scene || !dicomDatabaseFileName || !uid)
    {
    return NULL;
    }

  std::vector<vtkMRMLNode *> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLPatientHierarchyNode", patientHierarchyNodes);

  for (unsigned int i=0; i<numberOfNodes; i++)
    {
    vtkMRMLPatientHierarchyNode *node = vtkMRMLPatientHierarchyNode::SafeDownCast(patientHierarchyNodes[i]);
    if (node && node->GetUid()
             && !strcmp(dicomDatabaseFileName, node->GetDicomDatabaseFileName())
             && !strcmp(uid, node->GetUid()) )
      {
      return node;
      }
    }

  return NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::InsertDicomSeriesInHierarchy(
  vtkMRMLScene *scene, const char* dicomDatabaseFileName, 
  const char* patientId, const char* studyInstanceUid, const char* seriesInstanceUid )
{
  if ( !scene || !dicomDatabaseFileName
    || !patientId || !studyInstanceUid || !seriesInstanceUid )
    {
    return;
    }

  vtkMRMLPatientHierarchyNode* patientNode = NULL;
  vtkMRMLPatientHierarchyNode* studyNode = NULL;
  vtkMRMLPatientHierarchyNode* seriesNode = NULL;

  std::vector<vtkMRMLNode *> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLPatientHierarchyNode", patientHierarchyNodes);

  // Find referenced nodes
  for (unsigned int i=0; i<numberOfNodes; i++)
    {
    vtkMRMLPatientHierarchyNode *node = vtkMRMLPatientHierarchyNode::SafeDownCast(patientHierarchyNodes[i]);
    if ( node && node->GetUid() && !strcmp(dicomDatabaseFileName, node->GetDicomDatabaseFileName()) )
      {
      if (!strcmp(patientId, node->GetUid()))
        {
        patientNode = node;
        }
      else if (!strcmp(studyInstanceUid, node->GetUid()))
        {
        studyNode = node;
        }
      else if (!strcmp(seriesInstanceUid, node->GetUid()))
        {
        seriesNode = node;
        }
      }
    }

  if (!seriesNode)
    {
    vtkErrorWithObjectMacro(scene,
      "vtkMRMLPatientHierarchyNode::InsertDicomSeriesInHierarchy: Patient hierarchy node with ID="
      << patientId << " cannot be found!");
    return;
    }

  // Create patient and study nodes if they do not exist yet
  if (!patientNode)
    {
    patientNode = vtkMRMLPatientHierarchyNode::New();
    patientNode->AllowMultipleChildrenOn();
    patientNode->HideFromEditorsOff();
    patientNode->SetUid(patientId);
    patientNode->SetDicomDatabaseFileName(dicomDatabaseFileName);
    patientNode->AddTag(PATIENTHIERARCHY_LEVEL_PATIENT);
    scene->AddNode(patientNode);
    }

  if (!studyNode)
    {
    studyNode = vtkMRMLPatientHierarchyNode::New();
    studyNode->AllowMultipleChildrenOn();
    studyNode->HideFromEditorsOff();
    studyNode->SetUid(studyInstanceUid);
    studyNode->SetDicomDatabaseFileName(dicomDatabaseFileName);
    studyNode->AddTag(PATIENTHIERARCHY_LEVEL_STUDY);
    studyNode->SetParentNodeID(patientNode->GetID());
    scene->AddNode(studyNode);
    }

  seriesNode->SetParentNodeID(studyNode->GetID());
}

//---------------------------------------------------------------------------
bool vtkMRMLPatientHierarchyNode::AreNodesInSameBranch( vtkMRMLScene *scene,
                                                       const char* nodeId1, const char* nodeId2,
                                                       int lowestCommonLevel )
{
  if ( !scene || !nodeId1 || !nodeId2 )
    {
    return false;
    }

  if (lowestCommonLevel < 0)
    {
    vtkErrorWithObjectMacro(scene,
      "vtkMRMLPatientHierarchyNode::AreNodesInSameBranch: Invalid lowest common level!");
    return false;
    }

  // Get input nodes
  vtkMRMLPatientHierarchyNode* node1 = vtkMRMLPatientHierarchyNode::SafeDownCast(scene->GetNodeByID(nodeId1));
  vtkMRMLPatientHierarchyNode* node2 = vtkMRMLPatientHierarchyNode::SafeDownCast(scene->GetNodeByID(nodeId2));

  // If not hierarchy nodes, check if they have an associated patient hierarchy node
  if (!node1)
    {
    node1 = vtkMRMLPatientHierarchyNode::SafeDownCast(
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, nodeId1) );
    }
  if (!node2)
    {
    node2 = vtkMRMLPatientHierarchyNode::SafeDownCast(
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, nodeId2) );
    }

  // Check if valid nodes are found
  if (!node1 || !node2)
    {
    return false;
    }

  // If either input node is already higher level than the lowest common level then return with false
  if (node1->GetLevel() < lowestCommonLevel || node2->GetLevel() < lowestCommonLevel)
    {
    return false;
    }

  // Walk the hierarchy up until we reach the lowest common level
  while (node1->GetLevel() > lowestCommonLevel)
    {
    node1 = vtkMRMLPatientHierarchyNode::SafeDownCast(node1->GetParentNode());
    }
  while (node2->GetLevel() > lowestCommonLevel)
    {
    node2 = vtkMRMLPatientHierarchyNode::SafeDownCast(node2->GetParentNode());
    }

  // Sanity check
  if (!node1 || !node2)
    {
    vtkErrorWithObjectMacro(scene,
      "vtkMRMLPatientHierarchyNode::AreNodesInSameBranch: Invalid patient hierarchy tree - level inconsistency!");
    return false;
    }
  if ( !node1->GetUid() || !node1->GetDicomDatabaseFileName()
    || !node2->GetUid() || !node2->GetDicomDatabaseFileName() )
    {
    vtkErrorWithObjectMacro(scene,
      "vtkMRMLPatientHierarchyNode::AreNodesInSameBranch: Found ancestor node contains empty Instance UID or DICOM database file name!");
    return false;
    }

  // Check if the found nodes match
  // (handling the case when two patient hierarchy nodes point to the same DICOM object)
  if ( !strcmp(node1->GetUid(), node2->GetUid())
    && !strcmp(node1->GetDicomDatabaseFileName(), node2->GetDicomDatabaseFileName()) )
    {
    return true;
    }

  return false;
}

//---------------------------------------------------------------------------
int vtkMRMLPatientHierarchyNode::GetLevel()
{
  for (std::vector<HierarchyTag>::iterator tagsIt = this->Tags.begin(); tagsIt != this->Tags.end(); ++tagsIt)
    {
    if (tagsIt->Level > -1)
      {
      return tagsIt->Level;
      }
    }

  return -1;
}

//---------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::AddTag(const char* name, int level)
{
  HierarchyTag tag( HierarchyTag::ValidateName(name), level );
  this->AddTag(tag);
}

//---------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::AddTag(HierarchyTag tag)
{
  this->Tags.push_back(tag);
}

//---------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode::HierarchyTag* vtkMRMLPatientHierarchyNode::GetTag(int index)
{
  if (index < this->Tags.size())
    {
    return &(this->Tags[index]);
    }
  else
    {
    return NULL;
    }
}
