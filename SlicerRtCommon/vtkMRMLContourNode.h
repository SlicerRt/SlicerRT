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

#ifndef __vtkMRMLContourNode_h
#define __vtkMRMLContourNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLScene.h>

class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;

class vtkMRMLContourNode : public vtkMRMLDisplayableNode
{
public:
  enum ContourRepresentationType
  {
    None = -1,
    RibbonModel,
    IndexedLabelmap,
    ClosedSurfaceModel,
    NumberOfRepresentationTypes
  };

public:
  static vtkMRMLContourNode *New();
  vtkTypeMacro(vtkMRMLContourNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Updates the referenced nodes from the updated scene
  virtual void UpdateScene(vtkMRMLScene *scene);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "Contour";};

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  /// Updates this node if it depends on other nodes 
  /// when the node is deleted in the scene
  void UpdateReferences();

  /// Handles events registered in the observer manager
  /// Namely invalidates (deletes) all non-active representations when the active is modified
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData);

public:
  /// Set default representation by the object instance
  void SetActiveRepresentationByNode(vtkMRMLDisplayableNode *node);

  /// Set default representation by the object instance
  void SetActiveRepresentationByType(ContourRepresentationType type);

  /// Get active representation type
  ContourRepresentationType GetActiveRepresentationType() { return this->ActiveRepresentationType; };

  /// Determines whether a representation exists in the contour node
  bool RepresentationExists(ContourRepresentationType type);

public:
  /// Get structure name
  vtkGetStringMacro(StructureName);
  /// Set structure name
  vtkSetStringMacro(StructureName);

  /// Get ribbon model node ID
  vtkGetStringMacro(RibbonModelNodeId);
  /// Get ribbon model node
  vtkMRMLModelNode* GetRibbonModelNode();
  /// Set and observe ribbon model node
  void SetAndObserveRibbonModelNodeId(const char *nodeID);

  /// Get indexed labelmap volume node ID
  vtkGetStringMacro(IndexedLabelmapVolumeNodeId);
  /// Get indexed labelmap volume node
  vtkMRMLScalarVolumeNode* GetIndexedLabelmapVolumeNode();
  /// Set and observe indexed labelmap volume node
  void SetAndObserveIndexedLabelmapVolumeNodeId(const char *nodeID);

  /// Get closed surface model node ID
  vtkGetStringMacro(ClosedSurfaceModelNodeId);
  /// Get closed surface model node
  vtkMRMLModelNode* GetClosedSurfaceModelNode();
  /// Set and observe closed surface model node
  void SetAndObserveClosedSurfaceModelNodeId(const char *nodeID);

  /// Get rasterization reference volume node ID
  vtkGetStringMacro(RasterizationReferenceVolumeNodeId);
  /// Set and observe rasterization reference volume node ID
  void SetAndObserveRasterizationReferenceVolumeNodeId(const char* id);

  /// Get rasterization downsampling factor
  vtkGetMacro(RasterizationDownsamplingFactor, double);
  /// Set rasterization downsampling factor
  vtkSetMacro(RasterizationDownsamplingFactor, double);

  /// Get target reduction factor
  vtkGetMacro(DecimationTargetReductionFactor, double);
  /// Set target reduction factor
  vtkSetMacro(DecimationTargetReductionFactor, double);

protected:
  /// Create a temporary vector for easier batch handling of representations
  std::vector<vtkMRMLDisplayableNode*> CreateTemporaryRepresentationsVector();

  /// Convert from a representation to another. Returns true after successful conversion, false otherwise
  bool ConvertToRepresentation(ContourRepresentationType type);

  /// Convert model representation to indexed labelmap
  vtkMRMLScalarVolumeNode* ConvertFromModelToIndexedLabelmap(vtkMRMLModelNode* modelNode);

  /// Convert indexed labelmap representation to closed surface model
  vtkMRMLModelNode* ConvertFromIndexedLabelmapToClosedSurfaceModel(vtkMRMLScalarVolumeNode* indexedLabelmapVolumeNode);

  /// Show (true) or hide (false) a representation completely (editors, viewers, slice intersections)
  void ShowRepresentation(vtkMRMLDisplayableNode* representation, bool show);

protected:
  /// Set ribbon model node ID
  vtkSetReferenceStringMacro(RibbonModelNodeId);

  /// Set indexed labelmap volume node ID
  vtkSetReferenceStringMacro(IndexedLabelmapVolumeNodeId);

  /// Set closed surface model node ID
  vtkSetReferenceStringMacro(ClosedSurfaceModelNodeId);

  /// Set rasterization reference volume node ID
  vtkSetStringMacro(RasterizationReferenceVolumeNodeId);

protected:
  vtkMRMLContourNode();
  ~vtkMRMLContourNode();
  vtkMRMLContourNode(const vtkMRMLContourNode&);
  void operator=(const vtkMRMLContourNode&);

protected:
  /// Name of the structure that corresponds to this contour
  char *StructureName;

  /// Ribbon model representation
  vtkMRMLModelNode* RibbonModelNode;
  /// Ribbon model representation model node ID
  char *RibbonModelNodeId;

  /// Indexed labelmap representation
  vtkMRMLScalarVolumeNode* IndexedLabelmapVolumeNode;
  /// Indexed labelmap representation volume node ID
  char *IndexedLabelmapVolumeNodeId;

  /// Closed surface model representation
  vtkMRMLModelNode* ClosedSurfaceModelNode;
  /// Closed surface model representation model node ID
  char *ClosedSurfaceModelNodeId;

  /// Active representation type
  ContourRepresentationType ActiveRepresentationType;

  /// Rasterization reference volume node ID. This node is used when converting from model to labelmap
  char* RasterizationReferenceVolumeNodeId;

  /// Downsampling factor for contour polydata to labelmap conversion (rasterization)
  double RasterizationDownsamplingFactor;

  /// Target reduction factor for decimation applied in labelmap to closed surface model conversion
  double DecimationTargetReductionFactor;
};

#endif // __vtkMRMLContourNode_h
