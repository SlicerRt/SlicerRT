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

#ifndef __qSlicerRTHierarchyModule_h
#define __qSlicerRTHierarchyModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerRTHierarchyModuleExport.h"

class qSlicerRTHierarchyModulePrivate;

/// \ingroup SlicerRt_QtModules_RtHierarchy
class Q_SLICER_QTMODULES_RTHIERARCHY_EXPORT qSlicerRTHierarchyModule : public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:
  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerRTHierarchyModule(QObject *parent=0);
  virtual ~qSlicerRTHierarchyModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);

  virtual QString helpText()const;
  virtual QString acknowledgementText()const;
  virtual QStringList contributors()const;

  virtual QStringList categories()const;
  virtual QStringList dependencies() const;

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
  QScopedPointer<qSlicerRTHierarchyModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerRTHierarchyModule);
  Q_DISABLE_COPY(qSlicerRTHierarchyModule);
};

#endif
