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

  This file was originally developed by Vinith Suriyakumar and Csaba Pinter,
  PerkLab, Queen's University and was supported through the Applied Cancer
  Research Unit program of Cancer Care Ontario with funds provided by the
  Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __vtkSlicerIECTransformLogic_h
#define __vtkSlicerIECTransformLogic_h

#include "vtkSlicerBeamsModuleLogicExport.h"

// VTK includes
#include <vtkObject.h>
#include <vtkTransform.h>
#include <vtkGeneralTransform.h>

class vtkMRMLRTBeamNode;

/// \ingroup SlicerRt_QtModules_Beams
/// \brief Logic representing the IEC standard coordinate systems and transforms.
///
/// The IEC standard describes coordinate systems and a transform hierarchy to
/// represent objects taking part in an external beam radiation therapy delivery in 3D space.
/// With this logic class it is possible to get a transform from any defined coordinate
/// system to another by simply inputting the coordinate systems. The logic can observe an
/// RT beam node to get the geometrical parameters defining the state of the objects involved.
/// Image describing these coordinate frames:
/// http://perk.cs.queensu.ca/sites/perkd7.cs.queensu.ca/files/Project/IEC_Transformations.PNG
///
class VTK_SLICER_BEAMS_LOGIC_EXPORT vtkSlicerIECTransformLogic : public vtkObject
{
public:
  enum CoordinateSystemIdentifier
  {
    FixedReference = 0,
    Gantry,
    CollimatorRotated,
    LeftImagingPanelTranslated,
    LeftImagingPanelRotated,
    LeftImagingPanelIsocenter,
    LeftImagingPanel,
    RightImagingPanelTranslated,
    RightImagingPanelRotated,
    RightImagingPanelIsocenter,
    RightImagingPanel,
    PatientSupport,
    PatientSupportScaledTranslated,
    PatientSupportScaled,
    PatientSupportPositiveVerticalTranslated,
    TableTopEccentricRotated,
    TableTop,
  };

public:
  static vtkSlicerIECTransformLogic *New();
  vtkTypeMacro(vtkSlicerIECTransformLogic, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Set and observe beam node. If a geometry-related parameter changes in the beam node, the transforms are updated
  void SetAndObserveBeamNode(vtkMRMLRTBeamNode* beamNode);

  /// Get transform from one coordinate frame to another
  /// \return Success flag (false on any error)
  bool GetTransformBetween(CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame, vtkMRMLRTBeamNode* beamNode, vtkGeneralTransform* outputTransform);

public:
  vtkGetObjectMacro(GantryToFixedReferenceTransform, vtkTransform);
  vtkGetObjectMacro(FixedReferenceIsocenterToCollimatorRotatedTransform, vtkTransform);
  vtkGetObjectMacro(LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform, vtkTransform);
  vtkGetObjectMacro(LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform, vtkTransform);
  vtkGetObjectMacro(LeftImagingPanelRotatedToGantryTransform, vtkTransform);
  vtkGetObjectMacro(LeftImagingPanelTranslationTransform, vtkTransform);
  vtkGetObjectMacro(RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform, vtkTransform);
  vtkGetObjectMacro(RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform, vtkTransform);
  vtkGetObjectMacro(RightImagingPanelRotatedToGantryTransform, vtkTransform);
  vtkGetObjectMacro(RightImagingPanelTranslationTransform, vtkTransform);
  vtkGetObjectMacro(PatientSupportToFixedReferenceTransform, vtkTransform);
  vtkGetObjectMacro(PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform, vtkTransform);
  vtkGetObjectMacro(PatientSupportScaledByTableTopVerticalMovementTransform, vtkTransform);
  vtkGetObjectMacro(PatientSupportPositiveVerticalTranslationTransform, vtkTransform);
  vtkGetObjectMacro(TableTopEccentricRotationToPatientSupportTransform, vtkTransform);
  vtkGetObjectMacro(TableTopToTableTopEccentricRotationTransform, vtkTransform);
  
protected:
  /// Update transforms according to beam node
  void UpdateTransformsFromBeamGeometry(vtkMRMLRTBeamNode* beamNode);

protected:
  vtkSetObjectMacro(GantryToFixedReferenceTransform, vtkTransform);
  vtkSetObjectMacro(FixedReferenceIsocenterToCollimatorRotatedTransform, vtkTransform);
  vtkSetObjectMacro(LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform, vtkTransform);
  vtkSetObjectMacro(LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform, vtkTransform);
  vtkSetObjectMacro(LeftImagingPanelRotatedToGantryTransform, vtkTransform);
  vtkSetObjectMacro(LeftImagingPanelTranslationTransform, vtkTransform);
  vtkSetObjectMacro(RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform, vtkTransform);
  vtkSetObjectMacro(RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform, vtkTransform);
  vtkSetObjectMacro(RightImagingPanelRotatedToGantryTransform, vtkTransform);
  vtkSetObjectMacro(RightImagingPanelTranslationTransform, vtkTransform);
  vtkSetObjectMacro(PatientSupportToFixedReferenceTransform, vtkTransform);
  vtkSetObjectMacro(PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform, vtkTransform);
  vtkSetObjectMacro(PatientSupportScaledByTableTopVerticalMovementTransform, vtkTransform);
  vtkSetObjectMacro(PatientSupportPositiveVerticalTranslationTransform, vtkTransform);
  vtkSetObjectMacro(TableTopEccentricRotationToPatientSupportTransform, vtkTransform);
  vtkSetObjectMacro(TableTopToTableTopEccentricRotationTransform, vtkTransform);

protected:
  // TODO: Determine if these are still necessary?
  /// Gantry to fixed reference transform
  vtkTransform* GantryToFixedReferenceTransform;
  vtkTransform* FixedReferenceIsocenterToCollimatorRotatedTransform;
  vtkTransform* LeftImagingPanelTranslationTransform;
  vtkTransform* LeftImagingPanelRotatedToGantryTransform;
  vtkTransform* LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform;
  vtkTransform* LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
  vtkTransform* RightImagingPanelTranslationTransform;
  vtkTransform* RightImagingPanelRotatedToGantryTransform;
  vtkTransform* RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform;
  vtkTransform* RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
  vtkTransform* PatientSupportToFixedReferenceTransform;
  vtkTransform* PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform;
  vtkTransform* PatientSupportScaledByTableTopVerticalMovementTransform;
  vtkTransform* PatientSupportPositiveVerticalTranslationTransform;
  vtkTransform* TableTopEccentricRotationToPatientSupportTransform;
  vtkTransform* TableTopToTableTopEccentricRotationTransform;

protected:
  vtkSlicerIECTransformLogic();
  virtual ~vtkSlicerIECTransformLogic();
};

#endif