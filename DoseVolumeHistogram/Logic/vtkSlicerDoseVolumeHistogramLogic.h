/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// .NAME vtkSlicerDoseVolumeHistogramLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDoseVolumeHistogramLogic_h
#define __vtkSlicerDoseVolumeHistogramLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// VTK includes
#include "vtkImageAccumulate.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLChartNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLChartViewNode;
class vtkMRMLDoubleArrayNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DOSEVOLUMEHISTOGRAM_MODULE_LOGIC_EXPORT vtkSlicerDoseVolumeHistogramLogic :
  public vtkSlicerModuleLogic
{
public:
  static const std::string DVH_TYPE_ATTRIBUTE_NAME;
  static const std::string DVH_TYPE_ATTRIBUTE_VALUE;
  static const std::string DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_MODEL_NODE_ID_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME;
  static const std::string DVH_METRIC_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_LIST_ATTRIBUTE_NAME;
  static const char DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  static const std::string DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  static const std::string DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_VOXEL_COUNT_ATTRIBUTE_NAME;
  static const std::string DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX;

public:
  static vtkSlicerDoseVolumeHistogramLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramLogic, vtkSlicerModuleLogic);
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
  void ComputeDMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> volumeSizes, std::vector<double> &dMetrics);

public:
  void SetDoseVolumeNode( vtkMRMLVolumeNode* );
  void SetStructureSetModelNode( vtkMRMLNode* );
  void SetChartNode( vtkMRMLChartNode* );

  vtkGetObjectMacro( DoseVolumeNode, vtkMRMLVolumeNode );
  vtkGetObjectMacro( StructureSetModelNode, vtkMRMLNode );
  vtkGetObjectMacro( ChartNode, vtkMRMLChartNode );
  vtkGetObjectMacro( DvhDoubleArrayNodes, vtkCollection );

  vtkSetMacro( SceneChanged, bool );
  vtkGetMacro( SceneChanged, bool );
  vtkBooleanMacro( SceneChanged, bool );

protected:
  vtkSlicerDoseVolumeHistogramLogic();
  virtual ~vtkSlicerDoseVolumeHistogramLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// Compute DVH for the given volume (which is the selected dose volume stenciled with a structure) with the given structure model node
  void ComputeDvh(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModelNode);

  /// Get stenciled dose volume for a structure (ROI)
  virtual void GetStenciledDoseVolumeForStructure(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModel);

  /// Return the chart view node object from the layout
  vtkMRMLChartViewNode* GetChartViewNode();

  /// Get selected structure model nodes (expands a hierarchy node if found)
  void GetSelectedStructureModelNodes(std::vector<vtkMRMLModelNode*> &structureModelNodes);

protected:
  vtkSetObjectMacro( DvhDoubleArrayNodes, vtkCollection );

private:
  vtkSlicerDoseVolumeHistogramLogic(const vtkSlicerDoseVolumeHistogramLogic&); // Not implemented
  void operator=(const vtkSlicerDoseVolumeHistogramLogic&);               // Not implemented

protected:
  /// Selected dose volume MRML node object
  vtkMRMLVolumeNode* DoseVolumeNode;

  /// Selected structure set MRML node object. Can be model node or model hierarchy node
  vtkMRMLNode* StructureSetModelNode;

  /// Selected chart MRML node object
  vtkMRMLChartNode* ChartNode;

  /// List of all the DVH double array MRML nodes that are present in the scene
  vtkCollection* DvhDoubleArrayNodes;

  /// Flag indicating if the scene has recently changed (update of the module GUI needed)
  bool SceneChanged;
};

#endif
