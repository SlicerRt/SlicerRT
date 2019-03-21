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

// .NAME vtkSlicerDoseVolumeHistogramModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDoseVolumeHistogramModuleLogic_h
#define __vtkSlicerDoseVolumeHistogramModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

class vtkOrientedImageData;
class vtkCallbackCommand;

class vtkMRMLDoseVolumeHistogramNode;
class vtkMRMLPlotChartNode;
class vtkMRMLPlotSeriesNode;
class vtkMRMLPlotViewNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLTableNode;

/// \ingroup SlicerRt_QtModules_DoseVolumeHistogram
/// \brief The DoseVolumeHistogram module computes dose volume histogram (DVH) and metrics from a dose map and segmentation.
///
/// The dimensions of the 3D elements (voxels) describing delineated structures are derived from the selected dose distribution volume,
/// in which the voxels have width in the transverse imaging plane as described in the DICOM image header. The image set volume is
/// defined by a grid of voxels derived from the voxel grid in the dose volume. The dose grid is oversampled by a factor currently
/// fixed to the value 2. The centre of each voxel is examined and if found to lie within a structure, is included in the volume for
/// that structure. The dose value at the centre of the cube is interpolated in 3D from the dose grid.
class VTK_SLICER_DOSEVOLUMEHISTOGRAM_LOGIC_EXPORT vtkSlicerDoseVolumeHistogramModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  // DoseVolumeHistogram constants
  static const std::string DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME;
  static const std::string DVH_CREATED_DVH_NODE_REFERENCE_ROLE;
  static const std::string DVH_PLOTSERIES_REFERENCE_ROLE;

  static const std::string DVH_DOSE_VOLUME_OVERSAMPLING_FACTOR_ATTRIBUTE_NAME;
  static const std::string DVH_SEGMENT_ID_ATTRIBUTE_NAME;
  static const std::string DVH_TABLE_ROW_ATTRIBUTE_NAME;
  static const std::string DVH_SURFACE_ATTRIBUTE_NAME;
  static const std::string DVH_SURFACE_INSIDE_ATTRIBUTE_NAME;

  static const std::string DVH_METRIC_STRUCTURE;
  static const std::string DVH_METRIC_TOTAL_VOLUME_CC;
  static const std::string DVH_METRIC_MEAN_PREFIX;
  static const std::string DVH_METRIC_MIN_PREFIX;
  static const std::string DVH_METRIC_MAX_PREFIX;
  static const std::string DVH_METRIC_DOSE_POSTFIX;
  static const std::string DVH_METRIC_INTENSITY_POSTFIX;
  static const std::string DVH_TABLE_NODE_NAME_POSTFIX;
  static const std::string DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE;
  static const std::string DVH_CSV_HEADER_VOLUME_FIELD_END;

public:
  static vtkSlicerDoseVolumeHistogramModuleLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramModuleLogic, vtkSlicerModuleLogic);

public:
  /// Compute DVH based on parameter node selections (dose volume, segmentation, segment IDs)
  std::string ComputeDvh(vtkMRMLDoseVolumeHistogramNode* parameterNode);

  /// Compute V metrics for existing DVHs using the given dose values and add them in the metrics table
  bool ComputeVMetrics(vtkMRMLDoseVolumeHistogramNode* parameterNode);

  /// Compute D metrics for existing DVHs using the given dose values and add them in the metrics table
  bool ComputeDMetrics(vtkMRMLDoseVolumeHistogramNode* parameterNode);

  /// Add dose volume histogram of a structure (ROI) to the selected plot given its table node
  /// \return Plot series node corresponding to the given table in the given chart
  vtkMRMLPlotSeriesNode* AddDvhToChart(vtkMRMLPlotChartNode* chartNode, vtkMRMLTableNode* tableNode);

  /// Remove dose volume histogram of a structure from the selected chart
  void RemoveDvhFromChart(vtkMRMLPlotChartNode* chartNode, vtkMRMLTableNode* tableNode);

  /// Determine if a DVH table is added to the given chart
  /// \return Plot series node belonging to table in chart if visible, nullptr otherwise
  vtkMRMLPlotSeriesNode* IsDvhAddedToChart(vtkMRMLPlotChartNode* chartNode, vtkMRMLTableNode* tableNode);

  /// Get plot series node from chart for given table
  /// \return Plot series node belonging to table in chart if created, nullptr otherwise
  vtkMRMLPlotSeriesNode* GetPlotSeriesNodeForTable(vtkMRMLPlotChartNode* chartNode, vtkMRMLTableNode* tableNode);

  /// Export DVH values
  /// \param comma Flag determining if the CSV file to be saved is deliminated using commas or tabs (regional considerations)
  /// \return True if file written and saved successfully, false otherwise
  bool ExportDvhToCsv(vtkMRMLDoseVolumeHistogramNode* parameterNode, const char* fileName, bool comma=true);

  /// Export DVH metrics
  bool ExportDvhMetricsToCsv(vtkMRMLDoseVolumeHistogramNode* parameterNode, const char* fileName, bool comma=true);

  /// Read DVH tables from a CSV file
  /// \return a vtkCollection containing vtkMRMLTableNode. Each node represents one structure DVH and contains the vtkTable as well as the name and total volume attributes for the structure.
  vtkCollection* ReadCsvToTableNode(std::string csvFilename);

  /// Assemble dose metric name, e.g. "Mean dose (Gy)". If selected volume is not a dose, it will contain "intensity" instead of "dose"
  /// \param doseMetricAttributeNamePrefix Prefix of the desired dose metric attribute name, e.g. "Mean "
  std::string AssembleDoseMetricName(vtkMRMLScalarVolumeNode* doseVolumeNode, std::string doseMetricAttributeNamePrefix);

public:
  vtkGetMacro(StartValue, double);
  vtkSetMacro(StartValue, double);

  vtkGetMacro(StepSize, double);
  vtkSetMacro(StepSize, double);

  vtkGetMacro(NumberOfSamplesForNonDoseVolumes, int);
  vtkSetMacro(NumberOfSamplesForNonDoseVolumes, int);

  vtkGetMacro(DefaultDoseVolumeOversamplingFactor, double);
  vtkSetMacro(DefaultDoseVolumeOversamplingFactor, double);

  vtkGetMacro(UseLinearInterpolationForDoseVolume, bool);
  vtkSetMacro(UseLinearInterpolationForDoseVolume, bool);
  vtkBooleanMacro(UseLinearInterpolationForDoseVolume, bool);

  vtkGetMacro(LogSpeedMeasurements, bool);
  vtkSetMacro(LogSpeedMeasurements, bool);
  vtkBooleanMacro(LogSpeedMeasurements, bool);

protected:
  /// Compute DVH for the given structure segment with the stenciled dose volume
  /// (the labelmap representation of a segment but with dose values instead of the labels)
  /// \param parameterNode Dose volume histogram parameter set node
  /// \param segmentLabelmap Binary representation of the labelmap representation of the segment the DVH is calculated on
  /// \param oversampledDoseVolume Dose volume resampled to match the geometry of the segment labelmap (to allow stenciling)
  /// \param segmentID ID of segment the DVH is calculated on
  /// \param maxDoseGy Maximum dose determining the number of DVH bins (passed as argument so that it is only calculated once in \sa ComputeDvh() )
  /// \return Error message, empty string if no error
  std::string ComputeDvh(
    vtkMRMLDoseVolumeHistogramNode* parameterNode,
    vtkOrientedImageData* segmentLabelmap, vtkOrientedImageData* oversampledDoseVolume,
    std::string segmentID, double maxDoseGy );

  /// Return the plot view node object from the layout
  vtkMRMLPlotViewNode* GetPlotViewNode();

  /// Set up metrics table by creating the default columns
  void InitializeMetricsTable(vtkMRMLDoseVolumeHistogramNode* parameterNode);

  /// Determine if a metric name belongs to a V metric
  bool IsVMetricName(std::string name);

  /// Determine if a metric name belongs to a V metric
  bool IsDMetricName(std::string name);

  /// Get numbers from V or D metric parameters list
  void GetNumbersFromMetricString(std::string metricStr, std::vector<double> &metricNumbers);

  /// Calculate one D metric. Called from \sa ComputeDMetrics
  double ComputeDMetric(vtkMRMLTableNode* tableNode, double volume, double structureVolume, bool isPercent);

  /// Callback function observing the visibility column of the metrics table
  static void OnVisibilityChanged(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

protected:
  vtkSlicerDoseVolumeHistogramModuleLogic();
  ~vtkSlicerDoseVolumeHistogramModuleLogic() override;

  void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;

  void OnMRMLSceneEndClose() override;

private:
  vtkSlicerDoseVolumeHistogramModuleLogic(const vtkSlicerDoseVolumeHistogramModuleLogic&) = delete;
  void operator=(const vtkSlicerDoseVolumeHistogramModuleLogic&) = delete;

protected:
  /// Start value for the dose axis of the DVH table
  double StartValue;

  /// Step size for the dose axis of the DVH table
  double StepSize;

  /// Number of bins to sample when input is non-dose volumes
  int NumberOfSamplesForNonDoseVolumes;

  /// Forced oversampling factor for the dose volume.
  /// The structure labelmap is resampled temporarily to the same lattice as the oversampled dose volume if needed.
  double DefaultDoseVolumeOversamplingFactor;

  /// Flag determining whether linear interpolation is used to oversample the dose volume.
  /// Interpolation may cause skewed metrics near maximum and minimum, if the oversampled segment
  /// does not reach the end of the dose voxel. False by default
  bool UseLinearInterpolationForDoseVolume;

  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;
};

#endif
