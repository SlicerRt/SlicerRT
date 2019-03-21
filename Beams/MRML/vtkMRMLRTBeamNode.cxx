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
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// MRML includes
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkDoubleArray.h>
#include <vtkCellArray.h>

//------------------------------------------------------------------------------
const char* vtkMRMLRTBeamNode::NEW_BEAM_NODE_NAME_PREFIX = "NewBeam_";
const char* vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX = "_BeamTransform";

//------------------------------------------------------------------------------
static const char* MLCPOSITION_REFERENCE_ROLE = "MLCPositionRef";
static const char* DRR_REFERENCE_ROLE = "DRRRef";
static const char* CONTOUR_BEV_REFERENCE_ROLE = "contourBEVRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTBeamNode);

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode::vtkMRMLRTBeamNode()
{
  this->BeamNumber = -1;
  this->BeamDescription = nullptr;
  this->BeamWeight = 1.0;

  this->X1Jaw = -100.0;
  this->X2Jaw = 100.0;
  this->Y1Jaw = -100.0;
  this->Y2Jaw = 100.0;

  this->GantryAngle = 0.0;
  this->CollimatorAngle = 0.0;
  this->CouchAngle = 0.0;

  this->SAD = 2000.0;
}

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode::~vtkMRMLRTBeamNode()
{
  this->SetBeamDescription(nullptr);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  of << " BeamNumber=\"" << this->BeamNumber << "\"";
  of << " BeamDescription=\"" << (this->BeamDescription ? BeamDescription : "") << "\"";
  of << " BeamWeight=\"" << this->BeamWeight << "\"";

  of << " X1Jaw=\"" << this->X1Jaw << "\"";
  of << " X2Jaw=\"" << this->X2Jaw << "\"";
  of << " Y1Jaw=\"" << this->Y1Jaw << "\"";
  of << " Y2Jaw=\"" << this->Y2Jaw << "\"";
  of << " SAD=\"" << this->SAD << "\"";

  of << " GantryAngle=\"" << this->GantryAngle << "\"";
  of << " CollimatorAngle=\"" << this->CollimatorAngle << "\"";
  of << " CouchAngle=\"" << this->CouchAngle << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = nullptr;
  const char* attValue = nullptr;

  while (*atts != nullptr)
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "BeamNumber"))
    {
      this->BeamNumber = vtkVariant(attValue).ToInt();
    }
    else if (!strcmp(attName, "BeamDescription"))
    {
      this->SetBeamDescription(attValue);
    }
    else if (!strcmp(attName, "BeamWeight"))
    {
      this->BeamWeight = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "X1Jaw"))
    {
      this->X1Jaw = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "X2Jaw"))
    {
      this->X2Jaw = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "Y1Jaw"))
    {
      this->Y1Jaw = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "Y2Jaw"))
    {
      this->Y2Jaw = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "SAD"))
    {
      this->SAD = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "GantryAngle"))
    {
      this->GantryAngle = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "CollimatorAngle"))
    {
      this->CollimatorAngle = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "CouchAngle"))
    {
      this->CouchAngle = vtkVariant(attValue).ToDouble();
    }
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTBeamNode::Copy(vtkMRMLNode *anode)
{
  // Do not call Copy function of the direct model base class, as it copies the poly data too,
  // which is undesired for beams, as they generate their own poly data from their properties.
  vtkMRMLDisplayableNode::Copy(anode);

  vtkMRMLRTBeamNode* node = vtkMRMLRTBeamNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  // Create transform node for beam
  this->CreateNewBeamTransformNode();

  // Add beam in the same plan if beam nodes are in the same scene
  vtkMRMLRTPlanNode* planNode = node->GetParentPlanNode();
  if (planNode && node->GetScene() == this->Scene)
  {
    planNode->AddBeam(this);
  }

  // Copy beam parameters
  this->DisableModifiedEventOn();

  this->SetBeamNumber(node->GetBeamNumber());
  this->SetBeamDescription(node->GetBeamDescription());
  this->SetBeamWeight(node->GetBeamWeight());

  this->SetX1Jaw(node->GetX1Jaw());
  this->SetX2Jaw(node->GetX2Jaw());
  this->SetY1Jaw(node->GetY1Jaw());
  this->SetY2Jaw(node->GetY2Jaw());

  this->SetGantryAngle(node->GetGantryAngle());
  this->SetCollimatorAngle(node->GetCollimatorAngle());
  this->SetCouchAngle(node->GetCouchAngle());

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetScene(vtkMRMLScene* scene)
{
  Superclass::SetScene(scene);

  if (scene)
  {
    // Create beam model
    vtkSmartPointer<vtkPolyData> beamModelPolyData = vtkSmartPointer<vtkPolyData>::New();
    this->CreateBeamPolyData(beamModelPolyData);
    this->SetAndObservePolyData(beamModelPolyData);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " BeamNumber:   " << this->BeamNumber << "\n";
  os << indent << " BeamDescription:   " << (this->BeamDescription ? BeamDescription : "nullptr") << "\n";
  os << indent << " BeamWeight:   " << this->BeamWeight << "\n";

  os << indent << " X1Jaw:   " << this->X1Jaw << "\n";
  os << indent << " X2Jaw:   " << this->X2Jaw << "\n";
  os << indent << " Y1Jaw:   " << this->Y1Jaw << "\n";
  os << indent << " Y2Jaw:   " << this->Y2Jaw << "\n";
  os << indent << " SAD:   " << this->SAD << "\n";

  os << indent << " GantryAngle:   " << this->GantryAngle << "\n";
  os << indent << " CollimatorAngle:   " << this->CollimatorAngle << "\n";
  os << indent << " CouchAngle:   " << this->CouchAngle << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CreateDefaultDisplayNodes()
{
  // Create default model display node
  Superclass::CreateDefaultDisplayNodes();

  // Set beam-specific parameters
  vtkMRMLModelDisplayNode* displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetDisplayNode());
  if (!displayNode)
  {
    vtkErrorMacro("CreateDefaultDisplayNodes: Failed to create default display node");
    return;
  }

  displayNode->SetColor(0.0, 1.0, 0.2);
  displayNode->SetOpacity(0.3);
  displayNode->SetBackfaceCulling(0); // Disable backface culling to make the back side of the contour visible as well
  displayNode->VisibilityOn();
  displayNode->SliceIntersectionVisibilityOn();
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CreateDefaultTransformNode()
{
  if (this->GetParentTransformNode())
  {
    return;
  }

  this->CreateNewBeamTransformNode();
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CreateNewBeamTransformNode()
{
  // Create transform node for beam
  vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  std::string transformName = std::string(this->GetName()) + BEAM_TRANSFORM_NODE_NAME_POSTFIX;
  transformNode->SetName(transformName.c_str());
  this->GetScene()->AddNode(transformNode);

  // Hide transform node from subject hierarchy as it is not supposed to be used by the user
  transformNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str(), "1");

  this->SetAndObserveTransformNodeID(transformNode->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLDoubleArrayNode* vtkMRMLRTBeamNode::GetMLCPositionDoubleArrayNode()
{
  return vtkMRMLDoubleArrayNode::SafeDownCast( this->GetNodeReference(MLCPOSITION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(MLCPOSITION_REFERENCE_ROLE, (node ? node->GetID() : nullptr));

  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLRTBeamNode::GetDRRVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(DRR_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveDRRVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(DRR_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLRTBeamNode::GetContourBEVVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(CONTOUR_BEV_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveContourBEVVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(CONTOUR_BEV_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode* vtkMRMLRTBeamNode::GetParentPlanNode()
{
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetScene());
  if (!shNode)
  {
    vtkErrorMacro("GetParentPlanNode: Failed to access subject hierarchy node");
    return nullptr;
  }

  return vtkMRMLRTPlanNode::SafeDownCast(shNode->GetParentDataNode(this));
}

//----------------------------------------------------------------------------
bool vtkMRMLRTBeamNode::GetPlanIsocenterPosition(double isocenter[3])
{
  vtkMRMLRTPlanNode* parentPlanNode = this->GetParentPlanNode();
  if (!parentPlanNode)
  {
    vtkErrorMacro("GetPlanIsocenterPosition: Failed to access parent plan node");
    return false;
  }
  vtkMRMLMarkupsFiducialNode* poisMarkupsNode = parentPlanNode->GetPoisMarkupsFiducialNode();
  if (!poisMarkupsNode)
  {
    vtkErrorMacro("GetPlanIsocenterPosition: Failed to access POIs markups node");
    return false;
  }

  poisMarkupsNode->GetNthFiducialPosition(vtkMRMLRTPlanNode::ISOCENTER_FIDUCIAL_INDEX, isocenter);
  return true;
}

//----------------------------------------------------------------------------
bool vtkMRMLRTBeamNode::GetSourcePosition(double source[3])
{
  double sourcePosition_Beam[3] = {0.0, 0.0, this->SAD};
  this->TransformPointToWorld(sourcePosition_Beam, source);

  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetX1Jaw(double x1Jaw)
{
  this->X1Jaw = x1Jaw;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetX2Jaw(double x2Jaw)
{
  this->X2Jaw = x2Jaw;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetY1Jaw(double y1Jaw)
{
  this->Y1Jaw = y1Jaw;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetY2Jaw(double y2Jaw)
{
  this->Y2Jaw = y2Jaw;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetGantryAngle(double angle)
{
  this->GantryAngle = angle;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetCollimatorAngle(double angle)
{
  this->CollimatorAngle = angle;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetCouchAngle(double angle)
{
  this->CouchAngle = angle;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetSAD(double sad)
{
  this->SAD = sad;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//---------------------------------------------------------------------------
void vtkMRMLRTBeamNode::UpdateGeometry()
{
  // Make sure display node exists
  this->CreateDefaultDisplayNodes();

  // Update beam poly data based on jaws and MLC
  this->CreateBeamPolyData(this->GetPolyData());
}

//---------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CreateBeamPolyData(vtkPolyData* beamModelPolyData)
{
  if (!beamModelPolyData)
  {
    vtkErrorMacro("CreateBeamPolyData: Invalid beam node");
    return;
  }

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();

  vtkMRMLDoubleArrayNode* mlcArrayNode = this->GetMLCPositionDoubleArrayNode();
  if (mlcArrayNode)
  {
    // First we extract the shape of the MLC
    int y2count = -(this->Y2Jaw/10.0);
    int y1count = -(this->Y1Jaw/10.0);
    int numLeavesVisible = y1count - y2count; // Calculate the number of leaves visible
    int numPointsEachSide = numLeavesVisible *2;

    double x1LeafPosition[40];
    double x2LeafPosition[40];

    // Calculate X1 first
    for (int i = y1count; i >= y2count; i--)
    {
      double leafPosition = mlcArrayNode->GetArray()->GetComponent(i-20, 1);
      if (leafPosition < this->X1Jaw)
      {
        x1LeafPosition[i-20] = leafPosition;
      }
      else
      {
        x1LeafPosition[i-20] = this->X1Jaw;
      }
    }
    // Calculate X2 next
    for (int i = y1count; i >= y2count; i--)
    {
      double leafPosition = mlcArrayNode->GetArray()->GetComponent(i-20, 0);
      if (leafPosition < this->X2Jaw)
      {
        x2LeafPosition[i-20] = leafPosition;
      }
      else
      {
        x2LeafPosition[i-20] = this->X2Jaw;
      }
    }

    // Create beam model
    points->InsertPoint(0,0,0,this->SAD);

    int count = 1;
    for (int i = y1count; i > y2count; i--)
    {
      points->InsertPoint(count,-x1LeafPosition[i-20]*2, i*10*2, -this->SAD );
      count ++;
      points->InsertPoint(count,-x1LeafPosition[i-20]*2, (i-1)*10*2, -this->SAD );
      count ++;
    }

    for (int i = y2count; i < y1count; i++)
    {
      points->InsertPoint(count,x2LeafPosition[i-20]*2, i*10*2, -this->SAD );
      count ++;
      points->InsertPoint(count,x2LeafPosition[i-20]*2, (i+1)*10*2, -this->SAD );
      count ++;
    }

    for (int i = 1; i <numPointsEachSide; i++)
    {
      cellArray->InsertNextCell(3);
      cellArray->InsertCellPoint(0);
      cellArray->InsertCellPoint(i);
      cellArray->InsertCellPoint(i+1);
    }
    // Add connection between X1 and X2
    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(numPointsEachSide);
    cellArray->InsertCellPoint(numPointsEachSide+1);
    for (int i = numPointsEachSide+1; i <2*numPointsEachSide; i++)
    {
      cellArray->InsertNextCell(3);
      cellArray->InsertCellPoint(0);
      cellArray->InsertCellPoint(i);
      cellArray->InsertCellPoint(i+1);
    }

    // Add connection between X1 and X2
    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(2*numPointsEachSide);
    cellArray->InsertCellPoint(1);

    // Add the cap to the bottom
    cellArray->InsertNextCell(2*numPointsEachSide);
    for (int i = 1; i <= 2*numPointsEachSide; i++)
    {
      cellArray->InsertCellPoint(i);
    }
  }
  else
  {
    points->InsertPoint(0,0,0,this->SAD);
    points->InsertPoint(1, 2*this->X1Jaw, 2*this->Y1Jaw, -this->SAD );
    points->InsertPoint(2, 2*this->X1Jaw, 2*this->Y2Jaw, -this->SAD );
    points->InsertPoint(3, 2*this->X2Jaw, 2*this->Y2Jaw, -this->SAD );
    points->InsertPoint(4, 2*this->X2Jaw, 2*this->Y1Jaw, -this->SAD );

    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(1);
    cellArray->InsertCellPoint(2);

    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(2);
    cellArray->InsertCellPoint(3);

    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(3);
    cellArray->InsertCellPoint(4);

    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(4);
    cellArray->InsertCellPoint(1);

    // Add the cap to the bottom
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(1);
    cellArray->InsertCellPoint(2);
    cellArray->InsertCellPoint(3);
    cellArray->InsertCellPoint(4);
  }

  beamModelPolyData->SetPoints(points);
  beamModelPolyData->SetPolys(cellArray);
}

//---------------------------------------------------------------------------
void vtkMRMLRTBeamNode::RequestCloning()
{
  this->InvokeEvent(vtkMRMLRTBeamNode::CloningRequested);

  // Update display
  if (this->GetDisplayNode())
  {
    this->GetDisplayNode()->Modified();
  }
}
