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

#ifndef __vtkMRMLContourComparisonNode_h
#define __vtkMRMLContourComparisonNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerContourComparisonModuleLogicExport.h"

class vtkMRMLContourNode;
class vtkMRMLScalarVolumeNode;

class VTK_SLICER_CONTOURCOMPARISON_MODULE_LOGIC_EXPORT vtkMRMLContourComparisonNode : public vtkMRMLNode
{
public:
  static vtkMRMLContourComparisonNode *New();
  vtkTypeMacro(vtkMRMLContourComparisonNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "ContourComparison";};

public:
  /// Get reference contour node
  vtkMRMLContourNode* GetReferenceContourNode();
  /// Set reference contour node
  void SetAndObserveReferenceContourNode(vtkMRMLContourNode* node);

  /// Get compare contour node
  vtkMRMLContourNode* GetCompareContourNode();
  /// Set compare contour node
  void SetAndObserveCompareContourNode(vtkMRMLContourNode* node);

  /// Get rasterization reference volume node
  vtkMRMLScalarVolumeNode* GetRasterizationReferenceVolumeNode();
  /// Set rasterization reference volume node
  void SetAndObserveRasterizationReferenceVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get result dice coefficient
  vtkGetMacro(DiceCoefficient, float);
  /// Set result dice coefficient
  vtkSetMacro(DiceCoefficient, float);

  /// Get percentage of true positive labelmap voxels
  vtkGetMacro(TruePositivesPercent, double);
  /// Set percentage of true positive labelmap voxels
  vtkSetMacro(TruePositivesPercent, double);

  /// Get percentage of true negative labelmap voxels
  vtkGetMacro(TrueNegativesPercent, double);
  /// Set percentage of true negative labelmap voxels
  vtkSetMacro(TrueNegativesPercent, double);

  /// Get percentage of false positive labelmap voxels
  vtkGetMacro(FalsePositivesPercent, double);
  /// Set percentage of false positive labelmap voxels
  vtkSetMacro(FalsePositivesPercent, double);

  /// Get percentage of false negative labelmap voxels
  vtkGetMacro(FalseNegativesPercent, double);
  /// Set percentage of false negative labelmap voxels
  vtkSetMacro(FalseNegativesPercent, double);

  /// Get location of the center of mass of the reference structure
  vtkGetVector3Macro(ReferenceCenter, double);
  /// Set location of the center of mass of the reference structure
  vtkSetVector3Macro(ReferenceCenter, double);

  /// Get location of the center of mass of the compare structure
  vtkGetVector3Macro(CompareCenter, double);
  /// Set location of the center of mass of the compare structure
  vtkSetVector3Macro(CompareCenter, double);

  /// Get volume of the reference structure
  vtkGetMacro(ReferenceVolumeCc, double);
  /// Set volume of the reference structure
  vtkSetMacro(ReferenceVolumeCc, double);

  /// Get volume of the compare structure
  vtkGetMacro(CompareVolumeCc, double);
  /// Set volume of the compare structure
  vtkSetMacro(CompareVolumeCc, double);

  /// Get/Set Dice results valid flag
  vtkGetMacro(DiceResultsValid, bool);
  vtkSetMacro(DiceResultsValid, bool);
  vtkBooleanMacro(DiceResultsValid, bool);

  /// Get maximum Hausdorff distance for the whole volume
  vtkGetMacro(MaximumHausdorffDistanceForVolumeMm, double);
  /// Set maximum Hausdorff distance for the whole volume
  vtkSetMacro(MaximumHausdorffDistanceForVolumeMm, double);

  /// Get maximum Hausdorff distance for the boundary voxels
  vtkGetMacro(MaximumHausdorffDistanceForBoundaryMm, double);
  /// Set maximum Hausdorff distance for the boundary voxels
  vtkSetMacro(MaximumHausdorffDistanceForBoundaryMm, double);

  /// Get average Hausdorff distance for the whole volume
  vtkGetMacro(AverageHausdorffDistanceForVolumeMm, double);
  /// Set average Hausdorff distance for the whole volume
  vtkSetMacro(AverageHausdorffDistanceForVolumeMm, double);

  /// Get average Hausdorff distance for the boundary voxels
  vtkGetMacro(AverageHausdorffDistanceForBoundaryMm, double);
  /// Set average Hausdorff distance for the boundary voxels
  vtkSetMacro(AverageHausdorffDistanceForBoundaryMm, double);

  /// Get 95% Hausdorff distance for whole volume
  vtkGetMacro(Percent95HausdorffDistanceForVolumeMm, double);
  /// Set 95% Hausdorff distance for whole volume
  vtkSetMacro(Percent95HausdorffDistanceForVolumeMm, double);

  /// Get 95% Hausdorff distance for the boundary voxels
  vtkGetMacro(Percent95HausdorffDistanceForBoundaryMm, double);
  /// Set 95% Hausdorff distance for the boundary voxels
  vtkSetMacro(Percent95HausdorffDistanceForBoundaryMm, double);

  /// Get/Set results Hausdorff valid flag
  vtkGetMacro(HausdorffResultsValid, bool);
  vtkSetMacro(HausdorffResultsValid, bool);
  vtkBooleanMacro(HausdorffResultsValid, bool);

protected:
  vtkMRMLContourComparisonNode();
  ~vtkMRMLContourComparisonNode();
  vtkMRMLContourComparisonNode(const vtkMRMLContourComparisonNode&);
  void operator=(const vtkMRMLContourComparisonNode&);

protected:
  /// Result dice coefficient
  float DiceCoefficient;

  /// Percentage of true positive labelmap voxels, i.e. positive reference voxels that are positive in compare image
  double TruePositivesPercent;

  /// Percentage of true negative labelmap voxels, i.e. negative reference voxels that are negative in compare image
  double TrueNegativesPercent;

  /// Percentage of false positive labelmap voxels, i.e. negative reference voxels that are positive in compare image
  double FalsePositivesPercent;

  /// Percentage of false negative labelmap voxels, i.e. positive reference voxels that are negative in compare image
  double FalseNegativesPercent;

  /// Location of the center of mass of the reference structure
  double ReferenceCenter[3];

  /// Location of the center of mass of the compare structure
  double CompareCenter[3];

  /// Volume of the reference structure
  double ReferenceVolumeCc;

  /// Volume of the compare structure
  double CompareVolumeCc;

  /// Flag telling whether the Dice similarity results are valid
  bool DiceResultsValid;

  /// Maximum Hausdorff distance for the whole volume
  double MaximumHausdorffDistanceForVolumeMm;

  /// Maximum Hausdorff distance for the boundary voxels
  double MaximumHausdorffDistanceForBoundaryMm;

  /// Average Hausdorff distance for the whole volume
  double AverageHausdorffDistanceForVolumeMm;

  /// Average Hausdorff distance for the boundary voxels
  double AverageHausdorffDistanceForBoundaryMm;

  /// 95% Hausdorff distance for the whole volume
  double Percent95HausdorffDistanceForVolumeMm;

  /// 95% Hausdorff distance for the boundary voxels
  double Percent95HausdorffDistanceForBoundaryMm;

  /// Flag telling whether the Hausdorff results are valid
  bool HausdorffResultsValid;
};

#endif
