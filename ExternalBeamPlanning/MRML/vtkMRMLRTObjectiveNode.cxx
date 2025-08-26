/*==============================================================================

		Copyright (c) German Cancer Research Center (DKFZ),
		Heidelberg, Germany. All Rights Reserved.

		See COPYRIGHT.txt
		or http://www.slicer.org/copyright/copyright.txt for details.

		Unless required by applicable law or agreed to in writing, software
		distributed under the License is distributed on an "AS IS" BASIS,
		WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
		See the License for the specific language governing permissions and
		limitations under the License.

		This file was originally developed by Lina Bucher, German Cancer
		Research Center (DKFZ) and Institute for Biomedical Engineering (IBT),
		Karlsruhe Institute of Technology (KIT).

==============================================================================*/

// ExternalBeamPlanning includes
#include "vtkMRMLRTObjectiveNode.h"

// MRML includes
#include <vtkMRMLSegmentationNode.h>

// Qt includes
#include <QDebug>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTObjectiveNode);


//----------------------------------------------------------------------------
vtkMRMLRTObjectiveNode::vtkMRMLRTObjectiveNode()
{
		this->Name = nullptr;
}

//----------------------------------------------------------------------------
vtkMRMLRTObjectiveNode::~vtkMRMLRTObjectiveNode()
{
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

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLRTObjectiveNode::GetSegmentationNode()
{
		return this->SegmentationNode;
}

//----------------------------------------------------------------------------
std::string vtkMRMLRTObjectiveNode::GetSegmentID()
{
		return this->SegmentID;
}

//---------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::SetSegmentationAndSegmentID(vtkMRMLSegmentationNode* segmentationNode, std::string segmentID)
{
		if (!segmentationNode->GetSegmentation()->GetSegment(segmentID))
		{
				qCritical() << Q_FUNC_INFO << ": Segment with ID" << segmentID.c_str() << "not found in segmentation";
				return;
		}

		this->SegmentationNode = segmentationNode;
		this->SegmentID = segmentID;
}
