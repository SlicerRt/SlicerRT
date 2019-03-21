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
#ifdef Slicer_HAVE_QT5
  Q_PLUGIN_METADATA(IID "org.slicer.modules.loadable.qSlicerLoadableModule/1.0");
#endif
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  qSlicerDosxyzNrc3dDoseFileReaderModule(QObject *parent=nullptr);
  ~qSlicerDosxyzNrc3dDoseFileReaderModule() override;

  QString helpText()const override;
  QString acknowledgementText()const override;
  QStringList contributors()const override;
  QStringList categories()const override;
  qSlicerGetTitleMacro(QTMODULE_TITLE);

  // Makes the module hidden
  bool isHidden()const override { return true; };

protected:
  /// Initialize the module. Register the volumes reader/writer
  void setup() override;

  /// Create and return the widget representation associated to this module
  qSlicerAbstractModuleRepresentation* createWidgetRepresentation() override;

  /// Create and return the logic associated to this module
  vtkMRMLAbstractLogic* createLogic() override;

protected:
  QScopedPointer<qSlicerDosxyzNrc3dDoseFileReaderModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDosxyzNrc3dDoseFileReaderModule);
  Q_DISABLE_COPY(qSlicerDosxyzNrc3dDoseFileReaderModule);
};

#endif
