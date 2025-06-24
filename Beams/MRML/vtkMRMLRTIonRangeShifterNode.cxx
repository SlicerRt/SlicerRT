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

// Beams includes
#include "vtkMRMLRTIonRangeShifterNode.h"
#include "vtkMRMLRTPlanNode.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// MRML includes
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTableNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>

// VTK includes
#include <vtkCommand.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkTransform.h>
#include <vtkDoubleArray.h>
#include <vtkTable.h>
#include <vtkCellArray.h>
#include <vtkCubeSource.h>
#include <vtkAppendPolyData.h>

namespace
{
const char* PARENT_ION_BEAM_NODE_REFERENCE_ROLE = "parentIonBeamRef";
}

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTIonRangeShifterNode);

//----------------------------------------------------------------------------
vtkMRMLRTIonRangeShifterNode::vtkMRMLRTIonRangeShifterNode()
  :
  Superclass()
{
}

//----------------------------------------------------------------------------
vtkMRMLRTIonRangeShifterNode::~vtkMRMLRTIonRangeShifterNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkMRMLWriteXMLBeginMacro(of);
  // Write all MRML node attributes into output stream
  vtkMRMLWriteXMLIntMacro(number, Number);
  vtkMRMLWriteXMLStringMacro(rangeShifterID, RangeShifterID);
  vtkMRMLWriteXMLFloatMacro(materialDensity, MaterialDensity);
  vtkMRMLWriteXMLEnumMacro(type, Type);
  vtkMRMLWriteXMLStringMacro(accessoryCode, AccessoryCode);
  vtkMRMLWriteXMLStringMacro(description, Description);
  vtkMRMLWriteXMLStringMacro(materialID, MaterialID);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);
  
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLIntMacro(number, Number);
  vtkMRMLReadXMLStringMacro(rangeShifterID, RangeShifterID);
  vtkMRMLReadXMLFloatMacro(materialDensity, MaterialDensity);
  vtkMRMLReadXMLEnumMacro(type, Type);
  vtkMRMLReadXMLStringMacro(accessoryCode, AccessoryCode);
  vtkMRMLReadXMLStringMacro(description, Description);
  vtkMRMLReadXMLStringMacro(materialID, MaterialID);
  vtkMRMLReadXMLEndMacro();
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTIonRangeShifterNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  // Do not call Copy function of the direct model base class, as it copies the poly data too,
  // which is undesired for beams, as they generate their own poly data from their properties.
  vtkMRMLDisplayableNode::Copy(anode);

  vtkMRMLRTIonRangeShifterNode* node = vtkMRMLRTIonRangeShifterNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }
/*
  // Create transform node for beam
  this->CreateNewBeamTransformNode();

  // Add beam in the same plan if beam nodes are in the same scene
  vtkMRMLRTPlanNode* planNode = node->GetParentPlanNode();
  if (planNode && node->GetScene() == this->Scene)
  {
    planNode->AddBeam(this);
  }
*/
  // Copy beam parameters
  this->DisableModifiedEventOn();

  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyIntMacro(Number);
  vtkMRMLCopyStringMacro(RangeShifterID);
  vtkMRMLCopyFloatMacro(MaterialDensity);
  vtkMRMLCopyEnumMacro(Type);
  vtkMRMLCopyStringMacro(AccessoryCode);
  vtkMRMLCopyStringMacro(Description);
  vtkMRMLCopyStringMacro(MaterialID);
  vtkMRMLCopyEndMacro();

  this->EndModify(disabledModify);
  
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::CopyContent(vtkMRMLNode *anode, bool deepCopy/*=true*/)
{
  MRMLNodeModifyBlocker blocker(this);
  Superclass::CopyContent( anode, deepCopy);

  vtkMRMLRTIonRangeShifterNode* node = vtkMRMLRTIonRangeShifterNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyIntMacro(Number);
  vtkMRMLCopyStringMacro(RangeShifterID);
  vtkMRMLCopyFloatMacro(MaterialDensity);
  vtkMRMLCopyEnumMacro(Type);
  vtkMRMLCopyStringMacro(AccessoryCode);
  vtkMRMLCopyStringMacro(Description);
  vtkMRMLCopyStringMacro(MaterialID);
  vtkMRMLCopyEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::SetScene(vtkMRMLScene* scene)
{
  Superclass::SetScene(scene);

  if (!this->GetPolyData())
  {
    // Create range shifter model
    vtkSmartPointer<vtkPolyData> beamModelPolyData = vtkSmartPointer<vtkPolyData>::New();
    this->SetAndObservePolyData(beamModelPolyData);
  }

  this->CreateRangeShifterPolyData();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintIntMacro(Number);
  vtkMRMLPrintStringMacro(RangeShifterID);
  vtkMRMLPrintFloatMacro(MaterialDensity);
  vtkMRMLPrintEnumMacro(Type);
  vtkMRMLPrintStringMacro(AccessoryCode);
  vtkMRMLPrintStringMacro(Description);
  vtkMRMLPrintStringMacro(MaterialID);
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::CreateDefaultDisplayNodes()
{
  // Create default model display node
  Superclass::CreateDefaultDisplayNodes();

  // Set beam-specific parameters when first created
  vtkMRMLModelDisplayNode* displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetDisplayNode());
  if (displayNode != nullptr)
  {
    displayNode->SetColor(85./255., 170./255., 1.0);
    displayNode->SetOpacity(0.3);
    displayNode->SetBackfaceCulling(0); // Disable backface culling to make the back side of the contour visible as well
    displayNode->VisibilityOn();
    displayNode->Visibility2DOff();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::UpdateGeometry()
{
  // Make sure display node exists
  this->CreateDefaultDisplayNodes();

  // Update range shifters poly data based on configuration and settings
  this->CreateRangeShifterPolyData();
}

//---------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::CreateRangeShifterPolyData(vtkPolyData* rangeShifterModelPolyData/*=nullptr*/)
{
  if (!rangeShifterModelPolyData)
  {
    rangeShifterModelPolyData = this->GetPolyData();
  }
  if (!rangeShifterModelPolyData)
  {
    vtkErrorMacro("CreateRangeShifterPolyData: Invalid range shifter polydata");
    return;
  }

  vtkMRMLRTIonBeamNode* ionBeamNode = this->GetParentBeamNode();
  if (!ionBeamNode)
  {
    vtkErrorMacro("CreateRangeShifterPolyData: Invalid parent ion beam node");
    return;
  }
  double isoToRsDistance = ionBeamNode->GetIsocenterToRangeShifterDistance();
  double wet = ionBeamNode->GetRangeShifterWET();
  const char* setting = ionBeamNode->GetRangeShifterSetting();
  double beamAxisOffset = 0.; // in/out beam offset
  if (setting && (!std::strcmp(setting, "OUT") || !std::strcmp(setting, "0")))
  {
    beamAxisOffset = -200.; // out of the beam
  }
  else if (setting && (!std::strcmp(setting, "IN") || !std::strcmp(setting, "1")))
  {
    beamAxisOffset = 0.; // into the beam
  }
  if (isoToRsDistance <= 0.)
  {
    vtkErrorMacro("CreateRangeShifterPolyData: Invalid isocenter to range shifter distance");
    return;
  }

  if (wet > 0)
  {
    vtkNew< vtkCubeSource > rs;
    rs->SetBounds( -100., 100., beamAxisOffset + -100., beamAxisOffset + 100., isoToRsDistance, isoToRsDistance + wet);
    rs->Update();
    rangeShifterModelPolyData->DeepCopy(rs->GetOutput());
  }
  else
  {
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> cellArray;

    points->InsertPoint( 0, -100., beamAxisOffset + -100., isoToRsDistance);
    points->InsertPoint( 1, -100., beamAxisOffset + 100., isoToRsDistance);
    points->InsertPoint( 2, 100., beamAxisOffset + 100., isoToRsDistance);
    points->InsertPoint( 3, 100., beamAxisOffset + -100., isoToRsDistance);

    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(1);
    cellArray->InsertCellPoint(2);
    cellArray->InsertCellPoint(3);
    rangeShifterModelPolyData->SetPoints(points);
    rangeShifterModelPolyData->SetPolys(cellArray);
  }
}

//----------------------------------------------------------------------------
vtkMRMLRTIonBeamNode* vtkMRMLRTIonRangeShifterNode::GetParentBeamNode()
{
  return vtkMRMLRTIonBeamNode::SafeDownCast( this->GetNodeReference(PARENT_ION_BEAM_NODE_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::SetAndObserveParentBeamNode(vtkMRMLRTIonBeamNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
  }

  this->SetNodeReferenceID(PARENT_ION_BEAM_NODE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//---------------------------------------------------------------------------
const char* vtkMRMLRTIonRangeShifterNode::GetTypeAsString(int id)
{
  switch (id)
  {
  case vtkMRMLRTIonRangeShifterNode::ANALOG:
    return "ANALOG";
  case vtkMRMLRTIonRangeShifterNode::BINARY:
    return "BINARY";
  default:
    return "RangeShifter_Last";
  }
}

//---------------------------------------------------------------------------
int vtkMRMLRTIonRangeShifterNode::GetTypeFromString(const char* name)
{
  if (name == nullptr)
  {
    // invalid name
    return -1;
  }
  for (int i = 0; i < vtkMRMLRTIonRangeShifterNode::RangeShifter_Last; i++)
  {
    if (std::strcmp(name, vtkMRMLRTIonRangeShifterNode::GetTypeAsString(i)) == 0)
    {
      // found a matching name
      return i;
    }
  }
  // unknown name
  return -1;
}

//---------------------------------------------------------------------------
void vtkMRMLRTIonRangeShifterNode::SetType(int id)
{
  switch (id)
  {
  case 0:
    this->SetType(vtkMRMLRTIonRangeShifterNode::ANALOG);
    break;
  case 1:
    this->SetType(vtkMRMLRTIonRangeShifterNode::BINARY);
    break;
  default:
    this->SetType(vtkMRMLRTIonRangeShifterNode::RangeShifter_Last);
    break;
  }
}
