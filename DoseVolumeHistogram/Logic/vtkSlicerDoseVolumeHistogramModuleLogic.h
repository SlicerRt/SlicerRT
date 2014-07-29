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

class vtkImageStencilData;
class vtkMRMLScalarVolumeNode;
class vtkMRMLDoubleArrayNode;
class vtkMRMLContourNode;
class vtkMRMLModelNode;
class vtkMRMLChartViewNode;
class vtkMRMLDoseVolumeHistogramNode;

/// \ingroup SlicerRt_QtModules_DoseVolumeHistogram
/// \brief The DoseVolumeHistogram module computes dose volume histogram (DVH) and metrics from a dose map and contour set(s).
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
  static vtkSlicerDoseVolumeHistogramModuleLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramModuleLogic, vtkSlicerModuleLogic);

public:
  /// Compute DVH for the given structure contour node volume with the stenciled dose volume (the indexed labelmap representation but with dose values instead of the labels)
  void ComputeDvh(vtkMRMLContourNode* structureContourNodes, std::string &errorMessage);

  /// Add dose volume histogram of a structure (ROI) to the selected chart given its double array node ID
  void AddDvhToSelectedChart(const char* dvhArrayNodeId);

  /// Add dose volume histogram of a structure (ROI) to the a given chart given the double array node ID and the chart node ID
  void AddDvhToChart(const char* dvhArrayNodeId, const char* chartNodeID);
  
  /// Remove dose volume histogram of a structure from the selected chart
  void RemoveDvhFromSelectedChart(const char* dvhArrayNodeId);

  /// Determine if a DVH array is added to the selected chart
  bool IsDvhAddedToSelectedChart(const char* dvhArrayNodeId);

  /// Compute V metrics for the given DVH using the given dose values and put them in the vMetricsCc and vMetricsPercent output lists
  void ComputeVMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> doseValues, std::vector<double> &vMetricsCc, std::vector<double> &vMetricsPercent);

  /// Compute D metrics for the given DVH using the given volume sizes and put them in the dMetrics output list
  /// \param isPercent If on, then dMetrics values are interpreted as percentage values, otherwise as Cc
  void ComputeDMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> volumeSizes, std::vector<double> &dMetrics, bool isPercent);

  /// Return false if the dose volume contains a volume that is really a dose volume
  bool DoseVolumeContainsDose();

  /// Refreshes DVH double array MRML node vector from the scene
  void RefreshDvhDoubleArrayNodesFromScene();

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

  /// Collect DVH metrics from a collection of DVH double array nodes and try to order some of them
  void CollectMetricsForDvhNodes(std::vector<vtkMRMLNode*> dvhNodes, std::vector<std::string> &metricList);

  /// Assemble attribute name for dose metric, e.g. DVH_Metric_Mean dose (Gy), where
  /// \param doseMetricAttributeNamePrefix Prefix of the desired dose metric attribute name, e.g. "Mean dose"
  /// \param doseUnitName Dose unit name, e.g. "Gy"
  /// \param Output string for the attribute name. has to be allocated first
  static void AssembleDoseMetricAttributeName(std::string doseMetricAttributeNamePrefix, const char* doseUnitName, std::string &attributeName);

  /// Read DVH double arrays from a CSV file
  /// \return a vtkCollection containing vtkMRMLDoubleArrayNodes. Each node represents one structure DVH and contains the vtkDoubleArray as well as the name and total volume attributes for the structure.
  vtkCollection* ReadCsvToDoubleArrayNode(std::string csvFilename);
  
public:
  void SetAndObserveDoseVolumeHistogramNode(vtkMRMLDoseVolumeHistogramNode* node);
  vtkGetObjectMacro(DoseVolumeHistogramNode, vtkMRMLDoseVolumeHistogramNode);

  vtkGetMacro(StartValue, double);
  vtkSetMacro(StartValue, double);

  vtkGetMacro(StepSize, double);
  vtkSetMacro(StepSize, double);

  vtkGetMacro(NumberOfSamplesForNonDoseVolumes, int);
  vtkSetMacro(NumberOfSamplesForNonDoseVolumes, int);

  vtkGetMacro(DoseVolumeOversamplingFactor, double);
  vtkSetMacro(DoseVolumeOversamplingFactor, double);

  vtkGetMacro(LogSpeedMeasurements, bool);
  vtkSetMacro(LogSpeedMeasurements, bool);
  vtkBooleanMacro(LogSpeedMeasurements, bool);

protected:
  vtkSlicerDoseVolumeHistogramModuleLogic();
  virtual ~vtkSlicerDoseVolumeHistogramModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

  /// Get the resampled dose volume and the stencil for a structure on the resampled dose volume
  /// \param structureContourNode Input contour node containing the structure to stencil
  /// \param resampledDoseVolumeNode Output volume node that will contain the resampled dose volume
  /// \param consolidatedStructureContourNode Output contour containing the structure indexed labelmap if its lattice matches the resampled dose volume, or a temporarily resampled copy otherwise
  virtual void GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour(vtkMRMLContourNode* structureContourNode, vtkMRMLScalarVolumeNode* resampledDoseVolumeNode, vtkMRMLContourNode* consolidatedStructureContourNode);

  /// Return the chart view node object from the layout
  vtkMRMLChartViewNode* GetChartViewNode();

  /// Get selected contour nodes (expands a hierarchy node if found)
  void GetSelectedContourNodes(std::vector<vtkMRMLContourNode*> &contourNodes);

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
  double DoseVolumeOversamplingFactor;

  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;
};

#endif
