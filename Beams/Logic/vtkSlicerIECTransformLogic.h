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

class vtkGeneralTransform;
class vtkMRMLScene;
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
    RAS = 0,
    FixedReference,
    Gantry,
    Collimator,
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

  // Transform node names
  static const char* FIXEDREFERENCE_TO_RAS_TRANSFORM_NODE_NAME;
  static const char* GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME;
  static const char* COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME;

  static const char* COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME;
  static const char* FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME;

  static const char* LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME;
  static const char* LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME;
  static const char* LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME;
  static const char* LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME;

  static const char* RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME;
  static const char* RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME;
  static const char* RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME;
  static const char* RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME;

  static const char* PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME;
  static const char* PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME;
  static const char* PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME;
  static const char* PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME;
  static const char* TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME;
  static const char* TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME;

public:
  static vtkSlicerIECTransformLogic *New();
  vtkTypeMacro(vtkSlicerIECTransformLogic, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Set and observe beam node. If a geometry-related parameter changes in the beam node, the transforms are updated
  void UpdateTransformForBeam(vtkMRMLRTBeamNode* beamNode);

  /// Create or get transforms taking part in the IEC logic, and build the transform hierarchy
  void BuildIECTransformHierarchy(vtkMRMLScene* scene);

  /// Get transform from one coordinate frame to another
  /// \return Success flag (false on any error)
  bool GetTransformBetween(
    CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame,
    vtkMRMLScene* scene, vtkGeneralTransform* outputTransform );
  
protected:
  /// Update transforms according to beam node
  void UpdateTransformsFromBeam(vtkMRMLRTBeamNode* beamNode);

protected:
  vtkSlicerIECTransformLogic();
  virtual ~vtkSlicerIECTransformLogic();
};

#endif