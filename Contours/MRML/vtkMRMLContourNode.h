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

// VTK includes
#include <vtkPolyData.h>

#include "vtkSlicerContoursModuleMRMLExport.h"

class vtkMRMLColorTableNode;
class vtkMRMLModelNode;
class vtkMRMLScalarVolumeNode;
class vtkPlane;
class vtkPlaneCollection;

/// \ingroup SlicerRt_QtModules_Contours
class VTK_SLICER_CONTOURS_MODULE_MRML_EXPORT vtkMRMLContourNode : public vtkMRMLDisplayableNode
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
  vtkTypeMacro(vtkMRMLContourNode,vtkMRMLDisplayableNode);
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
  virtual const char* GetNodeTagName() {return "Contour";};

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  /// Updates this node if it depends on other nodes 
  /// when the node is deleted in the scene
  void UpdateReferences();

  /// Handles events registered in the observer manager
  /// - Invalidates (deletes) all non-active representations when the active is modified
  /// - Follows parent transform changes
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

  /// Overridden function to get visibility from active representation
  virtual int GetDisplayVisibility();

  /// Overridden function to set visibility to active representations
  virtual void SetDisplayVisibility(int visible);

  /// Returns true if the transformable node can apply non linear transforms
  /// \sa ApplyTransform
  virtual bool CanApplyNonLinearTransforms()const;

  /// Apply a transform on the representations
  /// \sa SetAndObserveTransformNodeID, CanApplyNonLinearTransforms
  virtual void ApplyTransform(vtkAbstractTransform* transform);

public:
  /// Set default representation by the object instance
  void SetActiveRepresentationByNode(vtkMRMLDisplayableNode *node);

  /// Set default representation by the object instance
  void SetActiveRepresentationByType(ContourRepresentationType type);

  /// Get active representation type
  ContourRepresentationType GetActiveRepresentationType() { return this->ActiveRepresentationType; };

  /// Determines whether a representation exists in the contour node
  bool RepresentationExists(ContourRepresentationType type);

  /// Returns true if ribbon model is empty, false otherwise
  bool RibbonModelContainsEmptyPolydata();

  /// Determines if the contour has been created from an already existing indexed labelmap
  /// \sa RasterizationReferenceVolumeNodeId
  bool HasBeenCreatedFromIndexedLabelmap();

  /// Get structure name from subject hierarchy
  const char* GetStructureName();

  /// Update representation objects: observe nodes, update pointers
  void UpdateRepresentations();

  /// Get the color table and color index for the contour
  /// /param colorIndex Index of the found color in the associated color table.
  ///   If COLOR_INDEX_INVALID is set, then only the color node is acquired.
  /// /param colorNode Output argument for the found color node (optional)
  /// \param scene MRML scene pointer (in case the associated node is not in the scene any more). If not specified, then the scene of the argument node is used.
  void GetColor(int &colorIndex, vtkMRMLColorTableNode* &colorNode, vtkMRMLScene* scene=NULL);

public:
  /// Set name (changes names of representations too)
  virtual void SetName(const char* newName);

  /// Get ribbon model node ID
  vtkGetStringMacro(RibbonModelNodeId);
  /// Get ribbon model node
  vtkMRMLModelNode* GetRibbonModelNode();
  /// Set and observe ribbon model node. All other representations will be deleted
  void SetAndObserveRibbonModelNodeId(const char *nodeID);

  /// Get indexed labelmap volume node ID
  vtkGetStringMacro(IndexedLabelmapVolumeNodeId);
  /// Get indexed labelmap volume node
  vtkMRMLScalarVolumeNode* GetIndexedLabelmapVolumeNode();
  /// Set and observe indexed labelmap volume node. All other representations will be deleted
  void SetAndObserveIndexedLabelmapVolumeNodeId(const char *nodeID);

  /// Get closed surface model node ID
  vtkGetStringMacro(ClosedSurfaceModelNodeId);
  /// Get closed surface model node
  vtkMRMLModelNode* GetClosedSurfaceModelNode();
  /// Set and observe closed surface model node. All other representations will be deleted
  void SetAndObserveClosedSurfaceModelNodeId(const char *nodeID);

  /// Get rasterization reference volume node ID
  vtkGetStringMacro(RasterizationReferenceVolumeNodeId);
  /// Set and observe rasterization reference volume node ID
  void SetAndObserveRasterizationReferenceVolumeNodeId(const char* id);

  /// Get original ROI points from DICOM-RT
  vtkGetObjectMacro(DicomRtRoiPoints, vtkPolyData);
  /// Set original ROI points from DICOM-RT
  vtkSetObjectMacro(DicomRtRoiPoints, vtkPolyData);

  /// Get rasterization oversampling factor
  vtkGetMacro(RasterizationOversamplingFactor, double);
  /// Set rasterization oversampling factor
  void SetRasterizationOversamplingFactor(double oversamplingFactor);

  /// Get target reduction factor
  vtkGetMacro(DecimationTargetReductionFactor, double);
  /// Set target reduction factor
  void SetDecimationTargetReductionFactor(double targetReductionFactor);

  /// Get the ordered contour planes
  const std::map<double, vtkSmartPointer<vtkPlane> >& GetOrderedContourPlanes() const;
  /// Get the ordered contour planes, caller is responsible for collection deallocation (not internals!)
  vtkPlaneCollection* GetOrderedContourPlanesVtk() const;
  /// Set the ordered contour planes
  void SetOrderedContourPlanes(std::map<double, vtkSmartPointer<vtkPlane> >& orderedContourPlanes);

protected:
  /// Create a temporary vector for easier batch handling of representations
  std::vector<vtkMRMLDisplayableNode*> CreateTemporaryRepresentationsVector();

  /// Show (true) or hide (false) a representation completely (editors, viewers, slice intersections)
  void ShowRepresentation(vtkMRMLDisplayableNode* representation, bool show);

  /// Set default conversion parameters if none were explicitly specified
  void SetDefaultConversionParametersForRepresentation(ContourRepresentationType type);

  /// Delete all representations except for the active one
  void DeleteNonActiveRepresentations();

protected:
  /// Set ribbon model node ID
  vtkSetReferenceStringMacro(RibbonModelNodeId);

  /// Set indexed labelmap volume node ID
  vtkSetReferenceStringMacro(IndexedLabelmapVolumeNodeId);

  /// Set closed surface model node ID
  vtkSetReferenceStringMacro(ClosedSurfaceModelNodeId);

  /// Set rasterization reference volume node ID
  vtkSetStringMacro(RasterizationReferenceVolumeNodeId);

private:
  /// Set and observe ribbon model node while preserving the existing representations (only the should converter call this)
  void SetAndObserveRibbonModelNodeIdOnly(const char *nodeID);
  /// Set and observe indexed labelmap volume node while preserving the existing representations (only the should converter call this)
  void SetAndObserveIndexedLabelmapVolumeNodeIdOnly(const char *nodeID);
  /// Set and observe closed surface model node while preserving the existing representations (only the should converter call this)
  void SetAndObserveClosedSurfaceModelNodeIdOnly(const char *nodeID);

protected:
  vtkMRMLContourNode();
  ~vtkMRMLContourNode();
  vtkMRMLContourNode(const vtkMRMLContourNode&);
  void operator=(const vtkMRMLContourNode&);
  friend class vtkConvertContourRepresentations;
  friend class vtkSlicerContoursModuleLogic;

protected:
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

  /// Original ROI points from DICOM-RT. For various purposes, e.g. re-ribbonization for a different slice thickness
  /// TODO: Temporary solution. Re-ribbonization (or ribbon model) will not be needed any more when we have the direct
  ///   conversion from points to closed surface points
  vtkPolyData* DicomRtRoiPoints;

  /// Active representation type
  ContourRepresentationType ActiveRepresentationType;

  /// Rasterization reference volume node ID. This node is used when converting from model to labelmap
  /// IMPORTANT: If the reference volume node ID is the same as the indexed labelmap volume node ID, then it
  /// means that the contour was created from labelmap, and there was no conversion from any model representation
  char* RasterizationReferenceVolumeNodeId;

  /// Oversampling factor for contour polydata to labelmap conversion (rasterization)
  double RasterizationOversamplingFactor;

  /// Target reduction factor for decimation applied in labelmap to closed surface model conversion
  double DecimationTargetReductionFactor;

  /// A stored representation of each plane of the contour data, ordered by distance along the normal line
  /// The distance is calculated by taking the dot product of a planes origin and it's normal
  std::map<double, vtkSmartPointer<vtkPlane> > OrderedContourPlanes;
};

#endif // __vtkMRMLContourNode_h
