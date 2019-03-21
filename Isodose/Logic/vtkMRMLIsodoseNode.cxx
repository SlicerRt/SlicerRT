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

// MRMLIsodose includes
#include "vtkMRMLIsodoseNode.h"

// MRML includes
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* DOSE_VOLUME_REFERENCE_ROLE = "doseVolumeRef";
const char* vtkMRMLIsodoseNode::COLOR_TABLE_REFERENCE_ROLE = "colorTableRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIsodoseNode);

//----------------------------------------------------------------------------
vtkMRMLIsodoseNode::vtkMRMLIsodoseNode()
{
  this->ShowIsodoseLines = true;
  this->ShowIsodoseSurfaces = true;
  this->ShowScalarBar = false;
  this->ShowDoseVolumesOnly = true;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLIsodoseNode::~vtkMRMLIsodoseNode() = default;

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLBooleanMacro(ShowIsodoseLines, ShowIsodoseLines);
  vtkMRMLWriteXMLBooleanMacro(ShowIsodoseSurfaces, ShowIsodoseSurfaces);
  vtkMRMLWriteXMLBooleanMacro(ShowScalarBar, ShowScalarBar);
  vtkMRMLWriteXMLBooleanMacro(ShowDoseVolumesOnly, ShowDoseVolumesOnly);
  vtkMRMLWriteXMLEndMacro(); 
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  vtkMRMLNode::ReadXMLAttributes(atts);

  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLBooleanMacro(ShowIsodoseLines, ShowIsodoseLines);
  vtkMRMLReadXMLBooleanMacro(ShowIsodoseSurfaces, ShowIsodoseSurfaces);
  vtkMRMLReadXMLBooleanMacro(ShowScalarBar, ShowScalarBar);
  vtkMRMLReadXMLBooleanMacro(ShowDoseVolumesOnly, ShowDoseVolumesOnly);
  vtkMRMLReadXMLEndMacro();

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLIsodoseNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyBooleanMacro(ShowIsodoseLines);
  vtkMRMLCopyBooleanMacro(ShowIsodoseSurfaces);
  vtkMRMLCopyBooleanMacro(ShowScalarBar);
  vtkMRMLCopyBooleanMacro(ShowDoseVolumesOnly);
  vtkMRMLCopyEndMacro();

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintBooleanMacro(ShowIsodoseLines);
  vtkMRMLPrintBooleanMacro(ShowIsodoseSurfaces);
  vtkMRMLPrintBooleanMacro(ShowScalarBar);
  vtkMRMLPrintBooleanMacro(ShowDoseVolumesOnly);
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLIsodoseNode::GetDoseVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(DOSE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(DOSE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLIsodoseNode::GetColorTableNode()
{
  vtkMRMLScalarVolumeNode* doseVolumeNode = this->GetDoseVolumeNode();
  if (!doseVolumeNode)
  {
    vtkWarningMacro("GetColorTableNode: No dose volume node found. Isodose color table node is associated to dose volume nodes.");
    return nullptr;
  }

  return vtkMRMLColorTableNode::SafeDownCast( doseVolumeNode->GetNodeReference(COLOR_TABLE_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::SetAndObserveColorTableNode(vtkMRMLColorTableNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  vtkMRMLScalarVolumeNode* doseVolumeNode = this->GetDoseVolumeNode();
  if (!doseVolumeNode)
  {
    vtkErrorMacro("SetAndObserveColorTableNode: No dose volume node found. Isodose color table node is associated to dose volume nodes.");
    return;
  }

  doseVolumeNode->SetNodeReferenceID(COLOR_TABLE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}
