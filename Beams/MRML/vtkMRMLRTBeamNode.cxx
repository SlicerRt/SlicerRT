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

// SlicerRt includes
#include "PlmCommon.h"
#include "SlicerRtCommon.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Plastimatch includes
#include "image_center.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageResample.h>
#include <vtkGeneralTransform.h>
#include <vtkCollection.h>
#include <vtkDoubleArray.h>
#include <vtkCellArray.h>

//------------------------------------------------------------------------------
const char* vtkMRMLRTBeamNode::NEW_BEAM_NODE_NAME_PREFIX = "NewBeam_";

//------------------------------------------------------------------------------
static const char* ISOCENTER_FIDUCIAL_REFERENCE_ROLE = "isocenterFiducialRef";
static const char* TARGET_SEGMENTATION_REFERENCE_ROLE = "targetContourRef";
static const char* MLCPOSITION_REFERENCE_ROLE = "MLCPositionRef";
static const char* DRR_REFERENCE_ROLE = "DRRRef";
static const char* CONTOUR_BEV_REFERENCE_ROLE = "contourBEVRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTBeamNode);

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode::vtkMRMLRTBeamNode()
{
  this->BeamNumber = -1;
  this->BeamDescription = 0;

  this->TargetSegmentID = 0;

  this->BeamType = Static;
  this->RadiationType = Proton;
  this->CollimatorType = SquareHalfMM;

  this->NominalEnergy = 80.0;
  this->NominalmA = 1.0;
  this->BeamOnTime = 0.0;

  this->IsocenterSpec = CenterOfTarget;
  this->Isocenter[0] = 0.0;
  this->Isocenter[1] = 0.0;
  this->Isocenter[2] = 0.0;  
  this->ReferenceDosePoint[0] = 0.0;
  this->ReferenceDosePoint[1] = 0.0;
  this->ReferenceDosePoint[2] = 0.0;  

  this->X1Jaw = 100;
  this->X2Jaw = 100;
  this->Y1Jaw = 100;
  this->Y2Jaw = 100;

  this->GantryAngle = 0;
  this->CollimatorAngle = 0;
  this->CouchAngle = 0;
  this->Smearing = 0;

  this->SAD = 2000.0;
  this->BeamWeight = 1.0;

  // Register parent transform modified event so that the representations
  //   can be put under the same transform node
  vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  vtkObserveMRMLObjectEventsMacro(this, events);
}

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode::~vtkMRMLRTBeamNode()
{
  this->SetBeamDescription(NULL);
  this->SetTargetSegmentID(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if (this->TargetSegmentID != NULL) 
  {
    of << indent << " TargetSegmentID=\"" << this->TargetSegmentID << "\"";
  }
  //TODO: Beam parameters
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

  while (*atts != NULL) 
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "TargetSegmentID")) 
    {
      std::stringstream ss;
      ss << attValue;
      this->SetTargetSegmentID(ss.str().c_str());
    }
    //TODO: Beam parameters
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTBeamNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLRTBeamNode *node = (vtkMRMLRTBeamNode *) anode;

  //TODO: Beam parameters
  this->SetSmearing( node->GetSmearing() );
  this->SetSAD(node->GetSAD());
  double iso[3];
  node->GetIsocenterPosition (iso);
  this->Isocenter[0] = iso[0];
  this->Isocenter[1] = iso[1];
  this->Isocenter[2] = iso[2];

  this->SetTargetSegmentID(node->TargetSegmentID);

  this->SetIsocenterSpec(node->GetIsocenterSpec());

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  //TODO: Beam parameters
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (!this->Scene)
  {
    vtkErrorMacro("ProcessMRMLEvents: Invalid MRML scene!");
    return;
  }
  if (this->Scene->IsBatchProcessing())
  {
    return;
  }

  if (eventID == vtkMRMLMarkupsNode::PointModifiedEvent)
  {
    // Update the model
    this->UpdateBeamTransform();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkMRMLRTBeamNode::GetIsocenterFiducialNode()
{
  return vtkMRMLMarkupsFiducialNode::SafeDownCast( this->GetNodeReference(ISOCENTER_FIDUCIAL_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveIsocenterFiducialNode(vtkMRMLMarkupsFiducialNode* node)
{
  this->SetNodeReferenceID(ISOCENTER_FIDUCIAL_REFERENCE_ROLE, (node ? node->GetID() : NULL));

  if (node)
  {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLMarkupsNode::PointModifiedEvent);
    vtkSetAndObserveMRMLObjectEventsMacro(node, node, events);
  }
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLRTBeamNode::GetTargetSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(TARGET_SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveTargetSegmentationNode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(TARGET_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkOrientedImageData> vtkMRMLRTBeamNode::GetTargetLabelmap()
{
  vtkSmartPointer<vtkOrientedImageData> targetLabelmap;
  vtkMRMLSegmentationNode* targetSegmentationNode = this->GetTargetSegmentationNode();
  if (!targetSegmentationNode)
  {
    vtkErrorMacro("GetTargetLabelmap: Failed to get target segmentation node");
    return targetLabelmap;
  }

  vtkSegmentation *segmentation = targetSegmentationNode->GetSegmentation();
  if (!segmentation)
  {
    vtkErrorMacro("GetTargetLabelmap: Failed to get segmentation");
    return targetLabelmap;
  }

  vtkSegment *segment = segmentation->GetSegment(this->GetTargetSegmentID());
  if (!segment) 
  {
    vtkErrorMacro("GetTargetLabelmap: Failed to get segment");
    return targetLabelmap;
  }

  //segmentationNode->GetImageData ();
  if (segmentation->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    targetLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
    targetLabelmap->DeepCopy( vtkOrientedImageData::SafeDownCast(
        segment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) ) );
  }
  else
  {
    // Need to convert
    targetLabelmap = vtkSmartPointer<vtkOrientedImageData>::Take(
      vtkOrientedImageData::SafeDownCast(
        vtkSlicerSegmentationsModuleLogic::CreateRepresentationForOneSegment(
          segmentation,
          this->GetTargetSegmentID(),
          vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName())));
    if (!targetLabelmap.GetPointer())
    {
      std::string errorMessage("Failed to convert target segment into binary labelmap");
      vtkErrorMacro("GetTargetLabelmap: " << errorMessage);
      return targetLabelmap;
    }
  }

  // Apply parent transformation nodes if necessary
  if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(targetSegmentationNode, targetLabelmap))
  {
    std::string errorMessage("Failed to apply parent transformation to target segment!");
    vtkErrorMacro("GetTargetLabelmap: " << errorMessage);
    return targetLabelmap;
  }
  return targetLabelmap;
}

//----------------------------------------------------------------------------
bool vtkMRMLRTBeamNode::ComputeTargetVolumeCenter(double* center)
{
  if (!this->Scene)
  {
    vtkErrorMacro("ComputeTargetVolumeCenter: Invalid MRML scene");
    return false;
  }

  // Get a labelmap for the target
  vtkSmartPointer<vtkOrientedImageData> targetLabelmap = this->GetTargetLabelmap();
  if (targetLabelmap == NULL)
  {
    return false;
  }

  // Convert inputs to plm image
  Plm_image::Pointer plmTgt 
    = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(targetLabelmap);
  if (!plmTgt)
  {
    std::string errorMessage("Failed to convert reference segment labelmap into Plm_image");
    vtkErrorMacro("ComputeTargetVolumeCenter: " << errorMessage);
    return false;
  }

  // Compute image center
  Image_center ic;
  ic.set_image(plmTgt);
  ic.run();
  itk::Vector<double,3> com = ic.get_image_center_of_mass();

  // Copy to output argument, and convert LPS -> RAS
  center[0] = -com[0];
  center[1] = -com[1];
  center[2] = com[2];
  return true;
}

//----------------------------------------------------------------------------
vtkMRMLDoubleArrayNode* vtkMRMLRTBeamNode::GetMLCPositionDoubleArrayNode()
{
  return vtkMRMLDoubleArrayNode::SafeDownCast( this->GetNodeReference(MLCPOSITION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode* node)
{
  this->SetNodeReferenceID(MLCPOSITION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLRTBeamNode::GetDRRVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(DRR_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveDRRVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(DRR_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLRTBeamNode::GetContourBEVVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(CONTOUR_BEV_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveContourBEVVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(CONTOUR_BEV_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode* vtkMRMLRTBeamNode::GetPlanNode()
{
  vtkMRMLSubjectHierarchyNode* beamShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this);
  if (!beamShNode)
  {
    vtkErrorMacro("GetRTPlanNode: Subject hierarchy must be enabled for this operation!");
    return NULL;
  }

  if (!beamShNode->GetParentNode())
  {
    vtkErrorMacro("GetRTPlanNode: Beam node " << this->Name << " does not have a subject hierarchy parent, so cannot access RT plan!");
    return NULL;
  }

  vtkMRMLSubjectHierarchyNode* planShNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(beamShNode->GetParentNode());
  return vtkMRMLRTPlanNode::SafeDownCast(planShNode->GetAssociatedNode());
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetIsocenterSpec(vtkMRMLRTBeamNode::IsocenterSpecification isoSpec)
{
  if (isoSpec == this->GetIsocenterSpec())
  {
    return;
  }
  if (isoSpec == CenterOfTarget)
  {
    this->SetIsocenterToTargetCenter();
  }

  this->IsocenterSpec = isoSpec;
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetIsocenterToTargetCenter()
{
    double center[3];
    if (this->ComputeTargetVolumeCenter(center))
    {
      this->CopyIsocenterCoordinatesToMarkups(center);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::GetIsocenterPosition(double* iso)
{
  this->CopyIsocenterCoordinatesFromMarkups(iso);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetIsocenterPosition(double* iso)
{
  this->CopyIsocenterCoordinatesToMarkups(iso);
}

//----------------------------------------------------------------------------
const double* vtkMRMLRTBeamNode::GetReferenceDosePointPosition()
{
  return this->ReferenceDosePoint;
}

//----------------------------------------------------------------------------
double vtkMRMLRTBeamNode::GetReferenceDosePointPosition(int dim)
{
  return this->ReferenceDosePoint[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetReferenceDosePointPosition(const float* position)
{
  for (int d = 0; d < 3; d++) 
  {
    this->ReferenceDosePoint[d] = position[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetReferenceDosePointPosition(const double* position)
{
  for (int d = 0; d < 3; d++) 
  {
    this->ReferenceDosePoint[d] = position[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::UpdateBeamTransform()
{
  if (!this->Scene)
  {
    vtkErrorMacro ("UpdateBeamTransform: Invalid MRML scene");
    return;
  }

  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = this->GetIsocenterFiducialNode();
  if (!isocenterMarkupsNode)
  {
    vtkErrorMacro ("UpdateBeamTransform: isocenterMarkupNode is not initialized");
    return;
  }

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(this->GetGantryAngle());
  transform->RotateY(this->GetCollimatorAngle());
  transform->RotateX(-90);

  vtkDebugMacro ("Gantry angle update to " << this->GetGantryAngle());

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  isocenterMarkupsNode->GetNthFiducialPosition(0,isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkMRMLLinearTransformNode *transformNode 
    = vtkMRMLLinearTransformNode::SafeDownCast(
      this->Scene->GetNodeByID(this->GetTransformNodeID()));
  if (transformNode)
  {
    transformNode->SetMatrixTransformToParent(transform->GetMatrix());
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CopyIsocenterCoordinatesToMarkups(double* iso)
{
  vtkMRMLMarkupsFiducialNode* fiducialNode = this->GetIsocenterFiducialNode();
  fiducialNode->SetNthFiducialPositionFromArray(0,iso);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CopyIsocenterCoordinatesFromMarkups(double* iso)
{
  vtkMRMLMarkupsFiducialNode* fiducialNode = this->GetIsocenterFiducialNode();
  fiducialNode->GetNthFiducialPosition(0,iso);
}

//---------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CreateDefaultBeamModel()
{
  if (!this->Scene)
  {
    vtkErrorMacro ("CreateDefaultBeamModel: Invalid MRML scene");
    return;
  }

  // Create beam model
  vtkSmartPointer<vtkPolyData> beamModelPolyData = this->CreateBeamPolyData();

  // Transform for visualization
  //TODO: Awful names for transforms. They should be barToFooTransform
  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(0);
  transform->RotateY(0);
  transform->RotateX(-90);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  transform2->Translate(0.0, 0.0, 0.0);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  // Create transform node for beam
  vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  transformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->Scene->AddNode(transformNode));
  transformNode->SetMatrixTransformToParent(transform->GetMatrix());
  transformNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str(), "1");

  // Create model node for beam
  vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  beamModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->Scene->AddNode(beamModelDisplayNode));
  beamModelDisplayNode->SetColor(0.0, 1.0, 0.2);
  beamModelDisplayNode->SetOpacity(0.3);
  beamModelDisplayNode->SetBackfaceCulling(0); // Disable backface culling to make the back side of the contour visible as well
  beamModelDisplayNode->VisibilityOn(); 
  beamModelDisplayNode->SliceIntersectionVisibilityOn();

  // Setup beam model node
  this->SetAndObservePolyData(beamModelPolyData);
  this->SetAndObserveTransformNodeID(transformNode->GetID());
  this->SetAndObserveDisplayNodeID(beamModelDisplayNode->GetID());
}

//---------------------------------------------------------------------------
void vtkMRMLRTBeamNode::UpdateBeamGeometry()
{
  vtkSmartPointer<vtkPolyData> beamModelPolyData = this->CreateBeamPolyData();
  this->SetAndObservePolyData(beamModelPolyData);
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkMRMLRTBeamNode::CreateBeamPolyData()
{
  vtkSmartPointer<vtkPolyData> beamModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();

  vtkMRMLDoubleArrayNode* mlcArrayNode = this->GetMLCPositionDoubleArrayNode();
  if (mlcArrayNode)
  {
    // First we extract the shape of the MLC
    int X2count = this->X2Jaw/10;
    int X1count = this->X1Jaw/10;
    int numLeavesVisible = X2count - (-X1count); // Calculate the number of leaves visible
    int numPointsEachSide = numLeavesVisible *2;

    double Y2LeafPosition[40];
    double Y1LeafPosition[40];

    // Calculate Y2 first
    for (int i = X2count; i >= -X1count; i--)
    {
      double leafPosition = mlcArrayNode->GetArray()->GetComponent(-(i-20), 1);
      if (leafPosition < this->Y2Jaw)
      {
        Y2LeafPosition[-(i-20)] = leafPosition;
      }
      else
      {
        Y2LeafPosition[-(i-20)] = this->Y2Jaw;
      }
    }
    // Calculate Y1 next
    for (int i = X2count; i >= -X1count; i--)
    {
      double leafPosition = mlcArrayNode->GetArray()->GetComponent(-(i-20), 0);
      if (leafPosition < this->Y1Jaw)
      {
        Y1LeafPosition[-(i-20)] = leafPosition;
      }
      else
      {
        Y1LeafPosition[-(i-20)] = this->Y1Jaw;
      }
    }

    // Create beam model
    points->InsertPoint(0,0,0,this->SAD);

    int count = 1;
    for (int i = X2count; i > -X1count; i--)
    {
      points->InsertPoint(count,-Y2LeafPosition[-(i-20)]*2, i*10*2, -this->SAD );
      count ++;
      points->InsertPoint(count,-Y2LeafPosition[-(i-20)]*2, (i-1)*10*2, -this->SAD );
      count ++;
    }

    for (int i = -X1count; i < X2count; i++)
    {
      points->InsertPoint(count,Y1LeafPosition[-(i-20)]*2, i*10*2, -this->SAD );
      count ++;
      points->InsertPoint(count,Y1LeafPosition[-(i-20)]*2, (i+1)*10*2, -this->SAD );
      count ++;
    }

    for (int i = 1; i <numPointsEachSide; i++)
    {
      cellArray->InsertNextCell(3);
      cellArray->InsertCellPoint(0);
      cellArray->InsertCellPoint(i);
      cellArray->InsertCellPoint(i+1);
    }
    // Add connection between Y2 and Y1
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

    // Add connection between Y2 and Y1
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
    points->InsertPoint(0,0,0,SAD);
    points->InsertPoint(1,-2*this->Y2Jaw, -2*this->X2Jaw, -this->SAD );
    points->InsertPoint(2,-2*this->Y2Jaw,  2*this->X1Jaw, -this->SAD );
    points->InsertPoint(3, 2*this->Y1Jaw,  2*this->X1Jaw, -this->SAD );
    points->InsertPoint(4, 2*this->Y1Jaw, -2*this->X2Jaw, -this->SAD );

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

  return beamModelPolyData;
}
