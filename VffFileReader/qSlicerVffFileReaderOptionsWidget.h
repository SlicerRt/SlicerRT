/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

#ifndef __qSlicerVffFileReaderOptionsWidget_h
#define __qSlicerVffFileReaderOptionsWidget_h

// CTK includes
#include <ctkPimpl.h>

// SlicerQt includes
#include "qSlicerIOOptionsWidget.h"

// VffFileReader includes
#include "qSlicerVffFileReaderModuleExport.h"

class qSlicerVffFileReaderOptionsWidgetPrivate;

/// \ingroup SlicerRt_QtModules_VffFileReader
class Q_SLICER_VFFFILEREADER_EXPORT qSlicerVffFileReaderOptionsWidget :
  public qSlicerIOOptionsWidget
{
  Q_OBJECT
public:
  typedef qSlicerIOOptionsWidget Superclass;
  qSlicerVffFileReaderOptionsWidget(QWidget *parent=0);
  virtual ~qSlicerVffFileReaderOptionsWidget();


protected slots:
  void updateProperties();

private:
  Q_DECLARE_PRIVATE_D(qGetPtrHelper(qSlicerIOOptions::d_ptr), qSlicerVffFileReaderOptionsWidget);
  //Q_DECLARE_PRIVATE(qSlicerVffFileReaderOptionsWidget);
  Q_DISABLE_COPY(qSlicerVffFileReaderOptionsWidget);
};

#endif
