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

  This file was originally developed by Kevin Wang, RMP, Princess Margaret Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerDicomRtExportModuleWidget_h
#define __qSlicerDicomRtExportModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDicomRtExportModuleExport.h"

class qSlicerDicomRtExportModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_QtModules_DicomRtExport
class Q_SLICER_QTMODULES_DICOMRTEXPORT_EXPORT qSlicerDicomRtExportModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDicomRtExportModuleWidget(QWidget *parent=0);
  virtual ~qSlicerDicomRtExportModuleWidget();

public slots:

protected:
  QScopedPointer<qSlicerDicomRtExportModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

protected slots:
  /// TODO:
  void onSaveClicked();

private:
  Q_DECLARE_PRIVATE(qSlicerDicomRtExportModuleWidget);
  Q_DISABLE_COPY(qSlicerDicomRtExportModuleWidget);
};

#endif
