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

// VTK includes
#include "vtkImageAccumulate.h"

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

class vtkOrientedImageData;
class vtkCallbackCommand;
class vtkMRMLDoubleArrayNode;
class vtkMRMLChartViewNode;
class vtkMRMLDoseVolumeHistogramNode;

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
  static const std::string DVH_DOSE_VOLUME_NODE_REFERENCE_ROLE;
  static const std::string DVH_CREATED_DVH_NODE_REFERENCE_ROLE;
  static const std::string DVH_SEGMENTATION_NODE_REFERENCE_ROLE;

  static const std::string DVH_DOSE_VOLUME_OVERSAMPLING_FACTOR_ATTRIBUTE_NAME;
  static const std::string DVH_SEGMENT_ID_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_TABLE_ROW_ATTRIBUTE_NAME;

  static const std::string DVH_METRIC_STRUCTURE;
  static const std::string DVH_METRIC_TOTAL_VOLUME_CC;
  static const std::string DVH_METRIC_MEAN_PREFIX;
  static const std::string DVH_METRIC_MIN_PREFIX;
  static const std::string DVH_METRIC_MAX_PREFIX;
  static const std::string DVH_METRIC_DOSE_POSTFIX;
  static const std::string DVH_METRIC_INTENSITY_POSTFIX;
  static const std::string DVH_ARRAY_NODE_NAME_POSTFIX;
  static const std::string DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE;
  static const std::string DVH_CSV_HEADER_VOLUME_FIELD_END;

public:
  static vtkSlicerDoseVolumeHistogramModuleLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramModuleLogic, vtkSlicerModuleLogic);

public:
  /// Compute DVH based on parameter node selections (dose volume, segmentation, segment IDs)
  std::string ComputeDvh();

  /// Add dose volume histogram of a structure (ROI) to the selected chart given its double array node ID
  void AddDvhToChart(const char* dvhArrayNodeId);
  
  /// Remove dose volume histogram of a structure from the selected chart
  void RemoveDvhFromChart(const char* dvhArrayNodeId);

  /// Determine if a DVH array is added to the selected chart
  bool IsDvhAddedToChart(const char* dvhArrayNodeId);

  /// Compute V metrics for existing DVHs using the given dose values and add them in the metrics table
  bool ComputeVMetrics();

  /// Compute D metrics for existing DVHs using the given dose values and add them in the metrics table
  bool ComputeDMetrics();

  /// Return false if the dose volume contains a volume that is really a dose volume
  bool DoseVolumeContainsDose();

  /// Export DVH values 
  /// \param comma Flag determining if the CSV file to be saved is deliminated using commas or tabs (regional considerations)
  /// \return True if file written and saved successfully, false otherwise
  bool ExportDvhToCsv(const char* fileName, bool comma=true);

  /// Export DVH metrics
  /// \param vDoseValuesCc List of doses for V(cc) metrics to be computed and exported
  /// \param vDoseValuesPercent List of doses for V(%) metrics to be computed and exported
  /// \param dVolumeValuesCc List of volume sizes in cc's for D metrics to be computed and exported
  /// \param dVolumeValuesPercent List of volume sizes in percentage of the structure size for D metrics to be computed and exported
  /// \param comma Flag determining if the CSV file to be saved is deliminated using commas or tabs (regional considerations)
  /// \return True if file written and saved successfully, false otherwise
  bool ExportDvhMetricsToCsv(const char* fileName, std::vector<double> vDoseValuesCc, std::vector<double> vDoseValuesPercent, std::vector<double> dVolumeValuesCc, std::vector<double> dVolumeValuesPercent, bool comma=true);

  /// Read DVH double arrays from a CSV file
  /// \return a vtkCollection containing vtkMRMLDoubleArrayNodes. Each node represents one structure DVH and contains the vtkDoubleArray as well as the name and total volume attributes for the structure.
  vtkCollection* ReadCsvToDoubleArrayNode(std::string csvFilename);

  /// Assemble dose metric name, e.g. "Mean dose (Gy)". If selected volume is not a dose, it will contain "intensity" instead of "dose"
  /// \param doseMetricAttributeNamePrefix Prefix of the desired dose metric attribute name, e.g. "Mean "
  std::string AssembleDoseMetricName(std::string doseMetricAttributeNamePrefix);

public:
  void SetAndObserveDoseVolumeHistogramNode(vtkMRMLDoseVolumeHistogramNode* node);
  vtkGetObjectMacro(DoseVolumeHistogramNode, vtkMRMLDoseVolumeHistogramNode);

  vtkGetMacro(StartValue, double);
  vtkSetMacro(StartValue, double);

  vtkGetMacro(StepSize, double);
  vtkSetMacro(StepSize, double);

  vtkGetMacro(NumberOfSamplesForNonDoseVolumes, int);
  vtkSetMacro(NumberOfSamplesForNonDoseVolumes, int);

  vtkGetMacro(DefaultDoseVolumeOversamplingFactor, double);
  vtkSetMacro(DefaultDoseVolumeOversamplingFactor, double);

  vtkGetMacro(LogSpeedMeasurements, bool);
  vtkSetMacro(LogSpeedMeasurements, bool);
  vtkBooleanMacro(LogSpeedMeasurements, bool);

protected:
  /// Compute DVH for the given structure segment with the stenciled dose volume
  /// (the labelmap representation of a segment but with dose values instead of the labels)
  /// \param segmentLabelmap Binary representation of the labelmap representation of the segment the DVH is calculated on
  /// \param oversampledDoseVolume Dose volume resampled to match the geometry of the segment labelmap (to allow stenciling)
  /// \param segmentID ID of segment the DVH is calculated on
  /// \param maxDoseGy Maximum dose determining the number of DVH bins (passed as argument so that it is only calculated once in \sa ComputeDvh() )
  /// \return Error message, empty string if no error
  std::string ComputeDvh(vtkOrientedImageData* segmentLabelmap, vtkOrientedImageData* oversampledDoseVolume, std::string segmentID, double maxDoseGy);

  /// Return the chart view node object from the layout
  vtkMRMLChartViewNode* GetChartViewNode();

  /// Set up metrics table by creating the default columns
  void InitializeMetricsTable();

  /// Determine if a metric name belongs to a V metric
  bool IsVMetricName(std::string name);

  /// Determine if a metric name belongs to a V metric
  bool IsDMetricName(std::string name);

  /// Get numbers from V or D metric parameters list
  void GetNumbersFromMetricString(std::string metricStr, std::vector<double> &metricNumbers);

  /// Calculate one D metric. Called from \sa ComputeDMetrics
  double ComputeDMetric(vtkMRMLDoubleArrayNode* dvhArrayNode, double volume, double structureVolume, bool isPercent);

  /// Callback function observing the visibility column of the metrics table
  static void OnVisibilityChanged(vtkObject* caller, unsigned long eid, void* clientData, void* callData);
  
protected:
  vtkSlicerDoseVolumeHistogramModuleLogic();
  virtual ~vtkSlicerDoseVolumeHistogramModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerDoseVolumeHistogramModuleLogic(const vtkSlicerDoseVolumeHistogramModuleLogic&); // Not implemented
  void operator=(const vtkSlicerDoseVolumeHistogramModuleLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLDoseVolumeHistogramNode* DoseVolumeHistogramNode;

  /// Start value for the dose axis of the DVH table
  double StartValue;

  /// Step size for the dose axis of the DVH table
  double StepSize;

  /// Number of bins to sample when input is non-dose volumes
  int NumberOfSamplesForNonDoseVolumes;

  /// Forced oversampling factor for the dose volume.
  /// The structure labelmap is resampled temporarily to the same lattice as the oversampled dose volume if needed.
  double DefaultDoseVolumeOversamplingFactor;

  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;

  /// Command handling visibility column value changes
  vtkCallbackCommand* VisibilityChangedCallbackCommand;
};

#endif
