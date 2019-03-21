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

#ifndef __qSlicerDosxyzNrc3dDoseFileReaderPlugin
#define __qSlicerDosxyzNrc3dDoseFileReaderPlugin

// SlicerQt includes
#include "qSlicerFileReader.h"

class qSlicerDosxyzNrc3dDoseFileReaderPluginPrivate;
class vtkSlicerDosxyzNrc3dDoseFileReaderLogic;

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DosxyzNrc3dDoseFileReader
class qSlicerDosxyzNrc3dDoseFileReaderPlugin
  : public qSlicerFileReader
{
  Q_OBJECT

public:
  typedef qSlicerFileReader Superclass;
  qSlicerDosxyzNrc3dDoseFileReaderPlugin(QObject* parent = nullptr);
  qSlicerDosxyzNrc3dDoseFileReaderPlugin(vtkSlicerDosxyzNrc3dDoseFileReaderLogic* logic, QObject* parent = nullptr);
  virtual ~qSlicerDosxyzNrc3dDoseFileReaderPlugin();

  vtkSlicerDosxyzNrc3dDoseFileReaderLogic* logic()const;
  void setLogic(vtkSlicerDosxyzNrc3dDoseFileReaderLogic* logic);

  virtual QString description()const;
  virtual IOFileType fileType()const;
  virtual QStringList extensions()const;
  virtual qSlicerIOOptions* options()const;
  virtual bool load(const IOProperties& properties);

protected:
  QScopedPointer<qSlicerDosxyzNrc3dDoseFileReaderPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDosxyzNrc3dDoseFileReaderPlugin);
  Q_DISABLE_COPY(qSlicerDosxyzNrc3dDoseFileReaderPlugin);
};

#endif
