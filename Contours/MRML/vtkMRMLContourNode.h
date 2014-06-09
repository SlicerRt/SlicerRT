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

// Contour includes
#include "vtkSlicerContoursModuleMRMLExport.h"

// ITK includes
#include "itkMetaDataDictionary.h"

class vtkImageData;
class vtkMRMLColorTableNode;
class vtkMRMLContourModelDisplayNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLScene;
class vtkPlane;
class vtkPlaneCollection;
class vtkPolyData;

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

  /// RibbonModelPolyDataModifiedEvent is fired when the ribbon model PolyData is changed.
  /// ClosedSurfacePolyDataModifiedEvent is fired when closed surface PolyData is changed.
  /// While it is possible for the subclasses to fire the events without modifying the polydata, it is not recommended to do so as it
  /// doesn't mark the polydata as modified, which my result in an incorrect
  /// return value for GetModifiedSinceRead()
  /// \sa GetModifiedSinceRead()
  /// TODO : when original surface points are changed, an event should be fired
  enum
  {
    RibbonModelPolyDataModifiedEvent = 65001,
    ClosedSurfacePolyDataModifiedEvent = 65002,
    LabelmapImageDataModifiedEvent = 65003
  };

  static vtkMRMLContourNode *New();
  vtkTypeMacro(vtkMRMLContourNode, vtkMRMLDisplayableNode);
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
  virtual void DeepCopy(vtkMRMLNode* otherNode);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "Contour";};

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  /// Get bounding box in global RAS the form (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual void GetRASBounds(double bounds[6]);

  /// Transforms bounds from the local coordinate system to the RAS (world)
  /// coordinate system. Only the corner points are used for determining the
  /// new bounds, therefore in case of non-linear transforms the transformed
  /// bounds may not fully contain the transformed model points.
  virtual void TransformBoundsToRAS(double inputBounds_Local[6], double outputBounds_RAS[6]);

  ///
  /// Copy the node's attributes to this object
  void CopyOrientation(vtkMRMLContourNode *node);

  ///
  /// Copy the node's attributes to this object
  void CopyOrientation(vtkMRMLScalarVolumeNode *node);

  /// Updates this node if it depends on other nodes 
  /// when the node is deleted in the scene
  void UpdateReferences();

  /// Handles events registered in the observer manager
  /// - Invalidates (deletes) all non-active representations when the active is modified
  /// - Follows parent transform changes
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

  /// Show representation by type
  void ShowRepresentation(ContourRepresentationType type, bool show);

  /// Get representation by type
  bool IsRepresentationVisible(ContourRepresentationType type);

  /// Returns true if the transformable node can apply non linear transforms
  /// \sa ApplyTransform
  virtual bool CanApplyNonLinearTransforms()const;

  /// Apply a transform on the representations
  /// \sa SetAndObserveTransformNodeID, CanApplyNonLinearTransforms
  virtual void ApplyTransform(vtkAbstractTransform* transform);

  /// Determines whether a representation exists in the contour node
  bool HasRepresentation(ContourRepresentationType type);

  /// Remove a representation
  void RemoveRepresentation(ContourRepresentationType type);

  /// Returns true if ribbon model is empty, false otherwise
  bool RibbonModelContainsEmptyPolydata();

  /// Get structure name from subject hierarchy
  const char* GetStructureName();

  /// Update representation objects: observe nodes, update pointers
  /// This is used when a scene has finished loading and node connections need to be restablished
  void UpdateRepresentations();

  /// Get the color table and color index for the contour
  /// /param colorIndex Index of the found color in the associated color table.
  ///   If COLOR_INDEX_INVALID is set, then only the color node is acquired.
  /// /param colorNode Output argument for the found color node (optional)
  /// \param scene MRML scene pointer (in case the associated node is not in the scene any more). If not specified, then the scene of the argument node is used.
  void GetColor(int &colorIndex, vtkMRMLColorTableNode* &colorNode, vtkMRMLScene* scene=NULL);

  /// Set name (changes names of representations too)
  virtual void SetName(const char* newName);

  /// Set and observe ribbon model data
  virtual void SetAndObserveRibbonModelPolyData(vtkPolyData *PolyData);
  /// Get ribbon model data
  vtkGetObjectMacro(RibbonModelPolyData, vtkPolyData);

  /// Set and observe closed surface model data
  virtual void SetAndObserveClosedSurfacePolyData(vtkPolyData *PolyData);
  /// Get closed surface model data
  virtual vtkPolyData* GetClosedSurfacePolyData();

  /// Set and observe the image data
  virtual void SetAndObserveLabelmapImageData(vtkImageData* imageData);
  /// Get labelmap data
  virtual vtkImageData* GetLabelmapImageData();

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

  /// Set default conversion parameters if none were explicitly specified
  void SetDefaultConversionParametersForRepresentation(ContourRepresentationType type);

  /// Record whether or not the contour was created from a labelmap
  /// This prevents certain actions from happening that would break the contour behaviour
  bool HasBeenCreatedFromIndexedLabelmap();
  vtkSetMacro(CreatedFromIndexLabelmap, bool);

  /// Retrieve the display node associated with the image data
  //vtkMRMLContourLabelmapDisplayNode* GetLabelmapVolumeDisplayNode();
  /// Retrieve the display node associated with the ribbon model data
  vtkMRMLContourModelDisplayNode* GetRibbonModelDisplayNode();
  /// Retrieve the display node associated with the closed surface model data
  vtkMRMLContourModelDisplayNode* GetClosedSurfaceModelDisplayNode();

  /// Get the ordered contour planes
  const std::map<double, vtkSmartPointer<vtkPlane> >& GetOrderedContourPlanes() const;
  /// Get the ordered contour planes, caller is responsible for collection deallocation (not internals!)
  vtkPlaneCollection* GetOrderedContourPlanesVtk() const;
  /// Set the ordered contour planes
  void SetOrderedContourPlanes(std::map<double, vtkSmartPointer<vtkPlane> >& orderedContourPlanes);

  /// Create a contour storage node
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode();

  /// Reimplemented to take into account the modified time of the internal data.
  /// Returns true if the node (default behavior) or the internal data are modified
  /// since read/written.
  /// Note: The MTime of the internal data is used to know if it has been modified.
  /// So if you invoke one of the data modified events without calling Modified() on the
  /// internal data, GetModifiedSinceRead() won't return true.
  /// \sa vtkMRMLStorableNode::GetModifiedSinceRead()
  virtual bool GetModifiedSinceRead();

  // Labelmap functionality
public:
  ///
  /// Get the IJKToRAS Matrix that includes the spacing and origin
  /// information (assumes the image data is Origin 0 0 0 and Spacing 1 1 1)
  /// RASToIJK is the inverse of this
  void GetIJKToRASMatrix(vtkMatrix4x4* mat);
  void GetRASToIJKMatrix(vtkMatrix4x4* mat);

  ///
  /// Convenience methods to set the directions, spacing, and origin
  /// from a matrix
  void SetIJKToRASMatrix(vtkMatrix4x4* mat);
  void SetRASToIJKMatrix(vtkMatrix4x4* mat);

  void SetIJKToRASDirections(double dirs[3][3]);
  void GetIJKToRASDirections(double dirs[3][3]);

  ///
  /// Spacing and Origin, with the Directions, are the independent
  /// parameters that go to make up the IJKToRAS matrix
  /// In setter methods, StorableModifiedTime may need to be updated,
  /// which cannot be achieved by using vtkGetVector3Macro.
  vtkGetVector3Macro (Spacing, double);
  virtual void SetSpacing(double arg1, double arg2, double arg3);
  virtual void SetSpacing(double arg[3]);
  vtkGetVector3Macro (Origin, double);
  virtual void SetOrigin(double arg1, double arg2, double arg3);
  virtual void SetOrigin(double arg[3]);

  ///
  /// Set/Get the ITK MetaDataDictionary
  void SetMetaDataDictionary( const itk::MetaDataDictionary& );
  const itk::MetaDataDictionary& GetMetaDataDictionary() const;

  /// Create ribbon model display node
  vtkMRMLContourModelDisplayNode* CreateRibbonModelDisplayNode();

  /// Create closed surface display node
  vtkMRMLContourModelDisplayNode* CreateClosedSurfaceDisplayNode();

  /// Create labelmap display node
  // TODO : 2d vis readdition
  //void CreateLabelmapDisplayNode();

public:
  /// Note: Interpolation mode is the default one: nearest neighbor
  /// \param scene The scene to operate on
  /// \param inContourNode the input contour to resample
  /// \param refVolumeNode the reference volume to determine resampling output
  /// \param outContourNode the output contour to put the results into
  /// TODO : If contours ever make it into Slicer core, move this function back to rtcommon
  static bool ResampleInputContourNodeToReferenceVolumeNode(vtkMRMLScene* scene,
    vtkMRMLContourNode* inContourNode, 
    vtkMRMLScalarVolumeNode* refVolumeNode,
    vtkMRMLContourNode* outContourNode);

  /// Check if the lattice (grid, geometry) of a contour and a volume are the same
  /// This is a re-implementation of a slicerrt common function
  /// TODO : If contours ever make it into Slicer core, move this function back to rtcommon
  /// TODO : Do implies an action, this is a query. Consider renaming using Is or Are
  static bool DoVolumeLatticesMatch(vtkMRMLContourNode* contour1, vtkMRMLScalarVolumeNode* volume2);
    
protected:
  /// Set rasterization reference volume node ID
  vtkSetStringMacro(RasterizationReferenceVolumeNodeId);

  /// For logging purposes
  static std::string GetRepresentationTypeAsString(ContourRepresentationType type);

  /// Internal function to clean up getting RAS bounds for the labelmap representation
  void GetLabelmapRASBounds( double bounds[6] );

  /// Called when a node reference ID is added (list size increased).
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference);

  /// Called when a node reference ID is modified.
  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference);

  /// Internal function that sets the polydata to all the display nodes.
  /// Can be called if the polydata is changed.
  void SetPolyDataToDisplayNodes(vtkPolyData* polyData, ContourRepresentationType type);

  /// Internal function that sets the polydata to a display node.
  /// Can be reimplemented if you want to set a different polydata
  virtual void SetPolyDataToDisplayNode(vtkPolyData* polyData, vtkMRMLContourModelDisplayNode* modelDisplayNode);

  /// Internal function that sets the image data to all the display nodes.
  /// Can be called if the image data is changed.
  void SetImageDataToDisplayNodes();

  /// Internal function that sets the image data to a display node.
  /// Can be reimplemented if you want to set a different image data
  //virtual void SetImageDataToDisplayNode(vtkMRMLContourLabelmapDisplayNode* labelmapDisplayNode);

  /// Set the ribbon model data
  void SetRibbonModelPolyData(vtkPolyData* polyData);

  /// Set the closed surface data
  void SetClosedSurfacePolyData(vtkPolyData* polyData);

  /// Set the labelmap data
  void SetLabelmapImageData(vtkImageData* imageData);

protected:
  vtkMRMLContourNode();
  ~vtkMRMLContourNode();
  vtkMRMLContourNode(const vtkMRMLContourNode&);
  void operator=(const vtkMRMLContourNode&);

  /// Ribbon model representation
  vtkPolyData* RibbonModelPolyData;

  /// Closed surface model representation
  vtkPolyData* ClosedSurfacePolyData;

  /// Labelmap representation
  vtkImageData* LabelmapImageData;

  /// TODO : rename these to indicate labelmap only application
  /// these are unit length direction cosines
  double IJKToRASDirections[3][3];

  /// these are mappings to mm space
  double Spacing[3];
  double Origin[3];

  itk::MetaDataDictionary Dictionary;

  /// Original ROI points from DICOM-RT. For various purposes, e.g. re-ribbonization for a different slice thickness
  /// TODO: Temporary solution. Re-ribbonization (or ribbon model) will not be needed any more when we have the direct
  ///   conversion from points to closed surface points
  /// TODO: Rename to remove indication of origin of data
  vtkPolyData* DicomRtRoiPoints;

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

  /// Record whether or not this contour was generated from a labelmap
  /// TODO : this actually tells what is the ground-truth representation, in the future this should be any of the representations
  bool CreatedFromIndexLabelmap;
};

#endif // __vtkMRMLContourNode_h
