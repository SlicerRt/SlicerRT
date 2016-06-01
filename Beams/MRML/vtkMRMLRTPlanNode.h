/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __vtkMRMLRTPlanNode_h
#define __vtkMRMLRTPlanNode_h

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>

#include "vtkSlicerBeamsModuleMRMLExport.h"

class vtkCollection;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLRTBeamNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLSegmentationNode;
class vtkMRMLSubjectHierarchyNode;

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_MODULE_MRML_EXPORT vtkMRMLRTPlanNode : public vtkMRMLNode
{
public:
  enum DoseEngineType
  {
    Plastimatch = 0,
    PMH = 1,
    Matlab = 2
  };

  static const char* ISOCENTER_FIDUCIAL_NAME;
  static const int ISOCENTER_FIDUCIAL_INDEX;

  enum
  {
    /// Fired if isocenter position changes
    IsocenterModifiedEvent = 62300
  };

public:
  static vtkMRMLRTPlanNode *New();
  vtkTypeMacro(vtkMRMLRTPlanNode,vtkMRMLNode);
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
  virtual const char* GetNodeTagName() { return "RTPlan"; };

  /// Handles events registered in the observer manager
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

public:
  /// Generate new beam name from new beam name prefix and next beam number
  std::string GenerateNewBeamName();

  /// Check validity of current POIs markups fiducial node (whether it contains the default fiducials at the right indices)
  bool IsPoisMarkupsFiducialNodeValid();

  /// Get Subject Hierarchy node associated with this node. Create if missing
  vtkMRMLSubjectHierarchyNode* GetPlanSubjectHierarchyNode();

  /// Add given beam node to plan
  void AddBeam(vtkMRMLRTBeamNode* beam);
  /// Remove given beam node from plan
  void RemoveBeam(vtkMRMLRTBeamNode* beam);

  /// Get beam nodes belonging to this plan
  void GetBeams(vtkCollection* beams);
  /// Get beam nodes belonging to this plan
  void GetBeams(std::vector<vtkMRMLRTBeamNode*>& beams);
  /// Get number of beams
  int GetNumberOfBeams();

  /// Search for a beam of a given name.  Return NULL if beam not found
  /// Note: beam names *are not* unique within a plan
  vtkMRMLRTBeamNode* GetBeamByName(const std::string& beamName);

  /// Search for a beam with given beam number.  Return NULL if beam not found
  /// Note: beam numbers *are* unique within a plan
  vtkMRMLRTBeamNode* GetBeamByNumber(int beamNumber);

  /// Get plan reference volume
  vtkMRMLScalarVolumeNode* GetReferenceVolumeNode();
  /// Set plan reference volume
  void SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get plan POIs (isocenters, weight points, CT reference points, beam entry points)
  vtkMRMLMarkupsFiducialNode* GetPoisMarkupsFiducialNode();
  /// Set plan POIs (isocenters, weight points, CT reference points, beam entry points)
  void SetAndObservePoisMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode* node);
  /// Get isocenter position
  /// \return Success flag
  bool GetIsocenterPosition(double isocenter[3]);
  /// Set isocenter position
  /// \return Success flag
  bool SetIsocenterPosition(double isocenter[3]);

  /// Get plan segmentation (structure set)
  vtkMRMLSegmentationNode* GetSegmentationNode();
  /// Set plan segmentation (structure set)
  void SetAndObserveSegmentationNode(vtkMRMLSegmentationNode* node);

  /// Get output total dose volume node
  vtkMRMLScalarVolumeNode* GetOutputTotalDoseVolumeNode();
  /// Set output total dose volume node
  void SetAndObserveOutputTotalDoseVolumeNode(vtkMRMLScalarVolumeNode* node);

  ///TODO:
  vtkSetVector3Macro(ReferenceDosePoint, double);
  vtkGetVector3Macro(ReferenceDosePoint, double);

  void SetDoseEngine(DoseEngineType doseEngineType) { this->DoseEngine = doseEngineType; };
  DoseEngineType GetDoseEngine() { return this->DoseEngine; };

  ///TODO:
  vtkSetVector3Macro(DoseGrid, double);
  vtkGetVector3Macro(DoseGrid, double);

  /// Get prescription dose
  vtkGetMacro(RxDose, double);
  /// Set prescription dose
  vtkSetMacro(RxDose, double);

protected:
  /// Create default plan POIs markups node
  vtkMRMLMarkupsFiducialNode* CreateMarkupsFiducialNode();

protected:
  vtkMRMLRTPlanNode();
  ~vtkMRMLRTPlanNode();
  vtkMRMLRTPlanNode(const vtkMRMLRTPlanNode&);
  void operator=(const vtkMRMLRTPlanNode&);

protected:
  ///TODO:
  int NextBeamNumber;

  ///TODO:
  DoseEngineType DoseEngine;

  /// Prescription dose (Gy)
  double RxDose;

  ///TODO:
  double DoseGrid[3];

  ///TODO:
  double ReferenceDosePoint[3];
};

#endif // __vtkMRMLRTPlanNode_h
