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

#ifndef __vtkMRMLDoseComparisonNode_h
#define __vtkMRMLDoseComparisonNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <vector>
#include <set>

#include "vtkSlicerDoseComparisonModuleLogicExport.h"

class VTK_SLICER_DOSECOMPARISON_LOGIC_EXPORT vtkMRMLDoseComparisonNode : public vtkMRMLNode
{
public:
  static vtkMRMLDoseComparisonNode *New();
  vtkTypeMacro(vtkMRMLDoseComparisonNode,vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "DoseComparison";};

public:
  /// Get/Set reference dose volume node ID
  vtkGetStringMacro(ReferenceDoseVolumeNodeId);
  vtkSetStringMacro(ReferenceDoseVolumeNodeId);

  /// Get/Set compare dose volume node ID
  vtkGetStringMacro(CompareDoseVolumeNodeId);
  vtkSetStringMacro(CompareDoseVolumeNodeId);

  /// Get/Set output gamma volume node ID
  vtkGetStringMacro(GammaVolumeNodeId);
  vtkSetStringMacro(GammaVolumeNodeId);

  /// Get/Set distance to agreement (DTA) tolerance, in mm
  vtkGetMacro(DtaDistanceToleranceMm, double);
  vtkSetMacro(DtaDistanceToleranceMm, double);

  /// Get/Set dose difference tolerance in percent
  vtkGetMacro(DoseDifferenceTolerancePercent, double);
  vtkSetMacro(DoseDifferenceTolerancePercent, double);

  /// Get/Set reference dose (prescription dose) in Gy
  vtkGetMacro(ReferenceDoseGy, double);
  vtkSetMacro(ReferenceDoseGy, double);

  /// Get/Set dose threshold for gamma analysis
  vtkGetMacro(AnalysisThresholdPercent, double);
  vtkSetMacro(AnalysisThresholdPercent, double);

  /// Get/Set maximum gamma
  vtkGetMacro(MaximumGamma, double);
  vtkSetMacro(MaximumGamma, double);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
 
protected:
  vtkMRMLDoseComparisonNode();
  ~vtkMRMLDoseComparisonNode();
  vtkMRMLDoseComparisonNode(const vtkMRMLDoseComparisonNode&);
  void operator=(const vtkMRMLDoseComparisonNode&);

protected:
  /// Selected reference dose volume MRML node object ID
  char* ReferenceDoseVolumeNodeId;

  /// Selected compare dose volume MRML node object ID
  char* CompareDoseVolumeNodeId;

  /// Selected output gamma volume MRML node object ID
  char* GammaVolumeNodeId;

  /// Distance to agreement (DTA) tolerance, in mm
  double DtaDistanceToleranceMm;

  /// Dose difference tolerance. If a reference dose (prescription dose) is specified,
  ///   the dose difference tolerance is treated as a percent of the reference dose.
  ///   Otherwise it is treated as a percent of the maximum dose in the reference volume.
  ///   To use a 3% dose tolerance, you would set this value to 0.03
  double DoseDifferenceTolerancePercent;

  /// Reference dose (prescription dose). This is used in dose comparison
  double ReferenceDoseGy;

  /// Dose threshold for gamma analysis. This is used to ignore voxels which have dose below
  ///   a certain value. For example, to consider only voxels which have dose greater than
  ///   10% of the prescription dose, set the analysis threshold to 0.10. The threshold is
  ///   applied to the reference image.
  double AnalysisThresholdPercent;

  /// Maximum gamma computed by the class. This is used to speed up computation. A typical value is 2 or 3.
  double MaximumGamma;
};

#endif
