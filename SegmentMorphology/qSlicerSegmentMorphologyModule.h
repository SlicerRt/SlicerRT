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

#ifndef __qSlicerSegmentMorphologyModule_h
#define __qSlicerSegmentMorphologyModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerSegmentMorphologyModuleExport.h"

class qSlicerSegmentMorphologyModulePrivate;

/// \ingroup SlicerRt_QtModules_SegmentMorphology
class Q_SLICER_QTMODULES_SEGMENTMORPHOLOGY_EXPORT qSlicerSegmentMorphologyModule :
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerSegmentMorphologyModule(QObject *parent=0);
  virtual ~qSlicerSegmentMorphologyModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);

  virtual QString helpText()const;
  virtual QString acknowledgementText()const;
  virtual QStringList contributors()const;

  /// Return a custom icon for the module
  virtual QIcon icon()const;

  virtual QStringList categories()const;
  virtual QStringList dependencies() const;

protected:

  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerSegmentMorphologyModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentMorphologyModule);
  Q_DISABLE_COPY(qSlicerSegmentMorphologyModule);

};

#endif
