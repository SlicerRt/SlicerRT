/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerVffFileReaderOptionsWidget_h
#define __qSlicerVffFileReaderOptionsWidget_h

// CTK includes
#include <ctkPimpl.h>

// SlicerQt includes
#include "qSlicerIOOptionsWidget.h"

// VffFileReader includes
#include "qSlicerVffFileReaderModuleExport.h"

class qSlicerVffFileReaderOptionsWidgetPrivate;

/// \ingroup Slicer_QtModules_VffFileReader
class Q_SLICER_VFFFILEREADER_EXPORT qSlicerVffFileReaderOptionsWidget :
  public qSlicerIOOptionsWidget
{
  Q_OBJECT
public:
  qSlicerVffFileReaderOptionsWidget(QWidget *parent=0);
  virtual ~qSlicerVffFileReaderOptionsWidget();

//public slots:
//  virtual void setFileName(const QString& fileName);
//  virtual void setFileNames(const QStringList& fileNames);

protected slots:
 //QScopedPointer<qSlicerVffFileReaderOptionsWidgetPrivate> d_ptr;
  void updateProperties();

private:
  //Q_DECLARE_PRIVATE_D(qGetPtrHelper(qSlicerIOOptions::d_ptr), qSlicerVffFileReaderOptionsWidget);
  Q_DECLARE_PRIVATE(qSlicerVffFileReaderOptionsWidget);
  Q_DISABLE_COPY(qSlicerVffFileReaderOptionsWidget);
};

#endif
