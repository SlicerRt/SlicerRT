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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __qSlicerDicomRtImportExportModuleWidget_h
#define __qSlicerDicomRtImportExportModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDicomRtImportExportModuleExport.h"

class qSlicerDicomRtImportExportModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_QtModules_DicomRtImport
class Q_SLICER_QTMODULES_DICOMRTIMPORTEXPORT_EXPORT qSlicerDicomRtImportExportModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDicomRtImportExportModuleWidget(QWidget *parent=0);
  virtual ~qSlicerDicomRtImportExportModuleWidget();

protected:
  QScopedPointer<qSlicerDicomRtImportExportModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerDicomRtImportExportModuleWidget);
  Q_DISABLE_COPY(qSlicerDicomRtImportExportModuleWidget);
};

#endif
