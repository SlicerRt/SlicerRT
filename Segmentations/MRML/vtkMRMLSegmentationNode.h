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

#ifndef __vtkMRMLSegmentationNode_h
#define __vtkMRMLSegmentationNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLLabelMapVolumeNode.h>

// Segmentation includes
#include "vtkSegmentation.h"

#include "vtkSlicerSegmentationsModuleMRMLExport.h"

class vtkCallbackCommand;
class vtkMRMLScene;
class vtkMRMLSubjectHierarchyNode;

/// \ingroup Segmentations
class VTK_SLICER_SEGMENTATIONS_MODULE_MRML_EXPORT vtkMRMLSegmentationNode : public vtkMRMLLabelMapVolumeNode
{
public:
  // Define constants
  static const char* GetSegmentIDAttributeName() { return "segmentID"; };

  static vtkMRMLSegmentationNode *New();
  vtkTypeMacro(vtkMRMLSegmentationNode, vtkMRMLLabelMapVolumeNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Copy the entire contents of the node into this node
  virtual void DeepCopy(vtkMRMLNode* node);

  /// Get unique node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "Segmentation";};

  /// Get bounding box in global RAS in the form (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual void GetRASBounds(double bounds[6]);

  /// Handles events registered in the observer manager
  /// - Invalidates (deletes) all non-active representations when the active is modified
  /// - Follows parent transform changes
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

  /// Returns true if the transformable node can apply non linear transforms
  /// \sa ApplyTransform
  virtual bool CanApplyNonLinearTransforms()const;

  /// Apply a transform matrix on the segmentation
  /// \sa SetAndObserveTransformNodeID, ApplyTransform, CanApplyNonLinearTransforms
  virtual void ApplyTransformMatrix(vtkMatrix4x4* transformMatrix);

  /// Apply a transform on the segmentation
  /// \sa SetAndObserveTransformNodeID, CanApplyNonLinearTransforms
  virtual void ApplyTransform(vtkAbstractTransform* transform);

  /// Create a segmentation storage node
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode();

  /// Create and observe a segmentation display node
  virtual void CreateDefaultDisplayNodes();

  /// Reimplemented to take into account the modified time of the internal data.
  /// Returns true if the node (default behavior) or the internal data are modified
  /// since read/written.
  /// Note: The MTime of the internal data is used to know if it has been modified.
  /// So if you invoke one of the data modified events without calling Modified() on the
  /// internal data, GetModifiedSinceRead() won't return true.
  /// \sa vtkMRMLStorableNode::GetModifiedSinceRead()
  virtual bool GetModifiedSinceRead();

  /// Function called from segmentation logic when UID is added in a subject hierarchy node.
  /// In case the newly added UID is a volume node referenced from this segmentation,
  /// its geometry will be set as image geometry conversion parameter.
  /// The "other order", i.e. when the volume is loaded first and the segmentation second,
  /// should be handled at loading time of the segmentation (because then we already know about the volume)
  /// \param shNodeWithNewUID Subject hierarchy node that just got a new UID
  void OnSubjectHierarchyUIDAdded(vtkMRMLSubjectHierarchyNode* shNodeWithNewUID);

  /// Get subject hierarchy node belonging to a certain segment
  vtkMRMLSubjectHierarchyNode* GetSegmentSubjectHierarchyNode(std::string segmentID);

//BTX
  /// Build merged labelmap of the binary labelmap representations of the specified segments
  /// \param mergedImageData Output image data for the merged labelmap image data
  /// \param mergedImageToWorldMatrix Image to world matrix for the output labelmap
  ///    (can be set to a volume node or to an oriented image data)
  /// \param segmentIDs List of IDs of segments to include in the merged labelmap. If empty or missing, then all segments are included
  /// \return Success flag
  virtual bool GenerateMergedLabelmap(vtkImageData* mergedImageData, vtkMatrix4x4* mergedImageToWorldMatrix, const std::vector<std::string>& segmentIDs=std::vector<std::string>());
//ETX

  /// Re-generate displayed merged labelmap
  void ReGenerateDisplayedMergedLabelmap();

  /// Make sure image data of a volume node has extents that start at zero.
  /// This needs to be done for compatibility reasons, as many components assume the extent has a form of
  /// (0,dim[0],0,dim[1],0,dim[2]), which is not the case many times for segmentation merged labelmaps.
  static void ShiftVolumeNodeExtentToZeroStart(vtkMRMLScalarVolumeNode* volumeNode);

public:
  /// Get segmentation object
  vtkGetObjectMacro(Segmentation, vtkSegmentation);
  /// Set and observe segmentation object
  void SetAndObserveSegmentation(vtkSegmentation* segmentation);

  /// Generate merged labelmap image data for display if needed
  virtual vtkImageData* GetImageData();
  /// Query existence or merged labelmap. Do not trigger merged labelmap generation
  bool HasMergedLabelmap();

protected:
  /// Build merged labelmap for 2D labelmap display from all contained segments
  virtual bool GenerateDisplayedMergedLabelmap(vtkImageData* imageData);

  /// Add display properties for segment with given ID
  virtual bool AddSegmentDisplayProperties(std::string segmentId);

  /// Reset all display related data. Called when display node reference is added or modified
  virtual void ResetSegmentDisplayProperties();

protected:
  /// Set segmentation object
  vtkSetObjectMacro(Segmentation, vtkSegmentation);

  /// Called when a node reference ID is added (list size increased).
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference);

  /// Called when a node reference ID is modified.
  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference);

  /// Callback function observing the master representation of the segmentation (and each segment within)
  /// Invalidates all representations other than the master. These representations will be automatically converted later on demand.
  static void OnMasterRepresentationModified(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  /// Callback function observing segment added events.
  /// Triggers update of display properties
  static void OnSegmentAdded(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  /// Callback function observing segment removed events.
  /// Triggers update of display properties
  static void OnSegmentRemoved(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  /// Callback function observing segment modified events.
  /// Forwards event from the node.
  static void OnSegmentModified(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  /// Callback function observing representation modified events.
  /// Forwards event from the node.
  static void OnRepresentationCreated(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

protected:
  vtkMRMLSegmentationNode();
  ~vtkMRMLSegmentationNode();
  vtkMRMLSegmentationNode(const vtkMRMLSegmentationNode&);
  void operator=(const vtkMRMLSegmentationNode&);

  /// Segmentation object to store the actual data
  vtkSegmentation* Segmentation;

  /// Keep track of merged labelmap modification time
  vtkTimeStamp LabelmapMergeTime;

  /// Command handling master representation modified events
  vtkCallbackCommand* MasterRepresentationCallbackCommand;

  /// Command handling segment added event
  vtkCallbackCommand* SegmentAddedCallbackCommand;

  /// Command handling segment removed event
  vtkCallbackCommand* SegmentRemovedCallbackCommand;

  /// Command handling segment modified event
  vtkCallbackCommand* SegmentModifiedCallbackCommand;

  /// Command handling representation created event
  vtkCallbackCommand* RepresentationCreatedCallbackCommand;
};

#endif // __vtkMRMLSegmentationNode_h
