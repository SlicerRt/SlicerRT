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

#ifndef __qSlicerVffFileReaderModule_h
#define __qSlicerVffFileReaderModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

// VffFileReader includes
#include "qSlicerVffFileReaderModuleExport.h"

class qSlicerVffFileReaderModulePrivate;

/// \ingroup Slicer_QtModules_VffFileReader
class Q_SLICER_VFFFILEREADER_EXPORT qSlicerVffFileReaderModule:
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  qSlicerVffFileReaderModule(QObject *parent=0);
  virtual ~qSlicerVffFileReaderModule();

  virtual QString helpText()const;
  virtual QString acknowledgementText()const;
  virtual QStringList contributors()const;
  virtual QStringList categories()const;
  qSlicerGetTitleMacro(QTMODULE_TITLE);

  // Makes the module hidden
  virtual bool isHidden()const { return true; };

protected:
  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation* createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerVffFileReaderModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerVffFileReaderModule);
  Q_DISABLE_COPY(qSlicerVffFileReaderModule);
};

#endif
