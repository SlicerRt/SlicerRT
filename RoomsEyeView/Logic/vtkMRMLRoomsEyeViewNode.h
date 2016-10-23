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

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class VTK_SLICER_ROOMSEYEVIEW_LOGIC_EXPORT vtkMRMLRoomsEyeViewNode : public vtkMRMLNode
{
public:
  static vtkMRMLRoomsEyeViewNode *New();
  vtkTypeMacro(vtkMRMLRoomsEyeViewNode,vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "RoomsEyeView";};

public:
  /// TODO:
  vtkMRMLLinearTransformNode* GetGantryToFixedReferenceTransformNode();
  void SetAndObserveGantryToFixedReferenceTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetCollimatorToFixedReferenceIsocenterTransformNode();
  void SetAndObserveCollimatorToFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetFixedReferenceIsocenterToCollimatorRotatedTransformNode();
  void SetAndObserveFixedReferenceIsocenterToCollimatorRotatedTransformNode(vtkMRMLLinearTransformNode* node);

  vtkMRMLLinearTransformNode* GetCollimatorToGantryTransformNode();
  void SetAndObserveCollimatorToGantryTransformNode(vtkMRMLLinearTransformNode* node);

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
 
  /// Get patient body segmentation node
  vtkMRMLSegmentationNode* GetPatientBodySegmentationNode();
  /// Set and observe patient body segmentation node
  void SetAndObservePatientBodySegmentationNode(vtkMRMLSegmentationNode* node);

  /// Get patient body segment ID
  vtkGetStringMacro(PatientBodySegmentID);
  /// Set patient body segment ID
  vtkSetStringMacro(PatientBodySegmentID);

  //void SetAndObserveRightImagingPanel
  /// Get TODO:
  vtkGetMacro(GantryRotationAngle, double);
  vtkGetMacro(CollimatorRotationAngle, double);
  vtkGetMacro(ImagingPanelMovement, double);
  vtkGetMacro(PatientSupportRotationAngle, double);
  vtkGetMacro(VerticalTableTopDisplacement, double);
  vtkGetMacro(LongitudinalTableTopDisplacement, double);
  vtkGetMacro(LateralTableTopDisplacement, double);
  /// Set TODO:
  vtkSetMacro(GantryRotationAngle, double);
  vtkSetMacro(CollimatorRotationAngle, double);
  vtkSetMacro(ImagingPanelMovement, double);
  vtkSetMacro(PatientSupportRotationAngle, double);
  vtkSetMacro(VerticalTableTopDisplacement, double);
  vtkSetMacro(LongitudinalTableTopDisplacement, double);
  vtkSetMacro(LateralTableTopDisplacement, double);

protected:
  vtkMRMLRoomsEyeViewNode();
  ~vtkMRMLRoomsEyeViewNode();
  vtkMRMLRoomsEyeViewNode(const vtkMRMLRoomsEyeViewNode&);
  void operator=(const vtkMRMLRoomsEyeViewNode&);

protected:
  /// Patient body segment ID in selected segmentation node
  char* PatientBodySegmentID;

  /// TODO:
  double GantryRotationAngle;
  double CollimatorRotationAngle;
  double ImagingPanelMovement;
  double PatientSupportRotationAngle;
  double VerticalTableTopDisplacement;
  double LongitudinalTableTopDisplacement;
  double LateralTableTopDisplacement;
};

#endif
