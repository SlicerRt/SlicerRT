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
 
#ifndef __qSlicerPlanarImageModule_h
#define __qSlicerPlanarImageModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerPlanarImageModuleExport.h"

class qSlicerPlanarImageModulePrivate;

/// \ingroup SlicerRt_QtModules_PlanarImage
class Q_SLICER_QTMODULES_PLANARIMAGE_EXPORT qSlicerPlanarImageModule :
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:
  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerPlanarImageModule(QObject *parent=0);
  virtual ~qSlicerPlanarImageModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);
  
  /// Help to use the module
  virtual QString helpText()const;

  /// Return acknowledgements
  virtual QString acknowledgementText()const;

  /// Return the authors of the module
  virtual QStringList contributors()const;

  /// Return the categories for the module
  virtual QStringList categories()const;

  /// Make this module hidden
  virtual bool isHidden()const { return true; };
 
protected:
  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerPlanarImageModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlanarImageModule);
  Q_DISABLE_COPY(qSlicerPlanarImageModule);

};

#endif
