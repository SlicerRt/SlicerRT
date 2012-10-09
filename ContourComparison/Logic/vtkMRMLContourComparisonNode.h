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

// STD includes
#include <vector>
#include <set>

#include "vtkSlicerContourComparisonModuleLogicExport.h"

class VTK_SLICER_CONTOURCOMPARISON_LOGIC_EXPORT vtkMRMLContourComparisonNode : public vtkMRMLNode
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

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

public:
  /// Get reference contour labelmap volume node ID
  vtkGetStringMacro(ReferenceContourNodeId);
  /// Set and observe reference contour labelmap volume node ID
  void SetAndObserveReferenceContourNodeId(const char* id);

  /// Get compare contour labelmap volume node ID
  vtkGetStringMacro(CompareContourNodeId);
  /// Set and observe reference contour labelmap volume node ID
  void SetAndObserveCompareContourNodeId(const char* id);

  /// Get reference volume node ID
  vtkGetStringMacro(RasterizationReferenceVolumeNodeId);
  /// Set and observe reference volume node ID
  void SetAndObserveReferenceVolumeNodeId(const char* id);

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

  /// Get/Set results valid flag
  vtkGetMacro(ResultsValid, bool);
  vtkSetMacro(ResultsValid, bool);
  vtkBooleanMacro(ResultsValid, bool);

protected:
  /// Set reference contour labelmap volume node ID
  vtkSetStringMacro(ReferenceContourNodeId);

  /// Set compare contour labelmap volume node ID
  vtkSetStringMacro(CompareContourNodeId);

  /// Set reference volume node ID
  vtkSetStringMacro(RasterizationReferenceVolumeNodeId);

protected:
  vtkMRMLContourComparisonNode();
  ~vtkMRMLContourComparisonNode();
  vtkMRMLContourComparisonNode(const vtkMRMLContourComparisonNode&);
  void operator=(const vtkMRMLContourComparisonNode&);

protected:
  /// Selected reference contour labelmap volume MRML node object ID
  char* ReferenceContourNodeId;

  /// Selected compare contour labelmap volume MRML node object ID
  char* CompareContourNodeId;

  /// Selected reference volume MRML node object ID (needed for rasterization)
  char* RasterizationReferenceVolumeNodeId;

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

  /// Flag telling whether the results are valid
  bool ResultsValid;
};

#endif
