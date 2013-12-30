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

#ifndef __qSlicerVffFileReaderPlugin
#define __qSlicerVffFileReaderPlugin

// SlicerQt includes
#include "qSlicerFileReader.h"

class qSlicerVffFileReaderPluginPrivate;
class vtkSlicerVffFileReaderLogic;

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_VffFileReader
class qSlicerVffFileReaderPlugin
  : public qSlicerFileReader
{
  Q_OBJECT

public:
  typedef qSlicerFileReader Superclass;
  qSlicerVffFileReaderPlugin(QObject* parent = 0);
  qSlicerVffFileReaderPlugin(vtkSlicerVffFileReaderLogic* logic, QObject* parent = 0);
  virtual ~qSlicerVffFileReaderPlugin();

  vtkSlicerVffFileReaderLogic* logic()const;
  void setLogic(vtkSlicerVffFileReaderLogic* logic);

  virtual QString description()const;
  virtual IOFileType fileType()const;
  virtual QStringList extensions()const;
  virtual qSlicerIOOptions* options()const;
  virtual bool load(const IOProperties& properties);

protected:
  QScopedPointer<qSlicerVffFileReaderPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerVffFileReaderPlugin);
  Q_DISABLE_COPY(qSlicerVffFileReaderPlugin);
};

#endif
