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

#ifndef __vtkMRMLDrrImageComputationNode_h
#define __vtkMRMLDrrImageComputationNode_h

// Beams includes
#include "vtkSlicerDrrImageComputationModuleMRMLExport.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLModelNode.h>

// PlanarImage includes
#include <vtkSlicerPlanarImageModuleLogic.h>
#include <vtkMRMLPlanarImageNode.h>

class vtkMRMLLinearTransformNode;
class vtkMRMLRTBeamNode;
class vtkMRMLMarkupsClosedCurveNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLMarkupsLineNode;
class vtkMRMLCameraNode;

/// \ingroup SlicerRt_QtModules_DrrImageComputation
/// 
/// Parameter set node for DRR image (RTImage) computation. 
/// \warning Currently supports only plastimatch reconstruct library.
/// Node references:
///   Parameter -> RT beam (BEAM_REFERENCE_ROLE)
///

class VTK_SLICER_DRRIMAGECOMPUTATION_MODULE_MRML_EXPORT vtkMRMLDrrImageComputationNode : public vtkMRMLPlanarImageNode
{
public:
  enum PlastimatchAlgorithmReconstuctionType { Exact, Uniform };
  enum PlastimatchHounsfieldUnitsConversionType { Preprocess, Inline, None };
  enum PlastimatchThreadingType { CPU, CUDA, OpenCL };

  static vtkMRMLDrrImageComputationNode *New();
  vtkTypeMacro(vtkMRMLDrrImageComputationNode,vtkMRMLPlanarImageNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Create instance of a GAD node. 
  vtkMRMLNode* CreateNodeInstance() override;

  /// Set node attributes from name/value pairs 
  void ReadXMLAttributes(const char** atts) override;

  /// Write this node's information to a MRML file in XML format. 
  void WriteXML(ostream& of, int indent) override;

  /// Copy the node's attributes to this object 
  void Copy(vtkMRMLNode *node) override;

  /// Copy node content (excludes basic data, such a name and node reference)
  vtkMRMLCopyContentMacro(vtkMRMLDrrImageComputationNode);

  /// Get unique node XML tag name
  const char* GetNodeTagName() override { return "DrrImageComputation"; };

  void GetRTImagePosition(double position[2]);
  void GetIsocenterPositionLPS(double position[3]);

  /// Handles events registered in the observer manager
  void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData) override;

public:
  /// Get beam node
  vtkMRMLRTBeamNode* GetBeamNode();
  /// Set and observe beam node. This updates Normal and View-Up vectors.
  void SetAndObserveBeamNode(vtkMRMLRTBeamNode* node);

  /// Get camera node
  vtkMRMLCameraNode* GetCameraNode();
  /// Set and observe camera node.
  /// Camera updates beam parameters: gantry angle, couch angle, SAD.
  /// Beam transformation and beam geometry will be updated 
  /// if camera was used to calculate beam parameters from direction cosines.   
  void SetAndObserveCameraNode(vtkMRMLCameraNode* node);

  vtkGetVector3Macro(NormalVector, double);
  vtkSetVector3Macro(NormalVector, double);

  vtkGetVector3Macro(ViewUpVector, double);
  vtkSetVector3Macro(ViewUpVector, double);

  vtkGetMacro(AlgorithmReconstuction, PlastimatchAlgorithmReconstuctionType);
  vtkSetMacro(AlgorithmReconstuction, PlastimatchAlgorithmReconstuctionType);

  vtkGetMacro(HUConversion, PlastimatchHounsfieldUnitsConversionType);
  vtkSetMacro(HUConversion, PlastimatchHounsfieldUnitsConversionType);

  vtkGetMacro(Threading, PlastimatchThreadingType);
  vtkSetMacro(Threading, PlastimatchThreadingType);

  vtkGetMacro(ExponentialMappingFlag, bool);
  vtkSetMacro(ExponentialMappingFlag, bool);

  vtkGetMacro(AutoscaleFlag, bool);
  vtkSetMacro(AutoscaleFlag, bool);

  vtkGetVector2Macro(AutoscaleRange, float);
  vtkSetVector2Macro(AutoscaleRange, float);

  vtkGetMacro(IsocenterImagerDistance, double);
  vtkSetMacro(IsocenterImagerDistance, double);

  vtkGetVector2Macro(ImagerCenterOffset, double);
  vtkSetVector2Macro(ImagerCenterOffset, double);

  vtkGetVector2Macro(ImagerResolution, int);
  vtkSetVector2Macro(ImagerResolution, int);

  vtkGetVector2Macro(ImagerSpacing, double);
  vtkSetVector2Macro(ImagerSpacing, double);

  /// \brief This also updates image center data
  void GetImageCenter(double imageCenter[2]);
  vtkGetVector2Macro(ImageCenter, int);
  vtkSetVector2Macro(ImageCenter, int);

  vtkGetMacro(ImageWindowFlag, bool);
  vtkSetMacro(ImageWindowFlag, bool);

  vtkGetMacro(HUThresholdBelow, int);
  vtkSetMacro(HUThresholdBelow, int);

  vtkGetMacro(InvertIntensityFlag, bool);
  vtkSetMacro(InvertIntensityFlag, bool);

  vtkGetVector4Macro(ImageWindow, int);
  vtkSetVector4Macro(ImageWindow, int);

protected:
  vtkMRMLDrrImageComputationNode();
  ~vtkMRMLDrrImageComputationNode();
  vtkMRMLDrrImageComputationNode(const vtkMRMLDrrImageComputationNode&);
  void operator=(const vtkMRMLDrrImageComputationNode&);

  void SetAlgorithmReconstuction(int algorithmReconstuction = 0);
  void SetHUConversion(int huConversion = 0);
  void SetThreading(int threading = 0);

protected:
  double NormalVector[3]; // updated in logic
  double ViewUpVector[3]; // updated in logic
  double IsocenterImagerDistance; // fabs(SID - SAD)
  double ImagerCenterOffset[2]; // x,y
  int ImagerResolution[2]; // columns, rows
  double ImagerSpacing[2]; // columns, rows
  int ImageCenter[2]; // column, row (calculated from imager offset and image data)
  bool ImageWindowFlag; // use image window
  int ImageWindow[4]; // column1, row1, column2, row2 (y0, x0, y1, x1)
  PlastimatchAlgorithmReconstuctionType AlgorithmReconstuction;
  PlastimatchHounsfieldUnitsConversionType HUConversion;
  PlastimatchThreadingType Threading;
  bool ExponentialMappingFlag;
  bool AutoscaleFlag;
  bool InvertIntensityFlag;
  float AutoscaleRange[2];
  int HUThresholdBelow;
};

#endif
