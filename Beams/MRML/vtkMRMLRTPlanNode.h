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

// Beams includes
#include "vtkSlicerBeamsModuleMRMLExport.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>

// SegmentationCore includes
#include "vtkOrientedImageData.h"

class vtkCollection;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLRTBeamNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLSegmentationNode;

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_MODULE_MRML_EXPORT vtkMRMLRTPlanNode : public vtkMRMLNode
{
public:
  enum IsocenterSpecificationType
  {
    CenterOfTarget = 0,
    ArbitraryPoint
  };

  static const char* ISOCENTER_FIDUCIAL_NAME;
  static const int ISOCENTER_FIDUCIAL_INDEX;

  enum
  {
    /// Fired if isocenter position changes
    IsocenterModifiedEvent = 62300,
    /// Fired if beam is added
    BeamAdded,
    /// Fired if beam is removed
    BeamRemoved,
    /// Fired if dose engine is changed
    DoseEngineChanged
  };

public:
  static vtkMRMLRTPlanNode *New();
  vtkTypeMacro(vtkMRMLRTPlanNode,vtkMRMLNode);
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

  /// Get unique node XML tag name (like Volume, Model) 
  const char* GetNodeTagName() override { return "RTPlan"; };

  /// Handles events registered in the observer manager
  void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData) override;

public:
  /// Get subject hierarchy item associated with this node. Create if missing
  vtkIdType GetPlanSubjectHierarchyItemID();

  /// Get target segment as a labelmap oriented image data
  vtkSmartPointer<vtkOrientedImageData> GetTargetOrientedImageData();

  /// Add given beam node to plan
  void AddBeam(vtkMRMLRTBeamNode* beam);

  /// Add given proxy beam node to plan
  /// @param proxyBeam - Pointer to the selected beam of the beam sequence node,
  /// the selection process is controlled by a sequence browser node
  void AddProxyBeam(vtkMRMLRTBeamNode* proxyBeam);

  /// Remove given beam node from plan
  void RemoveBeam(vtkMRMLRTBeamNode* beam);
  /// Remove all beam nodes from plan
  void RemoveAllBeams();

  /// Generate new beam name from new beam name prefix and next beam number
  std::string GenerateNewBeamName();

  /// Check validity of current POIs markups fiducial node (whether it contains the default fiducials at the right indices)
  bool IsPoisMarkupsFiducialNodeValid();

// Isocenter parameters
public:
  /// Set isocenter specification
  /// If it's CenterOfTarget, then \sa SetIsocenterToTargetCenter is called to change isocenter to center of target
  /// \param isoSpec The new isocenter specification (CenterOfTarget or ArbitraryPoint)
  void SetIsocenterSpecification(vtkMRMLRTPlanNode::IsocenterSpecificationType isoSpec);
  /// Calculate center of current target and set isocenter to that point
  void SetIsocenterToTargetCenter();

  /// Get center of gravity of target segment, return true if successful
  /// or false if no target segment has been specified
  bool ComputeTargetVolumeCenter(double center[3]);

// Set/get functions
public:
  /// Get beam nodes belonging to this plan
  void GetBeams(vtkCollection* beams);
  /// Get beam nodes belonging to this plan
  void GetBeams(std::vector<vtkMRMLRTBeamNode*>& beams);
  /// Get number of beams
  int GetNumberOfBeams();

  /// Search for a beam of a given name.  Return nullptr if beam not found
  /// Note: beam names *are not* unique within a plan
  vtkMRMLRTBeamNode* GetBeamByName(const std::string& beamName);

  /// Search for a beam with given beam number.  Return nullptr if beam not found
  /// Note: beam numbers *are* unique within a plan
  vtkMRMLRTBeamNode* GetBeamByNumber(int beamNumber);

  /// Get plan reference volume
  vtkMRMLScalarVolumeNode* GetReferenceVolumeNode();
  /// Set plan reference volume
  void SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get plan POIs (isocenters, weight points, CT reference points, beam entry points)
  vtkMRMLMarkupsFiducialNode* GetPoisMarkupsFiducialNode();
  /// Create plan POIs (isocenters, weight points, CT reference points, beam entry points)
  vtkMRMLMarkupsFiducialNode* CreatePoisMarkupsFiducialNode();
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

  /// Get target segment ID
  vtkGetStringMacro(TargetSegmentID);
  /// Set target segment ID
  vtkSetStringMacro(TargetSegmentID);

  /// Get isocenter specification
  vtkGetMacro(IsocenterSpecification, vtkMRMLRTPlanNode::IsocenterSpecificationType);

  /// Get output total dose volume node
  vtkMRMLScalarVolumeNode* GetOutputTotalDoseVolumeNode();
  /// Set output total dose volume node
  void SetAndObserveOutputTotalDoseVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get dose engine name
  vtkGetStringMacro(DoseEngineName);
  /// Set dose engine name (and invoke setting the default beam parameters for the new engine)
  void SetDoseEngineName(const char* engineName);

  /// Get prescription dose
  vtkGetMacro(RxDose, double);
  /// Set prescription dose
  vtkSetMacro(RxDose, double);

  vtkSetVector3Macro(DoseGrid, double);
  vtkGetVector3Macro(DoseGrid, double);

  /// Get flag for ion plan
  vtkGetMacro( IonPlanFlag, bool);
  /// Set flag for ion plan
  vtkSetMacro( IonPlanFlag, bool);

protected:
  /// Create default plan POIs markups node
  vtkMRMLMarkupsFiducialNode* CreateMarkupsFiducialNode();

protected:
  vtkMRMLRTPlanNode();
  ~vtkMRMLRTPlanNode();
  vtkMRMLRTPlanNode(const vtkMRMLRTPlanNode&);
  void operator=(const vtkMRMLRTPlanNode&);

protected:
  /// Get next beam number (for Read, Write XML methods)
  vtkGetMacro(NextBeamNumber, int);
  /// Set next beam number (for Read, Write XML methods)
  vtkSetMacro(NextBeamNumber, int);

  /// Set isocenter specification (for ReadXMLAttributes method)
  void SetIsocenterSpecification(int isocenterSpec);

  /// Prescription dose (Gy)
  double RxDose;

  /// Target segment ID in target segmentation node
  char* TargetSegmentID;

  /// Isocenter specification determining whether it can be an arbitrary point or
  /// always calculated to be at the center of the target structure
  IsocenterSpecificationType IsocenterSpecification;

  /// Number of the next beam to be created. Incremented on each new beam addition
  int NextBeamNumber;

  /// Name of the selected dose engine
  char* DoseEngineName;

  ///TODO: Allow user to specify dose volume resolution different from reference volume
  /// (currently output dose volume has the same spacing as the reference anatomy)
  double DoseGrid[3];

  /// Flag, indicates that a plan node is an ion plan node
  bool IonPlanFlag;
};

#endif // __vtkMRMLRTPlanNode_h
