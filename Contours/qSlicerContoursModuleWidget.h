/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

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

#ifndef __qSlicerContoursModuleWidget_h
#define __qSlicerContoursModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerContoursModuleExport.h"

// SlicerRtCommon includes
#include "vtkMRMLContourNode.h"

class qSlicerContoursModuleWidgetPrivate;
class vtkMRMLContourNode;
class vtkMRMLNode;
class vtkMRMLScalarVolumeNode;
class Ui_qSlicerContoursModule;

/// \ingroup Slicer_QtModules_Contours
class Q_SLICER_QTMODULES_CONTOURS_EXPORT qSlicerContoursModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerContoursModuleWidget(QWidget *parent=0);
  virtual ~qSlicerContoursModuleWidget();

  virtual void enter();

public:
  // Functions to enable automatic testing
  void testInit();
  vtkMRMLScalarVolumeNode* testGetCurrentReferenceVolumeNode();
  vtkMRMLContourNode* testGetCurrentContourNode();
  void testSetReferenceVolumeNode(vtkMRMLScalarVolumeNode* node);
  void testSetContourNode(vtkMRMLContourNode* node);
  void testSetTargetRepresentationType(vtkMRMLContourNode::ContourRepresentationType targetRepresentationType);
  Ui_qSlicerContoursModule* testGetDPointer();

protected:
  /// Get reference volume node ID from the selected contours.
  /// \param referenceVolumeNodeId Output parameter for the reference volume node ID (is empty if they are not the same, or all have been created from labelmap)
  /// \return True if the reference volume node IDs are the same or all of them have been created from labelmap, false otherwise
  bool getReferenceVolumeNodeIdOfSelectedContours(QString &referenceVolumeNodeId);

  /// Get oversampling factor from the selected contours.
  /// \param oversamplingFactor Output parameter for the oversampling factor (is -1 if they are not the same)
  /// \return True if the oversampling factors are the same, false otherwise
  bool getOversamplingFactorOfSelectedContours(double &oversamplingFactor);

  /// Get target reduction factor from the selected contours.
  /// \param targetReductionFactor Output parameter for the target reduction factor (is -1 if they are not the same)
  /// \return True if the target reduction factors are the same, false otherwise
  bool getTargetReductionFactorOfSelectedContours(double &targetReductionFactor);

  /// Determines if all selected contours have been created from labelmap
  bool haveSelectedContoursBeenCreatedFromLabelmap();

  /// Set state according to change active representation widget group changes
  /// This function makes sure that all the conditions are good for the proposed representation change
  /// The Apply button is enabled only if those conditions are met. Otherwise, the appropriate messages are displayed 
  void updateWidgetsInChangeActiveRepresentationGroup();

  /// Get oversampling factor based on the value set on the slider
  /// (The factor is two on the power set on the slider, e.g. -1 -> 2^-1 = 0.5)
  double getOversamplingFactor();

  /// Show conversion parameters for selected target representation
  void showConversionParameterControlsForTargetRepresentation(vtkMRMLContourNode::ContourRepresentationType targetRepresentationType);

  /// Utility function for getting the target representation type from the combobox
  vtkMRMLContourNode::ContourRepresentationType getTargetRepresentationType();

  /// Determines if a suitable source representation is ready for the current conversion for a certain contour
  /// \return true if source representation is present and valid
  ///         (e.g. invalid if intermediate indexed labelmap has changed conversion parameters)
  bool isSuitableSourceAvailableForConversion(vtkMRMLContourNode* contourNode);
  /// Determines if a suitable source representation is ready for the current conversion for all selected nodes
  bool isSuitableSourceAvailableForConversionForAllSelectedContours();

  /// Determines if conversion parameters have changed for a certain contour.
  /// \return False if the current conversion needs a representation that have changed conversion parameters, true otherwise
  /// \sa haveConversionParametersChangedForIndexedLabelmap, \sa haveConversionParametersChangedForClosedSurfaceModel
  bool haveConversionParametersChanged(vtkMRMLContourNode* contourNode);
  /// Determines if conversion parameters have changed for indexed labelmap conversion
  /// \return True if contour has indexed labelmap representation, but indexed labelmap conversion parameters have changed
  /// \sa haveConversionParametersChanged
  bool haveConversionParametersChangedForIndexedLabelmap(vtkMRMLContourNode* contourNode);
  /// Determines if conversion parameters have changed for closed surface model conversion
  /// \return True if contour has closed surface model representation, but closed surface model conversion parameters have changed
  /// \sa haveConversionParametersChanged
  bool haveConversionParametersChangedForClosedSurfaceModel(vtkMRMLContourNode* contourNode);
  /// Determines if conversion parameters have changed in any of the selected contours
  /// \sa haveConversionParametersChanged
  bool haveConversionParametersChangedInAnySelectedContour();

  /// Determines if reference volume selection is valid for conversion for all contours in the selected contours
  /// \return False if any contour needs reference volume, but is not set, true otherwise
  bool isReferenceVolumeValidForAllContours();

  /// Perform conversion to indexed labelmap representation in a certain contour.
  /// NOTE: The active representation is not changed, only the parameters are set and the conversion is done.
  /// Used by \sa applyChangeRepresentationClicked
  bool convertToIndexedLabelmap(vtkMRMLContourNode* contourNode);
  /// Perform conversion to closed surface model representation in a certain contour
  /// NOTE: The active representation is not changed, only the parameters are set and the conversion is done.
  /// Used by \sa applyChangeRepresentationClicked
  bool convertToClosedSurfaceModel(vtkMRMLContourNode* contourNode);

  /// Utility function that returns the corresponding slider widget value for an oversampling factor (the slider is exponential)
  int getOversamplingFactorSliderValueFromOversamplingFactor(double oversamplingFactor);

public slots:
  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void contourNodeChanged(vtkMRMLNode*);
  void referenceVolumeNodeChanged(vtkMRMLNode* node);
  void activeRepresentationComboboxSelectionChanged(int index);
  void oversamplingFactorChanged(int value);
  void targetReductionFactorPercentChanged(double value);

  /// Perform conversions if needed and sets target representation for all selected contours
  /// This function should only be called if all conditions are good to perform the representation change
  /// Checking the conditions is done in \sa updateWidgetsFromChangeActiveRepresentationGroup
  void applyChangeRepresentationClicked();

protected:
  QScopedPointer<qSlicerContoursModuleWidgetPrivate> d_ptr;

  virtual void setup();
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerContoursModuleWidget);
  Q_DISABLE_COPY(qSlicerContoursModuleWidget);
};
#endif
