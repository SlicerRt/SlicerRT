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
#include "vtkMRMLRTIonBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// MRML includes
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLDoubleArrayNode.h>
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
#include <vtkTransformPolyDataFilter.h>
#include <vtkDoubleArray.h>
#include <vtkTable.h>
#include <vtkCellArray.h>

//------------------------------------------------------------------------------
namespace
{

const char* const SCANSPOT_REFERENCE_ROLE = "ScanSpotRef";
constexpr double FWHM_TO_SIGMA = 1. / (2. * sqrt(2. * log(2.)));

} // namespace

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTIonBeamNode);

//----------------------------------------------------------------------------
vtkMRMLRTIonBeamNode::vtkMRMLRTIonBeamNode()
  :
  Superclass(),
  VSADx(Superclass::SAD),
  IsocenterToJawsDistanceX(Superclass::SourceToJawsDistanceX),
  IsocenterToJawsDistanceY(Superclass::SourceToJawsDistanceY),
  IsocenterToMultiLeafCollimatorDistance(Superclass::SourceToMultiLeafCollimatorDistance),
  IsocenterToRangeShifterDistance(4000.),
  ScanningSpotSize({ 15.0, 15.0 })
{
  this->VSADx = 6500.0;
  this->VSADy = 6500.0;
  this->IsocenterToJawsDistanceX = 3000.;
  this->IsocenterToJawsDistanceY = 3000.;
  this->IsocenterToMultiLeafCollimatorDistance = 2500.;
}

//----------------------------------------------------------------------------
vtkMRMLRTIonBeamNode::~vtkMRMLRTIonBeamNode()
{
  this->SetBeamDescription(nullptr);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  of << " VSADx=\"" << this->VSADx << "\"";
  of << " VSADy=\"" << this->VSADy << "\"";
  of << " IsocenterToJawsDistanceX=\"" << this->IsocenterToJawsDistanceX << "\"";
  of << " IsocenterToJawsDistanceY=\"" << this->IsocenterToJawsDistanceY << "\"";
  of << " IsocenterToMultiLeafCollimatorDistance=\"" << this->IsocenterToMultiLeafCollimatorDistance << "\"";
  of << " IsocenterToRangeShifterDistance=\"" << this->IsocenterToRangeShifterDistance << "\"";
  of << " ScanningSpotSizeX=\"" << this->ScanningSpotSize[0] << "\"";
  of << " ScanningSpotSizeY=\"" << this->ScanningSpotSize[1] << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = nullptr;
  const char* attValue = nullptr;

  while (*atts != nullptr)
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp( attName, "VSADx"))
    {
      this->VSADx = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp( attName, "VSADy"))
    {
      this->VSADy = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp( attName, "IsocenterToJawsDistanceX"))
    {
      this->IsocenterToJawsDistanceX = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp( attName, "IsocenterToJawsDistanceY"))
    {
      this->IsocenterToJawsDistanceY = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp( attName, "IsocenterToMultiLeafCollimatorDistance"))
    {
      this->IsocenterToMultiLeafCollimatorDistance = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp( attName, "IsocenterToRangeShifterDistance"))
    {
      this->IsocenterToRangeShifterDistance = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp( attName, "ScanningSpotSizeX"))
    {
      this->ScanningSpotSize[0] = vtkVariant(attValue).ToFloat();
    }
    else if (!strcmp( attName, "ScanningSpotSizeY"))
    {
      this->ScanningSpotSize[1] = vtkVariant(attValue).ToFloat();
    }
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTIonBeamNode::Copy(vtkMRMLNode *anode)
{
  // Do not call Copy function of the direct model base class, as it copies the poly data too,
  // which is undesired for beams, as they generate their own poly data from their properties.
  vtkMRMLDisplayableNode::Copy(anode);

  vtkMRMLRTIonBeamNode* node = vtkMRMLRTIonBeamNode::SafeDownCast(anode);
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
  this->SetVSAD( node->GetVSADx(), node->GetVSADy());
  this->SetIsocenterToJawsDistanceX(node->GetIsocenterToJawsDistanceX());
  this->SetIsocenterToJawsDistanceY(node->GetIsocenterToJawsDistanceY());
  this->SetIsocenterToMultiLeafCollimatorDistance(node->GetIsocenterToMultiLeafCollimatorDistance());
  this->SetIsocenterToRangeShifterDistance(node->GetIsocenterToRangeShifterDistance());

  float* scanSpotSize = node->GetScanningSpotSize();
  this->SetScanningSpotSize({ scanSpotSize[0], scanSpotSize[1] });

  this->SetGantryAngle(node->GetGantryAngle());
  this->SetCollimatorAngle(node->GetCollimatorAngle());
  this->SetCouchAngle(node->GetCouchAngle());

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetScene(vtkMRMLScene* scene)
{
  Superclass::SetScene(scene);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " VSADx:   " << this->VSADx << "\n";
  os << indent << " VSADy:   " << this->VSADy << "\n";
  os << indent << " IsocenterToJawsDistanceX:   " << this->IsocenterToJawsDistanceX << "\n";
  os << indent << " IsocenterToJawsDistanceY:   " << this->IsocenterToJawsDistanceY << "\n";
  os << indent << " IsocenterToMultiLeafCollimatorDistance:   " << this->IsocenterToMultiLeafCollimatorDistance << "\n";
  os << indent << " IsocenterToRangeShifterDistance:   " << this->IsocenterToRangeShifterDistance << "\n";
  os << indent << " ScanningSpotSizeX:   " << this->ScanningSpotSize[0] << "\n";
  os << indent << " ScanningSpotSizeY:   " << this->ScanningSpotSize[1] << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetAndObserveScanSpotTableNode(vtkMRMLTableNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
  }

  this->SetNodeReferenceID( SCANSPOT_REFERENCE_ROLE, (node ? node->GetID() : nullptr));

  this->InvokeCustomModifiedEvent(Superclass::BeamGeometryModified);
}

//----------------------------------------------------------------------------
vtkMRMLTableNode* vtkMRMLRTIonBeamNode::GetScanSpotTableNode()
{
  return vtkMRMLTableNode::SafeDownCast( this->GetNodeReference(SCANSPOT_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::CreateDefaultDisplayNodes()
{
  // Create default model display node
  Superclass::CreateDefaultDisplayNodes();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::CreateDefaultTransformNode()
{
  Superclass::CreateDefaultTransformNode();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::CreateNewBeamTransformNode()
{
  // Create transform node for ion beam
  Superclass::CreateNewBeamTransformNode();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetVSAD( double xComponent, double yComponent)
{
  this->VSADx = xComponent;
  this->VSADy = yComponent;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetVSAD(double VSAD[2])
{
  this->SetVSAD( VSAD[0], VSAD[1]);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetVSAD(const std::array< double, 2 >& VSAD)
{
  this->SetVSAD( VSAD[0], VSAD[1]);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetScanningSpotSize(const std::array< float, 2 >& size)
{
  this->ScanningSpotSize = size;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetIsocenterToJawsDistanceX(double distance)
{
  this->IsocenterToJawsDistanceX = distance;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetIsocenterToJawsDistanceY(double distance)
{
  this->IsocenterToJawsDistanceY = distance;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetIsocenterToRangeShifterDistance(double distance)
{
  this->IsocenterToRangeShifterDistance = distance;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetIsocenterToMultiLeafCollimatorDistance(double distance)
{
  this->IsocenterToMultiLeafCollimatorDistance = distance;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//---------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::CreateBeamPolyData(vtkPolyData* beamModelPolyData/*=nullptr*/)
{
  if (!beamModelPolyData)
  {
    beamModelPolyData = this->GetPolyData();
  }
  if (!beamModelPolyData)
  {
    vtkErrorMacro("CreateBeamPolyData: Invalid beam node");
    return;
  }

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();

  // Scan spot position map & meterset weights
  vtkMRMLTableNode* scanSpotTableNode = this->GetScanSpotTableNode();
  if (scanSpotTableNode)
  {
    vtkDebugMacro("CreateBeamPolyData: Valid MLC nodes, number of leaves: " << nofLeaves);
  }
  else
  {
    vtkErrorMacro("CreateBeamPolyData: Invalid table node with " \
      "scan spot parameters for a node " 
      << "\"" << this->GetName() << "\"");
    return;
  }
    
  // Scanning spot beam
  if (scanSpotTableNode)
  {
    std::vector<double> positionX, positionY;
    vtkIdType rows = scanSpotTableNode->GetNumberOfRows();
    positionX.resize(rows);
    positionY.resize(rows);
    // copy scan scot map data for processing
    for ( vtkIdType row = 0; row < rows; row++)
    {
      vtkTable* table = scanSpotTableNode->GetTable();
      positionX[row] = table->GetValue( row, 0).ToDouble();
      positionY[row] = table->GetValue( row, 1).ToDouble();
    }
    double beamTopCap = std::min( this->VSADx, this->VSADy);
    double beamBottomCap = -beamTopCap;

    double M1x = (this->VSADx - beamTopCap) / this->VSADx;
    double M1y = (this->VSADy - beamTopCap) / this->VSADy;

    double M2x = (this->VSADx + beamTopCap) / this->VSADx;
    double M2y = (this->VSADy + beamTopCap) / this->VSADy;

    double sigmaX = this->ScanningSpotSize[0] * FWHM_TO_SIGMA;
    double sigmaY = this->ScanningSpotSize[1] * FWHM_TO_SIGMA;

    double borderMinX = *std::min_element( positionX.begin(), positionX.end());
    double borderMaxX = *std::max_element( positionX.begin(), positionX.end());
    double borderMinY = *std::min_element( positionY.begin(), positionY.end());
    double borderMaxY = *std::max_element( positionY.begin(), positionY.end());

    double sigmaX1 = M1x * sigmaX;
    double sigmaY1 = M1y * sigmaY;

    double sigmaX2 = M2x * sigmaX;
    double sigmaY2 = M2y * sigmaY;

    // beam begin cap
    points->InsertPoint( 0, borderMinX - sigmaX1, borderMinY - sigmaY1, beamTopCap);
    points->InsertPoint( 1, borderMinX - sigmaX1, borderMaxY + sigmaY1, beamTopCap);
    points->InsertPoint( 2, borderMaxX + sigmaX1, borderMaxY + sigmaY1, beamTopCap);
    points->InsertPoint( 3, borderMaxX + sigmaX1, borderMinY - sigmaY1, beamTopCap);

    // beam end cap
    points->InsertPoint( 4, borderMinX - sigmaX2, borderMinY - sigmaY2, beamBottomCap);
    points->InsertPoint( 5, borderMinX - sigmaX2, borderMaxY + sigmaY2, beamBottomCap);
    points->InsertPoint( 6, borderMaxX + sigmaX2, borderMaxY + sigmaY2, beamBottomCap);
    points->InsertPoint( 7, borderMaxX + sigmaX2, borderMinY - sigmaY2, beamBottomCap);

    // Add the cap to the top
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(1);
    cellArray->InsertCellPoint(2);
    cellArray->InsertCellPoint(3);

    // Side polygon 1
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(4);
    cellArray->InsertCellPoint(5);
    cellArray->InsertCellPoint(1);

    // Side polygon 2
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(1);
    cellArray->InsertCellPoint(5);
    cellArray->InsertCellPoint(6);
    cellArray->InsertCellPoint(2);

    // Side polygon 3
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(2);
    cellArray->InsertCellPoint(6);
    cellArray->InsertCellPoint(7);
    cellArray->InsertCellPoint(3);

    // Side polygon 4
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(3);
    cellArray->InsertCellPoint(7);
    cellArray->InsertCellPoint(4);
    cellArray->InsertCellPoint(0);

    // Add the cap to the bottom
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(4);
    cellArray->InsertCellPoint(5);
    cellArray->InsertCellPoint(6);
    cellArray->InsertCellPoint(7);
  }
  else // symmetric jaws, asymmetric jaws, MLC
  {
    std::array< double, 3 > distance{ IsocenterToJawsDistanceX, 
      IsocenterToJawsDistanceY, IsocenterToMultiLeafCollimatorDistance };

    double beamTopCap = *std::min_element( distance.begin(), distance.end());
    double beamBottomCap = -beamTopCap;

    double Mx = (this->VSADx - beamTopCap) / this->VSADx;
    double My = (this->VSADy - beamTopCap) / this->VSADy;

    // beam begin cap
    points->InsertPoint( 0, Mx * this->X1Jaw, My * this->Y1Jaw, beamTopCap);
    points->InsertPoint( 1, Mx * this->X1Jaw, My * this->Y2Jaw, beamTopCap);
    points->InsertPoint( 2, Mx * this->X2Jaw, My * this->Y2Jaw, beamTopCap);
    points->InsertPoint( 3, Mx * this->X2Jaw, My * this->Y1Jaw, beamTopCap);

    // beam end cap
    points->InsertPoint( 4, 2. * Mx * this->X1Jaw, 2. * My * this->Y1Jaw, beamBottomCap);
    points->InsertPoint( 5, 2. * Mx * this->X1Jaw, 2. * My * this->Y2Jaw, beamBottomCap);
    points->InsertPoint( 6, 2. * Mx * this->X2Jaw, 2. * My * this->Y2Jaw, beamBottomCap);
    points->InsertPoint( 7, 2. * Mx * this->X2Jaw, 2. * My * this->Y1Jaw, beamBottomCap);

    // Add the cap to the top
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(1);
    cellArray->InsertCellPoint(2);
    cellArray->InsertCellPoint(3);

    // Side polygon 1
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(4);
    cellArray->InsertCellPoint(5);
    cellArray->InsertCellPoint(1);

    // Side polygon 2
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(1);
    cellArray->InsertCellPoint(5);
    cellArray->InsertCellPoint(6);
    cellArray->InsertCellPoint(2);

    // Side polygon 3
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(2);
    cellArray->InsertCellPoint(6);
    cellArray->InsertCellPoint(7);
    cellArray->InsertCellPoint(3);

    // Side polygon 4
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(3);
    cellArray->InsertCellPoint(7);
    cellArray->InsertCellPoint(4);
    cellArray->InsertCellPoint(0);

    // Add the cap to the bottom
    cellArray->InsertNextCell(4);
    cellArray->InsertCellPoint(4);
    cellArray->InsertCellPoint(5);
    cellArray->InsertCellPoint(6);
    cellArray->InsertCellPoint(7);
  }

  beamModelPolyData->SetPoints(points);
  beamModelPolyData->SetPolys(cellArray);
}
