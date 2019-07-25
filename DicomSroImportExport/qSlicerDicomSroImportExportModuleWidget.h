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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __qSlicerDicomSroImportExportModuleWidget_h
#define __qSlicerDicomSroImportExportModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDicomSroImportExportModuleExport.h"

class qSlicerDicomSroImportExportModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_DicomSroImportExport
class Q_SLICER_QTMODULES_DICOMSROIMPORTEXPORT_EXPORT qSlicerDicomSroImportExportModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDicomSroImportExportModuleWidget(QWidget *parent=nullptr);
  ~qSlicerDicomSroImportExportModuleWidget() override;

public slots:
  void LoadDicomSro();

protected:
  QScopedPointer<qSlicerDicomSroImportExportModuleWidgetPrivate> d_ptr;
  
  void setup() override;

private:
  Q_DECLARE_PRIVATE(qSlicerDicomSroImportExportModuleWidget);
  Q_DISABLE_COPY(qSlicerDicomSroImportExportModuleWidget);
};

#endif
