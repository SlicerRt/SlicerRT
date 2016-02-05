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

// SlicerRtCommon includes
#include "PlmCommon.h"
#include "SlicerRtCommon.h"
#include "vtkPolyDataToLabelmapFilter.h"
#include "vtkLabelmapToModelFilter.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Plastimatch includes
#include "image_center.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageResample.h>
#include <vtkGeneralTransform.h>
#include <vtkCollection.h>

//------------------------------------------------------------------------------
static const char* ISOCENTER_FIDUCIAL_REFERENCE_ROLE = "isocenterFiducialRef";
static const char* TARGET_CONTOUR_REFERENCE_ROLE = "targetContourRef";
static const char* MLCPOSITION_REFERENCE_ROLE = "MLCPositionRef";
static const char* DRR_REFERENCE_ROLE = "DRRRef";
static const char* CONTOUR_BEV_REFERENCE_ROLE = "contourBEVRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTBeamNode);

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode::vtkMRMLRTBeamNode()
{
  this->BeamName = 0;
  this->SetBeamName ("Beam");
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

  this->BeamModelNode = NULL;
  this->BeamModelNodeId = NULL;

  this->HideFromEditorsOff();

  // Register parent transform modified event so that the representations
  //   can be put under the same transform node
  vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  vtkObserveMRMLObjectEventsMacro(this, events);
}

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode::~vtkMRMLRTBeamNode()
{
  delete[] this->BeamName;
  delete[] this->BeamDescription;
  delete[] this->TargetSegmentID;
  this->SetAndObserveBeamModelNodeId(NULL);
  this->SetTargetSegmentID(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if (this->GetBeamName() != NULL) 
  {
    of << indent << " BeamName=\"" << this->GetBeamName() << "\"";
  }
  if (this->BeamModelNodeId != NULL) 
  {
    of << indent << " BeamModelNodeId=\"" << this->BeamModelNodeId << "\"";
  }
  if (this->TargetSegmentID != NULL) 
  {
    of << indent << " TargetSegmentID=\"" << this->TargetSegmentID << "\"";
  }
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

    if (!strcmp(attName, "BeamName")) 
    {
      this->SetBeamName(attValue);
    }
    else if (!strcmp(attName, "BeamModelNodeId")) 
    {
      this->SetAndObserveBeamModelNodeId(NULL); // clear any previous observers
      // Do not add observers yet because updates may be wrong before reading all the xml attributes
      // Observers will be added when all the attributes are read and UpdateScene is called
      this->SetBeamModelNodeId(attValue);
    }
    else if (!strcmp(attName, "TargetSegmentID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetTargetSegmentID(ss.str().c_str());
      }
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

  this->SetBeamName( node->GetBeamName() );
  this->SetSmearing( node->GetSmearing() );
  this->SetSAD(node->GetSAD());
  double iso[3];
  node->GetIsocenterPosition (iso);
  this->Isocenter[0] = iso[0];
  this->Isocenter[1] = iso[1];
  this->Isocenter[2] = iso[2];

  // Observers must be removed here, otherwise MRML updates would activate nodes on the undo stack
  this->SetAndObserveBeamModelNodeId( NULL );
  this->SetBeamModelNodeId( node->BeamModelNodeId );

  this->SetTargetSegmentID(node->TargetSegmentID);

  this->SetIsocenterSpec(node->GetIsocenterSpec());

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::UpdateReferences()
{
  Superclass::UpdateReferences();

  if (this->BeamModelNodeId != NULL && this->Scene->GetNodeByID(this->BeamModelNodeId) == NULL)
  {
    this->SetBeamModelNodeId(NULL);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  Superclass::UpdateReferenceID(oldID, newID);

  if (this->BeamModelNodeId && !strcmp(oldID, this->BeamModelNodeId))
  {
    this->SetAndObserveBeamModelNodeId(newID);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);

  this->SetAndObserveBeamModelNodeId(this->BeamModelNodeId);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "BeamModelNodeId:   " << (this->BeamModelNodeId ? this->BeamModelNodeId : "NULL") << "\n";
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
bool vtkMRMLRTBeamNode::BeamNameIs (const std::string& beamName)
{
  return BeamNameIs (beamName.c_str());
}

//----------------------------------------------------------------------------
bool vtkMRMLRTBeamNode::BeamNameIs (const char *beamName)
{
  if (this->GetBeamName() == NULL || beamName == NULL) {
    return false;
  }
  return !strcmp(this->GetBeamName(), beamName);
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
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(TARGET_CONTOUR_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveTargetSegmentationNode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(TARGET_CONTOUR_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkOrientedImageData> vtkMRMLRTBeamNode::GetTargetLabelmap()
{
  vtkSmartPointer<vtkOrientedImageData> targetLabelmap;
  vtkMRMLSegmentationNode* targetSegmentationNode = this->GetTargetSegmentationNode();
  if (!targetSegmentationNode)
  {
    vtkErrorMacro("GetTargetLabelmap: Didn't get target segmentation node");
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
bool vtkMRMLRTBeamNode::ComputeTargetVolumeCenter (double* center)
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
vtkMRMLRTPlanNode* vtkMRMLRTBeamNode::GetRTPlanNode()
{
  vtkMRMLScene *scene = this->GetScene();
  vtkMRMLNode *mnode = NULL;
  vtkMRMLSubjectHierarchyNode *shnode = NULL;
  vtkMRMLSubjectHierarchyNode *shparentnode = NULL;

  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
  {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLSubjectHierarchyNode"))
    {
      shnode = vtkMRMLSubjectHierarchyNode::SafeDownCast(mnode);
      vtkMRMLRTBeamNode *bnode = vtkMRMLRTBeamNode::SafeDownCast(shnode->GetAssociatedNode());
      if (bnode && bnode == this)
      {
        shparentnode = vtkMRMLSubjectHierarchyNode::SafeDownCast(shnode->GetParentNode());
        vtkMRMLRTPlanNode *pnode = vtkMRMLRTPlanNode::SafeDownCast(shparentnode->GetAssociatedNode());
        if (pnode) 
        {
          return pnode;
        }
      }// end if
    }// end if
  }// end for

  return NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveBeamModelNodeId(const char *nodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->BeamModelNode, NULL);
  this->SetBeamModelNodeId(nodeID);
  if (!nodeID)
  {
    return;
  }

  vtkMRMLModelNode *tnode = this->GetBeamModelNode();
  if (tnode)
  {
    tnode->HideFromEditorsOn();
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    events->InsertNextValue(vtkMRMLModelNode::PolyDataModifiedEvent);
    vtkSetAndObserveMRMLObjectEventsMacro(this->BeamModelNode, tnode, events);
  }
  else
  {
    vtkErrorMacro("Failed to set BeamModel node ID!");
    this->SetBeamModelNodeId(NULL);
  }
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLRTBeamNode::GetBeamModelNode()
{
  vtkMRMLModelNode* node = NULL;
  if (this->Scene && this->BeamModelNodeId)
  {
    vtkMRMLNode* snode = this->Scene->GetNodeByID(this->BeamModelNodeId);
    node = vtkMRMLModelNode::SafeDownCast(snode);
  }

  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetIsocenterSpec (vtkMRMLRTBeamNode::IsocenterSpecification isoSpec)
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
void vtkMRMLRTBeamNode::SetIsocenterToTargetCenter ()
{
    double center[3];
    if (this->ComputeTargetVolumeCenter (center))
    {
      this->CopyIsocenterCoordinatesToMarkups (center);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::GetIsocenterPosition (double* iso)
{
  this->CopyIsocenterCoordinatesFromMarkups (iso);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetIsocenterPosition (double* iso)
{
  this->CopyIsocenterCoordinatesToMarkups (iso);
}

//----------------------------------------------------------------------------
const double* vtkMRMLRTBeamNode::GetReferenceDosePointPosition ()
{
  return this->ReferenceDosePoint;
}

//----------------------------------------------------------------------------
double vtkMRMLRTBeamNode::GetReferenceDosePointPosition (int dim)
{
  return this->ReferenceDosePoint[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetReferenceDosePointPosition (const float* position)
{
  for (int d = 0; d < 3; d++) 
  {
    this->ReferenceDosePoint[d] = position[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetReferenceDosePointPosition (const double* position)
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

  vtkSmartPointer<vtkMRMLModelNode> beamModelNode = this->GetBeamModelNode();

  vtkMRMLLinearTransformNode *transformNode 
    = vtkMRMLLinearTransformNode::SafeDownCast(
      this->Scene->GetNodeByID(beamModelNode->GetTransformNodeID()));
  if (transformNode)
  {
    transformNode->SetMatrixTransformToParent(transform->GetMatrix());
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CopyIsocenterCoordinatesToMarkups (double* iso)
{
  vtkMRMLMarkupsFiducialNode* fiducialNode = this->GetIsocenterFiducialNode();
  fiducialNode->SetNthFiducialPositionFromArray(0,iso);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::CopyIsocenterCoordinatesFromMarkups (double* iso)
{
  vtkMRMLMarkupsFiducialNode* fiducialNode = this->GetIsocenterFiducialNode();
  fiducialNode->GetNthFiducialPosition(0,iso);
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* vtkMRMLRTBeamNode::GetSHNode ()
{
  return vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this, this->GetScene());
}
