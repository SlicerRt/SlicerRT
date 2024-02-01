/*==============================================================================

  Program: 3D Slicer

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

#ifndef __qSlicerScriptedDoseEngine_h
#define __qSlicerScriptedDoseEngine_h

// Dose engines includes
#include "qSlicerAbstractDoseEngine.h"

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// Forward Declare PyObject*
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif
class qSlicerScriptedDoseEnginePrivate;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Scripted abstract engine for implementing dose engines in python
///
/// This class provides an interface to engines implemented in python.
/// USAGE: Subclass AbstractScriptedDoseEngine in Python subfolder,
///   and register engine by creating this class and setting python source to implemented
///   engine subclass. One example is the MockScriptedDoseEngine.
///
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerScriptedDoseEngine
  : public qSlicerAbstractDoseEngine
{
  Q_OBJECT

public:
  typedef qSlicerAbstractDoseEngine Superclass;
  qSlicerScriptedDoseEngine(QObject* parent = nullptr);
  ~qSlicerScriptedDoseEngine() override;

  Q_INVOKABLE QString pythonSource()const;

  /// Set python source for the implemented engine
  /// \param newPythonSource Python file path
  Q_INVOKABLE bool setPythonSource(const QString newPythonSource);

  /// Convenience method allowing to retrieve the associated scripted instance
  Q_INVOKABLE PyObject* self() const;

  /// Set the name property value.
  /// \sa name
  void setName(QString name) override;

  /// Set the inverse planning capability bool
  /// \sa isInverse
  void setIsInverse(bool isInverse) override;

// Dose calculation related functions (API functions to call from the subclass)
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
    vtkMRMLScalarVolumeNode* resultDoseVolumeNode );

  /// Calculate dose influence matrix for a single beam. Called by \sa CalculateDoseInfluenceMatrix that performs actions generic
  /// to any dose engine before and after calculation.
  /// This is the method that needs to be implemented in an engine if dose influence matrix calculation is supported.
  ///
  /// \param beamNode Beam for which the dose is calculated. Each beam has a parent plan from which the
  ///   plan-specific parameters are got
  /// \param resultDoseVolumeNode Output volume node for the result dose. It is created by \sa CalculateDoseInfluenceMatrix
  virtual QString calculateDoseInfluenceMatrixUsingEngine(
      vtkMRMLRTBeamNode* beamNode);

  /// Define engine-specific beam parameters.
  /// This is the method that needs to be implemented in each engine.
  void defineBeamParameters() override;

protected:
  QScopedPointer<qSlicerScriptedDoseEnginePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerScriptedDoseEngine);
  Q_DISABLE_COPY(qSlicerScriptedDoseEngine);
};

#endif
