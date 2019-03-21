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

  This file was originally developed by Anna Ilina, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario.

==============================================================================*/

#ifndef __qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget_h
#define __qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget_h

// CTK includes
#include <ctkPimpl.h>

// SlicerQt includes
#include "qSlicerIOOptionsWidget.h"

// DosxyzNrc3dDoseFileReader includes
#include "qSlicerDosxyzNrc3dDoseFileReaderModuleExport.h"

class qSlicerDosxyzNrc3dDoseFileReaderOptionsWidgetPrivate;

/// \ingroup SlicerRt_QtModules_DosxyzNrc3dDoseFileReader
class Q_SLICER_DOSXYZNRC3DDOSEFILEREADER_EXPORT qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget :
  public qSlicerIOOptionsWidget
{
  Q_OBJECT
public:
  typedef qSlicerIOOptionsWidget Superclass;
  qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget(QWidget *parent=nullptr);
  virtual ~qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget();

protected slots:
  void updateProperties();

private:
  Q_DECLARE_PRIVATE_D(qGetPtrHelper(qSlicerIOOptions::d_ptr), qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget);
  Q_DISABLE_COPY(qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget);
};

#endif
