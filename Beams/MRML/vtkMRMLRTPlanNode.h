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
#include <vtkMRML.h>
#include <vtkMRMLDisplayableNode.h>
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
class VTK_SLICER_BEAMS_MODULE_MRML_EXPORT vtkMRMLRTPlanNode : public vtkMRMLDisplayableNode
{
public:
  enum DoseEngineType
  {
    Plastimatch = 0,
    PMH = 1,
    Matlab = 2
  };

public:
  static vtkMRMLRTPlanNode *New();
  vtkTypeMacro(vtkMRMLRTPlanNode,vtkMRMLDisplayableNode);
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
  virtual const char* GetNodeTagName() {return "RTPlan";};

  /// Handles events registered in the observer manager
  /// - Invalidates (deletes) all non-active representations when the active is modified
  /// - Follows parent transform changes
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

public:
  /// RT Plan Reference volume
  vtkMRMLScalarVolumeNode* GetRTPlanReferenceVolumeNode();
  void SetAndObserveRTPlanReferenceVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// RT Plan POIs (isocenters, weight points)
  vtkMRMLMarkupsFiducialNode* GetMarkupsFiducialNode();
  void SetAndObserveMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode* node);
  vtkMRMLMarkupsFiducialNode* CreateMarkupsFiducialNode();

  /// RT Plan Segmentation (structure set)
  vtkMRMLSegmentationNode* GetRTPlanSegmentationNode();
  void SetAndObserveRTPlanSegmentationNode(vtkMRMLSegmentationNode* node);

  /// RT Plan Dose volume node
  vtkMRMLScalarVolumeNode* GetRTPlanDoseVolumeNode();
  void SetAndObserveRTPlanDoseVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// 
  void SetRTPlanDoseEngine(DoseEngineType doseEngineType) {this->RTPlanDoseEngine = doseEngineType;}
  DoseEngineType GetRTPlanDoseEngine() {return this->RTPlanDoseEngine;}

  ///
  void SetRTPlanDoseGrid(double* doseGrid) {this->RTPlanDoseGrid[0] = doseGrid[0]; this->RTPlanDoseGrid[1] = doseGrid[1]; this->RTPlanDoseGrid[2] = doseGrid[2];}
  double* GetRTPlanDoseGrid() {return this->RTPlanDoseGrid;}

  ///
  void AddRTBeamNode(vtkMRMLRTBeamNode *);

  ///
  void RemoveRTBeamNode(vtkMRMLRTBeamNode *);

  ///
  void GetRTBeamNodes(vtkCollection *);
  void GetRTBeamNodes(std::vector<vtkMRMLRTBeamNode*>& beams);

  /// Search for a beam of a given name.  Return NULL if beam not found
  /// Note: beam names *are not* unique within a plan
  vtkMRMLRTBeamNode* GetRTBeamNode(const std::string& beamName);

  /// Search for a beam with given beam number.  Return NULL if beam not found
  /// Note: beam numbers *are* unique within a plan
  vtkMRMLRTBeamNode* GetRTBeamNode(int beamNumber);

  /// Set RTPlanName
  vtkSetStringMacro(RTPlanName);
  vtkGetStringMacro(RTPlanName);

  /// Get/Set rx dose
  vtkGetMacro(RxDose, double);
  vtkSetMacro(RxDose, double);

protected:
  vtkMRMLRTPlanNode();
  ~vtkMRMLRTPlanNode();
  vtkMRMLRTPlanNode(const vtkMRMLRTPlanNode&);
  void operator=(const vtkMRMLRTPlanNode&);

protected:
  char* RTPlanName;
  int NextBeamNumber;
  DoseEngineType RTPlanDoseEngine;
  double RxDose;
  double RTPlanDoseGrid[3];
};

#endif // __vtkMRMLRTPlanNode_h
