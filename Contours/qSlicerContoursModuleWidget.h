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
  /// Determine whether the active representation is the 
  /// same in the selected contours and returns it if it is
  vtkMRMLContourNode::ContourRepresentationType getRepresentationTypeOfSelectedContours();

  /// Determine if the selected modules contain a certain representation
  /// /return True if every selected node has the given type of representation, false otherwise
  bool selectedContoursContainRepresentation(vtkMRMLContourNode::ContourRepresentationType representationType);

  /// Determines if a reference volume node is needed
  /// (if the desired representation is labelmap and there is
  /// at least one contour not having that representation)
  bool isReferenceVolumeNeeded();

  /// Set state according to currently selected representation type
  void onActiveRepresentationComboboxSelectionChanged(int index);

protected slots:
  void contourNodeChanged(vtkMRMLNode*);
  void referenceVolumeNodeChanged(vtkMRMLNode* node);
  void activeRepresentationComboboxSelectionChanged(int index);
  void downsamplingFactorChanged(double value);
  void targetReductionFactorChanged(double value);

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
