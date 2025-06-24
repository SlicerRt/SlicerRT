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
#include <vtkTransformPolyDataFilter.h>
#include <vtkDoubleArray.h>
#include <vtkTable.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkAppendPolyData.h>
#include <vtkLine.h>
#include <vtkAssignAttribute.h>

//------------------------------------------------------------------------------
namespace
{

const char* const SCANSPOT_REFERENCE_ROLE = "ScanSpotRef";
static const char* RANGE_SHIFTER_REFERENCE_ROLE = "RangeShifterRef";
constexpr double FWHM_TO_SIGMA = 1. / (2. * sqrt(2. * log(2.)));
constexpr int POINTS_PER_SCANSPOT = 60;

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
  IsocenterToMultiLeafCollimatorDistance(vtkMRMLRTBeamNode::SourceToMultiLeafCollimatorDistance)
{
  this->VSADx = 6500.0;
  this->VSADy = 6500.0;
  this->IsocenterToJawsDistanceX = 3000.;
  this->IsocenterToJawsDistanceY = 3000.;
  this->IsocenterToMultiLeafCollimatorDistance = 2500.;
  this->ScanningSpotSize[0] = 15.0f;
  this->ScanningSpotSize[1] = 15.0f;
  this->ReferencedRangeShifterNumber = -1;
  this->RangeShifterSetting = nullptr;
  this->IsocenterToRangeShifterDistance = 4000.;
  this->RangeShifterWET = -1.;
  this->IsocenterToBlockTrayDistance = -1.;
  this->SnoutID = nullptr;
  this->SnoutPosition = -1.;
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
  vtkMRMLWriteXMLFloatMacro(VSADx, VSADx);
  vtkMRMLWriteXMLFloatMacro(VSADy, VSADy);
  vtkMRMLWriteXMLFloatMacro(IsocenterToJawsDistanceX, IsocenterToJawsDistanceX);
  vtkMRMLWriteXMLFloatMacro(IsocenterToJawsDistanceY, IsocenterToJawsDistanceY);
  vtkMRMLWriteXMLFloatMacro(IsocenterToMultiLeafCollimatorDistance, IsocenterToMultiLeafCollimatorDistance);
  vtkMRMLWriteXMLVectorMacro(ScanningSpotSize, ScanningSpotSize, float, 2);
  vtkMRMLWriteXMLStringMacro(RangeShifterSetting, RangeShifterSetting);
  vtkMRMLWriteXMLIntMacro(ReferencedRangeShifterNumber, ReferencedRangeShifterNumber);
  vtkMRMLWriteXMLFloatMacro(IsocenterToRangeShifterDistance, IsocenterToRangeShifterDistance);
  vtkMRMLWriteXMLFloatMacro(RangeShifterWET, RangeShifterWET);
  vtkMRMLWriteXMLFloatMacro(IsocenterToBlockTrayDistance, IsocenterToBlockTrayDistance);
  vtkMRMLWriteXMLStringMacro(SnoutID, SnoutID);
  vtkMRMLWriteXMLFloatMacro(SnoutPosition, SnoutPosition);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);
  
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLFloatMacro(VSADx, VSADx);
  vtkMRMLReadXMLFloatMacro(VSADy, VSADy);
  vtkMRMLReadXMLFloatMacro(IsocenterToJawsDistanceX, IsocenterToJawsDistanceX);
  vtkMRMLReadXMLFloatMacro(IsocenterToJawsDistanceY, IsocenterToJawsDistanceY);
  vtkMRMLReadXMLFloatMacro(IsocenterToMultiLeafCollimatorDistance, IsocenterToMultiLeafCollimatorDistance);
  vtkMRMLReadXMLVectorMacro(ScanningSpotSize, ScanningSpotSize, float, 2);
  vtkMRMLReadXMLStringMacro(RangeShifterSetting, RangeShifterSetting);
  vtkMRMLReadXMLIntMacro(ReferencedRangeShifterNumber, ReferencedRangeShifterNumber);
  vtkMRMLReadXMLFloatMacro(IsocenterToRangeShifterDistance, IsocenterToRangeShifterDistance);
  vtkMRMLReadXMLFloatMacro(RangeShifterWET, RangeShifterWET);
  vtkMRMLReadXMLFloatMacro(IsocenterToBlockTrayDistance, IsocenterToBlockTrayDistance);
  vtkMRMLReadXMLStringMacro(SnoutID, SnoutID);
  vtkMRMLReadXMLFloatMacro(SnoutPosition, SnoutPosition);
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

  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyIntMacro(BeamNumber);
  vtkMRMLCopyStringMacro(BeamDescription);
  vtkMRMLCopyFloatMacro(BeamWeight);
  vtkMRMLCopyFloatMacro(X1Jaw);
  vtkMRMLCopyFloatMacro(X2Jaw);
  vtkMRMLCopyFloatMacro(Y1Jaw);
  vtkMRMLCopyFloatMacro(Y2Jaw);
  vtkMRMLCopyFloatMacro(VSADx);
  vtkMRMLCopyFloatMacro(VSADy);
  vtkMRMLCopyFloatMacro(IsocenterToJawsDistanceX);
  vtkMRMLCopyFloatMacro(IsocenterToJawsDistanceY);
  vtkMRMLCopyFloatMacro(IsocenterToMultiLeafCollimatorDistance);
  vtkMRMLCopyVectorMacro(ScanningSpotSize, float, 2);
  vtkMRMLCopyFloatMacro(GantryAngle);
  vtkMRMLCopyFloatMacro(CollimatorAngle);
  vtkMRMLCopyFloatMacro(CouchAngle);
  vtkMRMLCopyStringMacro(RangeShifterSetting);
  vtkMRMLCopyIntMacro(ReferencedRangeShifterNumber);
  vtkMRMLCopyFloatMacro(IsocenterToRangeShifterDistance);
  vtkMRMLCopyFloatMacro(RangeShifterWET);
  vtkMRMLCopyFloatMacro(IsocenterToBlockTrayDistance);
  vtkMRMLCopyStringMacro(SnoutID);
  vtkMRMLCopyFloatMacro(SnoutPosition);
  vtkMRMLCopyEndMacro();

  this->EndModify(disabledModify);
  
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::CopyContent(vtkMRMLNode *anode, bool deepCopy/*=true*/)
{
  MRMLNodeModifyBlocker blocker(this);
  Superclass::CopyContent( anode, deepCopy);

  vtkMRMLRTIonBeamNode* node = vtkMRMLRTIonBeamNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyIntMacro(BeamNumber);
  vtkMRMLCopyStringMacro(BeamDescription);
  vtkMRMLCopyFloatMacro(BeamWeight);
  vtkMRMLCopyFloatMacro(X1Jaw);
  vtkMRMLCopyFloatMacro(X2Jaw);
  vtkMRMLCopyFloatMacro(Y1Jaw);
  vtkMRMLCopyFloatMacro(Y2Jaw);
  vtkMRMLCopyFloatMacro(VSADx);
  vtkMRMLCopyFloatMacro(VSADy);
  vtkMRMLCopyFloatMacro(IsocenterToJawsDistanceX);
  vtkMRMLCopyFloatMacro(IsocenterToJawsDistanceY);
  vtkMRMLCopyFloatMacro(IsocenterToMultiLeafCollimatorDistance);
  vtkMRMLCopyVectorMacro(ScanningSpotSize, float, 2);
  vtkMRMLCopyFloatMacro(GantryAngle);
  vtkMRMLCopyFloatMacro(CollimatorAngle);
  vtkMRMLCopyFloatMacro(CouchAngle);
  vtkMRMLCopyStringMacro(RangeShifterSetting);
  vtkMRMLCopyIntMacro(ReferencedRangeShifterNumber);
  vtkMRMLCopyFloatMacro(IsocenterToRangeShifterDistance);
  vtkMRMLCopyFloatMacro(RangeShifterWET);
  vtkMRMLCopyFloatMacro(IsocenterToBlockTrayDistance);
  vtkMRMLCopyStringMacro(SnoutID);
  vtkMRMLCopyFloatMacro(SnoutPosition);
  vtkMRMLCopyEndMacro();
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
  vtkMRMLPrintVectorMacro(ScanningSpotSize, float, 2);
  vtkMRMLPrintStringMacro(RangeShifterSetting);
  vtkMRMLPrintIntMacro(ReferencedRangeShifterNumber);
  vtkMRMLPrintFloatMacro(IsocenterToRangeShifterDistance);
  vtkMRMLPrintFloatMacro(RangeShifterWET);
  vtkMRMLPrintFloatMacro(IsocenterToBlockTrayDistance);
  vtkMRMLPrintStringMacro(SnoutID);
  vtkMRMLPrintFloatMacro(SnoutPosition);
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
vtkMRMLLinearTransformNode* vtkMRMLRTIonBeamNode::CreateBeamTransformNode(vtkMRMLScene* scene)
{
  // Create transform node for ion beam
  return Superclass::CreateBeamTransformNode(scene);
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

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::UpdateScanSpotGeometry(vtkIntArray* highlightedScanSponRows)
{
  this->ScanSpotTableRows = vtkSmartPointer< vtkIntArray >::Take(highlightedScanSponRows);
  this->CreateBeamPolyData();
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
    vtkDebugMacro("CreateBeamPolyData: Invalid or absent table node with " \
      "scan spot parameters for a node " 
      << "\"" << this->GetName() << "\"");
  }

  vtkMRMLTableNode* mlcTableNode = this->GetMultiLeafCollimatorTableNode();

  vtkIdType nofLeafPairs = 0;
  if (mlcTableNode)
  {
    nofLeafPairs = mlcTableNode->GetNumberOfRows() - 1;
    if (nofLeafPairs <= 0)
    {
      vtkWarningMacro("CreateBeamPolyData: Wrong number of leaf pairs in the MLC table node, " \
        "beam model poly data will be created without MLC");
      mlcTableNode = nullptr; // draw beam polydata without MLC
      nofLeafPairs = 0;
    }
  }

  // valid MLC node
  if (nofLeafPairs > 0)
  {
    // MLC boundary and position data
    if (mlcTableNode->GetNumberOfColumns() == 3)
    {
      vtkDebugMacro("CreateBeamPolyData: MLC table node is present, number of leaf pairs = " << nofLeafPairs);
    }
    else
    {
      vtkWarningMacro("CreateBeamPolyData: Wrong number of columns in the MLC table node, " \
        "beam model poly data will be created without MLC");
      mlcTableNode = nullptr; // draw beam polydata without MLC
    }
  }

  bool xOpened = !vtkSlicerRtCommon::AreEqualWithTolerance( this->X2Jaw, this->X1Jaw);
  bool yOpened = !vtkSlicerRtCommon::AreEqualWithTolerance( this->Y2Jaw, this->Y1Jaw);

  // Scanning spot beam
  vtkTable* table = nullptr;
  if (scanSpotTableNode && (table = scanSpotTableNode->GetTable()))
  {
    std::list< int > highlightedRows;
    if (this->ScanSpotTableRows)
    {
      for (vtkIdType i = 0; i < this->ScanSpotTableRows->GetSize(); ++i)
      {
        highlightedRows.push_back(static_cast<int>(this->ScanSpotTableRows->GetTuple1(i)));
      }
    }
    double beamTopCap = std::min( this->VSADx, this->VSADy);
    double beamBottomCap = -500.;//-1. * beamTopCap;

    double M1x = 1.0;//(this->VSADx - beamTopCap) / this->VSADx;
    double M1y = 1.0;//(this->VSADy - beamTopCap) / this->VSADy;

    double M2x = 1.0;//(this->VSADx + beamTopCap) / this->VSADx;
    double M2y = 1.0;//(this->VSADy + beamTopCap) / this->VSADy;

    double sigmaX = this->ScanningSpotSize[0];
    double sigmaY = this->ScanningSpotSize[1];

    double sigmaX1 = M1x * sigmaX;
    double sigmaY1 = M1y * sigmaY;

    double sigmaX2 = M2x * sigmaX;
    double sigmaY2 = M2y * sigmaY;

    vtkIdType rows = scanSpotTableNode->GetNumberOfRows();
    vtkNew<vtkAppendPolyData> append;
    bool polydataAppended = false;
    // ScanSpot map data for processing
    for (vtkIdType row = 0; row < rows; row++)
    {
      vtkNew<vtkPolyData> beamPolyData;
      vtkNew<vtkPoints> points;
      vtkNew<vtkCellArray> cellArray;
      vtkNew<vtkUnsignedCharArray> selection;
      selection->SetNumberOfComponents(1);
      selection->SetName("selected");
      
      bool highlighCell = false;
      if (std::find(highlightedRows.begin(), highlightedRows.end(), row) != highlightedRows.end())
      {
        highlighCell = true;
      }
      // Rainbow table node: green = 85, red = 0
      // set color green for original, red for highlighted
      unsigned char colorData = (highlighCell) ? 0 : 85;

      double positionX = table->GetValue( row, 0).ToDouble();
      double positionY = table->GetValue( row, 1).ToDouble();
      double weight = table->GetValue( row, 2).ToDouble();

      double x1[POINTS_PER_SCANSPOT] = {};
      double y1[POINTS_PER_SCANSPOT] = {};
      double x2[POINTS_PER_SCANSPOT] = {};
      double y2[POINTS_PER_SCANSPOT] = {};

      for (int i = 0; i < POINTS_PER_SCANSPOT; ++i)
      {
        double phi = 2. * vtkMath::Pi() * double(i) / double(POINTS_PER_SCANSPOT);
        x1[i] = sigmaX1 * cos(phi);
        y1[i] = sigmaY1 * sin(phi);
        x2[i] = sigmaX2 * cos(phi);
        y2[i] = sigmaY2 * sin(phi);
      }

      for (int i = 0; i < POINTS_PER_SCANSPOT; ++i)
      {
        // beam begin cap points
        points->InsertPoint( i, positionX - x1[i], positionY - y1[i], 
          beamTopCap);
      }
      for (int i = 0; i < POINTS_PER_SCANSPOT; ++i)
      {
        // beam end cap points
        points->InsertPoint( POINTS_PER_SCANSPOT + i, positionX - x2[i], 
          positionY - y2[i], beamBottomCap);
      }
      // side cells
      for (int i = 0; i < POINTS_PER_SCANSPOT - 1; ++i)
      {
        cellArray->InsertNextCell(4);
        cellArray->InsertCellPoint(i);
        cellArray->InsertCellPoint(POINTS_PER_SCANSPOT + i);
        cellArray->InsertCellPoint(POINTS_PER_SCANSPOT + i + 1);
        cellArray->InsertCellPoint(i + 1);
      }
      // last side cell
      cellArray->InsertNextCell(4);
      cellArray->InsertCellPoint(POINTS_PER_SCANSPOT - 1);
      cellArray->InsertCellPoint(0);
      cellArray->InsertCellPoint(POINTS_PER_SCANSPOT);
      cellArray->InsertCellPoint(2 * POINTS_PER_SCANSPOT - 1);
      
      // beam begin cap
      cellArray->InsertNextCell(POINTS_PER_SCANSPOT + 1);
      for (int i = 0; i < POINTS_PER_SCANSPOT; ++i)
      {
        cellArray->InsertCellPoint(i);
      }
      cellArray->InsertCellPoint(0);

      // beam end cap
      cellArray->InsertNextCell(POINTS_PER_SCANSPOT + 1);
      for (int i = POINTS_PER_SCANSPOT; i < 2 * POINTS_PER_SCANSPOT; ++i)
      {
        cellArray->InsertCellPoint(i);
      }
      cellArray->InsertCellPoint(POINTS_PER_SCANSPOT);

      beamPolyData->SetPoints(points);
      beamPolyData->SetPolys(cellArray);

      // fill selection scalars: green for original, red for highlighted
      for(int i = 0; i < cellArray->GetNumberOfCells(); ++i)
      {        
        selection->InsertNextTuple1(colorData);
      }
      // Add scalars to show highlighted scan spot
      beamPolyData->GetCellData()->SetScalars(selection);

      if (!vtkSlicerRtCommon::AreEqualWithTolerance( weight, 0.0))
      {
        append->AddInputData(beamPolyData);
        polydataAppended = true;
      }
      else
      {
        vtkNew<vtkUnsignedCharArray> lineSelection;
        lineSelection->SetNumberOfComponents(1);
        lineSelection->SetName("selected");
        // fill selection scalar: green for original, red for highlighted
        lineSelection->InsertNextTuple1(colorData);

        double p0[3] = {positionX, positionY, beamTopCap};
        double p1[3] = {positionX, positionY, beamBottomCap};
        // store points in vtkPoints
        vtkNew<vtkPoints> linePoints;
        linePoints->InsertNextPoint(p0);
        linePoints->InsertNextPoint(p1);
        vtkNew<vtkCellArray> lineCell;
        // create a line cell
        vtkNew<vtkLine> line;
        line->GetPointIds()->SetId(0, 0);
        line->GetPointIds()->SetId(1, 1);
        lineCell->InsertNextCell(line);
        // Create a polydata to store everything in
        vtkNew<vtkPolyData> linesPolyData;
        // Add the points to the dataset
        linesPolyData->SetPoints(linePoints);
        // Add the lines to the dataset
        linesPolyData->SetLines(lineCell);
        // Add scalar to show highlighted scan spot
        beamPolyData->GetCellData()->SetScalars(lineSelection);
        append->AddInputData(linesPolyData);
        polydataAppended = true;
      }
    }

    if (polydataAppended)
    {
      append->Update();
      beamModelPolyData->DeepCopy(append->GetOutput());
      vtkDebugMacro("CreateBeamPolyData: Beam \"" << this->GetName() << "\" with ScanSpot data has been created!");
      return;
    }
  }
  else if (mlcTableNode && xOpened && yOpened) // MLC with opened Jaws
  {
    bool polydataAppended = false;
    MLCBoundaryPositionVector mlc;

    const char* mlcName = mlcTableNode->GetName();
    bool typeMLCX = !strncmp( "MLCX", mlcName, strlen("MLCX")); // MLCX by default
    bool typeMLCY = !strncmp( "MLCY", mlcName, strlen("MLCY"));
    if (typeMLCY && !typeMLCX)
    {
      typeMLCX = false;
    }

    // copy MLC data for easier processing
    for (vtkIdType leafPair = 0; leafPair < nofLeafPairs; leafPair++)
    {
      vtkTable* table = mlcTableNode->GetTable();
      double boundBegin = table->GetValue( leafPair, 0).ToDouble();
      double boundEnd = table->GetValue( leafPair + 1, 0).ToDouble();
      double pos1 = table->GetValue( leafPair, 1).ToDouble();
      double pos2 = table->GetValue( leafPair, 2).ToDouble();
      
      mlc.push_back({ boundBegin, boundEnd, pos1, pos2 });
    }

    auto firstLeafIterator = mlc.end();
    auto lastLeafIterator = mlc.end();
    double& jawBegin = this->Y1Jaw;
    double& jawEnd = this->Y2Jaw;
    if (typeMLCX) // MLCX
    {
      jawBegin = this->Y1Jaw;
      jawEnd = this->Y2Jaw;
    }
    else // MLCY
    {
      jawBegin = this->X1Jaw;
      jawEnd = this->X2Jaw;
    }

    // find first and last opened leaves visible within jaws
    // and fill sections vector for further processing
    MLCSectionVector sections; // sections (first & last leaf iterator) of opened MLC
    for (auto it = mlc.begin(); it != mlc.end(); ++it)
    {
      double& bound1 = (*it)[0]; // leaf pair boundary begin
      double& bound2 = (*it)[1]; // leaf pair boundary end
      double& pos1 = (*it)[2]; // leaf position "1"
      double& pos2 = (*it)[3]; // leaf position "2"
      // if leaf pair is outside the jaws, then it is closed
      bool mlcOpened = (bound2 < jawBegin || bound1 > jawEnd) ? false : !vtkSlicerRtCommon::AreEqualWithTolerance( pos1, pos2);
      bool withinJaw = false;
      if (typeMLCX) // MLCX
      {
        withinJaw = ((pos1 < this->X1Jaw && pos2 >= this->X1Jaw && pos2 <= this->X2Jaw) || 
          (pos1 >= this->X1Jaw && pos1 <= this->X2Jaw && pos2 > this->X2Jaw) || 
          (pos1 <= this->X1Jaw && pos2 >= this->X2Jaw) || 
          (pos1 >= this->X1Jaw && pos1 <= this->X2Jaw && 
            pos2 >= this->X1Jaw && pos2 <= this->X2Jaw));
      }
      else // MLCY
      {
        withinJaw = ((pos1 < this->Y1Jaw && pos2 >= this->Y1Jaw && pos2 <= this->Y2Jaw) || 
          (pos1 >= this->Y1Jaw && pos1 <= this->Y2Jaw && pos2 > this->Y2Jaw) || 
          (pos1 <= this->Y1Jaw && pos2 >= this->Y2Jaw) || 
          (pos1 >= this->Y1Jaw && pos1 <= this->Y2Jaw && 
            pos2 >= this->Y1Jaw && pos2 <= this->Y2Jaw));
      }

      if (withinJaw && mlcOpened && firstLeafIterator == mlc.end())
      {
        firstLeafIterator = it;
      }
      if (withinJaw && mlcOpened && firstLeafIterator != mlc.end())
      {
        lastLeafIterator = it;
      }
      // check if next leaf pair is not the last
      if (it != mlc.end() - 1)
      {
        auto next_it = it + 1;
        double& next_pos1 = (*next_it)[2]; // position "1" of the next leaf
        double& next_pos2 = (*next_it)[3]; // position "2" of the next leaf
        // if there is no open space between neighbors => start new section
        if (mlcOpened && (next_pos1 > pos2 || next_pos2 < pos1))
        {
          mlcOpened = false;
        }
      }
      if (firstLeafIterator != mlc.end() && lastLeafIterator != mlc.end() && !mlcOpened)
      {
        sections.push_back({ firstLeafIterator, lastLeafIterator});
        firstLeafIterator = mlc.end();
        lastLeafIterator = mlc.end();
      }
    }

    if (!sections.size()) // no visible sections
    {
      vtkWarningMacro("CreateBeamPolyData: Unable to calculate MLC visible data");
      return;
    }

    // append one or more visible sections to beam model poly data
    vtkNew<vtkAppendPolyData> append;
    for (const MLCSectionVector::value_type& section : sections)
    {
      if (section.first != mlc.end() && section.second != mlc.end())
      {
        vtkNew<vtkPolyData> beamPolyData;
        vtkNew<vtkPoints> points;
        vtkNew<vtkCellArray> cellArray;

        MLCVisiblePointVector side12; // real points for side "1" and "2"
        CreateMLCPointsFromSectionBorder( jawBegin, jawEnd, typeMLCX, section, side12);

        // fill vtk points
        double cy1 = (this->VSADy + this->IsocenterToMultiLeafCollimatorDistance) / this->VSADy;
        double cx1 = (this->VSADx + this->IsocenterToMultiLeafCollimatorDistance) / this->VSADx;
        double cy = (this->VSADy - this->IsocenterToMultiLeafCollimatorDistance) / this->VSADy;
        double cx = (this->VSADx - this->IsocenterToMultiLeafCollimatorDistance) / this->VSADx;

        // side "1" and "2" points vector
        vtkIdType pointIds = 0;
        // points on MLC side
        vtkIdType nofPoints = side12.size();
        for (const MLCVisiblePointVector::value_type& point : side12)
        {
          const double& x = point.first;
          const double& y = point.second;
          double x_ = x * cx;
          double y_ = y * cy;
          points->InsertPoint( pointIds++, x_, y_, this->IsocenterToMultiLeafCollimatorDistance);
        }

        // points on the opposite (from isocenter) side
        for (const MLCVisiblePointVector::value_type& point : side12)
        {
          const double& x = point.first;
          const double& y = point.second;

          double x_ = x * cx1;
          double y_ = y * cy1;
          points->InsertPoint( pointIds++, x_, y_, -500. /*-this->IsocenterToMultiLeafCollimatorDistance*/);
        }
        side12.clear(); // doesn't need anymore

        // fill cell array for side "1" and "2"
        for (vtkIdType i = 0; i < nofPoints - 1; ++i)
        {
          cellArray->InsertNextCell(4);
          cellArray->InsertCellPoint(i);
          cellArray->InsertCellPoint(i + 1);
          cellArray->InsertCellPoint(i + nofPoints + 1);
          cellArray->InsertCellPoint(i + nofPoints);
        }

        // fill cell connection between side "2" -> side "1"
        cellArray->InsertNextCell(4);
        cellArray->InsertCellPoint(0);
        cellArray->InsertCellPoint(nofPoints);
        cellArray->InsertCellPoint(2 * nofPoints - 1);
        cellArray->InsertCellPoint(nofPoints - 1);

        // Add the cap to the bottom
        cellArray->InsertNextCell(nofPoints);
        for (vtkIdType i = 0; i < nofPoints; i++)
        {
          cellArray->InsertCellPoint(i);
        }

        // Add the cap to the top
        cellArray->InsertNextCell(nofPoints);
        for (vtkIdType i = nofPoints; i < 2 * nofPoints; i++)
        {
          cellArray->InsertCellPoint(i);
        }

        beamPolyData->SetPoints(points);
        beamPolyData->SetPolys(cellArray);

        // append section to form final polydata
        append->AddInputData(beamPolyData);
        polydataAppended = true;
      }
    }
    if (polydataAppended)
    {
      append->Update();
      beamModelPolyData->DeepCopy(append->GetOutput());
      return;
    }
  }

  // Default beam polydata (symmetric or asymmetric jaws, no ScanSpot, no MLC)
  double beamTopCap = std::min( IsocenterToJawsDistanceX, IsocenterToJawsDistanceY);
  double beamBottomCap = -500.;//-beamTopCap;

  double Mx = (this->VSADx - beamTopCap) / this->VSADx;
  double My = (this->VSADy - beamTopCap) / this->VSADy;
  double Mx1 = (this->VSADx + beamTopCap) / this->VSADx;
  double My1 = (this->VSADy + beamTopCap) / this->VSADy;

  // beam begin cap
  points->InsertPoint( 0, Mx * this->X1Jaw, My * this->Y1Jaw, beamTopCap);
  points->InsertPoint( 1, Mx * this->X1Jaw, My * this->Y2Jaw, beamTopCap);
  points->InsertPoint( 2, Mx * this->X2Jaw, My * this->Y2Jaw, beamTopCap);
  points->InsertPoint( 3, Mx * this->X2Jaw, My * this->Y1Jaw, beamTopCap);

  // beam end cap
  points->InsertPoint( 4, Mx1 * this->X1Jaw, My1 * this->Y1Jaw, beamBottomCap);
  points->InsertPoint( 5, Mx1 * this->X1Jaw, My1 * this->Y2Jaw, beamBottomCap);
  points->InsertPoint( 6, Mx1 * this->X2Jaw, My1 * this->Y2Jaw, beamBottomCap);
  points->InsertPoint( 7, Mx1 * this->X2Jaw, My1 * this->Y1Jaw, beamBottomCap);

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

  beamModelPolyData->SetPoints(points);
  beamModelPolyData->SetPolys(cellArray);
}

//----------------------------------------------------------------------------
vtkMRMLRTIonRangeShifterNode* vtkMRMLRTIonBeamNode::GetRangeShifterNode()
{
  return vtkMRMLRTIonRangeShifterNode::SafeDownCast( this->GetNodeReference(RANGE_SHIFTER_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetAndObserveRangeShifterNode(vtkMRMLRTIonRangeShifterNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
  }

  this->SetNodeReferenceID(RANGE_SHIFTER_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}
