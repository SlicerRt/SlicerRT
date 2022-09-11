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
#include <vtkMRMLColorNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <cstring>
#include <sstream>

//------------------------------------------------------------------------------
static const char* DOSE_VOLUME_REFERENCE_ROLE = "doseVolumeRef";
static const char* ISOSURFACES_MODEL_REFERENCE_ROLE = "isosurfacesModelRef";
const char* vtkMRMLIsodoseNode::COLOR_TABLE_REFERENCE_ROLE = "colorTableRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIsodoseNode);

//----------------------------------------------------------------------------
vtkMRMLIsodoseNode::vtkMRMLIsodoseNode()
{
  this->ShowIsodoseLines = true;
  this->ShowIsodoseSurfaces = true;
  this->ShowDoseVolumesOnly = true;
  this->DoseUnits = DoseUnitsType::Unknown;
  this->ReferenceDoseValue = -1.;
  this->RelativeRepresentationFlag = false;

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
  vtkMRMLWriteXMLBooleanMacro(ShowDoseVolumesOnly, ShowDoseVolumesOnly);
  vtkMRMLWriteXMLEnumMacro(DoseUnits, DoseUnits);
  vtkMRMLWriteXMLFloatMacro(ReferenceDoseValue, ReferenceDoseValue);
  vtkMRMLWriteXMLBooleanMacro(RelativeRepresentationFlag, RelativeRepresentationFlag);

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
  vtkMRMLReadXMLBooleanMacro(ShowDoseVolumesOnly, ShowDoseVolumesOnly);
  vtkMRMLReadXMLEnumMacro(DoseUnits, DoseUnits);
  vtkMRMLReadXMLFloatMacro(ReferenceDoseValue, ReferenceDoseValue);
  vtkMRMLReadXMLBooleanMacro(RelativeRepresentationFlag, RelativeRepresentationFlag);
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
  vtkMRMLCopyBooleanMacro(ShowDoseVolumesOnly);
  vtkMRMLCopyEnumMacro(DoseUnits);
  vtkMRMLCopyFloatMacro(ReferenceDoseValue);
  vtkMRMLCopyBooleanMacro(RelativeRepresentationFlag);
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
  vtkMRMLPrintBooleanMacro(ShowDoseVolumesOnly);
  vtkMRMLPrintEnumMacro(DoseUnits);
  vtkMRMLPrintFloatMacro(ReferenceDoseValue);
  vtkMRMLPrintBooleanMacro(RelativeRepresentationFlag);
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

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLIsodoseNode::GetIsosurfacesModelNode()
{
  return vtkMRMLModelNode::SafeDownCast( this->GetNodeReference(ISOSURFACES_MODEL_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::SetAndObserveIsosurfacesModelNode(vtkMRMLModelNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
  }

  this->SetNodeReferenceID(ISOSURFACES_MODEL_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//---------------------------------------------------------------------------
void vtkMRMLIsodoseNode::SetDoseUnits(int id)
{
  switch (id)
  {
  case 0:
    this->SetDoseUnits(vtkMRMLIsodoseNode::Gy);
    break;
  case 1:
    this->SetDoseUnits(vtkMRMLIsodoseNode::Relative);
    break;
  default:
    this->SetDoseUnits(vtkMRMLIsodoseNode::Unknown);
    break;
  }
}

//---------------------------------------------------------------------------
const char* vtkMRMLIsodoseNode::GetDoseUnitsAsString(int id)
{
  switch (id)
  {
  case vtkMRMLIsodoseNode::Gy:
    return "Gy";
  case vtkMRMLIsodoseNode::Relative:
    return "Relative";
  case vtkMRMLIsodoseNode::Unknown:
  default:
    return "Unknown";
  }
}

//---------------------------------------------------------------------------
int vtkMRMLIsodoseNode::GetDoseUnitsFromString(const char* name)
{
  if (name == nullptr)
  {
    // invalid name
    return -1;
  }

  for (int i = 0; i < vtkMRMLIsodoseNode::DoseUnits_Last; i++)
  {
    if (std::strcmp(name, vtkMRMLIsodoseNode::GetDoseUnitsAsString(i)) == 0)
    {
      // found a matching name
      return i;
    }
  }

  // Check old values as "0", "1", "-1"
  // check only first byte
  int unitsType = -1; // unknown name
  switch (name[0])
  {
  case '0':
    unitsType = 0;
    break;
  case '1':
    unitsType = 1;
    break;
  case '-':
  default:
    break;
  }
  return unitsType;
}
