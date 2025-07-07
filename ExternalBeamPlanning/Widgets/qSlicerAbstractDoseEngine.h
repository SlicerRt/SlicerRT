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

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// Qt includes
#include <QObject>
#include <QSet> 
#include <QStringList>

class qSlicerAbstractDoseEnginePrivate;
class vtkMRMLScalarVolumeNode;
class vtkMRMLRTBeamNode;
class vtkMRMLNode;
class qMRMLBeamParametersTabWidget;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract dose calculation algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class for specific dose engine plugins
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerAbstractDoseEngine : public QObject
{
  Q_OBJECT

  /// This property stores the name of the dose engine.
  /// Cannot be empty.
  /// \sa name(), \sa setName()
  Q_PROPERTY(QString name READ name WRITE setName)
  Q_PROPERTY(bool isInverse READ isInverse WRITE setIsInverse)
  Q_PROPERTY(bool canDoIonPlan READ canDoIonPlan WRITE setCanDoIonPlan)

public:
  /// Maximum Gray value for visualization window/level of the newly created per-beam dose volumes
  static double DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM;

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerAbstractDoseEngine(QObject* parent=nullptr);
  /// Destructor
  ~qSlicerAbstractDoseEngine() override;

  /// Get dose engine name
  virtual QString name()const;
  /// Set the name of the dose engine
  /// NOTE: name must be defined in constructor in C++ engines, this can only be used in python scripted ones
  virtual void setName(QString name);

  /// Inverse dose calculation capabilities
  bool isInverse()const;

  /// set inverse capabilities
  /// NOTE: this can only be used in python scripted ones
  virtual void setIsInverse(bool isInverse);

  /// Ion plan capabilities
  bool canDoIonPlan()const;

  virtual void setCanDoIonPlan(bool canDoIonPlan);

// Dose calculation related functions
public:
  /// Perform dose calculation for a single beam
  /// \param Beam node for which the dose is calculated
  /// \return Error message. Empty string on success
  QString calculateDose(vtkMRMLRTBeamNode* beamNode);

  /// Perform dose influence matrix calculation for a single beam
  /// \param Beam node for which the dose is calculated
  /// \return Error message. Empty string on success
  QString calculateDoseInfluenceMatrix(vtkMRMLRTBeamNode* beamNode);

  /// Get result per-beam dose volume for given beam
  vtkMRMLScalarVolumeNode* getResultDoseForBeam(vtkMRMLRTBeamNode* beamNode);

  /// Remove intermediate nodes created by the dose engine for a certain beam
  Q_INVOKABLE void removeIntermediateResults(vtkMRMLRTBeamNode* beamNode);

  /// Register beam parameters tab widget. All updates related to engine changes will be propagated to these widgets.
  Q_INVOKABLE static void registerBeamParametersTabWidget(qMRMLBeamParametersTabWidget* tabWidget);

// API functions to implement in the subclass
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

  /// Calculate dose influence matrix for a single beam. Called by \sa CalculateDoseInfluenceMatrix that performs actions generic
  /// to any dose engine before and after calculation.
  /// This is the method that needs to be implemented in an engine if dose influence matrix calculation is supported.
  ///
  /// \param beamNode Beam for which the dose is calculated. Each beam has a parent plan from which the
  ///   plan-specific parameters are got
  /// \param resultDoseVolumeNode Output volume node for the result dose. It is created by \sa CalculateDose
  virtual QString calculateDoseInfluenceMatrixUsingEngine(
      vtkMRMLRTBeamNode* beamNode);

  /// Define engine-specific beam parameters.
  /// This is the method that needs to be implemented in each engine.
  virtual void defineBeamParameters() = 0;

  /// Update beam parameters for engine according to ion plan flag.
  virtual void updateBeamParametersForIonPlan(bool ionPlanFlag);

// Dose calculation related functions (functions to call from the subclass).
// Public so that they can be called from python.
public:
  /// Add intermediate results to beam. Doing so allows easily cleaning up the intermediate results
  /// \param result MRML node containing the intermediate result to add
  /// \param beamNode Beam to add the intermediate result to
  Q_INVOKABLE void addIntermediateResult(vtkMRMLNode* result, vtkMRMLRTBeamNode* beamNode);

  /// Add result per-beam dose volume to beam
  /// \param resultDose Dose volume to add to beam as result
  /// \param beamNode Beam node to add dose as result to
  /// \param replace Remove referenced dose volume if already exists. True by default
  Q_INVOKABLE void addResultDose(vtkMRMLScalarVolumeNode* resultDose, vtkMRMLRTBeamNode* beamNode, bool replace=true);

// Beam parameter definition functions.
// Need to be called from the implemented \sa defineBeamParameters method.
// Public so that they can be called from python.
public:
  /// Add new floating point parameter to beam parameters widget as a spin box with text edit
  /// \param tabName Name of the tab in the beam parameters widget the parameter is added to
  /// \param parameterName Name of the beam parameter. This is prefixed with the dose engine name
  ///   and added to the beam node as attribute
  /// \param parameterLabel Text to be shown in the beam parameters widget in the left column
  /// \param tooltip Tooltip describing the beam parameter that pop up on the parameter widget
  /// \param minimumValue Minimum parameter value
  /// \param maximumValue Maximum parameter value
  /// \param defaultValue Default parameter value
  /// \param stepSize Size of a step in the parameter spinbox widget
  /// \param precision Number of decimals to be shown in the spinbox widget
  Q_INVOKABLE void addBeamParameterSpinBox(
    QString tabName, QString parameterName, QString parameterLabel,
    QString tooltip, double minimumValue, double maximumValue,
    double defaultValue, double stepSize, int precision );
  /// Add new floating point parameter to beam parameters widget as a slider
  /// \param tabName Name of the tab in the beam parameters widget the parameter is added to
  /// \param parameterName Name of the beam parameter. This is prefixed with the dose engine name
  ///   and added to the beam node as attribute
  /// \param parameterLabel Text to be shown in the beam parameters widget in the left column
  /// \param tooltip Tooltip describing the beam parameter that pop up on the parameter widget
  /// \param minimumValue Minimum parameter value
  /// \param maximumValue Maximum parameter value
  /// \param defaultValue Default parameter value
  /// \param stepSize Size of a step in the parameter slider widget
  /// \param precision Number of decimals to be shown on the sides of the slider
  Q_INVOKABLE void addBeamParameterSlider(
    QString tabName, QString parameterName, QString parameterLabel,
    QString tooltip, double minimumValue, double maximumValue,
    double defaultValue, double stepSize, int precision );

  /// Add new multiple choice beam parameter to beam parameters widget as a combo box
  /// \param tabName Name of the tab in the beam parameters widget the parameter is added to
  /// \param parameterName Name of the beam parameter. This is prefixed with the dose engine name
  ///   and added to the beam node as attribute
  /// \param parameterLabel Text to be shown in the beam parameters widget in the left column
  /// \param tooltip Tooltip describing the beam parameter that pop up on the parameter widget
  /// \param options List of options in the combobox. Their order defines the index for \sa defaultIndex
  ///   and the integer parameter accessed with \sa integerParameter for calculation
  /// \param defaultIndex Default selection in the combobox
  Q_INVOKABLE void addBeamParameterComboBox(
    QString tabName, QString parameterName, QString parameterLabel,
    QString tooltip, QStringList options, int defaultIndex );

  /// Update existing beam parameter combo box in the beam parameters widget
  /// \param tabName Name of the tab in the beam parameters widget the parameter is updated in
  /// \param parameterName Name of the beam parameter that is updated.
  /// \param parameterLabel Text to be shown in the beam parameters widget in the left column
  /// \param tooltip Tooltip describing the beam parameter that pop up on the parameter widget
  /// \param options List of options in the combobox. Their order defines the index for \sa defaultIndex
  /// \param defaultIndex Default selection in the combobox
  Q_INVOKABLE void updateBeamParameterComboBox(
    QString tabName, QString parameterName, QString parameterLabel,
    QString tooltip, QStringList options, int defaultIndex);

  /// Add new boolean type beam parameter to beam parameters widget as a check box
  /// \param tabName Name of the tab in the beam parameters widget the parameter is added to
  /// \param parameterName Name of the beam parameter. This is prefixed with the dose engine name
  ///   and added to the beam node as attribute
  /// \param parameterLabel Text to be shown in the beam parameters widget in the left column
  /// \param tooltip Tooltip describing the beam parameter that pop up on the parameter widget
  /// \param defaultValue Default parameter value (on/off)
  /// \param dependentParameterNames Names of parameters (full names including engine prefix) that
  ///   are to be enabled/disabled based on the checked state of the created checkbox
  Q_INVOKABLE void addBeamParameterCheckBox(
    QString tabName, QString parameterName, QString parameterLabel,
    QString tooltip, bool defaultValue, QStringList dependentParameterNames=QStringList() );

  /// Add new string type beam parameter to beam parameters widget as a line edit
  /// \param tabName Name of the tab in the beam parameters widget the parameter is added to
  /// \param parameterName Name of the beam parameter. This is prefixed with the dose engine name
  ///   and added to the beam node as attribute
  /// \param parameterLabel Text to be shown in the beam parameters widget in the left column
  /// \param tooltip Tooltip describing the beam parameter that pop up on the parameter widget
  /// \param defaultValue Default parameter value
  Q_INVOKABLE void addBeamParameterLineEdit(
    QString tabName, QString parameterName, QString parameterLabel,
    QString tooltip, QString defaultValue );

  /// Add all engine-specific beam parameters to given beam node (do not override value if parameter exists)
  Q_INVOKABLE void addBeamParameterAttributesToBeamNode(vtkMRMLRTBeamNode* beamNode);

// Beam parameter get/set functions
public:
  /// Get beam parameter from beam node
  Q_INVOKABLE QString parameter(vtkMRMLRTBeamNode* beamNode, QString parameterName);

  /// Convenience function to get integer parameter
  Q_INVOKABLE int integerParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName);

  /// Convenience function to get double parameter
  Q_INVOKABLE double doubleParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName);

  /// Convenience function to get boolean parameter
  Q_INVOKABLE bool booleanParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName);

  /// Set beam parameter in beam node. This function is called by both convenience functions.
  /// \param parameterName Parameter name string
  /// \param parameterValue Parameter value string
  Q_INVOKABLE void setParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName, QString parameterValue);

  /// Convenience function to set integer parameter
  /// \param name Parameter name string
  /// \param parameterValue Parameter value integer
  Q_INVOKABLE void setParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName, int parameterValue);

  /// Convenience function to set double parameter
  /// \param parameterName Parameter name string
  /// \param parameterValue Parameter value double
  Q_INVOKABLE void setParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName, double parameterValue);

  /// Convenience function to set boolean parameter
  /// \param parameterName Parameter name string
  /// \param parameterValue Parameter value boolean
  Q_INVOKABLE void setParameter(vtkMRMLRTBeamNode* beamNode, QString parameterName, bool parameterValue);

// Private helper functions
private:
  /// Add engine name prefix to the parameter name.
  /// This prefixed parameter name will be the attribute name for the beam parameter in the beam nodes.
  QString assembleEngineParameterName(QString parameterName);

  /// Convenience function for getting beam parameters tab widget from beams module widget.
  /// Kept as a convenience function to register this widget, which is always available in SlicerRT.
  /// \return Beam parameters tab widget
  static qMRMLBeamParametersTabWidget* beamParametersTabWidgetFromBeamsModule();

  /// Show/hide all engine-specific beam parameters in the beam parameters tab widget in Beams module
  void setBeamParametersVisible(bool visible);

protected:
  /// Name of the engine. Must be set in dose engine constructor
  QString m_Name;
  
  /// Is the dose engine inverse? (i.e. it is able to calculate a beamlet dose matrix for optimization)
  /// Is false by default, but can be set in the dose engine constructor
  bool m_IsInverse = false;

  /// Can the dose engine do ion plan? (i.e. it is able to calculate a dose for ion beams)
  /// Is false by default, but can be set in the dose engine constructor
  bool m_CanDoIonPlan = false;

  /// List of registered tab widgets. Static so that it is common to all engines.
  static QSet<qMRMLBeamParametersTabWidget*> m_BeamParametersTabWidgets;

protected:
  QScopedPointer<qSlicerAbstractDoseEnginePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerAbstractDoseEngine);
  Q_DISABLE_COPY(qSlicerAbstractDoseEngine);
  friend class qSlicerDoseEnginePluginHandler;
  friend class qSlicerDoseEngineLogic;
  friend class qSlicerExternalBeamPlanningModuleWidget;
};

#endif
