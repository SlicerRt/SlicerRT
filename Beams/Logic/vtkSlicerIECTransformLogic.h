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

// STD includes
#include <map>
#include <vector>
#include <list>

// VTK includes
#include <vtkNew.h>
#include <vtkTransform.h>

class vtkGeneralTransform;

/// \ingroup SlicerRt_QtModules_Beams
/// \brief Logic representing the IEC standard coordinate systems and transforms.
///
/// The IEC standard describes coordinate systems and a transform hierarchy to
/// represent objects taking part in an external beam radiation therapy delivery in 3D space.
/// With this logic class it is possible to get a transform from any defined coordinate
/// system to another by simply inputting the coordinate systems. The logic can observe an
/// RT beam node to get the geometrical parameters defining the state of the objects involved.
/// Image describing these coordinate frames:
/// https://github.com/SlicerRt/SlicerRtDoc/blob/master/technical/IEC%2061217-2002_CoordinateSystemsDiagram_HiRes.png
///

/*
                          "IEC 61217:2011 Hierarchy"

                   -------------------("f")---------------------
                   |                    |                      |
        ---------("g")                ("i")                  ("s")
        |          |                    |                      |
      ("r")      ("b")                ("o")                  ("e")
                   |                                           |
                 ("w")                                       ("t")
                                                               |
                                                             ("p")

Legend:
  ("f") - Fixed reference system
  ("g") - GANTRY coordinate system
  ("b") - BEAM LIMITING DEVICE or DELINEATOR coordinate system
  ("w") - WEDGE FILTER coordinate system
  ("r") - X-RAY IMAGE RECEPTOR coordinate system
  ("s") - PATIENT SUPPORT coordinate system
  ("e") - Table top eccentric rotation coordinate system
  ("t") - Table top coordinate system
  ("p") - PATIENT coordinate system
  ("i") - Imager coordinate system
  ("o") - Focus coordinate system
*/
/*
 IEC Patient to DICOM Patient transformation:
     Counter clockwise rotation around X-axis, angle = -90 

                       1 0  0 
     Rotation Matrix = 0 0 -1
                       0 1  0

 IEC Patient to RAS Patient transformation:
     Counter clockwise rotation around X-axis, angle = -90 
     Clockwise rotation around Z-axis, angle = 180 

                       -1 0 0 
     Rotation Matrix =  0 0 1
                        0 1 0
*/

class VTK_SLICER_BEAMS_LOGIC_EXPORT vtkSlicerIECTransformLogic : public vtkObject
{
public:
  enum CoordinateSystemIdentifier
  {
    RAS = 0,
    FixedReference,
    Gantry,
    Collimator,
    LeftImagingPanel,
    RightImagingPanel,
    PatientSupportRotation, // Not part of the standard, but useful for visualization
    PatientSupport,
    TableTopEccentricRotation,
    TableTop,
    FlatPanel,
    WedgeFilter,
    Patient,
    Imager,
    Focus,
    LastIECCoordinateFrame // Last index used for adding more coordinate systems externally
  };
  typedef std::list< CoordinateSystemIdentifier > CoordinateSystemsList;

public:
  static vtkSlicerIECTransformLogic *New();
  vtkTypeMacro(vtkSlicerIECTransformLogic, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Update GantryToFixedReference transform based on gantry angle parameter
  void UpdateGantryToFixedReferenceTransform(double gantryRotationAngleDeg);
  /// Update CollimatorToGantry transform based on collimator angle parameter
  void UpdateCollimatorToGantryTransform(double collimatorRotationAngleDeg);
  /// Update PatientSupportRotrationToFixedReference transform based on patient support rotation parameter
  void UpdatePatientSupportRotationToFixedReferenceTransform(double patientSupportRotationAngleDeg);

  /// Get transform from one coordinate frame to another
  /// @param fromFrame - start transformation from frame
  /// @param toFrame - proceed transformation to frame
  /// @param outputTransform - General (linear) transform matrix fromFrame -> toFrame. Matrix is correct if return flag is true.  
  /// @param transformForBeam - calculate dynamic transformation for beam model or other models
  /// (e.g. transformation from Patient RAS frame to Collimation frame: RAS -> Patient -> TableTop -> Eccentric -> Patient Support -> Fixed reference -> Gantry -> Collimator)  //TODO: Deprecated
  /// \return Success flag (false on any error)
  bool GetTransformBetween(CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame,
    vtkGeneralTransform* outputTransform, bool transformForBeam=false);
  //TODO: See this transformForBeam part if still needed

  /// @brief Get coordinate system identifiers from root system down to frame system
  vtkTransform* GetElementaryTransformBetween(CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame);

public:
  //std::map<CoordinateSystemIdentifier, std::string> GetCoordinateSystemsMap()
  //{
  //  return CoordinateSystemsMap;
  //}

  /// Get name of transform node between two coordinate systems
  /// \return Transform node name between the specified coordinate frames.
  ///   Note: If IEC does not specify a transform between the given coordinate frames, then there will be no node with the returned name.
  std::string GetTransformNameBetween(CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame);

public:
  std::vector<std::pair<CoordinateSystemIdentifier, CoordinateSystemIdentifier>> GetIECTransforms()
  {
    return this->IECTransforms;
  }

  //std::map<CoordinateSystemIdentifier, std::list<CoordinateSystemIdentifier>> GetCoordinateSystemsHierarchy()
  //{
  //  return CoordinateSystemsHierarchy;
  //}

  //TODO: Use the frame names instead (nobody knows these variables outside)
  //vtkGetObjectMacro(FixedReferenceToRasTransform, vtkTransform);
  //vtkGetObjectMacro(GantryToFixedReferenceTransform, vtkTransform);
  //vtkGetObjectMacro(CollimatorToGantryTransform, vtkTransform);
  //vtkGetObjectMacro(WedgeFilterToCollimatorTransform, vtkTransform);
  //vtkGetObjectMacro(AdditionalCollimatorDevicesToCollimatorTransform, vtkTransform);
  //vtkGetObjectMacro(LeftImagingPanelToGantryTransform, vtkTransform);
  //vtkGetObjectMacro(RightImagingPanelToGantryTransform, vtkTransform);
  //vtkGetObjectMacro(PatientSupportRotationToFixedReferenceTransform, vtkTransform);
  //vtkGetObjectMacro(PatientSupportToPatientSupportRotationTransform, vtkTransform);
  //vtkGetObjectMacro(TableTopEccentricRotationToPatientSupportRotationTransform, vtkTransform);
  //vtkGetObjectMacro(TableTopToTableTopEccentricRotationTransform, vtkTransform);
  //vtkGetObjectMacro(PatientToTableTopTransform, vtkTransform);
  //vtkGetObjectMacro(RasToPatientTransform, vtkTransform);
  //vtkGetObjectMacro(FlatPanelToGantryTransform, vtkTransform);

protected:
  /// @brief Get coordinate system identifiers from frame system up to root system
  /// Root system = FixedReference system, see IEC 61217:2011 hierarchy
  bool GetPathToRoot(CoordinateSystemIdentifier frame, CoordinateSystemsList& path);

  /// @brief Get coordinate system identifiers from root system down to frame system
  /// Root system = FixedReference system, see IEC 61217:2011 hierarchy
  bool GetPathFromRoot(CoordinateSystemIdentifier frame, CoordinateSystemsList& path);

protected:
  /// Map from \sa CoordinateSystemIdentifier to coordinate system name. Used for getting transforms
  std::map<CoordinateSystemIdentifier, std::string> CoordinateSystemsMap;

  /// List of IEC transforms
  std::vector< std::pair<CoordinateSystemIdentifier, CoordinateSystemIdentifier> > IECTransforms;

  // TODO: for hierarchy use tree with nodes, something like graph
  /// Map of IEC coordinate systems hierarchy
  std::map< CoordinateSystemIdentifier, std::list< CoordinateSystemIdentifier > > CoordinateSystemsHierarchy;

protected:
  vtkNew<vtkTransform> FixedReferenceToRasTransform;
  vtkNew<vtkTransform> GantryToFixedReferenceTransform;
  vtkNew<vtkTransform> CollimatorToGantryTransform;
  vtkNew<vtkTransform> WedgeFilterToCollimatorTransform;
  vtkNew<vtkTransform> LeftImagingPanelToGantryTransform;
  vtkNew<vtkTransform> RightImagingPanelToGantryTransform;
  vtkNew<vtkTransform> PatientSupportRotationToFixedReferenceTransform;
  vtkNew<vtkTransform> PatientSupportToPatientSupportRotationTransform;
  vtkNew<vtkTransform> TableTopEccentricRotationToPatientSupportRotationTransform;
  vtkNew<vtkTransform> TableTopToTableTopEccentricRotationTransform;
  vtkNew<vtkTransform> PatientToTableTopTransform;
  vtkNew<vtkTransform> RasToPatientTransform;
  vtkNew<vtkTransform> FlatPanelToGantryTransform;

  vtkNew<vtkTransform> GantryToFixedReferenceConcatenatedTransform;
  vtkNew<vtkTransform> CollimatorToGantryConcatenatedTransform;
  vtkNew<vtkTransform> WedgeFilterToCollimatorConcatenatedTransform;
  vtkNew<vtkTransform> LeftImagingPanelToGantryConcatenatedTransform;
  vtkNew<vtkTransform> RightImagingPanelToGantryConcatenatedTransform;
  vtkNew<vtkTransform> FlatPanelToGantryConcatenatedTransform;
  vtkNew<vtkTransform> PatientSupportRotationToFixedReferenceConcatenatedTransform;
  vtkNew<vtkTransform> PatientSupportToPatientSupportRotationConcatenatedTransform;
  vtkNew<vtkTransform> TableTopEccentricRotationToPatientSupportRotationConcatenatedTransform;
  vtkNew<vtkTransform> TableTopToTableEccentricRotationConcatenatedTransform;
  vtkNew<vtkTransform> PatientToTableTopConcatenatedTransform;
  vtkNew<vtkTransform> RasToPatientConcatenatedTransform;

protected:
  vtkSlicerIECTransformLogic();
  ~vtkSlicerIECTransformLogic() override;

private:
  vtkSlicerIECTransformLogic(const vtkSlicerIECTransformLogic&) = delete;
  void operator=(const vtkSlicerIECTransformLogic&) = delete;

private:
  std::vector<vtkTransform*> ElementaryTransforms;
};

#endif
