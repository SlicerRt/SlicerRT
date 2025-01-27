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
#include "vtkMRMLObjectiveNode.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkNew.h>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLObjectiveNode);


//----------------------------------------------------------------------------
vtkMRMLObjectiveNode::vtkMRMLObjectiveNode()
{
	this->Name = nullptr;
}

//----------------------------------------------------------------------------
vtkMRMLObjectiveNode::~vtkMRMLObjectiveNode() {
	if (this->Name)
	{
		delete[] this->Name;
		this->Name = nullptr;
	}
}

//----------------------------------------------------------------------------
void vtkMRMLObjectiveNode::WriteXML(ostream& of, int nIndent)
{
	Superclass::WriteXML(of, nIndent);

	// Write all MRML node attributes into output stream
	vtkMRMLWriteXMLBeginMacro(of);

	vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLObjectiveNode::ReadXMLAttributes(const char** atts)
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
void vtkMRMLObjectiveNode::Copy(vtkMRMLNode* anode)
{
	int disabledModify = this->StartModify();

	Superclass::Copy(anode);

	vtkMRMLCopyBeginMacro(anode);

	vtkMRMLCopyEndMacro();

	this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLObjectiveNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintStringMacro(Name)
  vtkMRMLPrintEndMacro();
}

//---------------------------------------------------------------------------
void vtkMRMLObjectiveNode::SetSegmentation(std::string segmentationName)
{
	this->Segmentation = segmentationName;
}

//----------------------------------------------------------------------------
std::string vtkMRMLObjectiveNode::GetSegmentation()
{
	return this->Segmentation;
}

////----------------------------------------------------------------------------
//void vtkMRMLObjectiveNode::AddSegmentation(const std::string& segmentation)
//{
//    // Debug statement to check if 'this' is valid
//    if (this == nullptr)
//    {
//        std::cerr << "Error: 'this' is a null pointer in AddSegmentation." << std::endl;
//        return;
//    }
//
//    // Check if segmentation already exists in objective
//    if (std::find(this->Segmentations.begin(), this->Segmentations.end(), segmentation) != this->Segmentations.end())
//    {
//        std::cerr << "Warning: Segmentation already exists in AddSegmentation." << std::endl;
//        return;
//    }
//	else
//	{
//		this->Segmentations.push_back(segmentation);
//	}  
//}
//
////----------------------------------------------------------------------------
//void vtkMRMLObjectiveNode::RemoveSegmentation(const std::string& segmentation)
//{
//	// Debug statement to check if 'this' is valid
//	if (this == nullptr)
//	{
//		std::cerr << "Error: 'this' is a null pointer in RemoveSegmentation." << std::endl;
//		return;
//	}
//
//	auto it = std::find(this->Segmentations.begin(), this->Segmentations.end(), segmentation);
//	if (it != this->Segmentations.end())
//	{
//		this->Segmentations.erase(it);
//	}
//	else
//	{
//		std::cerr << "Error: Segmentation not found in RemoveSegmentation." << std::endl;
//	}
//}
//
////-------
//void vtkMRMLObjectiveNode::RemoveAllSegments()
//{
//	this->Segmentations.clear();
//}
//
//
////----------------------------------------------------------------------------
//const std::vector<std::string>& vtkMRMLObjectiveNode::GetSegmentations() const
//{
//	return this->Segmentations;
//}

//// ----------------------------------------------------------------------------
//void vtkMRMLObjectiveNode::SetDoseObjectiveFunctionAndGradient(ObjectiveFunctionAndGradient functions)
//{
//	this->ObjectiveFunction = functions.objectiveFunction;
//	this->ObjectiveGradient = functions.objectiveGradient;
//}
//
//// ----------------------------------------------------------------------------
//std::function<float(const vtkMRMLObjectiveNode::DoseType&)> vtkMRMLObjectiveNode::GetObjectiveFunction()
//{
//	return this->ObjectiveFunction;
//}
//
//// ----------------------------------------------------------------------------
//std::function<vtkMRMLObjectiveNode::DoseType&(const vtkMRMLObjectiveNode::DoseType&)> vtkMRMLObjectiveNode::GetObjectiveGradient()
//{
//	return this->ObjectiveGradient;
//}