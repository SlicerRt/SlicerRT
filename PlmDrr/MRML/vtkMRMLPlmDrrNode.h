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

#ifndef __vtkMRMLPlmDrrNode_h
#define __vtkMRMLPlmDrrNode_h

// Beams includes
#include "vtkSlicerPlmDrrModuleMRMLExport.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLModelNode.h>

// STD includes
#include <list>

class vtkMRMLLinearTransformNode;
class vtkMRMLRTBeamNode;
class vtkMRMLMarkupsClosedCurveNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLMarkupsLineNode;

/// \ingroup SlicerRt_QtModules_PlmDrr
class VTK_SLICER_PLMDRR_MODULE_MRML_EXPORT vtkMRMLPlmDrrNode : public vtkMRMLNode
{
public:
  enum AlgorithmReconstuctionType { EXACT, UNIFORM };
  enum HounsfieldUnitsConversionType { PREPROCESS, INLINE, NONE };
  enum ThreadingType { CPU, CUDA, OPENCL };

  static vtkMRMLPlmDrrNode *New();
  vtkTypeMacro(vtkMRMLPlmDrrNode,vtkMRMLNode);
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
  vtkMRMLCopyContentMacro(vtkMRMLRTPlanNode);

  /// Get unique node XML tag name
  const char* GetNodeTagName() override { return "PlmDrr"; };

  void GetRTImagePosition(double position[2]);

  /// @brief Generate plastimatch drr command line arguments as a list of strings
  /// @return command line arguments as a string (plastimatch drr included)
  std::string GenerateArguments(std::list< std::string >& plastimatchArguments);

public:
 
  /// Get beam node
  vtkMRMLRTBeamNode* GetBeamNode();
  /// Set and observe beam node
  void SetAndObserveBeamNode(vtkMRMLRTBeamNode* node);

  vtkGetVector4Macro(NormalVector, double);
  vtkSetVector4Macro(NormalVector, double);

  vtkGetVector4Macro(ViewUpVector, double);
  vtkSetVector4Macro(ViewUpVector, double);

  vtkGetMacro(AlgorithmReconstuction, AlgorithmReconstuctionType);
  vtkSetMacro(AlgorithmReconstuction, AlgorithmReconstuctionType);

  vtkGetMacro(HUConversion, HounsfieldUnitsConversionType);
  vtkSetMacro(HUConversion, HounsfieldUnitsConversionType);

  vtkGetMacro(Threading, ThreadingType);
  vtkSetMacro(Threading, ThreadingType);

  vtkGetMacro(ExponentialMappingFlag, bool);
  vtkSetMacro(ExponentialMappingFlag, bool);

  vtkGetMacro(AutoscaleFlag, bool);
  vtkSetMacro(AutoscaleFlag, bool);

  vtkGetVector2Macro(AutoscaleRange, signed long int);
  vtkSetVector2Macro(AutoscaleRange, signed long int);

  vtkGetMacro(IsocenterImagerDistance, double);
  vtkSetMacro(IsocenterImagerDistance, double);

  vtkGetVector2Macro(ImagerCenterOffset, double);
  vtkSetVector2Macro(ImagerCenterOffset, double);

  vtkGetVector2Macro(ImageDimention, int);
  vtkSetVector2Macro(ImageDimention, int);

  vtkGetVector2Macro(ImageSpacing, double);
  vtkSetVector2Macro(ImageSpacing, double);

  vtkGetVector2Macro(ImageCenter, int);
  vtkSetVector2Macro(ImageCenter, int);

  vtkGetMacro(ImageWindowFlag, bool);
  vtkSetMacro(ImageWindowFlag, bool);

  vtkGetVector4Macro(ImageWindow, int);
  vtkSetVector4Macro(ImageWindow, int);

  vtkGetMacro(RotateX, double);
  vtkSetMacro(RotateX, double);

  vtkGetMacro(RotateY, double);
  vtkSetMacro(RotateY, double);

  vtkGetMacro(RotateZ, double);
  vtkSetMacro(RotateZ, double);

protected:
  vtkMRMLPlmDrrNode();
  ~vtkMRMLPlmDrrNode();
  vtkMRMLPlmDrrNode(const vtkMRMLPlmDrrNode&);
  void operator=(const vtkMRMLPlmDrrNode&);

  void SetAlgorithmReconstuction(int algorithmReconstuction = 0);
  void SetHUConversion(int huConversion = 0);
  void SetThreading(int threading = 0);

protected:
  double NormalVector[4];
  double ViewUpVector[4];
  double IsocenterImagerDistance; // fabs(SID - SAD)
  double ImagerCenterOffset[2]; // x,y
  int ImageDimention[2]; // columns, rows
  double ImageSpacing[2]; // x,y
  int ImageCenter[2]; // column, row (calculated from imager offset and image data)
  bool ImageWindowFlag; // use image window
  int ImageWindow[4]; // column1, column2, row1, row2 (y0, y1, x0, x1)
  double RotateX; // not used
  double RotateY; // not used
  double RotateZ; // deg
  AlgorithmReconstuctionType AlgorithmReconstuction;
  HounsfieldUnitsConversionType HUConversion;
  ThreadingType Threading;
  bool ExponentialMappingFlag;
  bool AutoscaleFlag;
  signed long int AutoscaleRange[2];
};

#endif
