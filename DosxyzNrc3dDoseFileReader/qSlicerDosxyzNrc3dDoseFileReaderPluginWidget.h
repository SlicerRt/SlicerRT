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

#ifndef __qSlicerDosxyzNrc3dDoseFileReaderPluginWidget_h
#define __qSlicerDosxyzNrc3dDoseFileReaderPluginWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// DosxyzNrc3dDoseFileReader includes
#include "qSlicerDosxyzNrc3dDoseFileReaderModuleExport.h"

class qSlicerDosxyzNrc3dDoseFileReaderPluginWidgetPrivate;

/// \ingroup SlicerRt_QtModules_DosxyzNrc3dDoseFileReader
class Q_SLICER_DOSXYZNRC3DDOSEFILEREADER_EXPORT qSlicerDosxyzNrc3dDoseFileReaderPluginWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDosxyzNrc3dDoseFileReaderPluginWidget(QWidget *parent=0);
  virtual ~qSlicerDosxyzNrc3dDoseFileReaderPluginWidget();

protected:
  QScopedPointer<qSlicerDosxyzNrc3dDoseFileReaderPluginWidgetPrivate> d_ptr;
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerDosxyzNrc3dDoseFileReaderPluginWidget);
  Q_DISABLE_COPY(qSlicerDosxyzNrc3dDoseFileReaderPluginWidget);
};

#endif
