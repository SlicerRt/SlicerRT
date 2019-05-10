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

#ifndef __vtkMRMLRoomsEyeViewNode_h
#define __vtkMRMLRoomsEyeViewNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerRoomsEyeViewModuleLogicExport.h"

class vtkMRMLLinearTransformNode;
class vtkMRMLSegmentationNode;
class vtkMRMLRTBeamNode;

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class VTK_SLICER_ROOMSEYEVIEW_LOGIC_EXPORT vtkMRMLRoomsEyeViewNode : public vtkMRMLNode
{
public:
  static vtkMRMLRoomsEyeViewNode *New();
  vtkTypeMacro(vtkMRMLRoomsEyeViewNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Create instance of a GAD node. 
  vtkMRMLNode* CreateNodeInstance() override;

  /// Set node attributes from name/value pairs 
  void ReadXMLAttributes( const char** atts) override;

  /// Write this node's information to a MRML file in XML format. 
  void WriteXML(ostream& of, int indent) override;

  /// Copy the node's attributes to this object 
  void Copy(vtkMRMLNode *node) override;

  /// Get unique node XML tag name (like Volume, Model) 
  const char* GetNodeTagName() override { return "RoomsEyeView"; };

public:
  vtkMRMLLinearTransformNode* GetGantryToFixedReferenceTransformNode();
  void SetAndObserveGantryToFixedReferenceTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetCollimatorToFixedReferenceIsocenterTransformNode();
  void SetAndObserveCollimatorToFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetFixedReferenceIsocenterToCollimatorRotatedTransformNode();
  void SetAndObserveFixedReferenceIsocenterToCollimatorRotatedTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetCollimatorToGantryTransformNode();
  void SetAndObserveCollimatorToGantryTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetAdditionalCollimatorDevicesToCollimatorTransformNode();
  void SetAndObserveAdditionalCollimatorDevicesToCollimatorTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode();
  void SetAndObserveLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode(); 
  void SetAndObserveLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetLeftImagingPanelRotatedToGantryTransformNode();
  void SetAndObserveLeftImagingPanelRotatedToGantryTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetLeftImagingPanelTranslationTransformNode();
  void SetAndObserveLeftImagingPanelTranslationTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode();
  void SetAndObserveRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode();
  void SetAndObserveRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetRightImagingPanelRotatedToGantryTransformNode();
  void SetAndObserveRightImagingPanelRotatedToGantryTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetRightImagingPanelTranslationTransformNode();
  void SetAndObserveRightImagingPanelTranslationTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetPatientSupportToFixedReferenceTransformNode();
  void SetAndObservePatientSupportToFixedReferenceTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetPatientSupportScaledByTableTopVerticalMovementTransformNode();
  void SetAndObservePatientSupportScaledByTableTopVerticalMovementTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetPatientSupportPositiveVerticalTranslationTransformNode();
  void SetAndObservePatientSupportPositiveVerticalTranslationTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetTableTopToTableTopEccentricRotationTransformNode();
  void SetAndObservePatientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode(vtkMRMLLinearTransformNode* node);
  
  vtkMRMLLinearTransformNode* GetTableTopEccentricRotationToPatientSupportTransformNode();
  void SetAndObserveTableTopToTableTopEccentricRotationTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetPatientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode();
  void SetAndObserveTableTopEccentricRotationToPatientSupportTransformNode(vtkMRMLLinearTransformNode* node);
 
  /// Get beam node
  vtkMRMLRTBeamNode* GetBeamNode();
  /// Set and observe beam node
  void SetAndObserveBeamNode(vtkMRMLRTBeamNode* node);

  /// Get patient body segmentation node
  vtkMRMLSegmentationNode* GetPatientBodySegmentationNode();
  /// Set and observe patient body segmentation node
  void SetAndObservePatientBodySegmentationNode(vtkMRMLSegmentationNode* node);

  /// Get patient body segment ID
  vtkGetStringMacro(PatientBodySegmentID);
  /// Set patient body segment ID
  vtkSetStringMacro(PatientBodySegmentID);

  /// Get treatment machine type name
  vtkGetStringMacro(TreatmentMachineType);
  /// Set treatment machine type name
  vtkSetStringMacro(TreatmentMachineType);

  vtkGetMacro(CollisionDetectionEnabled, bool);
  vtkSetMacro(CollisionDetectionEnabled, bool);
  vtkBooleanMacro(CollisionDetectionEnabled, bool);

  vtkGetMacro(GantryRotationAngle, double);
  vtkSetMacro(GantryRotationAngle, double);

  vtkGetMacro(CollimatorRotationAngle, double);
  vtkSetMacro(CollimatorRotationAngle, double);

  vtkGetMacro(ImagingPanelMovement, double);
  vtkSetMacro(ImagingPanelMovement, double);

  vtkGetMacro(PatientSupportRotationAngle, double);
  vtkSetMacro(PatientSupportRotationAngle, double);

  vtkGetMacro(VerticalTableTopDisplacement, double);
  vtkSetMacro(VerticalTableTopDisplacement, double);

  vtkGetMacro(LongitudinalTableTopDisplacement, double);
  vtkSetMacro(LongitudinalTableTopDisplacement, double);

  vtkGetMacro(LateralTableTopDisplacement, double);
  vtkSetMacro(LateralTableTopDisplacement, double);

  vtkGetMacro(AdditionalModelLongitudinalDisplacement, double);
  vtkSetMacro(AdditionalModelLongitudinalDisplacement, double);

  vtkGetMacro(AdditionalModelVerticalDisplacement, double);
  vtkSetMacro(AdditionalModelVerticalDisplacement, double);

  vtkGetMacro(AdditionalModelLateralDisplacement, double);
  vtkSetMacro(AdditionalModelLateralDisplacement, double);

  vtkGetMacro(ApplicatorHolderVisibility, int);
  vtkSetMacro(ApplicatorHolderVisibility, int);

  vtkGetMacro(ElectronApplicatorVisibility, int);
  vtkSetMacro(ElectronApplicatorVisibility, int);

protected:
  vtkMRMLRoomsEyeViewNode();
  ~vtkMRMLRoomsEyeViewNode();
  vtkMRMLRoomsEyeViewNode(const vtkMRMLRoomsEyeViewNode&);
  void operator=(const vtkMRMLRoomsEyeViewNode&);

protected:
  /// Patient body segment ID in selected segmentation node
  char* PatientBodySegmentID;

  /// Name of treatment machine used (must match folder name where the models can be found)
  char* TreatmentMachineType;

  bool CollisionDetectionEnabled;

  /// TODO:
  double GantryRotationAngle;
  double CollimatorRotationAngle;
  double ImagingPanelMovement;
  double PatientSupportRotationAngle;
  double VerticalTableTopDisplacement;
  double LongitudinalTableTopDisplacement;
  double LateralTableTopDisplacement;
  double AdditionalModelVerticalDisplacement;
  double AdditionalModelLongitudinalDisplacement;
  double AdditionalModelLateralDisplacement;
  int ApplicatorHolderVisibility;
  int ElectronApplicatorVisibility;
};

#endif
