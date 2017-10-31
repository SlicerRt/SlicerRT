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

#ifndef __qSlicerDosxyzNrc3dDoseFileReaderModule_h
#define __qSlicerDosxyzNrc3dDoseFileReaderModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

// DosxyzNrc3dDoseFileReader includes
#include "qSlicerDosxyzNrc3dDoseFileReaderModuleExport.h"

class qSlicerDosxyzNrc3dDoseFileReaderModulePrivate;

/// \ingroup SlicerRt_QtModules_DosxyzNrc3dDoseFileReader
class Q_SLICER_DOSXYZNRC3DDOSEFILEREADER_EXPORT qSlicerDosxyzNrc3dDoseFileReaderModule:
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  qSlicerDosxyzNrc3dDoseFileReaderModule(QObject *parent=0);
  virtual ~qSlicerDosxyzNrc3dDoseFileReaderModule();

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
  QScopedPointer<qSlicerDosxyzNrc3dDoseFileReaderModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDosxyzNrc3dDoseFileReaderModule);
  Q_DISABLE_COPY(qSlicerDosxyzNrc3dDoseFileReaderModule);
};

#endif
