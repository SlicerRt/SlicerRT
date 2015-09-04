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
#include "SlicerRtCommon.h"
#include "vtkPolyDataToLabelmapFilter.h"
#include "vtkLabelmapToModelFilter.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanHierarchyNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLSegmentationNode.h"
#include "vtkRTBeamData.h"

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
  this->RadiationType = Proton;

  this->BeamType = Static;
  this->Isocenter[0] = 0.0;
  this->Isocenter[1] = 0.0;
  this->Isocenter[2] = 0.0;  

  this->NominalEnergy = 80.0;
  this->NominalmA = 1.0;
  this->BeamOnTime = 0.0;
  this->X1Jaw = 100;
  this->X2Jaw = 100;
  this->Y1Jaw = 100;
  this->Y2Jaw = 100;

  this->GantryAngle = 0;
  this->CollimatorAngle = 0;
  this->CouchAngle = 0;
  this->BeamWeight = 1.0;

  this->CollimatorType = SquareHalfMM;

  this->SAD = 2000.0;

  this->EnergyResolution = 2.0;
  this->BeamFlavor = 'a';

  this->ApertureOffset = 1500.0;
  this->ApertureSpacingAtIso = 2.0;
  this->ApertureSpacing[0] = this->ApertureSpacingAtIso * this->ApertureOffset / this->SAD;
  this->ApertureSpacing[1] = this->ApertureSpacingAtIso * this->ApertureOffset / this->SAD;
  this->ApertureOrigin[0] = this->X1Jaw * this->ApertureOffset / this->SAD;
  this->ApertureOrigin[1] = this->Y1Jaw * this->ApertureOffset / this->SAD;
  this->ApertureDim[0] = (int) ((this->X2Jaw + this->X1Jaw) / this->ApertureSpacingAtIso + 1);
  this->ApertureDim[1] = (int) ((this->Y2Jaw + this->Y1Jaw) / this->ApertureSpacingAtIso + 1);

  this->SourceSize = 0.0;

  this->BeamData = new vtkRTBeamData();

  this->BeamModelNode = NULL;
  this->BeamModelNodeId = NULL;

  this->TargetSegmentID = NULL;

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
  if (this->BeamData)
  {
    delete this->BeamData;
    this->BeamData = NULL;
  }

  this->SetAndObserveBeamModelNodeId(NULL);
  this->SetTargetSegmentID(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if (this->BeamData->GetBeamName() != NULL) 
  {
    of << indent << " BeamName=\"" << this->BeamData->GetBeamName() << "\"";
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
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "BeamName")) 
    {
      this->BeamData->SetBeamName(attValue);
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

  this->BeamData->SetBeamName( node->BeamData->GetBeamName() );

  // Observers must be removed here, otherwise MRML updates would activate nodes on the undo stack
  this->SetAndObserveBeamModelNodeId( NULL );
  this->SetBeamModelNodeId( node->BeamModelNodeId );

  this->SetTargetSegmentID(node->TargetSegmentID);

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
vtkMRMLMarkupsFiducialNode* vtkMRMLRTBeamNode::GetIsocenterFiducialNode()
{
  return vtkMRMLMarkupsFiducialNode::SafeDownCast( this->GetNodeReference(ISOCENTER_FIDUCIAL_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
const std::string vtkMRMLRTBeamNode::GetBeamName() const
{
  return std::string(BeamData->GetBeamName());
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetBeamName(const std::string& beamName)
{
  BeamData->SetBeamName(beamName.c_str());
}

//----------------------------------------------------------------------------
bool vtkMRMLRTBeamNode::BeamNameIs (const std::string& beamName)
{
  return BeamNameIs (beamName.c_str());
}

//----------------------------------------------------------------------------
bool vtkMRMLRTBeamNode::BeamNameIs (const char *beamName)
{
  return !strcmp(BeamData->GetBeamName(), beamName);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetAndObserveIsocenterFiducialNode(vtkMRMLMarkupsFiducialNode* node)
{
  this->SetNodeReferenceID(ISOCENTER_FIDUCIAL_REFERENCE_ROLE, (node ? node->GetID() : NULL));
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
  vtkMRMLRTPlanHierarchyNode *phnode = NULL;
  vtkMRMLRTPlanHierarchyNode *phrootnode = NULL;
  vtkMRMLRTPlanNode *pnode = NULL; 
  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
  {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLRTPlanHierachyNode"))
    {
      phnode = vtkMRMLRTPlanHierarchyNode::SafeDownCast(mnode);
      vtkMRMLRTBeamNode* pnode = vtkMRMLRTBeamNode::SafeDownCast(this->GetScene()->GetNodeByID(phnode->GetAssociatedNodeID()));
      if (pnode == this) 
      {
        phrootnode = phnode;
        break;
      }
    }// end if
  }// end for
  if (phrootnode)
  {
    pnode = vtkMRMLRTPlanNode::SafeDownCast(
        scene->GetNodeByID(vtkMRMLRTPlanHierarchyNode::SafeDownCast(phrootnode->GetParentNode())->GetAssociatedNodeID()));
  }
  return pnode;
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
  if (this->Scene && this->BeamModelNodeId != NULL )
  {
    vtkMRMLNode* snode = this->Scene->GetNodeByID(this->BeamModelNodeId);
    node = vtkMRMLModelNode::SafeDownCast(snode);
  }

  return node;
}

//----------------------------------------------------------------------------
const double* vtkMRMLRTBeamNode::GetIsocenterPosition ()
{
  return this->Isocenter;
}

//----------------------------------------------------------------------------
double vtkMRMLRTBeamNode::GetIsocenterPosition (int dim)
{
  return this->Isocenter[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetIsocenterPosition (const float* position)
{
  for (int d = 0; d < 3; d++) 
  {
    this->Isocenter[d] = position[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetIsocenterPosition (const double* position)
{
  for (int d = 0; d < 3; d++) 
  {
    this->Isocenter[d] = position[d];
  }
}

//----------------------------------------------------------------------------
const double* vtkMRMLRTBeamNode::GetApertureSpacing ()
{
  return this->ApertureSpacing;
}

//----------------------------------------------------------------------------
double vtkMRMLRTBeamNode::GetApertureSpacing (int dim)
{
  return this->ApertureSpacing[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetApertureSpacing (const float* spacing)
{
  for (int d = 0; d < 2; d++) 
  {
    this->ApertureSpacing[d] = spacing[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetApertureSpacing (const double* spacing)
{
  for (int d = 0; d < 2; d++) 
  {
    this->ApertureSpacing[d] = spacing[d];
  }
}

//----------------------------------------------------------------------------
const double* vtkMRMLRTBeamNode::GetApertureOrigin ()
{
  return this->ApertureOrigin;
}

//----------------------------------------------------------------------------
double vtkMRMLRTBeamNode::GetApertureOrigin (int dim)
{
  return this->ApertureOrigin[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetApertureOrigin (const float* position)
{
  for (int d = 0; d < 2; d++) 
  {
    this->ApertureOrigin[d] = position[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetApertureOrigin (const double* position)
{
  for (int d = 0; d < 2; d++) 
  {
    this->ApertureOrigin[d] = position[d];
  }
}

//----------------------------------------------------------------------------
const int* vtkMRMLRTBeamNode::GetApertureDim ()
{
  return this->ApertureDim;
}

//----------------------------------------------------------------------------
int vtkMRMLRTBeamNode::GetApertureDim (int dim)
{
  return this->ApertureDim[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::SetApertureDim (const int* dim)
{
  for (int d = 0; d < 2; d++) 
  {
    this->ApertureDim[d] = dim[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::UpdateApertureParameters()
{
  double origin[2] = {-this->X1Jaw * this->ApertureOffset / this->SAD , -this->Y1Jaw * this->ApertureOffset / this->SAD };
  this->SetApertureOrigin(origin);

  double spacing_at_aperture[2] = {this->ApertureSpacingAtIso * this->ApertureOffset / this->SAD, this->ApertureSpacingAtIso * this->ApertureOffset / this->SAD};
  this->SetApertureSpacing(spacing_at_aperture);

  int dim[2] = { (int) ((this->X2Jaw + this->X1Jaw) / this->ApertureSpacingAtIso + 1 ), (int) ((this->X2Jaw + this->X1Jaw) / this->ApertureSpacingAtIso + 1 )};
  this->SetApertureDim(dim);
}
