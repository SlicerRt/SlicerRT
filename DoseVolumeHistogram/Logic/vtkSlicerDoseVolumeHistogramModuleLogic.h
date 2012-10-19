/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// STD includes
#include <cstdlib>
#include <set>

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

class vtkImageData;
class vtkImageStencilData;
class vtkMRMLDoubleArrayNode;
class vtkMRMLContourNode;
class vtkMRMLModelNode;
class vtkMRMLChartViewNode;
class vtkMRMLDoseVolumeHistogramNode;

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

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DOSEVOLUMEHISTOGRAM_LOGIC_EXPORT vtkSlicerDoseVolumeHistogramModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDoseVolumeHistogramModuleLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Compute DVH for the selected structure set based on the selected dose volume
  void ComputeDvh();

  /// Add dose volume histogram of a structure (ROI) to the selected chart given its plot name (including table row number) and the corresponding DVH double array node ID
  void AddDvhToSelectedChart(const char* structurePlotName, const char* dvhArrayNodeId);

  /// Remove dose volume histogram of a structure from the selected chart
  void RemoveDvhFromSelectedChart(const char* dvhArrayNodeId);

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
  void CollectMetricsForDvhNodes(std::vector<std::string>* dvhNodeIds, std::vector<std::string> &metricList);

  /// Assemble attribute name for dose metric, e.g. DVH_Metric_Mean dose (Gy), where
  /// \param doseMetricAttributeNamePrefix Prefix of the desired dose metric attribute name, e.g. "Mean dose"
  /// \param doseUnitName Dose unit name, e.g. "Gy"
  /// \param Output string for the attribute name. has to be allocated first
  static void AssembleDoseMetricAttributeName(std::string doseMetricAttributeNamePrefix, const char* doseUnitName, std::string &attributeName);

public:
  void SetAndObserveDoseVolumeHistogramNode(vtkMRMLDoseVolumeHistogramNode* node);
  vtkGetObjectMacro(DoseVolumeHistogramNode, vtkMRMLDoseVolumeHistogramNode);

  vtkGetMacro(StartValue, double);
  vtkSetMacro(StartValue, double);

  vtkGetMacro(StepSize, double);
  vtkSetMacro(StepSize, double);

  vtkGetMacro(NumberOfSamplesForNonDoseVolumes, int);
  vtkSetMacro(NumberOfSamplesForNonDoseVolumes, int);

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

  /// Compute DVH for the given structure contour node volume with the stenciled dose volume (the indexed labelmap representation but with dose values instead of the labels)
  void ComputeDvh(vtkMRMLContourNode* structureContourNode);

  /// Get the resampled dose volume and the stencil for a structure on the resampled dose volume
  virtual void GetStencilForContour(vtkMRMLContourNode* structureContourNode, vtkImageData* resampledDoseVolume, vtkImageStencilData* structureStencil);

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

  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;
};

#endif
