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

  This file was originally developed by ...
==============================================================================*/


#ifndef __vtkMRMLObjectiveNode_h
#define __vtkMRMLObjectiveNode_h

// MRML includes
#include "vtkMRMLNode.h"

#include "vtkSlicerExternalBeamPlanningModuleMRMLExport.h"

#include <vector>
#include <vtkSmartPointer.h>

class  VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_MRML_EXPORT vtkMRMLObjectiveNode : public vtkMRMLNode
{
public:
  static vtkMRMLObjectiveNode *New();
  vtkTypeMacro(vtkMRMLObjectiveNode, vtkMRMLNode);
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


protected:
  vtkMRMLObjectiveNode();
  ~vtkMRMLObjectiveNode() override;
  vtkMRMLObjectiveNode(const vtkMRMLObjectiveNode&);
  void operator=(const vtkMRMLObjectiveNode&);

  char* Name;

};

#endif
