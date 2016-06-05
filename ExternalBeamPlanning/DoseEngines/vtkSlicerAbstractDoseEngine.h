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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkSlicerAbstractDoseEngine
// .SECTION Description
// This class represents the abstract dose calculation algorithm that can be used in the
// External Beam Planning SlicerRT module as a base class for specific dose engine plugins

#ifndef __vtkSlicerAbstractDoseEngine_h
#define __vtkSlicerAbstractDoseEngine_h

#include "vtkSlicerExternalBeamPlanningDoseEnginesExport.h"

// VTK includes
#include "vtkObject.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLRTBeamNode;
class vtkMRMLNode;

/// \ingroup SlicerRt_ExternalBeamPlanning
/// \brief Abstract dose calculation algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class for specific dose engine plugins
class VTK_SLICER_EXTERNALBEAMPLANNING_DOSE_ENGINES_EXPORT vtkSlicerAbstractDoseEngine : public vtkObject
{
public:
  /// Maximum Gray value for visualization window/level of the newly created per-beam dose volumes
  static double DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM;

public:
  vtkTypeMacro(vtkSlicerAbstractDoseEngine, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create beam node of type the dose engine uses.
  /// Dose engines need to override this to return beam node of type they use.
  /// By default it returns a vtkMRMLRTBeamNode.
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkDataObject>::Take
  virtual vtkMRMLRTBeamNode* CreateBeamForEngine();

  /// Perform dose calculation for a single beam
  /// \param Beam node for which the dose is calculated
  /// \return Error message. Empty string on success
  std::string CalculateDose(vtkMRMLRTBeamNode* beamNode);

  /// Get result per-beam dose volume for given beam
  vtkMRMLScalarVolumeNode* GetResultDoseForBeam(vtkMRMLRTBeamNode* beamNode);

  /// Remove intermediate nodes created by the dose engine for a certain beam
  void RemoveIntermediateResults(vtkMRMLRTBeamNode* beamNode);

  /// Get dose engine name
  vtkGetStringMacro(Name);

protected:
  /// Calculate dose for a single beam. Called by \sa CalculateDose that performs actions generic
  /// to any dose engine before and after calculation.
  /// This is the method that needs to be implemented in each engine.
  /// 
  /// \param beamNode Beam for which the dose is calculated. Each beam has a parent plan from which the
  ///   plan-specific parameters are got
  /// \param resultDoseVolumeNode Output volume node for the result dose. It is created by \sa CalculateDose
  virtual std::string CalculateDoseUsingEngine(
    vtkMRMLRTBeamNode* beamNode,
    vtkMRMLScalarVolumeNode* resultDoseVolumeNode ) = 0;

  /// Add intermediate results to beam. Doing so allows easily cleaning up the intermediate results
  /// \param result MRML node containing the intermediate result to add
  /// \param beamNode Beam to add the intermediate result to
  void AddIntermediateResult(vtkMRMLNode* result, vtkMRMLRTBeamNode* beamNode);

  /// Add result per-beam dose volume to beam
  void AddResultDose(vtkMRMLScalarVolumeNode* resultDose, vtkMRMLRTBeamNode* beamNode);

protected:
  vtkSlicerAbstractDoseEngine();
  virtual ~vtkSlicerAbstractDoseEngine();

private:
  vtkSlicerAbstractDoseEngine(const vtkSlicerAbstractDoseEngine&); // Not implemented
  void operator=(const vtkSlicerAbstractDoseEngine&);         // Not implemented

protected:
  /// Name of the engine. Must be set in dose engine constructor
  char* Name;
};

#endif
