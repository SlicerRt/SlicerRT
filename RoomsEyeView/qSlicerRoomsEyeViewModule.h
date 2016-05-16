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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/
 
#ifndef __qSlicerRoomsEyeViewModule_h
#define __qSlicerRoomsEyeViewModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerRoomsEyeViewModuleExport.h"

class qSlicerRoomsEyeViewModulePrivate;

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class Q_SLICER_QTMODULES_RoomsEyeView_EXPORT qSlicerRoomsEyeViewModule :
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:
  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerRoomsEyeViewModule(QObject *parent=0);
  virtual ~qSlicerRoomsEyeViewModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);
  
  /// Help to use the module
  virtual QString helpText()const;

  /// Return acknowledgements
  virtual QString acknowledgementText()const;

  /// Return the authors of the module
  virtual QStringList  contributors()const;

  /// Return the categories for the module
  virtual QStringList categories()const;

  /// List dependencies
  virtual QStringList dependencies()const;

  /// Make this module hidden
  virtual bool isHidden()const { return false; };
 
protected:
  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerRoomsEyeViewModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerRoomsEyeViewModule);
  Q_DISABLE_COPY(qSlicerRoomsEyeViewModule);

};

#endif
