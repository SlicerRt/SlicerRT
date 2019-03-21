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
#ifdef Slicer_HAVE_QT5
  Q_PLUGIN_METADATA(IID "org.slicer.modules.loadable.qSlicerLoadableModule/1.0");
#endif
  Q_INTERFACES(qSlicerLoadableModule);

public:
  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerPlanarImageModule(QObject *parent=nullptr);
  ~qSlicerPlanarImageModule() override;

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
 
protected:
  /// Initialize the module. Register the volumes reader/writer
  void setup() override;

  /// Create and return the widget representation associated to this module
  qSlicerAbstractModuleRepresentation * createWidgetRepresentation() override;

  /// Create and return the logic associated to this module
  vtkMRMLAbstractLogic* createLogic() override;

protected:
  QScopedPointer<qSlicerPlanarImageModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlanarImageModule);
  Q_DISABLE_COPY(qSlicerPlanarImageModule);

};

#endif
