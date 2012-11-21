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

const std::string SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX = "_ColorTable";
const std::string SlicerRtCommon::DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX = "_AllStructures";
const std::string SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX = "_Contour";
const std::string SlicerRtCommon::DICOMRTIMPORT_ROOT_CONTOUR_HIERARCHY_NODE_NAME_POSTFIX = "_AllContours";
const std::string SlicerRtCommon::DICOMRTIMPORT_ISOCENTER_HIERARCHY_NODE_NAME_POSTFIX = "_Isocenters";

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
const std::string SlicerRtCommon::DOSEACCUMULATION_OUTPUT_BASE_NAME_PREFIX = "Accumulated";

// Isodose constants
const std::string SlicerRtCommon::ISODOSE_MODEL_NODE_NAME_PREFIX = "IsodoseLevel_";
const std::string SlicerRtCommon::ISODOSE_COLOR_TABLE_NODE_NAME_POSTFIX = "_IsodoseColor";

// Dose comparison constants
const std::string SlicerRtCommon::DOSECOMPARISON_OUTPUT_BASE_NAME_PREFIX = "GammaVolume";

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
