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
class vtkMRMLNode;

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

protected:
  /// Get the active representation in the selected contours.
  /// Returns 'None' when they are not the same (in case of hierarchy)
  vtkMRMLContourNode::ContourRepresentationType getRepresentationTypeOfSelectedContours();

  /// Get reference volume node ID from the selected contours.
  /// \param referenceVolumeNodeId Output parameter for the reference volume node ID (is empty if they are not the same)
  /// \return True if the reference volume node IDs are the same, false otherwise
  bool getReferenceVolumeNodeIdOfSelectedContours(QString &referenceVolumeNodeId);

  /// Get oversampling factor from the selected contours.
  /// \param oversamplingFactor Output parameter for the oversampling factor (is -1 if they are not the same)
  /// \return True if the oversampling factors are the same, false otherwise
  bool getOversamplingFactorOfSelectedContours(double &oversamplingFactor);

  /// Get target reduction factor from the selected contours.
  /// \param targetReductionFactor Output parameter for the target reduction factor (is -1 if they are not the same)
  /// \return True if the target reduction factors are the same, false otherwise
  bool getTargetReductionFactorOfSelectedContours(double &targetReductionFactor);

  /// Determine if the selected modules contain a certain representation
  /// \param allMustContain If set to true, this function returns true only if all the selected
  ///        contours contain the representation. Otherwise it returns true even if only one contains it
  /// \return True if every selected node has the given type of representation, false otherwise
  bool selectedContoursContainRepresentation(vtkMRMLContourNode::ContourRepresentationType representationType, bool allMustContain=true);

  /// Determines if conversion is needed for a certain contour node
  /// \param contourNode The contour node to investigate
  /// \param representationToConvertTo The target representation
  /// \return True if the parameters set on the UI are different from the
  ///          parameters in the contour node, false otherwise
  bool isConversionNeeded(vtkMRMLContourNode* contourNode, vtkMRMLContourNode::ContourRepresentationType representationToConvertTo);

  /// Determines if conversion is needed for any of the selected contour nodes (\see isConversionNeeded)
  /// \param checkOnlyExistingRepresentations If true, only those contours will be examined
  ///        that already have the representation type (look for parameter changes in this case)
  bool isConversionNeededForSelectedNodes(vtkMRMLContourNode::ContourRepresentationType representationToConvertTo, bool checkOnlyExistingRepresentations=false);

  /// Set state according to change active representation widget group changes
  void updateWidgetsFromChangeActiveRepresentationGroup();

  /// Get oversampling factor based on the value set on the slider
  /// (The factor is two on the power set on the slider, e.g. -1 -> 2^-1 = 0.5)
  double getOversamplingFactor();

public slots:
  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void contourNodeChanged(vtkMRMLNode*);
  void referenceVolumeNodeChanged(vtkMRMLNode* node);
  void activeRepresentationComboboxSelectionChanged(int index);
  void oversamplingFactorChanged(int value);
  void targetReductionFactorPercentChanged(double value);

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
