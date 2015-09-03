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

// .NAME vtkSlicerSegmentationsModuleLogic - Logic class for segmentation handling
// .SECTION Description
// This class manages the logic associated with converting and handling
// segmentation node objects.

#ifndef __vtkSlicerSegmentationsModuleLogic_h
#define __vtkSlicerSegmentationsModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"
#include "vtkSlicerSegmentationsModuleLogicExport.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

class vtkCallbackCommand;
class vtkOrientedImageData;
class vtkDataObject;

class vtkMRMLScalarVolumeNode;
class vtkMRMLSegmentationStorageNode;
class vtkMRMLLabelMapVolumeNode;
class vtkMRMLModelNode;

/// \ingroup SlicerRt_QtModules_Segmentations
class VTK_SLICER_SEGMENTATIONS_LOGIC_EXPORT vtkSlicerSegmentationsModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerSegmentationsModuleLogic *New();
  vtkTypeMacro(vtkSlicerSegmentationsModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  //TODO:
  /// Create a segmentation node for the given segmentation node
  /// \param segmentationNode the node that requires a storage node be created
  //static vtkMRMLSegmentationStorageNode* CreateSegmentationStorageNode(vtkMRMLSegmentationNode* segmentationNode);

  /// Get segmentation node containing a segmentation object. As segmentation objects are out-of-MRML
  /// VTK objects, there is no direct link from it to its parent node, so must be found from the MRML scene.
  /// \param scene MRML scene
  /// \param segmentation Segmentation to find
  /// \return Segmentation node containing the given segmentation if any, NULL otherwise
  static vtkMRMLSegmentationNode* GetSegmentationNodeForSegmentation(vtkMRMLScene* scene, vtkSegmentation* segmentation);

  /// Get segmentation node containing a given segment. As segments are out-of-MRML
  /// VTK objects, there is no direct link from it to its parent node, so must be found from the MRML scene.
  /// \param scene MRML scene
  /// \param segment Segment to find
  /// \param segmentId Output argument for the ID of the found segment
  /// \return Segmentation node containing the given segment if any, NULL otherwise
  static vtkMRMLSegmentationNode* GetSegmentationNodeForSegment(vtkMRMLScene* scene, vtkSegment* segment, std::string& segmentId);

  /// Load segmentation from file
  vtkMRMLSegmentationNode* LoadSegmentationFromFile(const char* filename);

  /// Create labelmap volume MRML node from oriented image data
  /// \param orientedImageData Oriented image data to create labelmap from
  /// \param labelmapVolumeNode Labelmap volume to be populated with the oriented image data. The volume node needs to exist
  ///   and be added to the MRML scene
  /// \return Success flag
  static bool CreateLabelmapVolumeFromOrientedImageData(vtkOrientedImageData* orientedImageData, vtkMRMLLabelMapVolumeNode* labelmapVolumeNode);

  /// Create oriented image data from a volume node
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkOrientedImageData>::Take
  static vtkOrientedImageData* CreateOrientedImageDataFromVolumeNode(vtkMRMLScalarVolumeNode* volumeNode);

  /// Utility function to determine if a labelmap contains a single label
  /// \return 0 if contains no label or multiple labels, the label if it contains a single one
  static int DoesLabelmapContainSingleLabel(vtkMRMLLabelMapVolumeNode* labelmapVolumeNode);

  /// Create segment from labelmap volume MRML node. The contents are set as binary labelmap representation in the segment.
  /// Returns NULL if labelmap contains more than one label. In that case \sa ImportLabelmapToSegmentationNode needs to be used.
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkSegment>::Take
  static vtkSegment* CreateSegmentFromLabelmapVolumeNode(vtkMRMLLabelMapVolumeNode* labelmapVolumeNode);

  /// Create segment from model MRML node.
  /// The contents are set as closed surface model representation in the segment.
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkSegment>::Take
  static vtkSegment* CreateSegmentFromModelNode(vtkMRMLModelNode* modelNode);

  /// Utility function for getting the segmentation node for a segment subject hierarchy node
  static vtkMRMLSegmentationNode* GetSegmentationNodeForSegmentSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* segmentShNode);

  /// Utility function for getting the segment object for a segment subject hierarchy node
  static vtkSegment* GetSegmentForSegmentSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* segmentShNode);

  /// Export segment to representation MRML node.
  /// 1. If representation node is a labelmap node, then the binary labelmap representation of the
  ///    segment is copied
  /// 2. If representation node is a model node, then the closed surface representation is copied
  /// Otherwise return with failure.
  static bool ExportSegmentToRepresentationNode(vtkSegment* segment, vtkMRMLNode* representationNode);

  /// Export multiple segments into a multi-label labelmap volume node
  /// \param segmentationNode Segmentation node from which the the segments are exported
  /// \param segmentIds List of segment IDs to export
  /// \param labelmapNode Labelmap node to export the segments to
  static bool ExportSegmentsToLabelmapNode(vtkMRMLSegmentationNode* segmentationNode, std::vector<std::string>& segmentIDs, vtkMRMLLabelMapVolumeNode* labelmapNode);

  /// Export all segments into a multi-label labelmap volume node
  /// \param segmentationNode Segmentation node from which the the segments are exported
  /// \param labelmapNode Labelmap node to export the segments to
  static bool ExportAllSegmentsToLabelmapNode(vtkMRMLSegmentationNode* segmentationNode, vtkMRMLLabelMapVolumeNode* labelmapNode);

  /// Import all labels from a labelmap to a segmentation node, each label to a separate segment
  static bool ImportLabelmapToSegmentationNode(vtkMRMLLabelMapVolumeNode* labelmapNode, vtkMRMLSegmentationNode* segmentationNode);

  /// Create representation of only one segment in a segmentation.
  /// Useful if only one segment is processed, and we do not want to convert all segments to a certain
  /// segmentation to save time.
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkDataObject>::Take
  /// \return Representation of the specified segment if found or can be created, NULL otherwise
  static vtkDataObject* CreateRepresentationForOneSegment(vtkSegmentation* segmentation, std::string segmentID, std::string representationName);

  /// Apply the parent transform of a node to an oriented image data.
  /// Useful if we want to get a labelmap representation of a segmentation in the proper geometry for processing.
  /// \return Success flag
  static bool ApplyParentTransformToOrientedImageData(vtkMRMLTransformableNode* transformableNode, vtkOrientedImageData* orientedImageData);

protected:
  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  /// Callback function observing UID added events for subject hierarchy nodes.
  /// In case the newly added UID is a volume node referenced from a segmentation,
  /// its geometry will be set as image geometry conversion parameter.
  /// The "other order", i.e. when the volume is loaded first and the segmentation second,
  /// should be handled at loading time of the segmentation (because then we already know about the volume)
  static void OnSubjectHierarchyUIDAdded(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  /// Handle MRML node added events
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);

  /// Handle MRML node removed events
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// Handle MRML scene import ended event
  virtual void OnMRMLSceneEndImport();

protected:
  vtkSlicerSegmentationsModuleLogic();
  virtual ~vtkSlicerSegmentationsModuleLogic();

  /// Command handling subject hierarchy UID added events
  vtkCallbackCommand* SubjectHierarchyUIDCallbackCommand;

private:
  vtkSlicerSegmentationsModuleLogic(const vtkSlicerSegmentationsModuleLogic&); // Not implemented
  void operator=(const vtkSlicerSegmentationsModuleLogic&);               // Not implemented
};

#endif
