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
vtkMRMLNodeNewMacro(vtkMRMLRTIonBeamNode);

//----------------------------------------------------------------------------
vtkMRMLRTIonBeamNode::vtkMRMLRTIonBeamNode()
  :
  Superclass(),
  VSADx(SAD)
{
  this->VSADx = 2000.0;
  this->VSADy = 2500.0;
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

    if (!strcmp(attName, "VSADx"))
    {
      this->VSADx = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "VSADy"))
    {
      this->VSADy = vtkVariant(attValue).ToDouble();
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
  this->SetVSADx(node->GetVSADx());
  this->SetVSADy(node->GetVSADy());

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
void vtkMRMLRTIonBeamNode::SetVSADy(double VSADyComponent)
{
  this->VSADy = VSADyComponent;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTIonBeamNode::SetVSADx(double VSADxComponent)
{
  this->VSADx = VSADxComponent;
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

  vtkMRMLTableNode* mlcTableNode = nullptr;

  vtkIdType nofLeaves = 0;

  // MLC boundary data
  vtkMRMLDoubleArrayNode* arrayNode = this->GetMLCBoundaryDoubleArrayNode();
  vtkDoubleArray* mlcBoundArray = nullptr;
  if (arrayNode)
  {
    mlcBoundArray = arrayNode->GetArray();
    nofLeaves = mlcBoundArray ? (mlcBoundArray->GetNumberOfTuples() - 1) : 0;
  }

  // MLC position data
  if (nofLeaves)
  {
    mlcTableNode = this->GetMLCPositionTableNode();
    if (mlcTableNode && (mlcTableNode->GetNumberOfRows() == nofLeaves))
    {
      vtkDebugMacro("CreateBeamPolyData: Valid MLC nodes, number of leaves: " << nofLeaves);
    }
    else
    {
      vtkErrorMacro("CreateBeamPolyData: Invalid MLC nodes, or " \
        "number of MLC boundaries and positions are different");
      mlcTableNode = nullptr; // draw beam polydata without MLC
    }
  }

  // static function ptr
  bool(*AreEqual)(double, double) = vtkSlicerRtCommon::AreEqualWithTolerance;

  bool xOpened = !AreEqual( this->X2Jaw, this->X1Jaw);
  bool yOpened = !AreEqual( this->Y2Jaw, this->Y1Jaw);

  // Check that we have MLC with Jaws opening
  if (mlcTableNode && xOpened && yOpened)
  {
    using PointVector = std::vector< std::pair< double, double > >;
    using LeafDataVector = std::vector< std::array< double, 4 > >;
    
    LeafDataVector mlc; // temporary MLC vector (Boundary and Position)
    PointVector side12; // real points for side "1" and "2"

    const char* mlcName = mlcTableNode->GetName();
    bool typeMLCX = !strncmp( "MLCX", mlcName, strlen("MLCX"));
    bool typeMLCY = !strncmp( "MLCY", mlcName, strlen("MLCY"));

    // copy MLC data for easier processing
    for ( vtkIdType leaf = 0; leaf < nofLeaves; leaf++)
    {
      vtkTable* table = mlcTableNode->GetTable();
      double boundBegin = mlcBoundArray->GetTuple1(leaf);
      double boundEnd = mlcBoundArray->GetTuple1(leaf + 1);
      double pos1 = table->GetValue( leaf, 0).ToDouble();
      double pos2 = table->GetValue( leaf, 1).ToDouble();
      
      mlc.push_back({ boundBegin, boundEnd, pos1, pos2 });
    }

    auto firstLeafIterator = mlc.end();
    auto lastLeafIterator = mlc.end();
    double& jawBegin = this->Y1Jaw;
    double& jawEnd = this->Y2Jaw;
    if (typeMLCX)
    {
      jawBegin = this->Y1Jaw;
      jawEnd = this->Y2Jaw;
    }
    else if (typeMLCY)
    {
      jawBegin = this->X1Jaw;
      jawEnd = this->X2Jaw;
    }

    // find first and last opened leaves visible within jaws
    PointVector side1, side2; // temporary vectors to save visible points
    for ( auto it = mlc.begin(); it != mlc.end(); ++it)
    {
      double& pos1 = (*it)[2]; // leaf position "1"
      double& pos2 = (*it)[3]; // leaf position "2"
      bool mlcOpened = !AreEqual( pos1, pos2);
      bool withinJaw = false;
      if (typeMLCX)
      {
        withinJaw = ((pos1 < this->X1Jaw && pos2 >= this->X1Jaw && pos2 <= this->X2Jaw) || 
          (pos1 >= this->X1Jaw && pos1 <= this->X2Jaw && pos2 > this->X2Jaw) || 
          (pos1 <= this->X1Jaw && pos2 >= this->X2Jaw) || 
          (pos1 >= this->X1Jaw && pos1 <= this->X2Jaw && 
            pos2 >= this->X1Jaw && pos2 <= this->X2Jaw));
      }
      else if (typeMLCY)
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
      else if (withinJaw && mlcOpened && firstLeafIterator != mlc.end())
      {
        lastLeafIterator = it;
      }
    }
    
    // iterate through visible leaves to fill temporary points vectors
    if (firstLeafIterator != mlc.end() && lastLeafIterator != mlc.end())
    {
      auto firstLeafIteratorJaws = firstLeafIterator;
      auto lastLeafIteratorJaws = lastLeafIterator;
      // find first and last visible leaves using Jaws data
      for ( auto it = firstLeafIterator; it <= lastLeafIterator; ++it)
      {
        double& bound1 = (*it)[0]; // leaf begin boundary
        double& bound2 = (*it)[1]; // leaf end boundary
        if (bound1 <= jawBegin && bound2 > jawBegin)
        {
          firstLeafIteratorJaws = it;
        }
        else if (bound1 <= jawEnd && bound2 > jawEnd)
        {
          lastLeafIteratorJaws = it;
        }
      }

      // find opened MLC leaves into Jaws opening (logical AND)
      if (firstLeafIteratorJaws != firstLeafIterator)
      {
        firstLeafIterator = std::max( firstLeafIteratorJaws, firstLeafIterator);
      }
      if (lastLeafIteratorJaws != lastLeafIterator)
      {
        lastLeafIterator = std::min( lastLeafIteratorJaws, lastLeafIterator);
      }

      // add points for the visible leaves of side "1" and "2"
      // into side1 and side2 points vectors
      for ( auto it = firstLeafIterator; it <= lastLeafIterator; ++it)
      {
        double& bound1 = (*it)[0]; // leaf begin boundary
        double& bound2 = (*it)[1]; // leaf end boundary
        double& pos1 = (*it)[2]; // leaf position "1"
        double& pos2 = (*it)[3]; // leaf position "2"
        if (typeMLCX)
        {
          side1.push_back({ std::max( pos1, this->X1Jaw), bound1});
          side1.push_back({ std::max( pos1, this->X1Jaw), bound2});
          side2.push_back({ std::min( pos2, this->X2Jaw), bound1});
          side2.push_back({ std::min( pos2, this->X2Jaw), bound2});
        }
        else if (typeMLCY)
        {
          side1.push_back({ bound1, std::max( pos1, this->Y1Jaw)});
          side1.push_back({ bound2, std::max( pos1, this->Y1Jaw)});
          side2.push_back({ bound1, std::min( pos2, this->Y2Jaw)});
          side2.push_back({ bound2, std::min( pos2, this->Y2Jaw)});
        }
      }
      mlc.clear(); // doesn't need anymore

      // intersection between Jaws and MLC boundary (logical AND) lambda
      auto intersectJawsMLC = [ jawBegin, jawEnd, typeMLCX, typeMLCY](PointVector::value_type& point)
      {
        double& leafBoundary = point.second;
        if (typeMLCX) // JawsY and MLCX
        {
          leafBoundary = point.second;
        }
        else if (typeMLCY) // JawsX and MLCY
        {
          leafBoundary = point.first;
        }

        if (leafBoundary <= jawBegin)
        {
          leafBoundary = jawBegin;
        }
        else if (leafBoundary >= jawEnd)
        {
          leafBoundary = jawEnd;
        }
      };
      // apply lambda to side "1"
      std::for_each( side1.begin(), side1.end(), intersectJawsMLC);
      // apply lambda to side "2"
      std::for_each( side2.begin(), side2.end(), intersectJawsMLC);

      // reverse side "2"
      std::reverse( side2.begin(), side2.end());

      // fill real points vector side12 without excessive points from side1 vector
      PointVector::value_type& p = side1.front(); // start point
      double& px = p.first; // x coordinate of p point
      double& py = p.second; // y coordinate of p point
      side12.push_back(p);
      for ( size_t i = 1; i < side1.size() - 1; ++i)
      {
        double& pxNext = side1[i + 1].first; // x coordinate of next point
        double& pyNext = side1[i + 1].second; // y coordinate of next point
        if (!AreEqual( px, pxNext) && !AreEqual( py, pyNext))
        {
          p = side1[i];
          side12.push_back(p);
        }
      }
      side12.push_back(side1.back()); // end point

      // same for the side2 vector
      p = side2.front();
      side12.push_back(p);
      for ( size_t i = 1; i < side2.size() - 1; ++i)
      {
        double& pxNext = side2[i + 1].first;
        double& pyNext = side2[i + 1].second;
        if (!AreEqual( px, pxNext) && !AreEqual( py, pyNext))
        {
          p = side2[i];
          side12.push_back(p);
        }
      }
      side12.push_back(side2.back());
    }
    else
    {
      vtkErrorMacro("CreateBeamPolyData: Unable to calculate MLC visible data");
      return;
    }

    // fill vtk points
    points->InsertPoint( 0, 0, 0, this->SAD); // source

    // side "1" and "2" points vector
    vtkIdType pointIds = 0;
    for ( PointVector::value_type& point : side12)
    {
      double& x = point.first;
      double& y = point.second;
      points->InsertPoint( pointIds + 1, 2. * x, 2. * y, -this->SAD);
      pointIds++;
    }
    side12.clear(); // doesn't need anymore

    // fill cell array for side "1" and "2"
    for ( vtkIdType i = 1; i < pointIds; ++i)
    {
      cellArray->InsertNextCell(3);
      cellArray->InsertCellPoint(0);
      cellArray->InsertCellPoint(i);
      cellArray->InsertCellPoint(i + 1);
    }

    // fill cell connection between side "2" -> side "1"
    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(1);
    cellArray->InsertCellPoint(pointIds);

    // Add the cap to the bottom
    cellArray->InsertNextCell(pointIds);
    for ( vtkIdType i = 1; i <= pointIds; i++)
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
