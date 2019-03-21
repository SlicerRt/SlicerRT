/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __qSlicerDicomRtImportExportModule_h
#define __qSlicerDicomRtImportExportModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerDicomRtImportExportModuleExport.h"

class qSlicerDicomRtImportExportModulePrivate;

/// \ingroup SlicerRt_QtModules_DicomRtImport
class Q_SLICER_QTMODULES_DICOMRTIMPORTEXPORT_EXPORT qSlicerDicomRtImportExportModule :
  public qSlicerLoadableModule
{
  Q_OBJECT
#ifdef Slicer_HAVE_QT5
  Q_PLUGIN_METADATA(IID "org.slicer.modules.loadable.qSlicerLoadableModule/1.0");
#endif
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerDicomRtImportExportModule(QObject *parent=nullptr);
  ~qSlicerDicomRtImportExportModule() override;

  qSlicerGetTitleMacro(QTMODULE_TITLE);
  
  /// Help to use the module
  QString helpText()const override;

  /// Return acknowledgments
  QString acknowledgementText()const override;
  
  /// Return the authors of the module
  QStringList contributors()const override; 

  /// Return the categories for the module
  QStringList categories()const override; 

  /// Make this module hidden
  bool isHidden()const override { return true; };

  /// List dependencies
  QStringList dependencies()const override;

protected:

  /// Initialize the module. Register the volumes reader/writer
  void setup() override;

  /// Create and return the widget representation associated to this module
  qSlicerAbstractModuleRepresentation* createWidgetRepresentation() override;

  /// Create and return the logic associated to this module (will return only import logic!)
  vtkMRMLAbstractLogic* createLogic() override;

protected:
  QScopedPointer<qSlicerDicomRtImportExportModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDicomRtImportExportModule);
  Q_DISABLE_COPY(qSlicerDicomRtImportExportModule);
};

#endif
