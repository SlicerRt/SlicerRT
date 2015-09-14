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

#ifndef __vtkMRMLDoseComparisonNode_h
#define __vtkMRMLDoseComparisonNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <vector>
#include <set>

#include "vtkSlicerDoseComparisonModuleLogicExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLSegmentationNode;

/// \ingroup SlicerRt_QtModules_DoseComparison
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
  /// Get reference dose volume node
  vtkMRMLScalarVolumeNode* GetReferenceDoseVolumeNode();
  /// Set and observe reference dose volume node
  void SetAndObserveReferenceDoseVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get compare dose volume node
  vtkMRMLScalarVolumeNode* GetCompareDoseVolumeNode();
  /// Set and observe compare dose volume node
  void SetAndObserveCompareDoseVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get mask segmentation node
  vtkMRMLSegmentationNode* GetMaskSegmentationNode();
  /// Set and observe mask segmentation node
  void SetAndObserveMaskSegmentationNode(vtkMRMLSegmentationNode* node);

  /// Get output gamma volume node
  vtkMRMLScalarVolumeNode* GetGammaVolumeNode();
  /// Set and observe output gamma volume node
  void SetAndObserveGammaVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get mask segment ID
  vtkGetStringMacro(MaskSegmentID);
  /// Set mask segment ID
  vtkSetStringMacro(MaskSegmentID);

  /// Get distance to agreement (DTA) tolerance, in mm
  vtkGetMacro(DtaDistanceToleranceMm, double);
  /// Set distance to agreement (DTA) tolerance, in mm
  vtkSetMacro(DtaDistanceToleranceMm, double);

  /// Get dose difference tolerance in percent
  vtkGetMacro(DoseDifferenceTolerancePercent, double);
  /// Set dose difference tolerance in percent
  vtkSetMacro(DoseDifferenceTolerancePercent, double);

  /// Get reference dose (prescription dose) in Gy
  vtkGetMacro(ReferenceDoseGy, double);
  /// Set reference dose (prescription dose) in Gy
  vtkSetMacro(ReferenceDoseGy, double);

  /// Get dose threshold for gamma analysis
  vtkGetMacro(AnalysisThresholdPercent, double);
  /// Set dose threshold for gamma analysis
  vtkSetMacro(AnalysisThresholdPercent, double);

  /// Get maximum gamma
  vtkGetMacro(MaximumGamma, double);
  /// Set maximum gamma
  vtkSetMacro(MaximumGamma, double);

  /// Get use maximum dose
  vtkGetMacro(UseMaximumDose, bool);
  /// Set use maximum dose
  vtkSetMacro(UseMaximumDose, bool);
  /// Set use maximum dose
  vtkBooleanMacro(UseMaximumDose, bool);

  /// Get use linear interpolation flag
  vtkGetMacro(UseLinearInterpolation, bool);
  /// Set use linear interpolation flag
  vtkSetMacro(UseLinearInterpolation, bool);
  /// Set use linear interpolation flag
  vtkBooleanMacro(UseLinearInterpolation, bool);

  /// Get dose threshold on reference flag
  vtkGetMacro(DoseThresholdOnReferenceOnly, bool);
  /// Set dose threshold on reference flag
  vtkSetMacro(DoseThresholdOnReferenceOnly, bool);
  /// Set dose threshold on reference flag
  vtkBooleanMacro(DoseThresholdOnReferenceOnly, bool);

  /// Get valid flag
  vtkGetMacro(ResultsValid, bool);
  /// Set valid flag
  vtkSetMacro(ResultsValid, bool);
  /// Set valid flag
  vtkBooleanMacro(ResultsValid, bool);

  /// Get pass fraction
  vtkGetMacro(PassFractionPercent, double);
  /// Set pass fraction
  vtkSetMacro(PassFractionPercent, double);

  /// Get report string
  vtkGetStringMacro(ReportString);
  /// Set report string
  vtkSetStringMacro(ReportString);

protected:
  vtkMRMLDoseComparisonNode();
  ~vtkMRMLDoseComparisonNode();
  vtkMRMLDoseComparisonNode(const vtkMRMLDoseComparisonNode&);
  void operator=(const vtkMRMLDoseComparisonNode&);

protected:
  /// Mask segment ID in mask segmentation node
  char* MaskSegmentID;

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

  /// Flag indicating whether the Use maximum dose option is selected (else the Use custom value is selected)
  bool UseMaximumDose;

  /// Flag determining whether linear interpolation is used when resampling the compare dose volume to reference grid.
  /// Default value is true. On false value nearest neighbor is used.
  bool UseLinearInterpolation;
  
  /// Flag determining whether dose thresholding should be performed using only the reference image
  /// Default value is false, meaning that both images will be used
  bool DoseThresholdOnReferenceOnly;
  
  /// Percentage of voxels that passed (output)
  double PassFractionPercent;

  /// Flag indicating if the results are valid
  bool ResultsValid;

  /// Report string assembled by the gamma algorithm.
  /// It lists input parameters and some output, such as voxel counts and gamma histogram.
  char* ReportString;
};

#endif
