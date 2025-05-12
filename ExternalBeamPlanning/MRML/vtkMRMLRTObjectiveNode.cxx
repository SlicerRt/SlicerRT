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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// MRMLObjective includes
#include "vtkMRMLRTObjectiveNode.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkNew.h>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTObjectiveNode);


//----------------------------------------------------------------------------
vtkMRMLRTObjectiveNode::vtkMRMLRTObjectiveNode()
{
	this->Name = nullptr;
}

//----------------------------------------------------------------------------
vtkMRMLRTObjectiveNode::~vtkMRMLRTObjectiveNode() {
	if (this->Name)
	{
		delete[] this->Name;
		this->Name = nullptr;
	}
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::WriteXML(ostream& of, int nIndent)
{
	Superclass::WriteXML(of, nIndent);

	// Write all MRML node attributes into output stream
	vtkMRMLWriteXMLBeginMacro(of);

	vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::ReadXMLAttributes(const char** atts)
{
	int disabledModify = this->StartModify();
	vtkMRMLNode::ReadXMLAttributes(atts);

	vtkMRMLReadXMLBeginMacro(atts);
	
	vtkMRMLReadXMLEndMacro();

	this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTObjectiveNode::Copy(vtkMRMLNode* anode)
{
	int disabledModify = this->StartModify();

	Superclass::Copy(anode);

	vtkMRMLCopyBeginMacro(anode);

	vtkMRMLCopyEndMacro();

	this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintStringMacro(Name)
  vtkMRMLPrintEndMacro();
}

//---------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::SetSegmentation(std::string segmentationName)
{
	this->Segmentation = segmentationName;
}

//----------------------------------------------------------------------------
std::string vtkMRMLRTObjectiveNode::GetSegmentation()
{
	return this->Segmentation;
}
