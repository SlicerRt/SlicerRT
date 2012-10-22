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
class vtkMRMLContourNode;
class vtkMRMLScene;
class vtkMRMLColorTableNode;

#define EPSILON 0.0001

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

  static const std::string DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ROOT_CONTOUR_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX;

  // Contour (vtkMRMLContourNode) constants
  static const std::string CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX;
  static const std::string CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
  static const std::string CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX;

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
  static const std::string DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_ARRAY_NODE_NAME_POSTFIX;
  static const std::string DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE;
  static const std::string DVH_CSV_HEADER_VOLUME_FIELD_END;

  //----------------------------------------------------------------------------
  // Utility functions
  //----------------------------------------------------------------------------

public:
  /*!
    Compute transform between a model and a volume IJK coordinate system (to transform the model into the volume voxel space)
    /param fromModelNode Model node from which we want to compute the transform
    /param toVolumeNode Volume node to whose IJK space we want to compute the transform
    /param fromModelToVolumeIjkTransform Output transform
  */
  static void GetTransformFromModelToVolumeIjk(vtkMRMLModelNode* fromModelNode, vtkMRMLVolumeNode* toVolumeNode, vtkGeneralTransform* fromModelToVolumeIjkTransform);

  /*!
    Get the index of the color of a contour from the associated color table
    /param contourNode The contour object corresponding to the structure we seek the color of
    /param mrmlScene The MRML scene we do the search in
    /param colorIndex Index of the found color in the associated color table
    /param colorNode Output argument for the found color node (optional)
    /param referenceModelNode A model that we want to double check the found color against
             (we only select the color if its color is the same as the one we found)
  */
  static void GetColorIndexForContour(vtkMRMLContourNode* contourNode, vtkMRMLScene* mrmlScene, int &colorIndex, vtkMRMLColorTableNode* &colorNode, vtkMRMLModelNode* referenceModelNode=NULL);

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

private:
  /*!
    Compute transform between two transformable objects
    /param fromNode Displayable node from which we want to compute the transform
    /param toNode Displayable node to which we want to compute the transform
    /param fromNodeTotoNodeTransform Output transform
  */
  static void GetTransformBetweenDisplayables(vtkMRMLTransformableNode* fromNode, vtkMRMLTransformableNode* toNode, vtkGeneralTransform* fromNodeToToNodeTransform);
};

#include "SlicerRtCommon.txx"

#endif
