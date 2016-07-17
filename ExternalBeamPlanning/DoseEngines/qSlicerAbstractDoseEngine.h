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

#ifndef __qSlicerAbstractDoseEngine_h
#define __qSlicerAbstractDoseEngine_h

#include "qSlicerExternalBeamPlanningDoseEnginesExport.h"

// Qt includes
#include "QObject"

class vtkMRMLScalarVolumeNode;
class vtkMRMLRTBeamNode;
class vtkMRMLNode;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract dose calculation algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class for specific dose engine plugins
class Q_SLICER_EXTERNALBEAMPLANNING_DOSE_ENGINES_EXPORT qSlicerAbstractDoseEngine : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString name READ name)

public:
  /// Maximum Gray value for visualization window/level of the newly created per-beam dose volumes
  static double DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM;

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerAbstractDoseEngine(QObject* parent=NULL);
  /// Destructor
  virtual ~qSlicerAbstractDoseEngine();

public:
  /// Perform dose calculation for a single beam
  /// \param Beam node for which the dose is calculated
  /// \return Error message. Empty string on success
  QString calculateDose(vtkMRMLRTBeamNode* beamNode);

  /// Get result per-beam dose volume for given beam
  vtkMRMLScalarVolumeNode* getResultDoseForBeam(vtkMRMLRTBeamNode* beamNode);

  /// Remove intermediate nodes created by the dose engine for a certain beam
  void removeIntermediateResults(vtkMRMLRTBeamNode* beamNode);

  /// Get dose engine name
  QString name() const;

// Beam parameter functions
public:
  /// Get beam parameter from beam node
  Q_INVOKABLE QString parameter(vtkMRMLRTBeamNode* beamNode, QString name);

  /// Convenience function to get integer parameter
  Q_INVOKABLE int integerParameter(vtkMRMLRTBeamNode* beamNode, QString name);

  /// Convenience function to get double parameter
  Q_INVOKABLE double doubleParameter(vtkMRMLRTBeamNode* beamNode, QString name);

  /// Set beam parameter in beam node. This function is called by both convenience functions.
  /// \param name Parameter name string
  /// \param value Parameter value string
  Q_INVOKABLE void setParameter(vtkMRMLRTBeamNode* beamNode, QString name, QString value);

  /// Convenience function to set integer parameter
  /// \param name Parameter name string
  /// \param value Parameter value integer
  Q_INVOKABLE void setParameter(vtkMRMLRTBeamNode* beamNode, QString name, int value);

  /// Convenience function to set double parameter
  /// \param name Parameter name string
  /// \param value Parameter value double
  Q_INVOKABLE void setParameter(vtkMRMLRTBeamNode* beamNode, QString name, double value);

protected:
  /// Calculate dose for a single beam. Called by \sa CalculateDose that performs actions generic
  /// to any dose engine before and after calculation.
  /// This is the method that needs to be implemented in each engine.
  /// 
  /// \param beamNode Beam for which the dose is calculated. Each beam has a parent plan from which the
  ///   plan-specific parameters are got
  /// \param resultDoseVolumeNode Output volume node for the result dose. It is created by \sa CalculateDose
  virtual QString calculateDoseUsingEngine(
    vtkMRMLRTBeamNode* beamNode,
    vtkMRMLScalarVolumeNode* resultDoseVolumeNode ) = 0;

  /// Add intermediate results to beam. Doing so allows easily cleaning up the intermediate results
  /// \param result MRML node containing the intermediate result to add
  /// \param beamNode Beam to add the intermediate result to
  void addIntermediateResult(vtkMRMLNode* result, vtkMRMLRTBeamNode* beamNode);

  /// Add result per-beam dose volume to beam
  /// \param resultDose Dose volume to add to beam as result
  /// \param beamNode Beam node to add dose as result to
  /// \param replace Remove referenced dose volume if already exists. True by default
  void addResultDose(vtkMRMLScalarVolumeNode* resultDose, vtkMRMLRTBeamNode* beamNode, bool replace=true);

protected:
  /// Name of the engine. Must be set in dose engine constructor
  QString m_Name;

private:
  Q_DISABLE_COPY(qSlicerAbstractDoseEngine);
};

#endif
