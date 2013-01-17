#ifndef __SlicerRtCommon_h
#define __SlicerRtCommon_h

// STD includes
#include <cstdlib>
#include <string>

// ITK includes
#include "itkImage.h"

class vtkMRMLTransformableNode;
class vtkGeneralTransform;
class vtkMRMLVolumeNode;
class vtkMRMLModelNode;

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


class SlicerRtCommon
{
public:
  //----------------------------------------------------------------------------
  // Constant strings
  //----------------------------------------------------------------------------

  // DicomRtImport constants
  static const std::string DICOMRTIMPORT_ATTRIBUTE_PREFIX;
  static const std::string DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_SERIES_NAME_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_BEAM_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_BEAM_GANTRY_ANGLE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_BEAM_COUCH_ANGLE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_BEAM_COLLIMATOR_ANGLE_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME;

  static const std::string DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ROOT_CONTOUR_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX;

  // Contour (vtkMRMLContourNode) constants
  static const std::string CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX;
  static const std::string CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
  static const std::string CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX;
  static const double DEFAULT_RASTERIZATION_OVERSAMPLING_FACTOR;
  static const double DEFAULT_DECIMATION_TARGET_REDUCTION_FACTOR;

  // DoseVolumeHistogram constants
  static const std::string DVH_ATTRIBUTE_PREFIX;
  static const std::string DVH_TYPE_ATTRIBUTE_NAME;
  static const std::string DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_CONTOUR_NODE_ID_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME;
  static const std::string DVH_METRIC_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_LIST_ATTRIBUTE_NAME;

  static const std::string DVH_TYPE_ATTRIBUTE_VALUE;
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
  static const std::string ISODOSE_MODEL_NODE_NAME_PREFIX;
  static const std::string ISODOSE_COLOR_TABLE_NODE_NAME_POSTFIX;

  // DoseComparison constants
  static const std::string DOSECOMPARISON_OUTPUT_BASE_NAME_PREFIX;

  // Beams constants
  static const std::string BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX;
  static const std::string BEAMS_OUTPUT_SOURCE_FIDUCIAL_PREFIX;
  static const std::string BEAMS_PARAMETER_SET_BASE_NAME_PREFIX;

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
  static bool IsStringNullOrEmpty(char* aString);

  /*!
    Convert VTK image to ITK image
    \param inVolumeNode Input volume node
    \param outItkVolume Output ITK image
    \param paintForegroundTo1 Paint non-zero values to 1 (Optional)
    \return Success
  */
//BTX
  template<typename T> static bool ConvertVolumeNodeToItkImage(vtkMRMLVolumeNode* inVolumeNode, typename itk::Image<T, 3>::Pointer outItkVolume, bool paintForegroundTo1=false);
//ETX
};

#include "SlicerRtCommon.txx"

#endif
