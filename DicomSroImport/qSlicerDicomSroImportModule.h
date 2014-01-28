/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

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

#ifndef __qSlicerDicomSroImportModule_h
#define __qSlicerDicomSroImportModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerDicomSroImportModuleExport.h"

class qSlicerDicomSroImportModulePrivate;

/// \ingroup SlicerRt_DicomSroImport
class Q_SLICER_QTMODULES_DICOMSROIMPORT_EXPORT qSlicerDicomSroImportModule :
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerDicomSroImportModule(QObject *parent=0);
  virtual ~qSlicerDicomSroImportModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);
  
  /// Help to use the module
  virtual QString helpText()const;

  /// Return acknowledgements
  virtual QString acknowledgementText()const;
  
  /// Return the authors of the module
  virtual QStringList  contributors()const; 
  
  /// Return a custom icon for the module
  virtual QIcon icon()const;

  /// Return the categories for the module
  virtual QStringList categories()const; 

  /// Make this module hidden
  virtual bool isHidden()const { return true; };

  /// List dependencies
  virtual QStringList dependencies()const;

protected:

  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerDicomSroImportModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDicomSroImportModule);
  Q_DISABLE_COPY(qSlicerDicomSroImportModule);

};

#endif
