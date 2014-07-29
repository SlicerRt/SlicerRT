/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Adam Rankin, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// MRML includes
#include "vtkMRMLContourModelDisplayNode.h"

// VTK includes
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourModelDisplayNode);

//-----------------------------------------------------------------------------
vtkMRMLContourModelDisplayNode::vtkMRMLContourModelDisplayNode()
{
}

//-----------------------------------------------------------------------------
vtkMRMLContourModelDisplayNode::~vtkMRMLContourModelDisplayNode()
{
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode::ContourRepresentationType vtkMRMLContourModelDisplayNode::GetContourDisplayType()
{
  return this->ContourDisplayType;
}

//-----------------------------------------------------------------------------
void vtkMRMLContourModelDisplayNode::SetContourDisplayType( vtkMRMLContourNode::ContourRepresentationType aType)
{
  this->ContourDisplayType = aType;
}

//-----------------------------------------------------------------------------
void vtkMRMLContourModelDisplayNode::ReadXMLAttributes( const char** atts )
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "contourDisplayType"))
    {
      std::stringstream ss;
      ss << attValue;
      int type;
      ss >> type;
      this->ContourDisplayType = (vtkMRMLContourNode::ContourRepresentationType)(type);
    }
  }

  this->EndModify(disabledModify);
}

//-----------------------------------------------------------------------------
void vtkMRMLContourModelDisplayNode::WriteXML( ostream& of, int nIndent )
{ 
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  of << indent << " contourDisplayType=\"" << this->ContourDisplayType << "\"";
}

//-----------------------------------------------------------------------------
void vtkMRMLContourModelDisplayNode::Copy( vtkMRMLNode *node )
{
  Superclass::Copy(node);

  if( node == NULL )
  {
    return;
  }
  vtkMRMLContourModelDisplayNode* otherNode = vtkMRMLContourModelDisplayNode::SafeDownCast(node);

  if( otherNode == NULL )
  {
    return;
  }

  this->SetContourDisplayType(otherNode->GetContourDisplayType());
}
