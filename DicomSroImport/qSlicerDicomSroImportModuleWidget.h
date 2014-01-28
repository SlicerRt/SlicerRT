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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __qSlicerDicomSroImportModuleWidget_h
#define __qSlicerDicomSroImportModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDicomSroImportModuleExport.h"

class qSlicerDicomSroImportModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_DicomSroImport
class Q_SLICER_QTMODULES_DICOMSROIMPORT_EXPORT qSlicerDicomSroImportModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDicomSroImportModuleWidget(QWidget *parent=0);
  virtual ~qSlicerDicomSroImportModuleWidget();

public slots:
  void LoadDicomSro();

protected:
  QScopedPointer<qSlicerDicomSroImportModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerDicomSroImportModuleWidget);
  Q_DISABLE_COPY(qSlicerDicomSroImportModuleWidget);
};

#endif
