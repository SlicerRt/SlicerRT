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

This file was originally developed by Csaba Pinter, PerkLab, Queen's University
and was supported through the Applied Cancer Research Unit program of Cancer Care
Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentationStorageNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLStorageNode.h>
#include <vtkEventBroker.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLColorTableNode.h>

// VTK includes
#include <vtkCallbackCommand.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkTrivialProducer.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkGeneralTransform.h>
#include <vtkHomogeneousTransform.h>
#include <vtkTransform.h>
#include <vtkLookupTable.h>

// STD includes
#include <algorithm>

// SegmentationCore includes
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkCalculateOversamplingFactor.h"

// STD includes
#include <algorithm>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentationNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode::vtkMRMLSegmentationNode()
{
  this->MasterRepresentationCallbackCommand = vtkCallbackCommand::New();
  this->MasterRepresentationCallbackCommand->SetClientData( reinterpret_cast<void *>(this) );
  this->MasterRepresentationCallbackCommand->SetCallback( vtkMRMLSegmentationNode::OnMasterRepresentationModified );

  this->SegmentAddedCallbackCommand = vtkCallbackCommand::New();
  this->SegmentAddedCallbackCommand->SetClientData( reinterpret_cast<void *>(this) );
  this->SegmentAddedCallbackCommand->SetCallback( vtkMRMLSegmentationNode::OnSegmentAdded );

  this->SegmentRemovedCallbackCommand = vtkCallbackCommand::New();
  this->SegmentRemovedCallbackCommand->SetClientData( reinterpret_cast<void *>(this) );
  this->SegmentRemovedCallbackCommand->SetCallback( vtkMRMLSegmentationNode::OnSegmentRemoved );

  this->SegmentModifiedCallbackCommand = vtkCallbackCommand::New();
  this->SegmentModifiedCallbackCommand->SetClientData( reinterpret_cast<void *>(this) );
  this->SegmentModifiedCallbackCommand->SetCallback( vtkMRMLSegmentationNode::OnSegmentModified );

  this->RepresentationCreatedCallbackCommand = vtkCallbackCommand::New();
  this->RepresentationCreatedCallbackCommand->SetClientData( reinterpret_cast<void *>(this) );
  this->RepresentationCreatedCallbackCommand->SetCallback( vtkMRMLSegmentationNode::OnRepresentationCreated );

  // Create empty segmentations object
  this->Segmentation = NULL;
  vtkSmartPointer<vtkSegmentation> segmentation = vtkSmartPointer<vtkSegmentation>::New();
  this->SetAndObserveSegmentation(segmentation);
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode::~vtkMRMLSegmentationNode()
{
  this->SetAndObserveSegmentation(NULL);

  if (this->MasterRepresentationCallbackCommand)
  {
    this->MasterRepresentationCallbackCommand->SetClientData(NULL);
    this->MasterRepresentationCallbackCommand->Delete();
    this->MasterRepresentationCallbackCommand = NULL;
  }

  if (this->SegmentAddedCallbackCommand)
  {
    this->SegmentAddedCallbackCommand->SetClientData(NULL);
    this->SegmentAddedCallbackCommand->Delete();
    this->SegmentAddedCallbackCommand = NULL;
  }

  if (this->SegmentRemovedCallbackCommand)
  {
    this->SegmentRemovedCallbackCommand->SetClientData(NULL);
    this->SegmentRemovedCallbackCommand->Delete();
    this->SegmentRemovedCallbackCommand = NULL;
  }

  if (this->SegmentModifiedCallbackCommand)
  {
    this->SegmentModifiedCallbackCommand->SetClientData(NULL);
    this->SegmentModifiedCallbackCommand->Delete();
    this->SegmentModifiedCallbackCommand = NULL;
  }

  if (this->RepresentationCreatedCallbackCommand)
  {
    this->RepresentationCreatedCallbackCommand->SetClientData(NULL);
    this->RepresentationCreatedCallbackCommand->Delete();
    this->RepresentationCreatedCallbackCommand = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  this->Segmentation->WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::ReadXMLAttributes(const char** atts)
{
  // Read all MRML node attributes from two arrays of names and values
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);
  this->Segmentation->ReadXMLAttributes(atts);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLSegmentationNode::Copy(vtkMRMLNode *anode)
{
  //this->DisableModifiedEventOn();

  vtkMRMLSegmentationNode* otherNode = vtkMRMLSegmentationNode::SafeDownCast(anode);

  // Deep copy segmentation (containing the same segments from two segmentations is unstable)
  this->Segmentation->DeepCopy(otherNode->GetSegmentation());

  // Copy other parameters
  if (otherNode->GetAddToScene())
  {
    this->CopyOrientation(otherNode);
  }

  Superclass::Copy(anode);

  //this->DisableModifiedEventOff();
  //this->InvokePendingModifiedEvent(); // This call loses event parameters (i.e. callData)
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::DeepCopy(vtkMRMLNode* aNode)
{
  //this->DisableModifiedEventOn();

  vtkMRMLSegmentationNode *otherNode = vtkMRMLSegmentationNode::SafeDownCast(aNode);

  // Deep copy segmentation
  this->Segmentation->DeepCopy(otherNode->Segmentation);

  // Copy other parameters
  if (otherNode->GetAddToScene())
  {
    this->CopyOrientation(otherNode);
  }

  Superclass::Copy(aNode);

  //this->DisableModifiedEventOff();
  //this->InvokePendingModifiedEvent(); // This call loses event parameters (i.e. callData)
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  this->Segmentation->PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
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

  //if (eventID == vtkCommand::ModifiedEvent)
  //{
  //}
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::SetAndObserveSegmentation(vtkSegmentation* segmentation)
{
  if (segmentation == this->Segmentation)
  {
    return;
  }

  // Remove observation from segment's master representation (in case it has changed)
  if (this->Segmentation)
  {
    vtkEventBroker::GetInstance()->RemoveObservations(
      this->Segmentation, vtkSegmentation::MasterRepresentationModified, this, this->MasterRepresentationCallbackCommand );
    vtkEventBroker::GetInstance()->RemoveObservations(
      this->Segmentation, vtkSegmentation::SegmentAdded, this, this->SegmentAddedCallbackCommand );
    vtkEventBroker::GetInstance()->RemoveObservations(
      this->Segmentation, vtkSegmentation::SegmentRemoved, this, this->SegmentRemovedCallbackCommand );
    vtkEventBroker::GetInstance()->RemoveObservations(
      this->Segmentation, vtkSegmentation::SegmentModified, this, this->SegmentModifiedCallbackCommand );
    vtkEventBroker::GetInstance()->RemoveObservations(
      this->Segmentation, vtkSegmentation::RepresentationCreated, this, this->RepresentationCreatedCallbackCommand );
  }

  this->SetSegmentation(segmentation);

  // Observe segment's master representation
  if (this->Segmentation)
  {
    vtkEventBroker::GetInstance()->AddObservation(
      this->Segmentation, vtkSegmentation::MasterRepresentationModified, this, this->MasterRepresentationCallbackCommand );
    vtkEventBroker::GetInstance()->AddObservation(
      this->Segmentation, vtkSegmentation::SegmentAdded, this, this->SegmentAddedCallbackCommand );
    vtkEventBroker::GetInstance()->AddObservation(
      this->Segmentation, vtkSegmentation::SegmentRemoved, this, this->SegmentRemovedCallbackCommand );
    vtkEventBroker::GetInstance()->AddObservation(
      this->Segmentation, vtkSegmentation::SegmentModified, this, this->SegmentModifiedCallbackCommand );
    vtkEventBroker::GetInstance()->AddObservation(
      this->Segmentation, vtkSegmentation::RepresentationCreated, this, this->RepresentationCreatedCallbackCommand );
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::OnMasterRepresentationModified(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eid), void* clientData, void* vtkNotUsed(callData))
{
  vtkMRMLSegmentationNode* self = reinterpret_cast<vtkMRMLSegmentationNode*>(clientData);
  if (!self)
  {
    return;
  }
  if (!self->Segmentation)
  {
    vtkErrorWithObjectMacro(self, "vtkMRMLSegmentationNode::OnMasterRepresentationModified: No segmentation in segmentation node!");
    return;
  }

  // Reset supported write file types
  vtkMRMLSegmentationStorageNode* storageNode =  vtkMRMLSegmentationStorageNode::SafeDownCast(self->GetStorageNode());
  if (storageNode)
  {
    storageNode->ResetSupportedWriteFileTypes();
  }

  // Invoke node event
  self->InvokeCustomModifiedEvent(vtkSegmentation::MasterRepresentationModified, self);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::OnSegmentAdded(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eid), void* clientData, void* callData)
{
  vtkMRMLSegmentationNode* self = reinterpret_cast<vtkMRMLSegmentationNode*>(clientData);
  if (!self)
  {
    return;
  }
  if (!self->Segmentation)
  {
    vtkErrorWithObjectMacro(self, "vtkMRMLSegmentationNode::OnSegmentAdded: No segmentation in segmentation node!");
    return;
  }
  if (self->Scene && self->Scene->IsImporting())
  {
    return;
  }

  // Get segment ID
  char* segmentId = reinterpret_cast<char*>(callData);

  // Add segment display properties
  if (!self->AddSegmentDisplayProperties(segmentId))
  {
    vtkErrorWithObjectMacro(self, "vtkMRMLSegmentationNode::OnSegmentAdded: Failed to add display properties for segment " << segmentId);
    return;
  }

  // Re-generate merged labelmap with the added segment
  if (self->HasMergedLabelmap())
  {
    self->ReGenerateDisplayedMergedLabelmap();
  }

  // Invoke node event
  self->InvokeCustomModifiedEvent(vtkSegmentation::SegmentAdded, (void*)segmentId);

  self->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::OnSegmentRemoved(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eid), void* clientData, void* callData)
{
  vtkMRMLSegmentationNode* self = reinterpret_cast<vtkMRMLSegmentationNode*>(clientData);
  if (!self)
  {
    return;
  }
  if (!self->Segmentation)
  {
    vtkErrorWithObjectMacro(self, "vtkMRMLSegmentationNode::OnSegmentRemoved: No segmentation in segmentation node!");
    return;
  }

  // Get segment ID
  char* segmentId = reinterpret_cast<char*>(callData);

  // Remove display properties
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(self->GetDisplayNode());
  if (displayNode)
  {
    // Remove entry from segment display properties
    displayNode->RemoveSegmentDisplayProperties(segmentId);

    // Remove segment entry from color table
    vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(displayNode->GetColorNode());
    if (!colorTableNode)
    {
      vtkErrorWithObjectMacro(self, "vtkMRMLSegmentationNode::OnSegmentRemoved: No color table node associated with segmentation!");
      return;
    }
    int colorIndex = colorTableNode->GetColorIndexByName(segmentId);
    if (colorIndex < 0)
    {
      vtkErrorWithObjectMacro(self, "vtkMRMLSegmentationNode::OnSegmentRemoved: No color table entry found for segment " << segmentId);
      return;
    }
    colorTableNode->SetColor(colorIndex,vtkMRMLSegmentationDisplayNode::GetSegmentationColorNameRemoved(),
      vtkSegment::SEGMENT_COLOR_VALUE_INVALID[0], vtkSegment::SEGMENT_COLOR_VALUE_INVALID[1],
      vtkSegment::SEGMENT_COLOR_VALUE_INVALID[2], vtkSegment::SEGMENT_COLOR_VALUE_INVALID[3] );
  }

  // Re-generate merged labelmap without the removed segment
  if (self->HasMergedLabelmap())
  {
    self->ReGenerateDisplayedMergedLabelmap();
  }

  // Invoke node event
  self->InvokeCustomModifiedEvent(vtkSegmentation::SegmentRemoved, (void*)segmentId);

  self->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::OnSegmentModified(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eid), void* clientData, void* callData)
{
  vtkMRMLSegmentationNode* self = reinterpret_cast<vtkMRMLSegmentationNode*>(clientData);
  if (!self)
  {
    return;
  }
  if (!self->Segmentation)
  {
    vtkErrorWithObjectMacro(self, "vtkMRMLSegmentationNode::OnSegmentModified: No segmentation in segmentation node!");
    return;
  }

  // Get segment ID
  char* segmentId = reinterpret_cast<char*>(callData);

  // Invoke node event, but do not invoke general modified
  self->InvokeCustomModifiedEvent(vtkSegmentation::SegmentModified, (void*)segmentId);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::OnRepresentationCreated(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eid), void* clientData, void* callData)
{
  vtkMRMLSegmentationNode* self = reinterpret_cast<vtkMRMLSegmentationNode*>(clientData);
  if (!self)
  {
    return;
  }

  // Get created representation name
  char* targetRepresentationName = reinterpret_cast<char*>(callData);

  // Re-generate merged labelmap with modified representation
  if (!(self->Scene && self->Scene->IsImporting()) && self->HasMergedLabelmap())
  {
    self->ReGenerateDisplayedMergedLabelmap();
  }

  // Invoke node event
  self->InvokeCustomModifiedEvent(vtkSegmentation::RepresentationCreated, (void*)targetRepresentationName);

  self->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::OnSubjectHierarchyUIDAdded(vtkMRMLSubjectHierarchyNode* shNodeWithNewUID)
{
  if (!shNodeWithNewUID)
  {
    return;
  }
  // If already has geometry, then do not look for a new one
  if (!this->Segmentation->GetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName()).empty())
  {
    return;
  }

  // Get associated volume node from subject hierarchy node with new UID
  vtkMRMLScalarVolumeNode* referencedVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(shNodeWithNewUID->GetAssociatedNode());
  if (!referencedVolumeNode)
  {
    // If associated node is not a volume, then return
    return;
  }

  // Get associated subject hierarchy node
  vtkMRMLSubjectHierarchyNode* segmentationShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this);
  if (!segmentationShNode)
  {
    // If segmentation is not in subject hierarchy, then we cannot find its DICOM references
    return;
  }

  // Get DICOM references from segmentation subject hierarchy node
  const char* referencedInstanceUIDsAttribute = segmentationShNode->GetAttribute(
    vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName().c_str());
  if (!referencedInstanceUIDsAttribute)
  {
    // No references
    return;
  }

  // If the subject hierarchy node that got a new UID has a DICOM instance UID referenced
  // from this segmentation, then use its geometry as image geometry conversion parameter
  std::vector<std::string> referencedSopInstanceUids;
  vtkMRMLSubjectHierarchyNode::DeserializeUIDList(referencedInstanceUIDsAttribute, referencedSopInstanceUids);
  bool referencedVolumeFound = false;
  bool warningLogged = false;
  std::vector<std::string>::iterator uidIt;
  std::string nodeUidValueStr = shNodeWithNewUID->GetUID(vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName());
  if (nodeUidValueStr.empty())
  {
    // If the new UID is empty string, then do not look further
    return;
  }
  for (uidIt = referencedSopInstanceUids.begin(); uidIt != referencedSopInstanceUids.end(); ++uidIt)
  {
    // If we find the instance UID, then we set the geometry
    if (nodeUidValueStr.find(*uidIt) != std::string::npos)
    {
      // Only set the reference once, but check all UIDs
      if (!referencedVolumeFound)
      {
        // Set reference image geometry parameter if volume node is found
        vtkSmartPointer<vtkMatrix4x4> referenceImageGeometryMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
        referencedVolumeNode->GetIJKToRASMatrix(referenceImageGeometryMatrix);
        std::string serializedImageGeometry = vtkSegmentationConverter::SerializeImageGeometry(referenceImageGeometryMatrix, referencedVolumeNode->GetImageData());
        this->Segmentation->SetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName(), serializedImageGeometry);
        referencedVolumeFound = true;
      }
    }
    // If referenced UID is not contained in found node, then warn user
    else if (referencedVolumeFound && !warningLogged)
    {
      vtkWarningMacro("vtkMRMLSegmentationNode::OnSubjectHierarchyUIDAdded: Referenced volume for segmentation '"
        << this->Name << "' found (" << referencedVolumeNode->GetName() << "), but some referenced UIDs are not present in it! (maybe only partial volume was loaded?)");
      // Only log warning once for this node
      warningLogged = true;
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::OnNodeReferenceAdded(vtkMRMLNodeReference *reference)
{
  Superclass::OnNodeReferenceAdded(reference);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::OnNodeReferenceModified(vtkMRMLNodeReference *reference)
{
  Superclass::OnNodeReferenceModified(reference);
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationNode::AddSegmentDisplayProperties(std::string segmentId)
{
  if (!this->Scene)
  {
    vtkErrorMacro("AddSegmentDisplayProperties: Unable to update display properties outside a MRML scene");
    return false;
  }

  // Get segment ID and segment
  vtkSegment* segment = this->Segmentation->GetSegment(segmentId);
  if (!segment)
  {
    vtkErrorMacro("AddSegmentDisplayProperties: No segment found with ID " << segmentId);
    return false;
  }

  vtkSmartPointer<vtkMRMLSegmentationDisplayNode> displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(this->GetDisplayNode());
  if (!displayNode)
  {
    // Create default display node if not found
    displayNode = vtkSmartPointer<vtkMRMLSegmentationDisplayNode>::New();
    this->Scene->AddNode(displayNode);
    this->SetAndObserveDisplayNodeID(displayNode->GetID());
    displayNode->SetBackfaceCulling(0); //TODO: Needed only because of the ribbon model normal vectors probably
  }

  int wasModifyingDisplayNode = displayNode->StartModify();

  // Create color table for segmentation if does not exist
  vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(displayNode->GetColorNode());
  if (!colorTableNode)
  {
    colorTableNode = displayNode->CreateColorTableNode(this->Name);
  }

  int wasModifyingColorTableNode = colorTableNode->StartModify();

  // Add entry in color table for segment
  int numberOfColors = colorTableNode->GetNumberOfColors();
  colorTableNode->SetNumberOfColors(numberOfColors+1);
  colorTableNode->GetLookupTable()->SetTableRange(0, numberOfColors);

  // Set segment color for merged labelmap
  double defaultColor[3] = {0.0,0.0,0.0};
  segment->GetDefaultColor(defaultColor);
  colorTableNode->SetColor(numberOfColors, segmentId.c_str(), defaultColor[0], defaultColor[1], defaultColor[2], 1.0);

  // Add entry in segment display properties
  vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
  properties.Visible = true;
  properties.Color[0] = defaultColor[0];
  properties.Color[1] = defaultColor[1];
  properties.Color[2] = defaultColor[2];
  properties.PolyDataOpacity = 1.0;
  displayNode->SetSegmentDisplayProperties(segmentId, properties);

  colorTableNode->EndModify(wasModifyingColorTableNode);
  displayNode->EndModify(wasModifyingDisplayNode);

  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::ResetSegmentDisplayProperties()
{
  if (!this->Scene)
  {
    vtkErrorMacro("ResetSegmentDisplayProperties: Unable to update display properties outside a MRML scene");
    return;
  }

  // Clear display properties
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(this->GetDisplayNode());
  if (displayNode)
  {
    displayNode->ClearSegmentDisplayProperties();

    vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(displayNode->GetColorNode());
    if (colorTableNode && colorTableNode->GetNumberOfColors() > 0)
    {
      colorTableNode->SetNumberOfColors(0);
      colorTableNode->GetLookupTable()->SetTableRange(0, 0);
    }
  }

  // Add display properties for all segments
  vtkSegmentation::SegmentMap segmentMap = this->Segmentation->GetSegments();
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
  {
    this->AddSegmentDisplayProperties(segmentIt->first);
  }
}

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLSegmentationNode::CreateDefaultStorageNode()
{
  this->StorableModified(); // Workaround to make save dialog check the segmentation node
  return vtkMRMLSegmentationStorageNode::New();
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::CreateDefaultDisplayNodes()
{
  if (vtkMRMLSegmentationDisplayNode::SafeDownCast(this->GetDisplayNode()))
  {
    // Display node already exists
    return;
  }
  if (this->GetScene() == NULL)
  {
    vtkErrorMacro("vtkMRMLSegmentationNode::CreateDefaultDisplayNodes failed: Scene is invalid");
    return;
  }
  vtkNew<vtkMRMLSegmentationDisplayNode> dispNode;
  this->GetScene()->AddNode(dispNode.GetPointer());
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::ApplyTransformMatrix(vtkMatrix4x4* transformMatrix)
{
  vtkTransform* transform = vtkTransform::New();
  transform->SetMatrix(transformMatrix);
  this->ApplyTransform(transform);
  transform->Delete();
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationNode::ApplyTransform(vtkAbstractTransform* transform)
{
  // Apply transform on segmentation
  vtkSmartPointer<vtkTransform> linearTransform = vtkSmartPointer<vtkTransform>::New();
  if (vtkOrientedImageDataResample::IsTransformLinear(transform, linearTransform))
  {
    this->Segmentation->ApplyLinearTransform(transform);
  }
  else
  {
    this->Segmentation->ApplyNonLinearTransform(transform);
  }

  // Make sure preferred poly data display representation exists after transformation
  // (it was invalidated in the process unless it is the master representation)
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(this->GetDisplayNode());
  if (displayNode)
  {
    std::string preferredPolyDataDisplayRepresentation(displayNode->GetPreferredPolyDataDisplayRepresentationName());
    if (!preferredPolyDataDisplayRepresentation.empty())
    {
      this->Segmentation->CreateRepresentation(preferredPolyDataDisplayRepresentation);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkMRMLSegmentationNode::CanApplyNonLinearTransforms() const
{
  return true;
}

//---------------------------------------------------------------------------
// Global RAS in the form (Xmin, Xmax, Ymin, Ymax, Zmin, Zmax)
//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::GetRASBounds(double bounds[6])
{
  vtkMath::UninitializeBounds(bounds);
  this->Segmentation->GetBounds(bounds);
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationNode::GetModifiedSinceRead()
{
  // Avoid calling vtkMRMLVolumeNode::GetModifiedSinceRead as it calls GetImageData
  // which triggers merge. It is undesirable especially if it's only called when exiting.
  return this->vtkMRMLStorableNode::GetModifiedSinceRead()
    || (this->HasMergedLabelmap() && this->GetImageData()->GetMTime() > this->GetStoredTime())
    || this->Segmentation->GetModifiedSinceRead();
}

//---------------------------------------------------------------------------
vtkImageData* vtkMRMLSegmentationNode::GetImageData()
{
  if (this->Scene && this->Scene->IsImporting())
  {
    // Do not do conversion on this request when importing scene, just return the existing image data.
    // Conversions when importing scene should be done explicitly by the segmentation storage node.
    return Superclass::GetImageData();
  }

  bool mergeNecessary = false;

  // Create image data if it does not exist
  vtkImageData* imageData = Superclass::GetImageData();
  if (!imageData)
  {
    vtkSmartPointer<vtkImageData> internalImageData = vtkSmartPointer<vtkImageData>::New();
    this->SetAndObserveImageData(internalImageData);
    imageData = Superclass::GetImageData();
    mergeNecessary = true;
  }

  // Merge labelmap if merge time is older than segment modified time
  if (!mergeNecessary && this->Segmentation)
  {
    vtkSegmentation::SegmentMap segmentMap = this->Segmentation->GetSegments();
    for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
      // Get master representation from segment
      vtkSegment* currentSegment = segmentIt->second;
      vtkDataObject* masterRepresentation = currentSegment->GetRepresentation(this->Segmentation->GetMasterRepresentationName());
      if (masterRepresentation)
      {
        if (masterRepresentation->GetMTime() > this->LabelmapMergeTime.GetMTime())
        {
          mergeNecessary = true;
          break;
        }
      }
    }
  }

  // Perform merging if necessary
  if (mergeNecessary)
  {
    if (this->Segmentation->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
      bool success = this->GenerateDisplayedMergedLabelmap(imageData);
      if (!success)
      {
        vtkErrorMacro("GetImageData: Failed to create merged labelmap for 2D visualization!");
        this->LabelmapMergeTime.Modified();
      }
    }
    else
    {
      // Do not merge labelmap explicitly as CreateRepresentation triggers labelmap generation via RepresentationCreated
      bool success = this->Segmentation->CreateRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
      if (!success)
      {
        vtkErrorMacro("GetImageData: Unable to get labelmap representation from segments!");
        this->LabelmapMergeTime.Modified();
      }
    }
  }

  // If no merging is needed, simply return stored image data
  return imageData;
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationNode::HasMergedLabelmap()
{
  vtkImageData* imageData = Superclass::GetImageData();
  return (imageData != NULL);
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationNode::GenerateDisplayedMergedLabelmap(vtkImageData* imageData)
{
  vtkSmartPointer<vtkMatrix4x4> mergedImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (this->GenerateMergedLabelmap(imageData, mergedImageToWorldMatrix))
  {
    // Save common labelmap geometry in segmentation node
    this->SetIJKToRASMatrix(mergedImageToWorldMatrix);

    // Make sure merged labelmap extents starts at zeros for compatibility reasons
    vtkMRMLSegmentationNode::ShiftVolumeNodeExtentToZeroStart(this);

    // Save labelmap merge timestamp
    this->LabelmapMergeTime.Modified();

    return true;
  }
  
  return false;
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationNode::GenerateMergedLabelmap(vtkImageData* mergedImageData, vtkMatrix4x4* mergedImageToWorldMatrix, const std::vector<std::string>& segmentIDs/*=std::vector<std::string>()*/)
{
  if (!mergedImageData)
  {
    vtkErrorMacro("GenerateMergedLabelmap: Invalid image data!");
    return false;
  }
  if (!mergedImageToWorldMatrix)
  {
    vtkErrorMacro("GenerateMergedLabelmap: Invalid geometry matrix!");
    return false;
  }
  // If segmentation is missing or empty then we cannot create a merged image data
  if (!this->Segmentation)
  {
    vtkErrorMacro("GenerateMergedLabelmap: Invalid segmentation!");
    return false;
  }
  if (!this->Segmentation->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    vtkErrorMacro("GenerateMergedLabelmap: Segmentation does not contain binary labelmap representation!");
    return false;
  }

  // If segment IDs list is empty then include all segments
  std::vector<std::string> mergedSegmentIDs;
  if (segmentIDs.empty())
  {
    vtkSegmentation::SegmentMap segmentMap = this->Segmentation->GetSegments();
    for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
      mergedSegmentIDs.push_back(segmentIt->first);
    }
  }
  else
  {
    mergedSegmentIDs = segmentIDs;
  }

  // Determine common labelmap geometry that will be used for the merged labelmap
  std::string commonGeometryString = this->Segmentation->DetermineCommonLabelmapGeometry(mergedSegmentIDs);
  vtkSmartPointer<vtkOrientedImageData> commonGeometryImage = vtkSmartPointer<vtkOrientedImageData>::New();
  vtkSegmentationConverter::DeserializeImageGeometry(commonGeometryString, commonGeometryImage);

  commonGeometryImage->GetImageToWorldMatrix(mergedImageToWorldMatrix);
  int referenceDimensions[3] = {0,0,0};
  commonGeometryImage->GetDimensions(referenceDimensions);
  int referenceExtent[6] = {0,-1,0,-1,0,-1};
  commonGeometryImage->GetExtent(referenceExtent);

  // Allocate image data if empty or if reference extent changed
  int imageDataExtent[6] = {0,-1,0,-1,0,-1};
  mergedImageData->GetExtent(imageDataExtent);
  if ( imageDataExtent[0] != referenceExtent[0] || imageDataExtent[1] != referenceExtent[1] || imageDataExtent[2] != referenceExtent[2]
    || imageDataExtent[3] != referenceExtent[3] || imageDataExtent[4] != referenceExtent[4] || imageDataExtent[5] != referenceExtent[5] )
  {
    mergedImageData->SetExtent(referenceExtent);
#if (VTK_MAJOR_VERSION <= 5)
    mergedImageData->SetScalarType(VTK_UNSIGNED_CHAR);
    mergedImageData->SetNumberOfScalarComponents(1);
    mergedImageData->AllocateScalars();
#else
    mergedImageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
#endif
  }

  // Paint the image data background first
  unsigned short backgroundColor = vtkMRMLSegmentationDisplayNode::GetSegmentationColorIndexBackground();
  unsigned char* mergedImagePtr = (unsigned char*)mergedImageData->GetScalarPointerForExtent(referenceExtent);
  if (!mergedImagePtr)
  {
    return false; // Setting the extent may invoke this function again via ImageDataModified, in which case the pointer is NULL
  }
  for (vtkIdType i=0; i<mergedImageData->GetNumberOfPoints(); ++i)
  {
    (*mergedImagePtr) = backgroundColor;
    ++mergedImagePtr;
  }

  // Skip the rest if there are no segments
  if (this->Segmentation->GetNumberOfSegments() == 0)
  {
    return true;
  }

  // Get color table node
  vtkMRMLColorTableNode* colorTableNode = NULL;
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(this->GetDisplayNode());
  if (displayNode)
  {
    // Add entry in color table for segment
    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(displayNode->GetColorNode());
    if (!colorTableNode)
    {
      vtkErrorMacro("GenerateMergedLabelmap: No color table node associated with segmentation!");
      return false;
    }
  }

  // Create merged labelmap. Use color table to determine labels for segments
  for (unsigned short colorIndex = 2; colorIndex < colorTableNode->GetNumberOfColors(); ++colorIndex) // Color index starts from 2 (0 is background, 1 is invalid)
  {
    std::string segmentId(colorTableNode->GetColorName(colorIndex));
    bool segmentIncluded = ( std::find(mergedSegmentIDs.begin(), mergedSegmentIDs.end(), std::string(segmentId)) != mergedSegmentIDs.end() );
    if (!segmentIncluded || segmentId.empty() || !segmentId.compare(vtkMRMLSegmentationDisplayNode::GetSegmentationColorNameRemoved()))
    {
      // Workaround for handling color node storage limitation that it replaces spaces with underscores when saving and restores when loading,
      // so if the segment name contains underscores then those are replaced with spaces
      std::string segmentIdNoSpaces = colorTableNode->GetColorNameWithoutSpaces(colorIndex, "_");
      if ( std::find(mergedSegmentIDs.begin(), mergedSegmentIDs.end(), segmentIdNoSpaces) == mergedSegmentIDs.end() )
      {
        // No actual segment is associated with the color index (segment was removed from segmentation),
        // or segment is not included in the list of merged segments
        continue;
      }
      else
      {
        segmentId = segmentIdNoSpaces;
      }
    }

    // Skip segment if hidden
    vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
    if (!displayNode->GetSegmentDisplayProperties(segmentId, properties))
    {
      continue;
    }
    if (properties.Visible == false)
    {
      continue;
    }

    // Get binary labelmap from segment
    vtkSegment* currentSegment = this->Segmentation->GetSegment(segmentId);
    if (!currentSegment)
    {
      vtkErrorMacro("GenerateMergedLabelmap: Mismatch in color names and segment IDs!");
      continue;
    }
    vtkOrientedImageData* representationBinaryLabelmap = vtkOrientedImageData::SafeDownCast(
      currentSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
    if (!representationBinaryLabelmap)
    {
      continue;
    }

    // Set oriented image data used for merging to the representation (may change later if resampling is needed)
    vtkOrientedImageData* binaryLabelmap = representationBinaryLabelmap;

    // If labelmap geometries (spacings and directions) do not match reference then resample temporarily
    vtkSmartPointer<vtkOrientedImageData> resampledBinaryLabelmap;
    if (!vtkOrientedImageDataResample::DoGeometriesMatch(commonGeometryImage, representationBinaryLabelmap))
    {
      resampledBinaryLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();

      // Resample segment labelmap for merging
      if (!vtkOrientedImageDataResample::ResampleOrientedImageToReferenceGeometry(representationBinaryLabelmap, mergedImageToWorldMatrix, resampledBinaryLabelmap))
      {
        continue;
      }

      // Use resampled labelmap for merging
      binaryLabelmap = resampledBinaryLabelmap;
    }

    // Copy image data voxels into merged labelmap with the proper color index
    int labelmapExtent[6] = {0,-1,0,-1,0,-1};
    binaryLabelmap->GetExtent(labelmapExtent);
    int labelmapDimensions[3] = {0,0,0};
    binaryLabelmap->GetDimensions(labelmapDimensions);
    int labelmapCoordinates[3] = {0,0,0};
    long residualIndex = 0;

    int segmentLabelScalarType = binaryLabelmap->GetScalarType();
    if ( segmentLabelScalarType != VTK_UNSIGNED_CHAR
      && segmentLabelScalarType != VTK_UNSIGNED_SHORT
      && segmentLabelScalarType != VTK_SHORT )
    {
      vtkWarningMacro("GenerateMergedLabelmap: Segment " << segmentId << " cannot be merged! Binary labelmap scalar type must be unsigned char, unsighed short, or short!");
      continue;
    }
    void* voidScalarPointer = binaryLabelmap->GetScalarPointer();
    unsigned char* labelmapPtrUChar = (unsigned char*)voidScalarPointer;
    unsigned short* labelmapPtrUShort = (unsigned short*)voidScalarPointer;
    short* labelmapPtrShort = (short*)voidScalarPointer;

    mergedImagePtr = (unsigned char*)mergedImageData->GetScalarPointer();
    unsigned char* imagePtrMax = mergedImagePtr + referenceDimensions[0]*referenceDimensions[1]*referenceDimensions[2];
    for (long i=0; i<binaryLabelmap->GetNumberOfPoints(); ++i)
    {
      // Get labelmap color at voxel
      unsigned short color = 0;
      if (segmentLabelScalarType == VTK_UNSIGNED_CHAR)
      {
        color = (*labelmapPtrUChar);
      }
      else if (segmentLabelScalarType == VTK_UNSIGNED_SHORT)
      {
        color = (*labelmapPtrUShort);
      }
      else if (segmentLabelScalarType == VTK_SHORT)
      {
        color = (*labelmapPtrShort);
      }

      // Get labelmap coordinates
      labelmapCoordinates[2] = i / (labelmapDimensions[1]*labelmapDimensions[0]);
      residualIndex = i - labelmapCoordinates[2]*labelmapDimensions[1]*labelmapDimensions[0];
      labelmapCoordinates[1] = residualIndex / labelmapDimensions[0];
      residualIndex -= labelmapCoordinates[1]*labelmapDimensions[0];
      labelmapCoordinates[0] = residualIndex;

      // Apply extent offset and current offset
      unsigned char* offsetImagePtr = mergedImagePtr +
                                      (labelmapExtent[4]-referenceExtent[4]+labelmapCoordinates[2]) * referenceDimensions[1]*referenceDimensions[0] +
                                      (labelmapExtent[2]-referenceExtent[2]+labelmapCoordinates[1]) * referenceDimensions[0] +
                                      (labelmapExtent[0]-referenceExtent[0]+labelmapCoordinates[0]);

      // Paint merged labelmap voxel if foreground and in extent
      if ( color != backgroundColor
        && offsetImagePtr >= mergedImagePtr && offsetImagePtr < imagePtrMax )
      {
        (*offsetImagePtr) = colorIndex;
      }

      ++labelmapPtrUChar;
      ++labelmapPtrUShort;
      ++labelmapPtrShort;
    }
  }

  return true;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::ReGenerateDisplayedMergedLabelmap()
{
  if (this->Segmentation->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    this->GenerateDisplayedMergedLabelmap(Superclass::GetImageData());
  }
  else
  {
    // Do not call GenerateMergedLabelmap explicitly as CreateRepresentation triggers labelmap generation via RepresentationCreated
    bool success = this->Segmentation->CreateRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
    if (!success)
    {
      vtkErrorMacro("ReGenerateMergedLabelmap: Unable to get labelmap representation from segments!");
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* vtkMRMLSegmentationNode::GetSegmentSubjectHierarchyNode(std::string segmentID)
{
  vtkMRMLSubjectHierarchyNode* segmentationSubjectHierarchyNode =
    vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this);
  if (!segmentationSubjectHierarchyNode)
  {
    return NULL;
  }

  // Find child node of segmentation subject hierarchy node that has the requested segment ID
  std::vector<vtkMRMLHierarchyNode*> children = segmentationSubjectHierarchyNode->GetChildrenNodes();
  for (std::vector<vtkMRMLHierarchyNode*>::iterator childIt=children.begin(); childIt!=children.end(); ++childIt)
  {
    vtkMRMLSubjectHierarchyNode* childNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(*childIt);
    if ( childNode && childNode->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName())
      && !segmentID.compare(childNode->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName())) )
    {
      return childNode;
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationNode::ShiftVolumeNodeExtentToZeroStart(vtkMRMLScalarVolumeNode* volumeNode)
{
  if (!volumeNode || !volumeNode->vtkMRMLScalarVolumeNode::GetImageData())
  {
    return;
  }

  vtkImageData* imageData = volumeNode->vtkMRMLScalarVolumeNode::GetImageData();
  int extent[6] = {0,-1,0,-1,0,-1};
  imageData->GetExtent(extent);

  // No need to shift if extent already starts at zeros
  if (extent[0] == 0 && extent[2] == 0 && extent[4] == 0)
  {
    return;
  }

  double origin[3] = {0.0,0.0,0.0};
  volumeNode->GetOrigin(origin);
  double spacing[3] = {0.0,0.0,0.0};
  volumeNode->GetSpacing(spacing);
  // Need to apply directions on spacing
  vtkSmartPointer<vtkMatrix4x4> volumeNodeIjkToRasDirectionMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  volumeNode->GetIJKToRASDirectionMatrix(volumeNodeIjkToRasDirectionMatrix);

  for (int i=0; i<3; ++i)
  {
    spacing[i] *= volumeNodeIjkToRasDirectionMatrix->GetElement(i,i);
    origin[i] += extent[2*i] * spacing[i];
    extent[2*i+1] -= extent[2*i];
    extent[2*i] = 0;
  }

  imageData->SetExtent(extent);
  volumeNode->SetOrigin(origin);
}
