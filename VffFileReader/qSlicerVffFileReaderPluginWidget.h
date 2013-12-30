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

  This file was originally developed by Jennifer Andrea, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==============================================================================*/

#ifndef __qSlicerVffFileReaderPluginWidget_h
#define __qSlicerVffFileReaderPluginWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// VffFileReader includes
#include "qSlicerVffFileReaderModuleExport.h"

class qSlicerVffFileReaderPluginWidgetPrivate;

/// \ingroup SlicerRt_QtModules_VffFileReader
class Q_SLICER_VFFFILEREADER_EXPORT qSlicerVffFileReaderPluginWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerVffFileReaderPluginWidget(QWidget *parent=0);
  virtual ~qSlicerVffFileReaderPluginWidget();

protected:
  QScopedPointer<qSlicerVffFileReaderPluginWidgetPrivate> d_ptr;
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerVffFileReaderPluginWidget);
  Q_DISABLE_COPY(qSlicerVffFileReaderPluginWidget);
};

#endif
