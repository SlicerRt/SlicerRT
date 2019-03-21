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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __qSlicerExternalBeamPlanningModule_h
#define __qSlicerExternalBeamPlanningModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerExternalBeamPlanningModuleExport.h"

class qSlicerExternalBeamPlanningModulePrivate;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class Q_SLICER_QTMODULES_EXTERNALBEAMPLANNING_EXPORT qSlicerExternalBeamPlanningModule :
  public qSlicerLoadableModule
{
  Q_OBJECT
#ifdef Slicer_HAVE_QT5
  Q_PLUGIN_METADATA(IID "org.slicer.modules.loadable.qSlicerLoadableModule/1.0");
#endif
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerExternalBeamPlanningModule(QObject *parent=nullptr);
  ~qSlicerExternalBeamPlanningModule() override;

  qSlicerGetTitleMacro(QTMODULE_TITLE);
  
  /// Help to use the module
  QString helpText()const override;

  /// Return acknowledgments
  QString acknowledgementText()const override;

  /// Return the authors of the module
  QStringList contributors()const override;

  /// Return a custom icon for the module
  QIcon icon()const override;

  /// Return the categories for the module
  QStringList categories()const override;

  /// List dependencies
  QStringList dependencies()const override;

  /// Define associated node types
  QStringList associatedNodeTypes()const override;

protected:
  /// Initialize the module. Register the volumes reader/writer
  void setup() override;

  /// Create and return the logic associated to this module
  vtkMRMLAbstractLogic* createLogic() override;

  /// Create and return the widget representation associated to this module
  qSlicerAbstractModuleRepresentation * createWidgetRepresentation() override;

protected:
  QScopedPointer<qSlicerExternalBeamPlanningModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerExternalBeamPlanningModule);
  Q_DISABLE_COPY(qSlicerExternalBeamPlanningModule);

};

#endif
