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

#ifndef __vtkSegmentation_h
#define __vtkSegmentation_h

// VTK includes
#include <vtkObject.h>
#include <vtkSmartPointer.h>

// STD includes
#include <map>

// SegmentationCore includes
#include "vtkSegment.h"
#include "vtkSegmentationConverter.h"
#include "vtkSegmentationConverterRule.h"

#include "vtkSegmentationCoreConfigure.h"

class vtkAbstractTransform;
class vtkCallbackCommand;
class vtkStringArray;

/// \ingroup SegmentationCore
class vtkSegmentationCore_EXPORT vtkSegmentation : public vtkObject
{
public:
  enum
  {
    /// Fired when the master representation in ANY segment is changed.
    /// While it is possible for the subclasses to fire the events without modifying the actual data,
    /// it is not recommended to do so as it doesn't mark the data as modified, which may result in
    /// an incorrect return value for \sa GetModifiedSinceRead()
    MasterRepresentationModified = 62100,
    /// Fired if new segment is added
    SegmentAdded,
    /// Fired if segment is removed
    SegmentRemoved,
    /// Fired if segment is modified
    SegmentModified,
    /// Fired if representations are created on conversion
    RepresentationCreated
  };

  /// Container type for segments. Maps segment IDs to segment objects
  typedef std::map<std::string, vtkSmartPointer<vtkSegment> > SegmentMap;

public:
  static vtkSegmentation* New();
  vtkTypeMacro(vtkSegmentation, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes(const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Deep copy one segmentation into another
  virtual void DeepCopy(vtkSegmentation* aSegmentation);

  /// Copy conversion parameters from another segmentation
  virtual void CopyConversionParameters(vtkSegmentation* aSegmentation);

  /// Get bounding box in global RAS in the form (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual void GetBounds(double bounds[6]);

  /// Apply a linear transform on the master representation of the segments. The others will be invalidated
  /// Harden transform if poly data, apply to directions if oriented image data.
  virtual void ApplyLinearTransform(vtkAbstractTransform* transform);

  /// Apply a non-linear transform on the master representation of the segments. The others will be invalidated
  /// Harden transform both if oriented image data and poly data.
  virtual void ApplyNonLinearTransform(vtkAbstractTransform* transform);

  /// Returns true if the node (default behavior) or the internal data are modified
  /// since read/written.
  /// Note: The MTime of the internal data is used to know if it has been modified.
  /// So if you invoke one of the data modified events without calling Modified() on the
  /// internal data, GetModifiedSinceRead() won't return true.
  /// \sa vtkMRMLStorableNode::GetModifiedSinceRead()
  virtual bool GetModifiedSinceRead();

//BTX
  /// Determine common labelmap geometry for whole segmentation.
  /// If the segmentation has reference image geometry conversion parameter, then oversample it to
  /// be at least as fine resolution as the highest resolution labelmap contained, otherwise just use
  /// the geometry of the highest resolution labelmap in the segments.
  /// \param segmentIDs List of IDs of segments to include in the merged labelmap. If empty or missing, then all segments are included
  /// \return Geometry string that can be deserialized using \sa vtkSegmentationConverter::SerializeImageGeometry
  std::string DetermineCommonLabelmapGeometry(const std::vector<std::string>& segmentIDs=std::vector<std::string>());
//ETX

// Segment related methods
public:
  /// Add a segment to this segmentation, do necessary conversions, and observe underlying
  /// data for changes.
  /// Necessary conversions:
  ///   1. If the segment can be added (\sa CanAcceptSegment), and it does
  ///   not contain the master representation, then the master representation is converted
  ///   using the cheapest available path.
  ///   2. Make sure that the segment contains the same types of representations that are
  ///   present in the existing segments of the segmentation (because we expect all segments
  ///   in a segmentation to contain the same types of representations).
  /// \param segment the segment to observe
  /// \return Success flag
  bool AddSegment(vtkSegment* segment, std::string segmentId="");

  /// Remove a segment by ID
  /// \param segmentId Identifier of the segment to remove from the segmentation
  void RemoveSegment(std::string segmentId);

  /// Remove a segment by value
  /// \param segment the segment to remove from the segmentation
  void RemoveSegment(vtkSegment* segment);

  /// Access a segment by ID
  /// \param segmentId Segment identifier in the container to access
  vtkSegment* GetSegment(std::string segmentId);

  /// Return all contained segments
  SegmentMap GetSegments() { return this->Segments; };

  /// Get IDs for all contained segments
  void GetSegmentIDs(std::vector<std::string> &segmentIds);

  /// Get IDs for all contained segments, for python compatibility
  void GetSegmentIDs(vtkStringArray* segmentIds);

  /// Request the total number of segments, primarily used for iterating over all segments
  int GetNumberOfSegments() const;

  /// Find segment ID by segment instance
  std::string GetSegmentIdBySegment(vtkSegment* segment);

  /// Get segments that contain a certain tag
  std::vector<vtkSegment*> GetSegmentsByTag(std::string tag);

  /// Get representation from segment
  vtkDataObject* GetSegmentRepresentation(std::string segmentId, std::string representationName);

  /// Copy segment from one segmentation to this one
  /// \param fromSegmentation Source segmentation
  /// \param segmentId ID of segment to copy
  /// \param removeFromSource If true, then delete segment from source segmentation after copying.
  ///                        Default value is false.
  /// \return Success flag
  bool CopySegmentFromSegmentation(vtkSegmentation* fromSegmentation, std::string segmentId, bool removeFromSource=false);

// Representation related methods
public:
  /// Get representation names present in this segmentation in an output string vector
  /// Note: This assumes the first segment contains the same type of representations as
  ///       all segments (this should be the case by design)
  void GetContainedRepresentationNames(std::vector<std::string>& representationNames);

  /// Determines if segments contain a certain representation type
  /// Note: This assumes the first segment contains the same type of representations as
  ///       all segments (this should be the case by design)
  bool ContainsRepresentation(std::string representationName);

  /// Get all representations supported by the converter
  void GetAvailableRepresentationNames(std::set<std::string>& representationNames) { this->Converter->GetAvailableRepresentationNames(representationNames); };

  /// Invalidate (remove) non-master representations in all the segments if this segmentation node
  void InvalidateNonMasterRepresentations();

// Conversion related methods
public:
  /// Create a representation in all segments, using the conversion path with the
  /// lowest cost. The stored conversion parameters are used (which are the defaults if not changed by the user).
  /// Conversion starts from the master representation. If a representation along
  /// the path already exists then no conversion is performed.
  /// Note: The conversion functions are not in vtkSegmentationConverter, because
  ///       they need to know about the master representation which is segmentation-
  ///       specific, and also to allow optimizations (steps before per-segment conversion).
  /// \param targetRepresentationName Name of the representation to create
  /// \param alwaysConvert If true, then conversion takes place even if target representation exists. False by default.
  /// \return true on success
  bool CreateRepresentation(const std::string& targetRepresentationName, bool alwaysConvert=false);

  /// Generate or update a representation in all segments, using the specified conversion
  /// path and parameters.
  /// Conversion starts from the master representation, and all representations along the
  /// path get overwritten.
  /// \return true on success
  bool CreateRepresentation(const std::string& targetRepresentationName,
                            vtkSegmentationConverter::ConversionPathType path,
                            vtkSegmentationConverterRule::ConversionParameterListType parameters);

  /// Determine if the segmentation is ready to accept a certain type of representation
  /// by copy/move or import. It can accept a representation if it is the master representation
  /// of this segment or it is possible to convert to master representation (or the segmentation
  /// is empty).
  bool CanAcceptRepresentation(std::string representationName);

  /// Determine if the segmentation is ready to accept a certain segment. It can accept a
  /// segment if it contains a representation that is acceptable, or if it is empty.
  bool CanAcceptSegment(vtkSegment* segment);

  /// Add empty segment containing empty instances of the contained representations
  /// \param segmentId ID of added segment. If empty then a default ID will be generated \sa GenerateUniqueSegmentId
  /// \return Success flag
  bool AddEmptySegment(std::string segmentId="");

  /// Get all possible conversions between the master representation and a specified target representation
  void GetPossibleConversions(const std::string& targetRepresentationName,
    vtkSegmentationConverter::ConversionPathAndCostListType &pathsCosts) { this->Converter->GetPossibleConversions(this->MasterRepresentationName, targetRepresentationName, pathsCosts); };

  /// Set a conversion parameter to all rules having this parameter
  void SetConversionParameter(const std::string& name, const std::string& value) { this->Converter->SetConversionParameter(name, value); };

  /// Get a conversion parameter from first rule containing this parameter
  /// Note: all parameters with the same name should contain the same value
  std::string GetConversionParameter(const std::string& name) { return this->Converter->GetConversionParameter(name); };

  /// Get names of all conversion parameters used by the selected conversion path
  void GetConversionParametersForPath(vtkSegmentationConverterRule::ConversionParameterListType& conversionParameters,
    const vtkSegmentationConverter::ConversionPathType& path) { this->Converter->GetConversionParametersForPath(conversionParameters, path); };

  /// Serialize all conversion parameters.
  /// The resulting string can be parsed in a segmentation object using /sa DeserializeConversionParameters
  std::string SerializeAllConversionParameters();

  /// Parse conversion parameters in string and set it to the segmentation converter
  /// Such a string can be constructed in a segmentation object using /sa SerializeAllConversionParameters
  void DeserializeConversionParameters(std::string conversionParametersString);

// Get/set methods
public:
  /// Get master representation name
  vtkGetStringMacro(MasterRepresentationName);
  /// Set master representation name.
  /// Need to make sure before setting the name that the newly set master representation exists in
  /// the segmentation! Use \sa CreateRepresentation for that.
  virtual void SetMasterRepresentationName(const char* representationName);

protected:
  /// Convert given segment along a specified path
  /// \param segment Segment to convert
  /// \param path Path to do the conversion along
  /// \param overwriteExisting If true then do each conversion step regardless the target representation
  ///   exists. If false then skip those conversion steps that would overwrite existing representation
  /// \return Success flag
  bool ConvertSegmentUsingPath(vtkSegment* segment, vtkSegmentationConverter::ConversionPathType path, bool overwriteExisting=false);

  /// Remove segment by iterator. The two \sa RemoveSegment methods call this function after
  /// finding the iterator based on their different input arguments.
  void RemoveSegment(SegmentMap::iterator segmentIt);

  /// Generate unique segment ID. If argument is empty then a new ID will be generated in the form "SegmentN",
  /// where N is the number of segments. If argument is unique it is returned unchanged. If there is a segment
  /// with the given name, then it is postfixed by "_1"
  std::string GenerateUniqueSegmentId(std::string id);

protected:
  /// Callback function invoked when segment is modified.
  /// It calls Modified on the segmentation and rebuilds observations on the master representation of each segment
  static void OnSegmentModified(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  /// Callback function observing the master representation of each segment
  /// It fires a \sa MasterRepresentationModifiedEvent if master representation is changed in ANY segment
  static void OnMasterRepresentationModified(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

protected:
  vtkSegmentation();
  ~vtkSegmentation();
  void operator=(const vtkSegmentation&);

  /// Container of segments that belong to this segmentation
  SegmentMap Segments;

  /// Master representation type name.
  /// 1. This representation is saved on disk
  /// 2. If this representation is modified, the others are invalidated
  /// This value must be set by the creator of the segmentation object!
  char* MasterRepresentationName;

  /// Converter instance
  vtkSegmentationConverter* Converter;

  /// Command handling segment modified events
  vtkCallbackCommand* SegmentCallbackCommand;

  /// Command handling master representation modified events
  vtkCallbackCommand* MasterRepresentationCallbackCommand;
};

#endif // __vtkSegmentation_h
