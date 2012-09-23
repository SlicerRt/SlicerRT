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

class qSlicerContoursModuleWidgetPrivate;
class vtkMRMLNode;
class vtkMRMLContourNode;

/// \ingroup Slicer_QtModules_Contours
class Q_SLICER_QTMODULES_CONTOURS_EXPORT qSlicerContoursModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerContoursModuleWidget(QWidget *parent=0);
  virtual ~qSlicerContoursModuleWidget();

protected slots:
  void contourNodeChanged(vtkMRMLNode*);
  void activeRepresentationComboboxSelectionChanged(int index);

  void applyChangeRepresentationClicked();

protected:
  QScopedPointer<qSlicerContoursModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerContoursModuleWidget);
  Q_DISABLE_COPY(qSlicerContoursModuleWidget);
};

#endif
