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

==============================================================================*/

#ifndef __qSlicerDicomRtExportModuleWidget_h
#define __qSlicerDicomRtExportModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDicomRtExportModuleExport.h"

class qSlicerDicomRtExportModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_DicomRtExport
class Q_SLICER_QTMODULES_DICOMRTEXPORT_EXPORT qSlicerDicomRtExportModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDicomRtExportModuleWidget(QWidget *parent=0);
  virtual ~qSlicerDicomRtExportModuleWidget();

public slots:
  void SaveDicomRt();

protected:
  QScopedPointer<qSlicerDicomRtExportModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerDicomRtExportModuleWidget);
  Q_DISABLE_COPY(qSlicerDicomRtExportModuleWidget);
};

#endif
