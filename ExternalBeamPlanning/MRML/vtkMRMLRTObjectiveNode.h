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


#ifndef __vtkMRMLRTObjectiveNode_h
#define __vtkMRMLRTObjectiveNode_h

// ExternalBeamPlanning includes
#include "vtkSlicerExternalBeamPlanningModuleMRMLExport.h"

// MRML includes
#include "vtkMRMLNode.h"

class vtkMRMLSegmentationNode;

class  VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_MRML_EXPORT vtkMRMLRTObjectiveNode : public vtkMRMLNode
{
public:
  static vtkMRMLRTObjectiveNode *New();
  vtkTypeMacro(vtkMRMLRTObjectiveNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkMRMLNode* CreateNodeInstance() override;

  /// Set node attributes from name/value pairs 
  void ReadXMLAttributes(const char** atts) override;

  /// Write this node's information to a MRML file in XML format. 
  void WriteXML(ostream& of, int indent) override;

  /// Copy the node's attributes to this object 
  void Copy(vtkMRMLNode* node) override;

  /// Get unique node XML tag name (like Volume, Model) 
  const char* GetNodeTagName() override { return "Objective"; };

public:
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  /// Get node reference to segmentation
  vtkMRMLSegmentationNode* GetSegmentationNode();

		/// Set ID of segment assigned to this objective
  std::string GetSegmentID();

  /// Set node reference to segmentation
  void SetSegmentationAndSegmentID(vtkMRMLSegmentationNode* segmentationNode, std::string segmentID);

protected:
  vtkMRMLRTObjectiveNode();
  ~vtkMRMLRTObjectiveNode() override;
  vtkMRMLRTObjectiveNode(const vtkMRMLRTObjectiveNode&);
  void operator=(const vtkMRMLRTObjectiveNode&);

  char* Name;

		/// node reference to the segmentation containing the segment
		vtkMRMLSegmentationNode* SegmentationNode;
		/// ID of the segment assigned to this objective
  std::string SegmentID;


};

#endif
