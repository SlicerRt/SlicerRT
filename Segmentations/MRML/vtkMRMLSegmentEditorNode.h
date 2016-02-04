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

#ifndef __vtkMRMLSegmentEditorNode_h
#define __vtkMRMLSegmentEditorNode_h

// MRML includes
#include <vtkMRMLNode.h>

// Segmentations includes
#include "vtkSlicerSegmentationsModuleMRMLExport.h"

#include "vtkOrientedImageData.h"

class vtkMRMLScene;
class vtkMRMLScalarVolumeNode;
class vtkMRMLSegmentationNode;

/// \ingroup Segmentations
/// \brief Parameter set node for the segment editor widget
///
/// Stores parameters for a segment editor widget (selected segmentation, segment, master volume),
/// and all the editor effects. The effect parameters are stored as attributes with names
/// EffectName.ParameterName. If a parameter is changed, the node Modified event is not emitted,
/// but the custom EffectParameterModified event that triggers update of the effect options widget only.
///
class VTK_SLICER_SEGMENTATIONS_MODULE_MRML_EXPORT vtkMRMLSegmentEditorNode : public vtkMRMLNode
{
public:
  enum
  {
    /// Fired when an effect parameter is modified. As this node handles not only the effect parameters,
    /// but also the segment editor state, a full Modified event is an overkill, because it would trigger
    /// editor widget UI update, instead of simple update of the effect option widgets only.
    EffectParameterModified = 62200
  };

public:
  static vtkMRMLSegmentEditorNode *New();
  vtkTypeMacro(vtkMRMLSegmentEditorNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() { return "SegmentEditor"; };

public:
  /// Get master volume node
  vtkMRMLScalarVolumeNode* GetMasterVolumeNode();
  /// Set and observe master volume node
  void SetAndObserveMasterVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get segmentation node
  vtkMRMLSegmentationNode* GetSegmentationNode();
  /// Set and observe segmentation node
  void SetAndObserveSegmentationNode(vtkMRMLSegmentationNode* node);

  /// Get selected segment ID
  vtkGetStringMacro(SelectedSegmentID);
  /// Set selected segment ID
  vtkSetStringMacro(SelectedSegmentID);

  /// Get active effect name
  vtkGetStringMacro(ActiveEffectName);
  /// Set active effect name
  vtkSetStringMacro(ActiveEffectName);

  /// Get edited labelmap
  vtkGetObjectMacro(EditedLabelmap, vtkOrientedImageData);
  /// Get mask labelmap
  vtkGetObjectMacro(MaskLabelmap, vtkOrientedImageData);
  /// Get threshold labelmap
  vtkGetObjectMacro(ThresholdLabelmap, vtkOrientedImageData);

protected:
  vtkMRMLSegmentEditorNode();
  ~vtkMRMLSegmentEditorNode();
  vtkMRMLSegmentEditorNode(const vtkMRMLSegmentEditorNode&);
  void operator=(const vtkMRMLSegmentEditorNode&);

  /// Selected segment ID
  char* SelectedSegmentID;

  /// Active effect name
  char* ActiveEffectName;

  /// Active labelmap for editing. Mainly needed because the segment binary labelmaps are shrunk
  /// to the smallest possible extent, but the user wants to draw on the whole master volume.
  vtkOrientedImageData* EditedLabelmap;

  /// Mask labelmap containing a merged silhouette of all the segments other than the selected one.
  /// Used if the paint over feature is turned off.
  vtkOrientedImageData* MaskLabelmap;

  /// Threshold labelmap containing the thresholded master volume based on the threshold settings.
  vtkOrientedImageData* ThresholdLabelmap;
};

#endif // __vtkMRMLSegmentEditorNode_h
