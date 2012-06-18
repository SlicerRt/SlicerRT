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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerGammaDoseComparisonModule_h
#define __qSlicerGammaDoseComparisonModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerGammaDoseComparisonModuleExport.h"

class qSlicerGammaDoseComparisonModulePrivate;

/// \ingroup Slicer_QtModules_GammaDoseComparison
class Q_SLICER_QTMODULES_GAMMADOSECOMPARISON_EXPORT qSlicerGammaDoseComparisonModule :
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerGammaDoseComparisonModule(QObject *parent=0);
  virtual ~qSlicerGammaDoseComparisonModule();

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

protected:

  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerGammaDoseComparisonModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerGammaDoseComparisonModule);
  Q_DISABLE_COPY(qSlicerGammaDoseComparisonModule);

};

#endif
