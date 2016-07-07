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
  /// Get TODO:
  vtkMRMLLinearTransformNode* GetGantryToFixedReferenceTransformNode();
  vtkMRMLLinearTransformNode* GetCollimatorToGantryTransformNode();
  vtkMRMLLinearTransformNode* GetLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode();
  vtkMRMLLinearTransformNode* GetLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode(); 
  vtkMRMLLinearTransformNode* GetLeftImagingPanelRotatedToGantryTransformNode();
  vtkMRMLLinearTransformNode* GetLeftImagingPanelTranslationTransformNode();

  vtkMRMLLinearTransformNode* GetRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode();
  vtkMRMLLinearTransformNode* GetRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode();
  vtkMRMLLinearTransformNode* GetRightImagingPanelRotatedToGantryTransformNode();
  vtkMRMLLinearTransformNode* GetRightImagingPanelTranslationTransformNode();

  vtkMRMLLinearTransformNode* GetPatientSupportToFixedReferenceTransformNode();
  vtkMRMLLinearTransformNode* GetPatientSupportScaledByTableTopVerticalMovementTransformNode();
  vtkMRMLLinearTransformNode* GetPatientSupportPositiveVerticalTranslationTransformNode();

  vtkMRMLLinearTransformNode* GetTableTopToTableTopEccentricRotationTransformNode();
  vtkMRMLLinearTransformNode* GetTableTopEccentricRotationToPatientSupportTransformNode();
  vtkMRMLLinearTransformNode* GetPatientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode();

  /// Set TODO:
  void SetAndObserveGantryToFixedReferenceTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObserveCollimatorToGantryTransformNode(vtkMRMLLinearTransformNode* node);

  void SetAndObserveLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObserveLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObserveLeftImagingPanelRotatedToGantryTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObserveLeftImagingPanelTranslationTransformNode(vtkMRMLLinearTransformNode* node);

  void SetAndObserveRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObserveRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObserveRightImagingPanelRotatedToGantryTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObserveRightImagingPanelTranslationTransformNode(vtkMRMLLinearTransformNode* node);

  void SetAndObservePatientSupportToFixedReferenceTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObservePatientSupportScaledByTableTopVerticalMovementTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObservePatientSupportPositiveVerticalTranslationTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObservePatientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode(vtkMRMLLinearTransformNode* node);
  
  void SetAndObserveTableTopToTableTopEccentricRotationTransformNode(vtkMRMLLinearTransformNode* node);
  void SetAndObserveTableTopEccentricRotationToPatientSupportTransformNode(vtkMRMLLinearTransformNode* node);
 

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
