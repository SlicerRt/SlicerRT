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
class vtkMRMLColorTableNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLModelNode;
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

  static const char* OUTPUT_TOTAL_DOSE_VOLUME_REFERENCE_ROLE;

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
  /// - Invalidates (deletes) all non-active representations when the active is modified
  /// - Follows parent transform changes
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

public:
  /// Generate new beam name from new beam name prefix and next beam number
  std::string GenerateNewBeamName();

  /// Add given beam node to plan
  void AddBeam(vtkMRMLRTBeamNode* beam);
  /// Create a new beam based on another beam, and add it to the plan
  /// \return The new beam node that has been copied and added to the plan
  vtkMRMLRTBeamNode* CopyAndAddBeam(vtkMRMLRTBeamNode* copyFrom);

  /// Remove given beam node from plan
  void RemoveBeam(vtkMRMLRTBeamNode* beam);

  /// Get beam nodes belonging to this plan
  void GetBeams(vtkCollection* beams);
  /// Get beam nodes belonging to this plan
  void GetBeams(std::vector<vtkMRMLRTBeamNode*>& beams);

  /// Search for a beam of a given name.  Return NULL if beam not found
  /// Note: beam names *are not* unique within a plan
  vtkMRMLRTBeamNode* GetBeamByName(const std::string& beamName);

  /// Search for a beam with given beam number.  Return NULL if beam not found
  /// Note: beam numbers *are* unique within a plan
  vtkMRMLRTBeamNode* GetBeamByNumber(int beamNumber);

  /// Get RT Plan Reference volume
  vtkMRMLScalarVolumeNode* GetReferenceVolumeNode();
  /// Set RT Plan Reference volume
  void SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get RT Plan POIs (isocenters, weight points, CT reference points, beam entry points)
  vtkMRMLMarkupsFiducialNode* GetMarkupsFiducialNode();
  /// Set RT Plan POIs (isocenters, weight points, CT reference points, beam entry points)
  void SetAndObserveMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode* node);

  /// Get RT Plan Segmentation (structure set)
  vtkMRMLSegmentationNode* GetSegmentationNode();
  /// Set RT Plan Segmentation (structure set)
  void SetAndObserveSegmentationNode(vtkMRMLSegmentationNode* node);

  /// Get RT Plan Dose volume node
  vtkMRMLScalarVolumeNode* GetDoseVolumeNode();
  /// Set RT Plan Dose volume node
  void SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode* node);

  void SetDoseEngine(DoseEngineType doseEngineType) { this->DoseEngine = doseEngineType; };
  DoseEngineType GetDoseEngine() { return this->DoseEngine; };

  void SetDoseGrid(double* doseGrid) { this->DoseGrid[0] = doseGrid[0]; this->DoseGrid[1] = doseGrid[1]; this->DoseGrid[2] = doseGrid[2]; };
  double* GetDoseGrid() { return this->DoseGrid; };

  /// Get Subject Hierarchy node associated with this node. Create if missing
  vtkMRMLSubjectHierarchyNode* GetPlanSubjectHierarchyNode();

  /// Get prescription dose
  vtkGetMacro(RxDose, double);
  /// Set prescription dose
  vtkSetMacro(RxDose, double);

  /// Assemble reference volume node reference role for aperture volume node with given beam
  static std::string AssembleApertureVolumeReference(vtkMRMLNode* beamNode);
  /// Assemble reference volume node reference role for range compensator volume node with given beam
  static std::string AssembleRangeCompensatorVolumeReference(vtkMRMLNode* beamNode);
  /// Assemble reference volume node reference role for proton dose volume node with given beam
  static std::string AssembleProtonDoseVolumeReference(vtkMRMLNode* beamNode);

protected:
  /// Create default RT plan POIs markups node
  vtkMRMLMarkupsFiducialNode* CreateMarkupsFiducialNode();

protected:
  vtkMRMLRTPlanNode();
  ~vtkMRMLRTPlanNode();
  vtkMRMLRTPlanNode(const vtkMRMLRTPlanNode&);
  void operator=(const vtkMRMLRTPlanNode&);

protected:
  int NextBeamNumber;
  DoseEngineType DoseEngine;
  double RxDose;
  double DoseGrid[3];
};

#endif // __vtkMRMLRTPlanNode_h
