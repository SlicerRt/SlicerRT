#ifndef __SlicerRtCommon_h
#define __SlicerRtCommon_h

// STD includes
#include <cstdlib>
#include <string>

// ITK includes
#include "itkImage.h"

// VTK includes
#include <vtkVector.h>
#include <vtkSmartPointer.h>
#include <vtkPlane.h>

class vtkMRMLTransformableNode;
class vtkMRMLNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLColorTableNode;

class vtkImageData;
class vtkGeneralTransform;
class vtkMatrix4x4;

#define EPSILON 0.0001

#define UNUSED_VARIABLE(a) ((void) a)

/* Define case insensitive string compare for all supported platforms. */
#if defined( _WIN32 ) && !defined(__CYGWIN__)
#  if defined(__BORLANDC__)
#    define STRCASECMP stricmp
#  else
#    define STRCASECMP _stricmp
#  endif
#else
#  define STRCASECMP strcasecmp
#endif

#include "vtkSlicerRtCommonWin32Header.h"

/// \ingroup SlicerRt_SlicerRtCommon
class VTK_SLICERRTCOMMON_EXPORT SlicerRtCommon
{
public:
  //----------------------------------------------------------------------------
  // Constant strings (std::string types for easy concatenation)
  //----------------------------------------------------------------------------

  // SlicerRT constants
  static const char* SLICERRT_EXTENSION_NAME;
  static const std::string SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX;

  // Contour (vtkMRMLContourNode) constants
  static const std::string CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX;
  static const std::string CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
  static const std::string CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX;
  static const double DEFAULT_RASTERIZATION_OVERSAMPLING_FACTOR;
  static const double DEFAULT_DECIMATION_TARGET_REDUCTION_FACTOR;
  static const char* COLOR_NAME_BACKGROUND;
  static const char* COLOR_NAME_INVALID;
  static const char* COLOR_NAME_REMOVED;
  static const int COLOR_INDEX_BACKGROUND;
  static const int COLOR_INDEX_INVALID;
  static const double COLOR_VALUE_INVALID[4];

  static const std::string CONTOUR_NEW_CONTOUR_NAME;
  static const std::string CONTOURHIERARCHY_NEW_CONTOUR_SET_NAME;
  static const std::string CONTOURHIERARCHY_DUMMY_ANATOMICAL_VOLUME_NODE_NAME_PREFIX;
  static const char* CONTOUR_ORPHAN_CONTOURS_COLOR_TABLE_NODE_NAME;
  static const char* CONTOUR_SET_COLOR_TABLE_REFERENCE_ROLE;

  // DicomRtImport constants
  static const std::string DICOMRTIMPORT_ATTRIBUTE_PREFIX;
  static const std::string DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_COLLIMATOR_ANGLE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_BEAM_NUMBER_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_SOP_INSTANCE_UID_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME;

  static const std::string DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ROOT_CONTOUR_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_NO_NAME;
  static const std::string DICOMRTIMPORT_NO_DESCRIPTION;
  static const std::string DICOMRTIMPORT_SOURCE_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX;

  static const char* DICOMRTIMPORT_DEFAULT_DOSE_COLOR_TABLE_NAME;

  // DoseVolumeHistogram constants
  static const std::string DVH_ATTRIBUTE_PREFIX;
  static const std::string DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME;
  static const std::string DVH_DOSE_VOLUME_NODE_REFERENCE_ROLE;
  static const std::string DVH_CREATED_DVH_NODE_REFERENCE_ROLE;
  static const std::string DVH_STRUCTURE_CONTOUR_NODE_REFERENCE_ROLE;
  static const std::string DVH_DOSE_VOLUME_OVERSAMPLING_FACTOR_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME;
  static const std::string DVH_METRIC_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_LIST_ATTRIBUTE_NAME;

  static const char        DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  static const std::string DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  static const std::string DVH_METRIC_MEAN_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_MIN_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_MAX_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_DOSE_ATTRIBUTE_NAME_POSTFIX;
  static const std::string DVH_METRIC_INTENSITY_ATTRIBUTE_NAME_POSTFIX;
  static const std::string DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_ARRAY_NODE_NAME_POSTFIX;
  static const std::string DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE;
  static const std::string DVH_CSV_HEADER_VOLUME_FIELD_END;

  // DoseAccumulation constants
  static const std::string DOSEACCUMULATION_ATTRIBUTE_PREFIX;
  static const std::string DOSEACCUMULATION_DOSE_VOLUME_NODE_NAME_ATTRIBUTE_NAME;
  static const std::string DOSEACCUMULATION_OUTPUT_BASE_NAME_PREFIX;

  // Isodose constants
  static const char* ISODOSE_DEFAULT_ISODOSE_COLOR_TABLE_FILE_NAME;
  static const std::string ISODOSE_MODEL_NODE_NAME_PREFIX;
  static const std::string ISODOSE_PARAMETER_SET_BASE_NAME_PREFIX;
  static const std::string ISODOSE_ISODOSE_SURFACES_HIERARCHY_NODE_NAME_POSTFIX;

  // DoseComparison constants
  static const char* DOSECOMPARISON_GAMMA_VOLUME_IDENTIFIER_ATTRIBUTE_NAME;
  static const char* DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME;
  static const std::string DOSECOMPARISON_OUTPUT_BASE_NAME_PREFIX;

  // Beams constants
  static const std::string BEAMS_ATTRIBUTE_PREFIX;
  static const std::string BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX;
  static const std::string BEAMS_OUTPUT_ISOCENTER_FIDUCIAL_POSTFIX;
  static const std::string BEAMS_OUTPUT_SOURCE_FIDUCIAL_POSTFIX;
  static const std::string BEAMS_PARAMETER_SET_BASE_NAME_PREFIX;

  // PlanarImage constants
  static const std::string PLANARIMAGE_MODEL_NODE_NAME_PREFIX;
  static const std::string PLANARIMAGE_TEXTURE_NODE_NAME_PREFIX;
  static const std::string PLANARIMAGE_PARAMETER_SET_BASE_NAME_PREFIX;
  static const std::string PLANARIMAGE_RT_IMAGE_VOLUME_REFERENCE_ROLE;
  static const std::string PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE;
  static const std::string PLANARIMAGE_TEXTURE_VOLUME_REFERENCE_ROLE;

  // Attribute constants
  static const char* ATTRIBUTE_VOLUME_LABELMAP_IDENTIFIER;
  static const char* ATTRIBUTE_CONTOUR_REPRESENTATION_IDENTIFIER;

  //----------------------------------------------------------------------------
  // Utility functions
  //----------------------------------------------------------------------------

public:
  /*!
    Compute transform between two transformable objects
    /param fromNode Displayable node from which we want to compute the transform
    /param toNode Displayable node to which we want to compute the transform
    /param fromNodeTotoNodeTransform Output transform
  */
  static void GetTransformBetweenTransformables(vtkMRMLTransformableNode* fromNode, vtkMRMLTransformableNode* toNode, vtkGeneralTransform* fromNodeToToNodeTransform);

  /// Returns true if the string is null or empty, returns false otherwise
  static bool IsStringNullOrEmpty(const char* aString);

  /*!
    Computes origin, extent, and spacing of a volume for an oversampling factor
    \param inputVolumeNode Volume node that needs to be oversampled
    \param oversamplingFactor Oversampling factor (e.g. in case of 2, the voxels will be half the size in each dimension)
    \param outputImageDataExtent Output extent that has to be set to the reslice algorithm
    \param outputImageDataSpacing Output spacing that has to be set to the reslice algorithm
  */
  static void GetExtentAndSpacingForOversamplingFactor(vtkMRMLScalarVolumeNode* inputVolumeNode, double oversamplingFactor, int outputImageDataExtent[6], double outputImageDataSpacing[3]);

  /// Determine if a node is a dose volume node
  static bool IsDoseVolumeNode(vtkMRMLNode* node);

  /// Determine if a node is a labelmap volume node
  static bool IsLabelmapVolumeNode(vtkMRMLNode* node);

  /// Stretch a discrete color table that contains a few values into a full 256-color palette that
  /// has the first and last colors the same as the input one, the intermediate colors inserted in
  /// evenly, and the others linearly interpolated.
  static void StretchDiscreteColorTable(vtkMRMLColorTableNode* inputDiscreteColorTable, vtkMRMLColorTableNode* output256ValueColorTable, unsigned int numberOfColors=256);

  /// Check if the lattice (grid, geometry) of two volumes are the same
  static bool DoVolumeLatticesMatch(vtkMRMLScalarVolumeNode* volume1, vtkMRMLScalarVolumeNode* volume2);

  /// Determine if two bounds are equal
  static bool AreBoundsEqual(int boundsA[6], int boundsB[6]);

  /// Return an ordering of planes along a normal by taking the dot product of the origin and the normal
  /// \param inputPlanes the list of planes to compute
  /// \param outputPlaneOrdering the list of planes, ordered by distance along the normal line from the most extreme slice origin
  /// \return whether the function succeeded or not
  static bool OrderPlanesAlongNormal( std::vector< vtkSmartPointer<vtkPlane> > inputPlanes, std::map<double, vtkSmartPointer<vtkPlane> >& outputPlaneOrdering );

//BTX
  /*!
    Convert volume MRML node to ITK image
    \param inVolumeNode Input volume node
    \param outItkVolume Output ITK image
    \param applyRasToLpsConversion Apply RAS (Slicer) to LPS (ITK, DICOM) coordinate frame conversion
    \return Success
  */
  template<typename T> static bool ConvertVolumeNodeToItkImage(vtkMRMLScalarVolumeNode* inVolumeNode, typename itk::Image<T, 3>::Pointer outItkImage, bool applyRasToLpsConversion=false);

  /*!
    Convert ITK image to VTK image data. The image geometry is not considered!
    \param inItkImage Input ITK image
    \param outVtkImageData Output VTK image data
    \param vtkType Data scalar type (i.e VTK_FLOAT)
    \return Success
  */
  template<typename T> static bool ConvertItkImageToVtkImageData(typename itk::Image<T, 3>::Pointer inItkImage, vtkImageData* outVtkImageData, int vtkType);

//ETX
};

#include "SlicerRtCommon.txx"

#endif
