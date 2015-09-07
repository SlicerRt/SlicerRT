/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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
#include "vtkSlicerSegmentationsModuleLogic.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentationStorageNode.h"

// SegmentationCore includes
#include "vtkOrientedImageData.h"
#include "vtkSegmentationConverterFactory.h"
#include "vtkBinaryLabelmapToClosedSurfaceConversionRule.h"
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"
#include "vtkPlanarContourToClosedSurfaceConversionRule.h"

// Subject Hierarchy includes
#include <vtkMRMLSubjectHierarchyNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTrivialProducer.h>
#include <vtkMatrix4x4.h>
#include <vtkCallbackCommand.h>
#include <vtkPolyData.h>
#include <vtkImageAccumulate.h>
#include <vtkImageThreshold.h>
#include <vtkDataObject.h>
#include <vtkTransform.h>
#include <vtksys/SystemTools.hxx>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkEventBroker.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLLabelMapVolumeNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLTransformNode.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSegmentationsModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerSegmentationsModuleLogic::vtkSlicerSegmentationsModuleLogic()
{
  this->SubjectHierarchyUIDCallbackCommand = vtkCallbackCommand::New();
  this->SubjectHierarchyUIDCallbackCommand->SetClientData( reinterpret_cast<void *>(this) );
  this->SubjectHierarchyUIDCallbackCommand->SetCallback( vtkSlicerSegmentationsModuleLogic::OnSubjectHierarchyUIDAdded );
}

//----------------------------------------------------------------------------
vtkSlicerSegmentationsModuleLogic::~vtkSlicerSegmentationsModuleLogic()
{
  if (this->SubjectHierarchyUIDCallbackCommand)
  {
    this->SubjectHierarchyUIDCallbackCommand->SetClientData(NULL);
    this->SubjectHierarchyUIDCallbackCommand->Delete();
    this->SubjectHierarchyUIDCallbackCommand = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerSegmentationsModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentationsModuleLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  // events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerSegmentationsModuleLogic::RegisterNodes()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }

  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSegmentationNode>::New());
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSegmentationDisplayNode>::New());
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSegmentationStorageNode>::New());

  // Register converter rules
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(
    vtkSmartPointer<vtkBinaryLabelmapToClosedSurfaceConversionRule>::New() );
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(
    vtkSmartPointer<vtkClosedSurfaceToBinaryLabelmapConversionRule>::New() );
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(
    vtkSmartPointer<vtkPlanarContourToClosedSurfaceConversionRule>::New() );
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentationsModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node!");
    return;
  }

  if (node->IsA("vtkMRMLSubjectHierarchyNode"))
  {
    vtkEventBroker::GetInstance()->AddObservation(
      node, vtkMRMLSubjectHierarchyNode::SubjectHierarchyUIDAddedEvent, this, this->SubjectHierarchyUIDCallbackCommand );
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentationsModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or input node!");
    return;
  }

  if (node->IsA("vtkMRMLSegmentationNode"))
  {
    vtkEventBroker::GetInstance()->RemoveObservations(
      node, vtkMRMLSubjectHierarchyNode::SubjectHierarchyUIDAddedEvent, this, this->SubjectHierarchyUIDCallbackCommand );
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentationsModuleLogic::OnSubjectHierarchyUIDAdded(vtkObject* caller,
                                                                   unsigned long vtkNotUsed(eid),
                                                                   void* clientData,
                                                                   void* vtkNotUsed(callData))
{
  vtkSlicerSegmentationsModuleLogic* self = reinterpret_cast<vtkSlicerSegmentationsModuleLogic*>(clientData);
  if (!self)
  {
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNodeWithNewUID = reinterpret_cast<vtkMRMLSubjectHierarchyNode*>(caller);
  if (!shNodeWithNewUID)
  {
    return;
  }

  // Call callback function in all segmentation nodes. The callback function establishes the right
  // connection between loaded DICOM volumes and segmentations (related to reference image geometry)
  std::vector<vtkMRMLNode*> segmentationNodes;
  unsigned int numberOfNodes = self->GetMRMLScene()->GetNodesByClass("vtkMRMLSegmentationNode", segmentationNodes);
  for (unsigned int nodeIndex=0; nodeIndex<numberOfNodes; nodeIndex++)
  {
    vtkMRMLSegmentationNode* node = vtkMRMLSegmentationNode::SafeDownCast(segmentationNodes[nodeIndex]);
    if (node)
    {
      node->OnSubjectHierarchyUIDAdded(shNodeWithNewUID);
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentationsModuleLogic::OnMRMLSceneEndImport()
{
  // Re-generate merged labelmap for segmentation nodes after importing a scene, as their associated color table nodes
  // might have been loaded after the segmentation node, and merged labelmap generation relies on those nodes.
  std::vector<vtkMRMLNode*> segmentationNodes;
  unsigned int numberOfNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLSegmentationNode", segmentationNodes);
  for (unsigned int nodeIndex=0; nodeIndex<numberOfNodes; nodeIndex++)
  {
    vtkMRMLSegmentationNode* node = vtkMRMLSegmentationNode::SafeDownCast(segmentationNodes[nodeIndex]);
    if (node && node->HasMergedLabelmap())
    {
      node->ReGenerateDisplayedMergedLabelmap();
    }
  }
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentation(vtkMRMLScene* scene, vtkSegmentation* segmentation)
{
  if (!scene || !segmentation)
  {
    return NULL;
  }

  std::vector<vtkMRMLNode*> segmentationNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLSegmentationNode", segmentationNodes);
  for (unsigned int nodeIndex=0; nodeIndex<numberOfNodes; nodeIndex++)
  {
    vtkMRMLSegmentationNode* node = vtkMRMLSegmentationNode::SafeDownCast(segmentationNodes[nodeIndex]);
    if (node && node->GetSegmentation() == segmentation)
    {
      return node;
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegment(vtkMRMLScene* scene, vtkSegment* segment, std::string& segmentId)
{
  segmentId = "";
  if (!scene || !segment)
  {
    return NULL;
  }

  std::vector<vtkMRMLNode*> segmentationNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLSegmentationNode", segmentationNodes);
  for (unsigned int nodeIndex=0; nodeIndex<numberOfNodes; nodeIndex++)
  {
    vtkMRMLSegmentationNode* node = vtkMRMLSegmentationNode::SafeDownCast(segmentationNodes[nodeIndex]);
    vtkSegmentation::SegmentMap segmentMap = node->GetSegmentation()->GetSegments();
    for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
      if (segmentIt->second.GetPointer() == segment)
      {
        segmentId = segmentIt->first;
        return node;
      }
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkSlicerSegmentationsModuleLogic::LoadSegmentationFromFile(const char* filename)
{
  if (this->GetMRMLScene() == NULL || filename == NULL)
  {
    return NULL;
  }
  vtkSmartPointer<vtkMRMLSegmentationNode> segmentationNode = vtkSmartPointer<vtkMRMLSegmentationNode>::New();
  vtkSmartPointer<vtkMRMLSegmentationStorageNode> storageNode = vtkSmartPointer<vtkMRMLSegmentationStorageNode>::New();
  storageNode->SetFileName(filename);

  // Check to see which node can read this type of file
  if (!storageNode->SupportedFileType(filename))
  {
    vtkErrorMacro("LoadSegmentationFromFile: Segmentation storage node unable to load segmentation file.");
    return NULL;
  }

  std::string baseName = vtksys::SystemTools::GetFilenameWithoutExtension(filename);
  std::string uname( this->GetMRMLScene()->GetUniqueNameByString(baseName.c_str()));
  segmentationNode->SetName(uname.c_str());
  std::string storageUName = uname + "_Storage";
  storageNode->SetName(storageUName.c_str());
  this->GetMRMLScene()->SaveStateForUndo();
  this->GetMRMLScene()->AddNode(storageNode.GetPointer());

  segmentationNode->SetScene(this->GetMRMLScene());
  segmentationNode->SetAndObserveStorageNodeID(storageNode->GetID());

  this->GetMRMLScene()->AddNode(segmentationNode);

  // Read file
  vtkDebugMacro("LoadSegmentationFromFile: calling read on the storage node");
  int success = storageNode->ReadData(segmentationNode);
  if (success != 1)
  {
    vtkErrorMacro("LoadSegmentationFromFile: Error reading " << filename);
    this->GetMRMLScene()->RemoveNode(segmentationNode);
    return NULL;
  }

  // Show closed surface poly data if it exists (by default the master representation is shown
  // if it's poly data, but closed surface model is specifically for 3D visualization)
  if (segmentationNode->GetSegmentation()->ContainsRepresentation(
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName()) )
  {
    if (!segmentationNode->GetDisplayNode())
    {
      segmentationNode->CreateDefaultDisplayNodes();
    }
    vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
    if (displayNode)
    {
      displayNode->SetPolyDataDisplayRepresentationName(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());

      // If not loading segmentation from a scene (where display information is available),
      // then calculate and set auto-opacity for the displayed poly data for better visualization
      displayNode->CalculateAutoOpacitiesForSegments();
    }
  }

  return segmentationNode.GetPointer();
}

//-----------------------------------------------------------------------------
bool vtkSlicerSegmentationsModuleLogic::CreateLabelmapVolumeFromOrientedImageData(vtkOrientedImageData* orientedImageData, vtkMRMLLabelMapVolumeNode* labelmapVolumeNode)
{
  if (!orientedImageData)
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::CreateLabelmapVolumeFromOrientedImageData: Invalid input image data!";
    return false;
  }
  if (!labelmapVolumeNode)
  {
    vtkErrorWithObjectMacro(orientedImageData, "CreateLabelmapVolumeFromOrientedImageData: Invalid labelmap volume node!");
    return false;
  }

  // Create an identity (zero origin, unit spacing, identity orientation) vtkImageData that can be stored in vtkMRMLVolumeNode
  vtkSmartPointer<vtkImageData> identityImageData = vtkSmartPointer<vtkImageData>::New();
  identityImageData->ShallowCopy(orientedImageData);
  identityImageData->SetOrigin(0,0,0);
  identityImageData->SetSpacing(1,1,1);
  labelmapVolumeNode->SetAndObserveImageData(identityImageData);

  vtkSmartPointer<vtkMatrix4x4> labelmapImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  orientedImageData->GetImageToWorldMatrix(labelmapImageToWorldMatrix);
  labelmapVolumeNode->SetIJKToRASMatrix(labelmapImageToWorldMatrix);

  // Create default display node if it does not have one
  if (labelmapVolumeNode->GetScene())
  {
    vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode> labelmapVolumeDisplayNode = vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(
      labelmapVolumeNode->GetDisplayNode() );
    if (!labelmapVolumeDisplayNode.GetPointer())
    {
      labelmapVolumeDisplayNode = vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode>::New();
      labelmapVolumeNode->GetScene()->AddNode(labelmapVolumeDisplayNode);
      labelmapVolumeNode->SetAndObserveDisplayNodeID(labelmapVolumeDisplayNode->GetID());
    }
    labelmapVolumeDisplayNode->SetDefaultColorMap();
  }

  // Make sure merged labelmap extents starts at zeros for compatibility reasons
  vtkMRMLSegmentationNode::ShiftVolumeNodeExtentToZeroStart(labelmapVolumeNode);

  return true;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(vtkMRMLScalarVolumeNode* volumeNode)
{
  if (!volumeNode || !volumeNode->GetImageData())
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode: Invalid volume node!";
    return NULL;
  }

  vtkOrientedImageData* orientedImageData = vtkOrientedImageData::New();
  orientedImageData->vtkImageData::DeepCopy(volumeNode->GetImageData());

  vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  volumeNode->GetIJKToRASMatrix(ijkToRasMatrix);
  orientedImageData->SetGeometryFromImageToWorldMatrix(ijkToRasMatrix);

  return orientedImageData;
}

//-----------------------------------------------------------------------------
int vtkSlicerSegmentationsModuleLogic::DoesLabelmapContainSingleLabel(vtkMRMLLabelMapVolumeNode* labelmapVolumeNode)
{
  if (!labelmapVolumeNode)
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::DoesLabelmapContainSingleLabel: Invalid labelmap volume MRML node!";
    return 0;
  }
  vtkSmartPointer<vtkImageAccumulate> imageAccumulate = vtkSmartPointer<vtkImageAccumulate>::New();
#if (VTK_MAJOR_VERSION <= 5)
    imageAccumulate->SetInput(labelmapVolumeNode->GetImageData());
#else
    imageAccumulate->SetInputConnection(labelmapVolumeNode->GetImageDataConnection());
#endif
  imageAccumulate->Update();
  int highLabel = (int)imageAccumulate->GetMax()[0];
  if (highLabel == 0)
  {
    return 0;
  }

  imageAccumulate->IgnoreZeroOn();
  imageAccumulate->Update();
  int lowLabel = (int)imageAccumulate->GetMin()[0];
  highLabel = (int)imageAccumulate->GetMax()[0];
  if (lowLabel != highLabel)
  {
    return 0;
  }

  return lowLabel;
}

//-----------------------------------------------------------------------------
vtkSegment* vtkSlicerSegmentationsModuleLogic::CreateSegmentFromLabelmapVolumeNode(vtkMRMLLabelMapVolumeNode* labelmapVolumeNode)
{
  if (!labelmapVolumeNode)
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::CreateSegmentFromLabelmapVolumeNode: Invalid labelmap volume MRML node!";
    return NULL;
  }

  // Cannot create single segment from labelmap node if it contains more than one segment
  int label = vtkSlicerSegmentationsModuleLogic::DoesLabelmapContainSingleLabel(labelmapVolumeNode);
  if (!label)
  {
    vtkErrorWithObjectMacro(labelmapVolumeNode, "CreateSegmentFromLabelmapVolumeNode: Unable to create single segment from labelmap volume node, as labelmap contains more than one label!");
    return NULL;
  }

  // Create segment
  vtkSegment* segment = vtkSegment::New();
  segment->SetName(labelmapVolumeNode->GetName());

  // Set segment color
  double color[4] = { vtkSegment::SEGMENT_COLOR_VALUE_INVALID[0],
                      vtkSegment::SEGMENT_COLOR_VALUE_INVALID[1],
                      vtkSegment::SEGMENT_COLOR_VALUE_INVALID[2], 1.0 };
  vtkMRMLColorTableNode* colorNode = NULL;
  if (labelmapVolumeNode->GetDisplayNode())
  {
    colorNode = vtkMRMLColorTableNode::SafeDownCast(labelmapVolumeNode->GetDisplayNode()->GetColorNode());
    if (colorNode)
    {
      colorNode->GetColor(label, color);
    }
  }
  segment->SetDefaultColor(color[0], color[1], color[2]);

  // Create oriented image data from labelmap
  vtkSmartPointer<vtkOrientedImageData> orientedImageData = vtkSmartPointer<vtkOrientedImageData>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(labelmapVolumeNode) );

  // Add oriented image data as binary labelmap representation
  segment->AddRepresentation(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName(),
    orientedImageData );

  return segment;
}

//-----------------------------------------------------------------------------
vtkSegment* vtkSlicerSegmentationsModuleLogic::CreateSegmentFromModelNode(vtkMRMLModelNode* modelNode)
{
  if (!modelNode)
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::CreateSegmentFromModelNode: Invalid model MRML node!";
    return NULL;
  }
  if (!modelNode->GetPolyData())
  {
    vtkErrorWithObjectMacro(modelNode, "CreateSegmentFromModelNode: Model node does not contain poly data!");
    return NULL;
  }

  double color[3] = { vtkSegment::SEGMENT_COLOR_VALUE_INVALID[0],
                      vtkSegment::SEGMENT_COLOR_VALUE_INVALID[1],
                      vtkSegment::SEGMENT_COLOR_VALUE_INVALID[2] };

  // Create oriented image data from labelmap volume node
  vtkSegment* segment = vtkSegment::New();
    segment->SetName(modelNode->GetName());

  // Color from display node
  vtkMRMLDisplayNode* modelDisplayNode = modelNode->GetDisplayNode();
  if (modelDisplayNode)
  {
    modelDisplayNode->GetColor(color);
    segment->SetDefaultColor(color);
  }

  // Add model poly data as closed surface representation
  segment->AddRepresentation(
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName(),
    modelNode->GetPolyData() );
  
  return segment;
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* segmentShNode)
{
  if (!segmentShNode)
  {
    return NULL;
  }

  vtkMRMLSubjectHierarchyNode* parentShNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    segmentShNode->GetParentNode() );
  if (!parentShNode)
  {
    vtkWarningWithObjectMacro(segmentShNode, "vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyNode: Segment subject hierarchy node has no segmentation parent!");
    return NULL;
  }
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(parentShNode->GetAssociatedNode());
  if (!segmentationNode)
  {
    vtkWarningWithObjectMacro(segmentShNode, "vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyNode: Segment subject hierarchy node's parent has no associated segmentation node!");
    return NULL;
  }

  return segmentationNode;
}

//-----------------------------------------------------------------------------
vtkSegment* vtkSlicerSegmentationsModuleLogic::GetSegmentForSegmentSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* segmentShNode)
{
  vtkMRMLSegmentationNode* segmentationNode =
    vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyNode(segmentShNode);
  if (!segmentationNode)
  {
    return NULL;
  }

  const char* segmentId = segmentShNode->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName());
  if (!segmentId)
  {
    vtkWarningWithObjectMacro(segmentShNode, "vtkSlicerSegmentationsModuleLogic::GetSegmentForSegmentSubjectHierarchyNode: Segment subject hierarchy node does not contain segment ID!");
    return NULL;
  }

  vtkSegment* segment = segmentationNode->GetSegmentation()->GetSegment(segmentId);
  if (!segment)
  {
    vtkErrorWithObjectMacro(segmentShNode, "vtkSlicerSegmentationsModuleLogic::GetSegmentForSegmentSubjectHierarchyNode: Segmentation does not contain segment with given ID!");
  }

  return segment;
}

//-----------------------------------------------------------------------------
bool vtkSlicerSegmentationsModuleLogic::ExportSegmentToRepresentationNode(vtkSegment* segment, vtkMRMLNode* representationNode)
{
  if (!segment)
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::ExportSegmentToRepresentationNode: Invalid segment!";
    return false;
  }
  if (!representationNode)
  {
    vtkErrorWithObjectMacro(segment, "ExportSegmentToRepresentationNode: Invalid representation MRML node!");
    return false;
  }
  vtkMRMLLabelMapVolumeNode* labelmapNode = vtkMRMLLabelMapVolumeNode::SafeDownCast(representationNode);
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(representationNode);
  if (!labelmapNode && !modelNode)
  {
    vtkErrorWithObjectMacro(representationNode, "ExportSegmentToRepresentationNode: Representation MRML node should be either labelmap volume node or model node!");
    return false;
  }

  // Determine segment ID and set it as name of the representation node if found
  std::string segmentId("");
  vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegment(
    representationNode->GetScene(), segment, segmentId);
  if (segmentationNode)
  {
    representationNode->SetName(segmentId.c_str());
  }
  vtkMRMLTransformNode* parentTransformNode = segmentationNode->GetParentTransformNode();

  if (labelmapNode)
  {
    // Make sure binary labelmap representation exists in segment
    bool binaryLabelmapPresent = segment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
    if (!binaryLabelmapPresent && !segmentationNode)
    {
      vtkErrorWithObjectMacro(representationNode, "ExportSegmentToRepresentationNode: Segment does not contain binary labelmap representation and cannot convert, because it is not in a segmentation!");
      return false;
    }
    binaryLabelmapPresent = segmentationNode->GetSegmentation()->CreateRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
    if (!binaryLabelmapPresent)
    {
      vtkErrorWithObjectMacro(representationNode, "ExportSegmentToRepresentationNode: Unable to convert segment to binary labelmap representation!");
      return false;
    }

    // Export binary labelmap representation into labelmap volume node
    vtkOrientedImageData* orientedImageData = vtkOrientedImageData::SafeDownCast(
      segment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
    bool success = vtkSlicerSegmentationsModuleLogic::CreateLabelmapVolumeFromOrientedImageData(orientedImageData, labelmapNode);
    if (!success)
    {
      return false;
    }

    // Set segmentation's parent transform to exported node
    if (parentTransformNode)
    {
      labelmapNode->SetAndObserveTransformNodeID(parentTransformNode->GetID());
    }

    return true;
  }
  else if (modelNode)
  {
    // Make sure closed surface representation exists in segment
    bool closedSurfacePresent = segmentationNode->GetSegmentation()->CreateRepresentation(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
    if (!closedSurfacePresent)
    {
      vtkErrorWithObjectMacro(representationNode, "ExportSegmentToRepresentationNode: Unable to convert segment to closed surface representation!");
      return false;
    }

    // Export binary labelmap representation into labelmap volume node
    vtkPolyData* polyData = vtkPolyData::SafeDownCast(
      segment->GetRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName()) );
    vtkSmartPointer<vtkPolyData> polyDataCopy = vtkSmartPointer<vtkPolyData>::New();
    polyDataCopy->DeepCopy(polyData); // Make copy of poly data so that the model node does not change if segment changes
    modelNode->SetAndObservePolyData(polyDataCopy);

    // Set color of the exported model
    vtkMRMLSegmentationDisplayNode* segmentationDisplayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
    vtkMRMLDisplayNode* modelDisplayNode = modelNode->GetDisplayNode();
    if (!modelDisplayNode)
    {
      // Create display node
      vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
      displayNode = vtkMRMLModelDisplayNode::SafeDownCast(modelNode->GetScene()->AddNode(displayNode));
      displayNode->VisibilityOn(); 
      modelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
      modelDisplayNode = displayNode.GetPointer();
    }
    if (segmentationDisplayNode && modelDisplayNode)
    {
      vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
      if (segmentationDisplayNode->GetSegmentDisplayProperties(segmentId, properties))
      {
        modelDisplayNode->SetColor(properties.Color);
      }
    }

    // Set segmentation's parent transform to exported node
    if (parentTransformNode)
    {
      modelNode->SetAndObserveTransformNodeID(parentTransformNode->GetID());
    }

    return true;
  }

  // Representation node is neither labelmap, nor model
  return false;
}

//-----------------------------------------------------------------------------
bool vtkSlicerSegmentationsModuleLogic::ExportSegmentsToLabelmapNode(vtkMRMLSegmentationNode* segmentationNode, std::vector<std::string>& segmentIDs, vtkMRMLLabelMapVolumeNode* labelmapNode)
{
  if (!segmentationNode)
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::ExportSegmentsToLabelmapNode: Invalid segmentation node!";
    return false;
  }
  if (!labelmapNode)
  {
    vtkErrorWithObjectMacro(segmentationNode, "ExportSegmentsToLabelmapNode: Invalid labelmap volume node!");
    return false;
  }

  // Make sure binary labelmap representation exists in segment
  bool binaryLabelmapPresent = segmentationNode->GetSegmentation()->CreateRepresentation(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
  if (!binaryLabelmapPresent)
  {
    vtkErrorWithObjectMacro(segmentationNode, "ExportSegmentsToLabelmapNode: Unable to convert segment to binary labelmap representation!");
    return false;
  }

  // Generate merged labelmap for the exported segments
  vtkSmartPointer<vtkOrientedImageData> mergedImage = vtkSmartPointer<vtkOrientedImageData>::New();
  vtkSmartPointer<vtkMatrix4x4> mergedImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (!segmentationNode->GenerateMergedLabelmap(mergedImage, mergedImageToWorldMatrix, segmentIDs))
  {
    vtkErrorWithObjectMacro(segmentationNode, "ExportSegmentsToLabelmapNode: Failed to generate merged labelmap!");
    return false;
  }

  // Set calculated geometry to merged oriented image
  mergedImage->SetGeometryFromImageToWorldMatrix(mergedImageToWorldMatrix);

  // Export merged labelmap to the output node
  if (!vtkSlicerSegmentationsModuleLogic::CreateLabelmapVolumeFromOrientedImageData(mergedImage, labelmapNode))
  {
    vtkErrorWithObjectMacro(segmentationNode, "ExportSegmentsToLabelmapNode: Failed to create labelmap from merged segments image!");
    return false;
  }

  // Set segmentation's color table to labelmap so that the labels appear in the same color
  if (labelmapNode->GetDisplayNode())
  {
    if (segmentationNode->GetDisplayNode() && segmentationNode->GetDisplayNode()->GetColorNode())
    {
      labelmapNode->GetDisplayNode()->SetAndObserveColorNodeID(segmentationNode->GetDisplayNode()->GetColorNodeID());
    }
  }

  // Set segmentation's parent transform to exported node
  vtkMRMLTransformNode* parentTransformNode = segmentationNode->GetParentTransformNode();
  if (parentTransformNode)
  {
    labelmapNode->SetAndObserveTransformNodeID(parentTransformNode->GetID());
  }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSlicerSegmentationsModuleLogic::ExportAllSegmentsToLabelmapNode(vtkMRMLSegmentationNode* segmentationNode, vtkMRMLLabelMapVolumeNode* labelmapNode)
{
  std::vector<std::string> segmentIDs;
  return vtkSlicerSegmentationsModuleLogic::ExportSegmentsToLabelmapNode(segmentationNode, segmentIDs, labelmapNode);
}

//-----------------------------------------------------------------------------
bool vtkSlicerSegmentationsModuleLogic::ImportLabelmapToSegmentationNode(vtkMRMLLabelMapVolumeNode* labelmapNode, vtkMRMLSegmentationNode* segmentationNode)
{
  if (!segmentationNode)
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::ImportLabelmapToSegmentationNode: Invalid segmentation node!";
    return false;
  }
  if (!labelmapNode)
  {
    vtkErrorWithObjectMacro(segmentationNode, "ImportLabelmapToSegmentationNode: Invalid labelmap volume node!");
    return false;
  }

  // If master representation is not binary labelmap, then cannot add
  // (this should have been done by the UI classes, notifying the users about hazards of changing the master representation)
  if ( !segmentationNode->GetSegmentation()->GetMasterRepresentationName()
    || strcmp(segmentationNode->GetSegmentation()->GetMasterRepresentationName(), vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) )
  {
    vtkErrorWithObjectMacro(segmentationNode, "ImportLabelmapToSegmentationNode: Master representation of the target segmentation node " << (segmentationNode->GetName()?segmentationNode->GetName():"NULL") << " is not binary labelmap!");
    return false;
  }

  // Get labelmap geometry
  vtkSmartPointer<vtkMatrix4x4> labelmapIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  labelmapNode->GetIJKToRASMatrix(labelmapIjkToRasMatrix);

  // Note: Splitting code ported from EditorLib/HelperBox.py:split

  // Get color node
  vtkMRMLColorTableNode* colorNode = NULL;
  if (labelmapNode->GetDisplayNode())
  {
    colorNode = vtkMRMLColorTableNode::SafeDownCast(labelmapNode->GetDisplayNode()->GetColorNode());
  }

  // Split labelmap node into per-label image data
  vtkSmartPointer<vtkImageAccumulate> imageAccumulate = vtkSmartPointer<vtkImageAccumulate>::New();
#if (VTK_MAJOR_VERSION <= 5)
    imageAccumulate->SetInput(labelmapNode->GetImageData());
#else
    imageAccumulate->SetInputConnection(labelmapNode->GetImageDataConnection());
#endif
  imageAccumulate->IgnoreZeroOn(); // Do not create segment from background
  imageAccumulate->Update();
  int lowLabel = (int)imageAccumulate->GetMin()[0];
  int highLabel = (int)imageAccumulate->GetMax()[0];

  // Set master representation to binary labelmap
  segmentationNode->GetSegmentation()->SetMasterRepresentationName(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());

  vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
  //TODO: pending resolution of bug http://www.na-mic.org/Bug/view.php?id=1822,
  //   run the thresholding in single threaded mode to avoid data corruption observed on mac release builds
  threshold->SetNumberOfThreads(1);
  for (int label = lowLabel; label <= highLabel; ++label)
  {
#if (VTK_MAJOR_VERSION <= 5)
    threshold->SetInput(labelmapNode->GetImageData());
#else
    threshold->SetInputConnection(labelmapNode->GetImageDataConnection());
#endif
    threshold->SetInValue(label);
    threshold->SetOutValue(0);
    threshold->ReplaceInOn();
    threshold->ReplaceOutOn();
    threshold->ThresholdBetween(label, label);
    threshold->SetOutputScalarType(labelmapNode->GetImageData()->GetScalarType());
    threshold->Update();

    double* outputScalarRange = threshold->GetOutput()->GetScalarRange();
    if (outputScalarRange[0] == 0.0 && outputScalarRange[1] == 0.0)
    {
      continue;
    }

    // Create oriented image data for label
    vtkSmartPointer<vtkOrientedImageData> labelOrientedImageData = vtkSmartPointer<vtkOrientedImageData>::New();
    labelOrientedImageData->vtkImageData::DeepCopy(threshold->GetOutput());
    labelOrientedImageData->SetGeometryFromImageToWorldMatrix(labelmapIjkToRasMatrix);

    vtkSmartPointer<vtkSegment> segment = vtkSmartPointer<vtkSegment>::New();

    // Set segment color
    double color[4] = { vtkSegment::SEGMENT_COLOR_VALUE_INVALID[0],
                        vtkSegment::SEGMENT_COLOR_VALUE_INVALID[1],
                        vtkSegment::SEGMENT_COLOR_VALUE_INVALID[2], 1.0 };
    const char* labelName = NULL;
    if (colorNode)
    {
      labelName = colorNode->GetColorName(label);
      colorNode->GetColor(label, color);
    }
    segment->SetDefaultColor(color[0], color[1], color[2]);

    // If there is only one label, then the (only) segment name will be the labelmap name
    if (lowLabel == highLabel)
    {
      labelName = labelmapNode->GetName();
    }

    // Set segment name
    if (!labelName)
    {
      std::stringstream ss;
      ss << "Label_" << label;
      labelName = ss.str().c_str();
    }
    segment->SetName(labelName);
  
    // Add oriented image data as binary labelmap representation
    segment->AddRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName(),
      labelOrientedImageData );

    segmentationNode->GetSegmentation()->AddSegment(segment);
  } // for each label

  return true;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkSlicerSegmentationsModuleLogic::CreateRepresentationForOneSegment(vtkSegmentation* segmentation, std::string segmentID, std::string representationName)
{
  if (!segmentation)
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::CreateRepresentationForOneSegment: Invalid segmentation!";
    return NULL;
  }

  // Temporarily duplicate selected segment to only convert them, not the whole segmentation (to save time)
  vtkSmartPointer<vtkSegmentation> segmentationCopy = vtkSmartPointer<vtkSegmentation>::New();
  segmentationCopy->SetMasterRepresentationName(segmentation->GetMasterRepresentationName());
  segmentationCopy->CopyConversionParameters(segmentation);
  segmentationCopy->CopySegmentFromSegmentation(segmentation, segmentID);
  if (!segmentationCopy->CreateRepresentation(representationName, true))
  {
    vtkErrorWithObjectMacro(segmentation, "CreateRepresentationForOneSegment: Failed to convert segment " << segmentID << " to " << representationName);
    return NULL;
  }

  // If conversion succeeded, 
  vtkDataObject* segmentTempRepresentation = vtkDataObject::SafeDownCast(
    segmentationCopy->GetSegment(segmentID)->GetRepresentation(representationName) );
  if (!segmentTempRepresentation)
  {
    vtkErrorWithObjectMacro(segmentation, "CreateRepresentationForOneSegment: Failed to get representation " << representationName << " from segment " << segmentID);
    return NULL;
  }

  // Copy representation into new data object (the representation will be deleted when segmentation copy gets out of scope)
  vtkDataObject* representationCopy =
    vtkSegmentationConverterFactory::GetInstance()->ConstructRepresentationObjectByClass(segmentTempRepresentation->GetClassName());
  representationCopy->ShallowCopy(segmentTempRepresentation);
  return representationCopy;
}

//-----------------------------------------------------------------------------
bool vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(vtkMRMLTransformableNode* transformableNode, vtkOrientedImageData* orientedImageData)
{
  if (!transformableNode || !orientedImageData)
  {
    std::cerr << "vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData: Invalid inputs!";
    return false;
  }

  // Get world to reference RAS transform matrix
  vtkSmartPointer<vtkMatrix4x4> nodeToWorldTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  nodeToWorldTransformMatrix->Identity();
  vtkMRMLTransformNode* parentTransformNode = transformableNode->GetParentTransformNode();
  if (parentTransformNode)
  {
    if (!parentTransformNode->IsTransformToWorldLinear())
    {
      vtkErrorWithObjectMacro(transformableNode, "ApplyParentTransformToOrientedImageData: Segmentation " << transformableNode->GetName() << " has a deformable transformation applied, which is not allowed for this operation");
      return false;
    }
    else
    {
      parentTransformNode->GetMatrixTransformToWorld(nodeToWorldTransformMatrix);
    }
  }
  else
  {
    // There is no parent transform for segmentation, nothing to apply
    return true;
  }

  // Get image data IJK to segmentation transform matrix
  vtkSmartPointer<vtkMatrix4x4> imageDataIjkToSegmentationMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  orientedImageData->GetImageToWorldMatrix(imageDataIjkToSegmentationMatrix);

  // Apply transform on oriented image data to transform it to world space
  vtkSmartPointer<vtkTransform> imageDataToWorldTransform = vtkSmartPointer<vtkTransform>::New();
  imageDataToWorldTransform->Identity();
  imageDataToWorldTransform->PostMultiply();
  imageDataToWorldTransform->Concatenate(imageDataIjkToSegmentationMatrix);
  imageDataToWorldTransform->Concatenate(nodeToWorldTransformMatrix);

  vtkSmartPointer<vtkMatrix4x4> imageDataToWorldTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  imageDataToWorldTransform->GetMatrix(imageDataToWorldTransformMatrix);
  orientedImageData->SetGeometryFromImageToWorldMatrix(imageDataToWorldTransformMatrix);

  return true;
}
