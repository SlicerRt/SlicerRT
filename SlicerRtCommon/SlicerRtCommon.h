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

#ifndef __SlicerRtCommon_h
#define __SlicerRtCommon_h

// STD includes
#include <cstdlib>
#include <string>

// ITK includes
#include "itkImage.h"

// VTK includes
#include <vtkSmartPointer.h>

class vtkMRMLColorTableNode;
class vtkMRMLNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLScene;
class vtkMRMLTransformableNode;

class vtkImageData;
class vtkOrientedImageData;
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
  //----------------------------------------------------------------------------
  // Events
  //----------------------------------------------------------------------------
public:
  enum
  {
    /// Progress bar indicator event
    ProgressUpdated = 62200
  };

public:
  //----------------------------------------------------------------------------
  // Constant strings (std::string types for easy concatenation)
  //----------------------------------------------------------------------------

  // SlicerRT constants
  static const char* SLICERRT_EXTENSION_NAME;
  static const std::string STORAGE_NODE_POSTFIX;
  static const std::string DISPLAY_NODE_SUFFIX;

  // Segmentation constants
  static const char* SEGMENTATION_RIBBON_MODEL_REPRESENTATION_NAME;

  static const double COLOR_VALUE_INVALID[4];

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
  static const std::string DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_RTIMAGE_REFERENCED_PLAN_SOP_INSTANCE_UID_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_RTIMAGE_SID_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_RTIMAGE_POSITION_ATTRIBUTE_NAME;
  static const std::string DICOMRTIMPORT_ISODOSE_MODEL_IDENTIFIER_ATTRIBUTE_NAME;

  static const std::string DICOMRTIMPORT_FIDUCIALS_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_MODEL_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_NO_NAME;
  static const std::string DICOMRTIMPORT_NO_STUDY_DESCRIPTION;
  static const std::string DICOMRTIMPORT_SOURCE_HIERARCHY_NODE_NAME_POSTFIX;
  static const std::string DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX;

  static const char* DEFAULT_DOSE_COLOR_TABLE_NAME;

  //----------------------------------------------------------------------------
  // Utility functions
  //----------------------------------------------------------------------------

public:
  /// Compute transform between two transformable objects
  /// \param fromNode Displayable node from which we want to compute the transform
  /// \param toNode Displayable node to which we want to compute the transform
  /// \param fromNodeTotoNodeTransform Output transform
  static void GetTransformBetweenTransformables(vtkMRMLTransformableNode* fromNode, vtkMRMLTransformableNode* toNode, vtkGeneralTransform* fromNodeToToNodeTransform);

  /// Returns true if the string is null or empty, returns false otherwise
  static bool IsStringNullOrEmpty(const char* aString);

  /// Determine if a node is a dose volume node
  static bool IsDoseVolumeNode(vtkMRMLNode* node);

  /// Determine if a node is a isodose model node
  static bool IsIsodoseModelNode(vtkMRMLNode* node);

  /// Stretch a discrete color table that contains a few values into a full 256-color palette that
  /// has the first and last colors the same as the input one, the intermediate colors inserted in
  /// evenly, and the others linearly interpolated.
  static void StretchDiscreteColorTable(vtkMRMLColorTableNode* inputDiscreteColorTable, vtkMRMLColorTableNode* output256ValueColorTable, unsigned int numberOfColors=256);

  /// Check if the lattice (grid, geometry) of two volumes are the same
  static bool DoVolumeLatticesMatch(vtkMRMLScalarVolumeNode* volume1, vtkMRMLScalarVolumeNode* volume2);

  /// Determine if two numbers are equal within a small tolerance (0.0001)
  static bool AreEqualWithTolerance(double a, double b);

  /// Determine if two bounds are equal
  static bool AreExtentsEqual(int boundsA[6], int boundsB[6]);

  /// Generate a new color that is not already in use in a color table node
  /// \param colorNode Color table node to validate against
  static void GenerateRandomColor(vtkMRMLColorTableNode* colorNode, double* newColor);

  /// Write the contents of the image data to file using a scalar volume node and storage node
  /// \param scene the scene to use (can be private scene, or common scene)
  /// \param imageData the image data to write
  /// \param fileName the file to write to on disk
  /// \param dirs the IJKtoRAS directions for a scalar volume node
  /// \param spacing the spacing of a scalar volume node
  /// \param origin the origin of a scalar volume node
  /// \param overwrite whether or not to remove an existing file before re-writing (avoids warnings)
  static void WriteImageDataToFile(vtkMRMLScene* scene, vtkImageData* imageData, const char* fileName, double dirs[3][3], double spacing[3], double origin[3], bool overwrite);

  /*!
    Convert volume MRML node to oriented image data
    \param inVolumeNode Input volume node
    \param outImageData Output oriented image data
    \param applyRasToWorldConversion Apply parent linear transform to image. True by default.
    \return Success
  */
  static bool ConvertVolumeNodeToVtkOrientedImageData(vtkMRMLScalarVolumeNode* inVolumeNode, vtkOrientedImageData* outImageData, bool applyRasToWorldConversion=true);

//BTX
  /*!
    Convert volume MRML node to ITK image
    \param inVolumeNode Input volume node
    \param outItkVolume Output ITK image
    \param applyRasToWorldConversion Apply parent linear transform to image. True by default
    \param applyRasToLpsConversion Apply RAS (Slicer) to LPS (ITK, DICOM) coordinate frame conversion. True by default
    \return Success
  */
  template<typename T> static bool ConvertVolumeNodeToItkImage(vtkMRMLScalarVolumeNode* inVolumeNode, typename itk::Image<T, 3>::Pointer outItkImage, bool applyRasToWorldConversion=true, bool applyRasToLpsConversion=true);

  /*!
    Convert oriented image data to ITK image
    \param inImageData Input oriented image data
    \param outItkVolume Output ITK image
    \param applyRasToLpsConversion Apply RAS (Slicer) to LPS (ITK, DICOM) coordinate frame conversion. True by default
    \return Success
  */
  template<typename T> static bool ConvertVtkOrientedImageDataToItkImage(vtkOrientedImageData* inImageData, typename itk::Image<T, 3>::Pointer outItkImage, bool applyRasToLpsConversion=true);

  /*!
    Convert ITK image to VTK image data. The image geometry is not considered!
    \param inItkImage Input ITK image
    \param outVtkImageData Output VTK image data
    \param vtkType Data scalar type (i.e VTK_FLOAT)
    \return Success
  */
  template<typename T> static bool ConvertItkImageToVtkImageData(typename itk::Image<T, 3>::Pointer inItkImage, vtkImageData* outVtkImageData, int vtkType);

  /*!
    Convert ITK image to MRML volume node. Image geometry is transferred.
    \param inItkImage Input ITK image
    \param outVolumeNode Output MRML scalar volume node
    \param vtkType Data scalar type (i.e VTK_FLOAT)
    \param applyLpsToRasConversion Apply LPS (ITK, DICOM) to RAS (Slicer) coordinate frame conversion. True by default
    \return Success
  */
  template<typename T> static bool ConvertItkImageToVolumeNode(typename itk::Image<T, 3>::Pointer inItkImage, vtkMRMLScalarVolumeNode* outVolumeNode, int vtkType, bool applyLpsToRasConversion=true);
//ETX
};

#include "SlicerRtCommon.txx"

#endif
