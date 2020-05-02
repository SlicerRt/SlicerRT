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
double FWHM_TO_SIGMA = 1. / (2. * sqrt(2. * log(2.)));

} // namespace

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTIonBeamNode);

//----------------------------------------------------------------------------
vtkMRMLRTIonBeamNode::vtkMRMLRTIonBeamNode()
  :
  Superclass(),
  VSADx(vtkMRMLRTBeamNode::SAD),
  IsocenterToJawsDistanceX(vtkMRMLRTBeamNode::SourceToJawsDistanceX),
  IsocenterToJawsDistanceY(vtkMRMLRTBeamNode::SourceToJawsDistanceY),
  IsocenterToMultiLeafCollimatorDistance(vtkMRMLRTBeamNode::SourceToMultiLeafCollimatorDistance),
  IsocenterToRangeShifterDistance(4000.)
{
  this->VSADx = 6500.0;
  this->VSADy = 6500.0;
  this->IsocenterToJawsDistanceX = 3000.;
  this->IsocenterToJawsDistanceY = 3000.;
  this->IsocenterToMultiLeafCollimatorDistance = 2500.;
  this->ScanningSpotSize[0] = 15.0f;
  this->ScanningSpotSize[1] = 15.0f;
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

  vtkMRMLWriteXMLBeginMacro(of);
  // Write all MRML node attributes into output stream
  vtkMRMLWriteXMLFloatMacro( VSADx, VSADx);
  vtkMRMLWriteXMLFloatMacro( VSADy, VSADy);
  vtkMRMLWriteXMLFloatMacro( IsocenterToJawsDistanceX, IsocenterToJawsDistanceX);
  vtkMRMLWriteXMLFloatMacro( IsocenterToJawsDistanceY, IsocenterToJawsDistanceY);
  vtkMRMLWriteXMLFloatMacro( IsocenterToMultiLeafCollimatorDistance, IsocenterToMultiLeafCollimatorDistance);
  vtkMRMLWriteXMLFloatMacro( IsocenterToRangeShifterDistance, IsocenterToRangeShifterDistance);
  vtkMRMLWriteXMLVectorMacro( ScanningSpotSize, ScanningSpotSize, float, 2);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);
  
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLFloatMacro( VSADx, VSADx);
  vtkMRMLReadXMLFloatMacro( VSADy, VSADy);
  vtkMRMLReadXMLFloatMacro( IsocenterToJawsDistanceX, IsocenterToJawsDistanceX);
  vtkMRMLReadXMLFloatMacro( IsocenterToJawsDistanceY, IsocenterToJawsDistanceY);
  vtkMRMLReadXMLFloatMacro( IsocenterToMultiLeafCollimatorDistance, IsocenterToMultiLeafCollimatorDistance);
  vtkMRMLReadXMLFloatMacro( IsocenterToRangeShifterDistance, IsocenterToRangeShifterDistance);
  vtkMRMLReadXMLVectorMacro( ScanningSpotSize, ScanningSpotSize, float, 2);
  vtkMRMLReadXMLEndMacro();
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTIonBeamNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

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

  this->SetScanningSpotSize(node->GetScanningSpotSize());

  this->SetGantryAngle(node->GetGantryAngle());
  this->SetCollimatorAngle(node->GetCollimatorAngle());
  this->SetCouchAngle(node->GetCouchAngle());

  this->EndModify(disabledModify);
  
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

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintFloatMacro(VSADx);
  vtkMRMLPrintFloatMacro(VSADy);
  vtkMRMLPrintFloatMacro(IsocenterToJawsDistanceX);
  vtkMRMLPrintFloatMacro(IsocenterToJawsDistanceY);
  vtkMRMLPrintFloatMacro(IsocenterToMultiLeafCollimatorDistance);
  vtkMRMLPrintFloatMacro(IsocenterToRangeShifterDistance);
  vtkMRMLPrintVectorMacro( ScanningSpotSize, float, 2);
  vtkMRMLPrintEndMacro();
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

  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
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
void vtkMRMLRTIonBeamNode::SetScanningSpotSize(float size[2])
{
  this->ScanningSpotSize[0] = size[0];
  this->ScanningSpotSize[1] = size[1];
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetScanningSpotSize(const std::array< float, 2 >& size)
{
  this->ScanningSpotSize[0] = size[0];
  this->ScanningSpotSize[1] = size[1];
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
    vtkDebugMacro("CreateBeamPolyData: Valid scan spot parameters table node");
  }
  else
  {
    vtkWarningMacro("CreateBeamPolyData: Invalid or absent table node with " \
      "scan spot parameters for a node " 
      << "\"" << this->GetName() << "\"");
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
