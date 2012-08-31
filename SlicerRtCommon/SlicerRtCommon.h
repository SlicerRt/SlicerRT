#ifndef __SlicerRtCommon_h
#define __SlicerRtCommon_h

// STD includes
#include <cstdlib>
#include <string>

namespace SlicerRtCommon
{
  // DicomRtImport constants
  const std::string DICOMRTIMPORT_ATTRIBUTE_PREFIX = "DicomRtImport.";
  const std::string DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME = DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitName";
  const std::string DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME = DICOMRTIMPORT_ATTRIBUTE_PREFIX + "DoseUnitValue";

  const std::string DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX = " - Color table";
  const std::string DICOMRTIMPORT_ROOT_MODEL_HIERARCHY_NODE_NAME_POSTFIX = " - All structures";

  // DoseVolumeHistogram constants
  const std::string DVH_ATTRIBUTE_PREFIX = "DoseVolumeHistogram.";
  const std::string DVH_TYPE_ATTRIBUTE_NAME = DVH_ATTRIBUTE_PREFIX + "Type";
  const std::string DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME = DVH_ATTRIBUTE_PREFIX + "DoseVolumeNodeId";
  const std::string DVH_STRUCTURE_NAME_ATTRIBUTE_NAME = DVH_ATTRIBUTE_PREFIX + "StructureName";
  const std::string DVH_STRUCTURE_MODEL_NODE_ID_ATTRIBUTE_NAME = DVH_ATTRIBUTE_PREFIX + "StructureModelNodeId";
  const std::string DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME = DVH_ATTRIBUTE_PREFIX + "StructureColor";
  const std::string DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME = DVH_ATTRIBUTE_PREFIX + "StructurePlotName";
  const std::string DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME = DVH_ATTRIBUTE_PREFIX + "StructurePlotLineStyle";
  const std::string DVH_METRIC_ATTRIBUTE_NAME_PREFIX = DVH_ATTRIBUTE_PREFIX + "DvhMetric_";
  const std::string DVH_METRIC_LIST_ATTRIBUTE_NAME = DVH_ATTRIBUTE_PREFIX + "DvhMetricList";

  const std::string DVH_TYPE_ATTRIBUTE_VALUE = "DVH";
  const char        DVH_METRIC_LIST_SEPARATOR_CHARACTER = '|';
  const std::string DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME = "Total volume (cc)";
  const std::string DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX = "Mean dose";
  const std::string DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX = "Min dose";
  const std::string DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX = "Max dose";
  const std::string DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX = "V";
  const std::string DVH_ARRAY_NODE_NAME_POSTFIX = "_DVH";
  const std::string DVH_STRUCTURE_LABELMAP_NODE_NAME_POSTFIX = "_Labelmap";
  const std::string DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE = " Value (% of ";
  const std::string DVH_CSV_HEADER_VOLUME_FIELD_END = " cc)";
};

#endif
