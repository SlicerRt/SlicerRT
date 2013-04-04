/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
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

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageResample.h>
#include <vtkGeneralTransform.h>
#include <vtkCollection.h>

// STD includes
#include <algorithm> //TODO: workaround for issue #179

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTBeamNode);

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode::vtkMRMLRTBeamNode()
{
  this->BeamName = NULL;
  this->BeamNumber = 0.0;
  this->BeamDescription = NULL;
  this->BeamType = vtkMRMLRTBeamNode::RTBeamType::Static;
  this->RadiationType = vtkMRMLRTBeamNode::RTRadiationType::Photon;
  this->SAD = 0.0;
  this->ISOCenter[0] = 0.0;
  this->ISOCenter[1] = 0.0;
  this->ISOCenter[2] = 0.0;
  this->GantryAngle = 0.0;
  this->CollimatorType = vtkMRMLRTBeamNode::RTCollimatorType::SquareHalfMM;

  this->BeamModelNode = NULL;
  this->BeamModelNodeId = NULL;

  this->NominalEnergy = 0.0;
  this->NominalmA = 0.0;
  this->RxDose = 0.0;
  this->BeamOnTime = 0.0;

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
  this->SetAndObserveBeamModelNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if (this->BeamName != NULL) 
    {
    of << indent << " BeamName=\"" << this->BeamName << "\"";
    }
  if (this->BeamModelNodeId != NULL) 
    {
    of << indent << " BeamModelNodeId=\"" << this->BeamModelNodeId << "\"";
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
      this->SetBeamName(attValue);
      }
    else if (!strcmp(attName, "BeamModelNodeId")) 
      {
      this->SetAndObserveBeamModelNodeId(NULL); // clear any previous observers
      // Do not add observers yet because updates may be wrong before reading all the xml attributes
      // Observers will be added when all the attributes are read and UpdateScene is called
      this->SetBeamModelNodeId(attValue);
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

  // Observers must be removed here, otherwise MRML updates would activate nodes on the undo stack
  this->SetAndObserveBeamModelNodeId( NULL );
  this->SetBeamModelNodeId( node->BeamModelNodeId );

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
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "BeamModelNodeId:   " << this->BeamModelNodeId << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLRTBeamNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (this->Scene == NULL)
    {
    return;
    }
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
