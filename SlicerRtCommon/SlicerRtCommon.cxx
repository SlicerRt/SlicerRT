#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLDisplayNode.h>

// VTK includes
#include <vtkGeneralTransform.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>

//----------------------------------------------------------------------------
// Constant strings
//----------------------------------------------------------------------------

// DicomRtImport constants
const std::string SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX = "DicomRtImport.";
const std::string SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitName";
const std::string SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitValue";
const std::string SlicerRtCommon::DICOMRTIMPORT_SERIES_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "SeriesName";
const std::string SlicerRtCommon::DICOMRTIMPORT_BEAM_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "SourceAxisDistance";
const std::string SlicerRtCommon::DICOMRTIMPORT_BEAM_GANTRY_ANGLE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "GantryAngle";
const std::string SlicerRtCommon::DICOMRTIMPORT_BEAM_COUCH_ANGLE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "CouchAngle";
const std::string SlicerRtCommon::DICOMRTIMPORT_BEAM_COLLIMATOR_ANGLE_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "CollimatorAngle";
const std::string SlicerRtCommon::DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME = SlicerRtCommon::DICOMRTIMPORT_ATTRIBUTE_PREFIX + "JawPositions";

const std::string SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX = "_ColorTable";
const std::string SlicerRtCommon::DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX = "_AllStructures";
const std::string SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX = "_Contour";
const std::string SlicerRtCommon::DICOMRTIMPORT_ROOT_CONTOUR_HIERARCHY_NODE_NAME_POSTFIX = "_AllContours";
const std::string SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX = "_Isocenters";
const std::string SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX = "_PatientHierarchy";
const std::string SlicerRtCommon::DICOMRTIMPORT_NO_NAME = "No name";
const std::string SlicerRtCommon::DICOMRTIMPORT_NO_DESCRIPTION = "No description";
const std::string SlicerRtCommon::DICOMRTIMPORT_BEAMMODEL_HIERARCHY_NODE_NAME_POSTFIX = "_BeamModels";

// Contour (vtkMRMLContourNode) constants
const std::string SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX = "_RibbonModel";
const std::string SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX = "_Labelmap";
const std::string SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX = "_ClosedSurfaceModel";

const double SlicerRtCommon::DEFAULT_RASTERIZATION_OVERSAMPLING_FACTOR = 2.0;
const double SlicerRtCommon::DEFAULT_DECIMATION_TARGET_REDUCTION_FACTOR = 0.0;

// DoseVolumeHistogram constants
const std::string SlicerRtCommon::DVH_ATTRIBUTE_PREFIX = "DoseVolumeHistogram.";
const std::string SlicerRtCommon::DVH_TYPE_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "Type";
const std::string SlicerRtCommon::DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DoseVolumeNodeId";
const std::string SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructureName";
const std::string SlicerRtCommon::DVH_STRUCTURE_CONTOUR_NODE_ID_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructureModelNodeId";
const std::string SlicerRtCommon::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructureColor";
const std::string SlicerRtCommon::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructurePlotName";
const std::string SlicerRtCommon::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "StructurePlotLineStyle";
const std::string SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DvhMetric_";
const std::string SlicerRtCommon::DVH_METRIC_LIST_ATTRIBUTE_NAME = SlicerRtCommon::DVH_ATTRIBUTE_PREFIX + "DvhMetricList";

const std::string SlicerRtCommon::DVH_TYPE_ATTRIBUTE_VALUE = "DVH";
const char        SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER = '|';
const std::string SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME = "Total volume (cc)";
const std::string SlicerRtCommon::DVH_METRIC_MEAN_ATTRIBUTE_NAME_PREFIX = "Mean ";
const std::string SlicerRtCommon::DVH_METRIC_MIN_ATTRIBUTE_NAME_PREFIX = "Min ";
const std::string SlicerRtCommon::DVH_METRIC_MAX_ATTRIBUTE_NAME_PREFIX = "Max ";
const std::string SlicerRtCommon::DVH_METRIC_DOSE_ATTRIBUTE_NAME_POSTFIX = "dose";
const std::string SlicerRtCommon::DVH_METRIC_INTENSITY_ATTRIBUTE_NAME_POSTFIX = "intensity";
const std::string SlicerRtCommon::DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX = "V";
const std::string SlicerRtCommon::DVH_ARRAY_NODE_NAME_POSTFIX = "_DvhArray";
const std::string SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE = " Value (% of ";
const std::string SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_END = " cc)";

// DoseAccumulation constants
const std::string SlicerRtCommon::DOSEACCUMULATION_ATTRIBUTE_PREFIX = "DoseAccumulation.";
const std::string SlicerRtCommon::DOSEACCUMULATION_DOSE_VOLUME_NODE_NAME_ATTRIBUTE_NAME = SlicerRtCommon::DOSEACCUMULATION_ATTRIBUTE_PREFIX + "DoseVolumeNodeName";
const std::string SlicerRtCommon::DOSEACCUMULATION_OUTPUT_BASE_NAME_PREFIX = "Accumulated_";

// Isodose constants
const std::string SlicerRtCommon::ISODOSE_MODEL_NODE_NAME_PREFIX = "IsodoseLevel_";
const std::string SlicerRtCommon::ISODOSE_COLOR_TABLE_NODE_NAME_POSTFIX = "_IsodoseColor";

// Dose comparison constants
const std::string SlicerRtCommon::DOSECOMPARISON_OUTPUT_BASE_NAME_PREFIX = "GammaVolume_";

// Beams constants
const std::string SlicerRtCommon::BEAMS_OUTPUT_SOURCE_FIDUCIAL_PREFIX = "Source_";
const std::string SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX = "BeamModel_";
const std::string SlicerRtCommon::BEAMS_PARAMETER_SET_BASE_NAME_PREFIX = "BeamParameterSet_";

//----------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------

void SlicerRtCommon::GetTransformBetweenTransformables(vtkMRMLTransformableNode* fromNode, vtkMRMLTransformableNode* toNode, vtkGeneralTransform* fromNodeToToNodeTransform)
{
  if (!fromNodeToToNodeTransform)
  {
    return;
  }
  if (!fromNode || !toNode)
  {
    return;
  }

  vtkSmartPointer<vtkGeneralTransform> toNodeToWorldTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  vtkMRMLTransformNode* fromNodeTransformNode = fromNode->GetParentTransformNode();
  vtkMRMLTransformNode* toNodeTransformNode = toNode->GetParentTransformNode();

  if (toNodeTransformNode!=NULL)
  {
    // toNode is transformed
    toNodeTransformNode->GetTransformToWorld(toNodeToWorldTransform);    
    if (fromNodeTransformNode!=NULL)
    {
      fromNodeToToNodeTransform->PostMultiply(); // GetTransformToNode assumes PostMultiply
      fromNodeTransformNode->GetTransformToNode(toNodeTransformNode,fromNodeToToNodeTransform);
    }
    else
    {
      // fromNodeTransformNode is NULL => the transform will be computed for the world coordinate frame
      toNodeTransformNode->GetTransformToWorld(fromNodeToToNodeTransform);
      fromNodeToToNodeTransform->Inverse();
    }
  }
  else
  {
    // toNode is not transformed => fromNodeToToNodeTransform = fromNodeToWorldTransformMatrix
    if (fromNodeTransformNode!=NULL)
    {
      // fromNode is transformed
      fromNodeTransformNode->GetTransformToWorld(fromNodeToToNodeTransform);
    }
    else
    {
      // neither fromNode nor toNode is transformed
      fromNodeToToNodeTransform->Identity();
    }
  }
}

//----------------------------------------------------------------------------
bool SlicerRtCommon::IsStringNullOrEmpty(char* aString)
{
  if (aString == NULL)
  {
    return true;
  }
  else if (strcmp(aString, "") == 0)
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor( vtkMRMLVolumeNode* inputVolumeNode, double oversamplingFactor,
                                                               double outputNodeOrigin[3], double outputNodeSpacing[3],
                                                               int outputImageDataExtent[6], double outputImageDataSpacing[3] )
{
  if (!inputVolumeNode || !inputVolumeNode->GetImageData())
  {
    return;
  }

  // Sanity check
  if ( oversamplingFactor < 0.01
    || oversamplingFactor > 100.0 )
  {
    return;
  }

  double nodeOrigin[3];
  double nodeSpacing[3];
  int imageDataExtent[6];
  double imageDataSpacing[3];
  int extentMin, extentMax;
  inputVolumeNode->GetOrigin(nodeOrigin);
  inputVolumeNode->GetSpacing(nodeSpacing);
  inputVolumeNode->GetImageData()->GetWholeExtent(imageDataExtent);
  inputVolumeNode->GetImageData()->GetSpacing(imageDataSpacing);

  for (unsigned int axis=0; axis<3; ++axis)
  {
    extentMin = imageDataExtent[axis*2];
    unsigned int size = imageDataExtent[axis*2+1] - extentMin + 1;

    extentMax = (int)(floor(static_cast<double>(extentMin + size) * oversamplingFactor) - 1);
    extentMin = (int)(ceil(static_cast<double>(extentMin) * oversamplingFactor));

    outputImageDataExtent[axis*2] = extentMin;
    outputImageDataExtent[axis*2+1] = extentMax;
    outputImageDataSpacing[axis] = imageDataSpacing[axis] / oversamplingFactor;
    outputNodeSpacing[axis] = nodeSpacing[axis] / oversamplingFactor;
    outputNodeOrigin[axis] = nodeOrigin[axis] - ((nodeSpacing[axis]/2.0) - (outputNodeSpacing[axis]/2.0));
  }
}
