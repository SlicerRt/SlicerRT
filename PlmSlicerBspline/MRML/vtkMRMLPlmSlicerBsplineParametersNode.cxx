/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
// VTK includes
#include <vtkCommand.h>
#include <vtkObjectFactory.h>

// MRML includes
#include "vtkMRMLVolumeNode.h"

// CropModuleMRML includes
#include "vtkMRMLPlmSlicerBsplineParametersNode.h"

// AnnotationModuleMRML includes
#include "vtkMRMLAnnotationROINode.h"

// STD includes

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlmSlicerBsplineParametersNode);

//----------------------------------------------------------------------------
vtkMRMLPlmSlicerBsplineParametersNode::vtkMRMLPlmSlicerBsplineParametersNode()
{
  this->HideFromEditors = 1;

  this->FixedVolumeNodeID = NULL;
  this->FixedVolumeNode = NULL;
  this->MovingVolumeNodeID = NULL;
  this->MovingVolumeNode = NULL;
  this->WarpedVolumeNodeID = NULL;
  this->WarpedVolumeNode = NULL;
  this->XformVolumeNodeID = NULL;
  this->XformVolumeNode = NULL;
}

//----------------------------------------------------------------------------
vtkMRMLPlmSlicerBsplineParametersNode::~vtkMRMLPlmSlicerBsplineParametersNode()
{
#if 0
  if (this->InputVolumeNodeID)
    {
    this->SetAndObserveInputVolumeNodeID(NULL);
    }

  if (this->OutputVolumeNodeID)
    {
    this->SetAndObserveOutputVolumeNodeID(NULL);
    }

  if (this->ROINodeID)
    {
    this->SetAndObserveROINodeID(NULL);
    }
#endif
}

//----------------------------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::ReadXMLAttributes(const char** atts)
{
#if 0
  std::cerr << "Reading PlmSlicerBspline param node!" << std::endl;
  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "inputVolumeNodeID"))
    {
      this->SetInputVolumeNodeID(attValue);
      continue;
    }
    if (!strcmp(attName, "outputVolumeNodeID"))
    {
      this->SetOutputVolumeNodeID(attValue);
      continue;
    }
    if (!strcmp(attName, "ROINodeID"))
    {
      this->SetROINodeID(attValue);
      continue;
    }
    if (!strcmp(attName,"ROIVisibility"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->ROIVisibility;
      continue;
    }
    if (!strcmp(attName,"interpolationMode"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->InterpolationMode;
      continue;
    }
  }

  this->WriteXML(std::cout,1);
#endif
}

//----------------------------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::WriteXML(ostream& of, int nIndent)
{
#if 0
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  of << indent << " inputVolumeNodeID=\"" << (this->InputVolumeNodeID ? this->InputVolumeNodeID : "NULL") << "\"";
  of << indent << " outputVolumeNodeID=\"" << (this->OutputVolumeNodeID ? this->OutputVolumeNodeID : "NULL") << "\"";
  of << indent << " ROIVisibility=\""<< this->ROIVisibility << "\"";
  of << indent << " ROINodeID=\"" << (this->ROINodeID ? this->ROINodeID : "NULL") << "\"";
  of << indent << " interpolationMode=\"" << this->InterpolationMode << "\"";
#endif
}

//----------------------------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->FixedVolumeNodeID && !strcmp(oldID, this->FixedVolumeNodeID)) {
    this->SetAndObserveFixedVolumeNodeID(newID);
  }
  if (this->MovingVolumeNodeID && !strcmp(oldID, this->MovingVolumeNodeID)) {
    this->SetAndObserveMovingVolumeNodeID(newID);
  }
  if (this->WarpedVolumeNodeID && !strcmp(oldID, this->WarpedVolumeNodeID)) {
    this->SetAndObserveWarpedVolumeNodeID(newID);
  }
  if (this->XformVolumeNodeID && !strcmp(oldID, this->XformVolumeNodeID)) {
    this->SetAndObserveXformVolumeNodeID(newID);
  }
}

//-----------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::UpdateReferences()
{
   Superclass::UpdateReferences();

  if (this->FixedVolumeNodeID != NULL && this->Scene->GetNodeByID(this->FixedVolumeNodeID) == NULL) {
    this->SetAndObserveFixedVolumeNodeID(NULL);
  }
  if (this->MovingVolumeNodeID != NULL && this->Scene->GetNodeByID(this->MovingVolumeNodeID) == NULL) {
    this->SetAndObserveMovingVolumeNodeID(NULL);
  }
  if (this->WarpedVolumeNodeID != NULL && this->Scene->GetNodeByID(this->WarpedVolumeNodeID) == NULL) {
    this->SetAndObserveWarpedVolumeNodeID(NULL);
  }
  if (this->XformVolumeNodeID != NULL && this->Scene->GetNodeByID(this->XformVolumeNodeID) == NULL) {
    this->SetAndObserveXformVolumeNodeID(NULL);
  }
}

//----------------------------------------------------------------------------
// Copy the node\"s attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, SliceID
void vtkMRMLPlmSlicerBsplineParametersNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLPlmSlicerBsplineParametersNode *node = vtkMRMLPlmSlicerBsplineParametersNode::SafeDownCast(anode);
  this->DisableModifiedEventOn();

  this->SetFixedVolumeNodeID(node->GetFixedVolumeNodeID());
  this->SetMovingVolumeNodeID(node->GetMovingVolumeNodeID());
  this->SetWarpedVolumeNodeID(node->GetWarpedVolumeNodeID());
  this->SetXformVolumeNodeID(node->GetXformVolumeNodeID());

#if 0
  this->SetROINodeID(node->GetROINodeID());
  this->SetInterpolationMode(node->GetInterpolationMode());
  this->SetROIVisibility(node->GetROIVisibility());
#endif
  
  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::SetAndObserveFixedVolumeNodeID(const char *volumeNodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->FixedVolumeNode, NULL);

  if (volumeNodeID != NULL)
  {
    this->SetFixedVolumeNodeID(volumeNodeID);
    vtkMRMLVolumeNode *node = this->GetFixedVolumeNode();
    vtkSetAndObserveMRMLObjectMacro(this->FixedVolumeNode, node);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::SetAndObserveMovingVolumeNodeID(const char *volumeNodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->MovingVolumeNode, NULL);

  if (volumeNodeID != NULL)
  {
    this->SetMovingVolumeNodeID(volumeNodeID);
    vtkMRMLVolumeNode *node = this->GetMovingVolumeNode();
    vtkSetAndObserveMRMLObjectMacro(this->MovingVolumeNode, node);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::SetAndObserveWarpedVolumeNodeID(const char *volumeNodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->WarpedVolumeNode, NULL);

  if (volumeNodeID != NULL)
  {
    this->SetWarpedVolumeNodeID(volumeNodeID);
    vtkMRMLVolumeNode *node = this->GetWarpedVolumeNode();
    vtkSetAndObserveMRMLObjectMacro(this->WarpedVolumeNode, node);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::SetAndObserveXformVolumeNodeID(const char *volumeNodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->XformVolumeNode, NULL);

  if (volumeNodeID != NULL)
  {
    this->SetXformVolumeNodeID(volumeNodeID);
    vtkMRMLVolumeNode *node = this->GetXformVolumeNode();
    vtkSetAndObserveMRMLObjectMacro(this->XformVolumeNode, node);
  }
}

//----------------------------------------------------------------------------
vtkMRMLVolumeNode* vtkMRMLPlmSlicerBsplineParametersNode::GetFixedVolumeNode()
{
  if (this->FixedVolumeNodeID == NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->FixedVolumeNode, NULL);
    }
  else if (this->GetScene() &&
           ((this->FixedVolumeNode != NULL && strcmp(this->FixedVolumeNode->GetID(), this->FixedVolumeNodeID)) ||
            (this->FixedVolumeNode == NULL)) )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->FixedVolumeNodeID);
    vtkSetAndObserveMRMLObjectMacro(this->FixedVolumeNode, vtkMRMLVolumeNode::SafeDownCast(snode));
    }
  return this->FixedVolumeNode;
}

//----------------------------------------------------------------------------
vtkMRMLVolumeNode* vtkMRMLPlmSlicerBsplineParametersNode::GetMovingVolumeNode()
{
  if (this->MovingVolumeNodeID == NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->MovingVolumeNode, NULL);
    }
  else if (this->GetScene() &&
           ((this->MovingVolumeNode != NULL && strcmp(this->MovingVolumeNode->GetID(), this->MovingVolumeNodeID)) ||
            (this->MovingVolumeNode == NULL)) )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->MovingVolumeNodeID);
    vtkSetAndObserveMRMLObjectMacro(this->MovingVolumeNode, vtkMRMLVolumeNode::SafeDownCast(snode));
    }
  return this->MovingVolumeNode;
}

//----------------------------------------------------------------------------
vtkMRMLVolumeNode* vtkMRMLPlmSlicerBsplineParametersNode::GetWarpedVolumeNode()
{
  if (this->WarpedVolumeNodeID == NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->WarpedVolumeNode, NULL);
    }
  else if (this->GetScene() &&
           ((this->WarpedVolumeNode != NULL && strcmp(this->WarpedVolumeNode->GetID(), this->WarpedVolumeNodeID)) ||
            (this->WarpedVolumeNode == NULL)) )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->WarpedVolumeNodeID);
    vtkSetAndObserveMRMLObjectMacro(this->WarpedVolumeNode, vtkMRMLVolumeNode::SafeDownCast(snode));
    }
  return this->WarpedVolumeNode;
}

//----------------------------------------------------------------------------
vtkMRMLVolumeNode* vtkMRMLPlmSlicerBsplineParametersNode::GetXformVolumeNode()
{
  if (this->XformVolumeNodeID == NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->XformVolumeNode, NULL);
    }
  else if (this->GetScene() &&
           ((this->XformVolumeNode != NULL && strcmp(this->XformVolumeNode->GetID(), this->XformVolumeNodeID)) ||
            (this->XformVolumeNode == NULL)) )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->XformVolumeNodeID);
    vtkSetAndObserveMRMLObjectMacro(this->XformVolumeNode, vtkMRMLVolumeNode::SafeDownCast(snode));
    }
  return this->XformVolumeNode;
}


//-----------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
  this->SetAndObserveFixedVolumeNodeID(this->FixedVolumeNodeID);
  this->SetAndObserveMovingVolumeNodeID(this->MovingVolumeNodeID);
  this->SetAndObserveWarpedVolumeNodeID(this->WarpedVolumeNodeID);
  this->SetAndObserveXformVolumeNodeID(this->XformVolumeNodeID);
}

//---------------------------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::ProcessMRMLEvents ( vtkObject *caller,
                                                    unsigned long event,
                                                    void *callData )
{
    Superclass::ProcessMRMLEvents(caller, event, callData);
    this->InvokeEvent(vtkCommand::ModifiedEvent, NULL);
    return;
}

//----------------------------------------------------------------------------
void vtkMRMLPlmSlicerBsplineParametersNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

#if 0
  os << "InputVolumeNodeID: " << ( (this->InputVolumeNodeID) ? this->InputVolumeNodeID : "None" ) << "\n";
  os << "OutputVolumeNodeID: " << ( (this->OutputVolumeNodeID) ? this->OutputVolumeNodeID : "None" ) << "\n";
  os << "ROINodeID: " << ( (this->ROINodeID) ? this->ROINodeID : "None" ) << "\n";
  os << "ROIVisibility: " << this->ROIVisibility << "\n";
  os << "InterpolationMode: " << this->InterpolationMode << "\n";
  os << "IsotropicResampling: " << this->IsotropicResampling << "\n";
#endif
}

// End
