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

#ifndef __qSlicerPatientHierarchyModuleWidget_h
#define __qSlicerPatientHierarchyModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerPatientHierarchyModuleExport.h"

// SlicerRtCommon includes
#include "vtkMRMLPatientHierarchyNode.h"

class qSlicerPatientHierarchyModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_PatientHierarchy
class Q_SLICER_QTMODULES_PATIENTHIERARCHY_EXPORT qSlicerPatientHierarchyModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerPatientHierarchyModuleWidget(QWidget *parent=0);
  virtual ~qSlicerPatientHierarchyModuleWidget();

  virtual void enter();

protected:
  /// Setup scene model for the tree widget
  void setSceneModel();

public slots:
  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  /// Show or hide MRML IDs
  void setMRMLIDsVisible(bool visible);

protected:
  QScopedPointer<qSlicerPatientHierarchyModuleWidgetPrivate> d_ptr;

  virtual void setup();
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerPatientHierarchyModuleWidget);
  Q_DISABLE_COPY(qSlicerPatientHierarchyModuleWidget);
};

#endif
