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
#include <vtkTable.h>
#include <vtkCellArray.h>
#include <vtkAppendPolyData.h>

//------------------------------------------------------------------------------
const char* vtkMRMLRTBeamNode::NEW_BEAM_NODE_NAME_PREFIX = "NewBeam_";
const char* vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX = "_BeamTransform";

//------------------------------------------------------------------------------
static const char* MLC_BOUNDARY_POSITION_REFERENCE_ROLE = "MLCBoundaryAndPositionRef";
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

  this->SourceToJawsDistanceX = 500.;
  this->SourceToJawsDistanceY = 500.;
  this->SourceToMultiLeafCollimatorDistance = 400.;
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
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLIntMacro( BeamNumber, BeamNumber);
  vtkMRMLWriteXMLStringMacro( BeamDescription, BeamDescription);
  vtkMRMLWriteXMLFloatMacro( BeamWeight, BeamWeight);
  vtkMRMLWriteXMLFloatMacro( X1Jaw, X1Jaw);
  vtkMRMLWriteXMLFloatMacro( X2Jaw, X2Jaw);
  vtkMRMLWriteXMLFloatMacro( Y1Jaw, Y1Jaw);
  vtkMRMLWriteXMLFloatMacro( Y2Jaw, Y2Jaw);
  vtkMRMLWriteXMLFloatMacro( SAD, SAD);
  vtkMRMLWriteXMLFloatMacro( SourceToJawsDistanceX, SourceToJawsDistanceX);
  vtkMRMLWriteXMLFloatMacro( SourceToJawsDistanceY, SourceToJawsDistanceY);
  vtkMRMLWriteXMLFloatMacro( SourceToMultiLeafCollimatorDistance, SourceToMultiLeafCollimatorDistance);
  vtkMRMLWriteXMLFloatMacro( GantryAngle, GantryAngle);
  vtkMRMLWriteXMLFloatMacro( CollimatorAngle, CollimatorAngle);
  vtkMRMLWriteXMLFloatMacro( CouchAngle, CouchAngle);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLIntMacro( BeamNumber, BeamNumber);
  vtkMRMLReadXMLStringMacro( BeamDescription, BeamDescription);
  vtkMRMLReadXMLFloatMacro( BeamWeight, BeamWeight);
  vtkMRMLReadXMLFloatMacro( X1Jaw, X1Jaw);
  vtkMRMLReadXMLFloatMacro( X2Jaw, X2Jaw);
  vtkMRMLReadXMLFloatMacro( Y1Jaw, Y1Jaw);
  vtkMRMLReadXMLFloatMacro( Y2Jaw, Y2Jaw);
  vtkMRMLReadXMLFloatMacro( SAD, SAD);
  vtkMRMLReadXMLFloatMacro( SourceToJawsDistanceX, SourceToJawsDistanceX);
  vtkMRMLReadXMLFloatMacro( SourceToJawsDistanceY, SourceToJawsDistanceY);
  vtkMRMLReadXMLFloatMacro( SourceToMultiLeafCollimatorDistance, SourceToMultiLeafCollimatorDistance);
  vtkMRMLReadXMLFloatMacro( GantryAngle, GantryAngle);
  vtkMRMLReadXMLFloatMacro( CollimatorAngle, CollimatorAngle);
  vtkMRMLReadXMLFloatMacro( CouchAngle, CouchAngle);
  vtkMRMLReadXMLEndMacro();
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTBeamNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
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

  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyIntMacro(BeamNumber);
  vtkMRMLCopyStringMacro(BeamDescription);
  vtkMRMLCopyFloatMacro(BeamWeight);
  vtkMRMLCopyFloatMacro(X1Jaw);
  vtkMRMLCopyFloatMacro(X2Jaw);
  vtkMRMLCopyFloatMacro(Y1Jaw);
  vtkMRMLCopyFloatMacro(Y2Jaw);
  vtkMRMLCopyFloatMacro(SourceToJawsDistanceX);
  vtkMRMLCopyFloatMacro(SourceToJawsDistanceY);
  vtkMRMLCopyFloatMacro(SourceToMultiLeafCollimatorDistance);
  vtkMRMLCopyFloatMacro(GantryAngle);
  vtkMRMLCopyFloatMacro(CollimatorAngle);
  vtkMRMLCopyFloatMacro(CouchAngle);
  vtkMRMLCopyEndMacro();

  this->EndModify(disabledModify);

  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CopyContent(vtkMRMLNode *anode, bool deepCopy/*=true*/)
{
  MRMLNodeModifyBlocker blocker(this);
  Superclass::CopyContent( anode, deepCopy);

  vtkMRMLRTBeamNode* node = vtkMRMLRTBeamNode::SafeDownCast(anode);
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
  vtkMRMLCopyFloatMacro(SourceToJawsDistanceX);
  vtkMRMLCopyFloatMacro(SourceToJawsDistanceY);
  vtkMRMLCopyFloatMacro(SourceToMultiLeafCollimatorDistance);
  vtkMRMLCopyFloatMacro(GantryAngle);
  vtkMRMLCopyFloatMacro(CollimatorAngle);
  vtkMRMLCopyFloatMacro(CouchAngle);
  vtkMRMLCopyEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetScene(vtkMRMLScene* scene)
{
  Superclass::SetScene(scene);

  if (!scene)
  {
    return;
  }
  
  if (!this->GetPolyData())
  {
    // Create beam model
    vtkSmartPointer<vtkPolyData> beamModelPolyData = vtkSmartPointer<vtkPolyData>::New();
    this->SetAndObservePolyData(beamModelPolyData);
  }

  this->CreateBeamPolyData();
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintIntMacro(BeamNumber);
  vtkMRMLPrintStringMacro(BeamDescription);
  vtkMRMLPrintFloatMacro(BeamWeight);
  vtkMRMLPrintFloatMacro(X1Jaw);
  vtkMRMLPrintFloatMacro(X2Jaw);
  vtkMRMLPrintFloatMacro(Y1Jaw);
  vtkMRMLPrintFloatMacro(Y2Jaw);
  vtkMRMLPrintFloatMacro(SAD);
  vtkMRMLPrintFloatMacro(SourceToJawsDistanceX);
  vtkMRMLPrintFloatMacro(SourceToJawsDistanceY);
  vtkMRMLPrintFloatMacro(SourceToMultiLeafCollimatorDistance);
  vtkMRMLPrintFloatMacro(GantryAngle);
  vtkMRMLPrintFloatMacro(CollimatorAngle);
  vtkMRMLPrintFloatMacro(CouchAngle);
  vtkMRMLPrintEndMacro();
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
  displayNode->Visibility2DOn();
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
vtkMRMLLinearTransformNode* vtkMRMLRTBeamNode::CreateBeamTransformNode(vtkMRMLScene* scene)
{
  // Create transform node for beam
  vtkNew<vtkMRMLLinearTransformNode> transformNode;
  std::string transformName = std::string(this->GetName()) + BEAM_TRANSFORM_NODE_NAME_POSTFIX;
  transformNode->SetName(transformName.c_str());
  scene->AddNode(transformNode);

  // Hide transform node from subject hierarchy as it is not supposed to be used by the user
  transformNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str(), "1");

  return transformNode;
}

//----------------------------------------------------------------------------
vtkMRMLTableNode* vtkMRMLRTBeamNode::GetMultiLeafCollimatorTableNode()
{
  return vtkMRMLTableNode::SafeDownCast( this->GetNodeReference(MLC_BOUNDARY_POSITION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveMultiLeafCollimatorTableNode(vtkMRMLTableNode* node)
{
  if (node && this->Scene != node->GetScene())
  {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
  }

  this->SetNodeReferenceID(MLC_BOUNDARY_POSITION_REFERENCE_ROLE, (node ? node->GetID() : nullptr));

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
  vtkMRMLMarkupsFiducialNode* poisMarkupsNode = parentPlanNode->CreatePoisMarkupsFiducialNode();
  if (!poisMarkupsNode)
  {
    vtkErrorMacro("GetPlanIsocenterPosition: Failed to access POIs markups node");
    return false;
  }

  poisMarkupsNode->GetNthControlPointPosition(vtkMRMLRTPlanNode::ISOCENTER_FIDUCIAL_INDEX, isocenter);
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
void vtkMRMLRTBeamNode::SetSourceToJawsDistanceX(double distance)
{
  this->SourceToJawsDistanceX = distance;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetSourceToJawsDistanceY(double distance)
{
  this->SourceToJawsDistanceY = distance;
  this->Modified();
  this->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetSourceToMultiLeafCollimatorDistance(double distance)
{
  this->SourceToMultiLeafCollimatorDistance = distance;
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
  this->CreateBeamPolyData();
}

//---------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CreateBeamPolyData(vtkPolyData* beamModelPolyData/*=nullptr*/)
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

  // Check that we have MLC with Jaws opening
  bool polydataAppended = false;
  if (mlcTableNode && xOpened && yOpened)
  {
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
      vtkErrorMacro("CreateBeamPolyData: Unable to calculate MLC visible data");
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
        points->InsertPoint( 0, 0, 0, this->SAD); // source

        // side "1" and "2" points vector
        vtkIdType pointIds = 0;
        for (const MLCVisiblePointVector::value_type& point : side12)
        {
          const double& x = point.first;
          const double& y = point.second;
          points->InsertPoint( pointIds + 1, 2. * x, 2. * y, -this->SAD);
          pointIds++;
        }
        side12.clear(); // doesn't need anymore

        // fill cell array for side "1" and "2"
        for (vtkIdType i = 1; i < pointIds; ++i)
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
        for (vtkIdType i = 1; i <= pointIds; i++)
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
    }
  }
  if (polydataAppended)
  {
    vtkDebugMacro("CreateBeamPolyData: Beam \"" << this->GetName() << "\" with MLC data has been created!");
    return;
  }
 
  // Default beam polydata (no MLC)
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> cellArray;

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

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CreateMLCPointsFromSectionBorder( double jawBegin,
  double jawEnd, bool typeMLCX, const MLCSectionVector::value_type& sectionBorder, 
  MLCVisiblePointVector& side12)
{
  MLCVisiblePointVector side1, side2; // temporary vectors to save visible points

  MLCBoundaryPositionVector::iterator firstLeafIterator = sectionBorder.first;
  MLCBoundaryPositionVector::iterator lastLeafIterator = sectionBorder.second;
  MLCBoundaryPositionVector::iterator firstLeafIteratorJaws = firstLeafIterator;
  MLCBoundaryPositionVector::iterator lastLeafIteratorJaws = lastLeafIterator;

  // find first and last visible leaves using Jaws data
  for (auto it = firstLeafIterator; it <= lastLeafIterator; ++it)
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
  for (auto it = firstLeafIterator; it <= lastLeafIterator; ++it)
  {
    double& bound1 = (*it)[0]; // leaf begin boundary
    double& bound2 = (*it)[1]; // leaf end boundary
    double& pos1 = (*it)[2]; // leaf position "1"
    double& pos2 = (*it)[3]; // leaf position "2"
    if (typeMLCX) // MLCX
    {
      side1.push_back({ std::max( pos1, this->X1Jaw), bound1});
      side1.push_back({ std::max( pos1, this->X1Jaw), bound2});
      side2.push_back({ std::min( pos2, this->X2Jaw), bound1});
      side2.push_back({ std::min( pos2, this->X2Jaw), bound2});
    }
    else // MLCY
    {
      side1.push_back({ bound1, std::max( pos1, this->Y1Jaw)});
      side1.push_back({ bound2, std::max( pos1, this->Y1Jaw)});
      side2.push_back({ bound1, std::min( pos2, this->Y2Jaw)});
      side2.push_back({ bound2, std::min( pos2, this->Y2Jaw)});
    }
  }

  // intersection between Jaws and MLC boundary (logical AND) lambda
  // typeMLCX true for MLCX, false for MLCY 
  auto intersectJawsMLC = [ jawBegin, jawEnd, typeMLCX](MLCVisiblePointVector::value_type& point)
  {
    double leafBoundary = (typeMLCX) ? point.second : point.first;

    if (leafBoundary <= jawBegin)
    {
      leafBoundary = jawBegin;
    }
    else if (leafBoundary >= jawEnd)
    {
      leafBoundary = jawEnd;
    }

    if (typeMLCX) // JawsY and MLCX
    {
      point.second = leafBoundary;
    }
    else // JawsX and MLCY
    {
      point.first = leafBoundary;
    }
  };

  // apply lambda to side "1"
  std::for_each( side1.begin(), side1.end(), intersectJawsMLC);
  // apply lambda to side "2"
  std::for_each( side2.begin(), side2.end(), intersectJawsMLC);

  // reverse side "2"
  std::reverse( side2.begin(), side2.end());

  // fill real points vector side12 without excessive points from side1 vector
  MLCVisiblePointVector::value_type& p = side1.front(); // start point
  double& px = p.first; // x coordinate of p point
  double& py = p.second; // y coordinate of p point
  side12.push_back(p);
  for (size_t i = 1; i < side1.size() - 1; ++i)
  {
    double& pxNext = side1[i + 1].first; // x coordinate of next point
    double& pyNext = side1[i + 1].second; // y coordinate of next point
    if (!vtkSlicerRtCommon::AreEqualWithTolerance( px, pxNext) && 
      !vtkSlicerRtCommon::AreEqualWithTolerance( py, pyNext))
    {
      p = side1[i];
      side12.push_back(p);
    }
  }
  side12.push_back(side1.back()); // end point

  // same for the side2 vector
  p = side2.front();
  side12.push_back(p);
  for (size_t i = 1; i < side2.size() - 1; ++i)
  {
    double& pxNext = side2[i + 1].first;
    double& pyNext = side2[i + 1].second;
    if (!vtkSlicerRtCommon::AreEqualWithTolerance( px, pxNext) && 
      !vtkSlicerRtCommon::AreEqualWithTolerance( py, pyNext))
    {
      p = side2[i];
      side12.push_back(p);
    }
  }
  side12.push_back(side2.back());
}

//----------------------------------------------------------------------------
bool vtkMRMLRTBeamNode::AreEqual( double v1, double v2)
{
  return vtkSlicerRtCommon::AreEqualWithTolerance( v1, v2);
}
