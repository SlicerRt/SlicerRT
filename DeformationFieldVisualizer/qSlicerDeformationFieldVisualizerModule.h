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
  
  This file was originally developed by Franklin King, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerDeformationFieldVisualizerModule_h
#define __qSlicerDeformationFieldVisualizerModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerDeformationFieldVisualizerModuleExport.h"

class qSlicerDeformationFieldVisualizerModulePrivate;

/// \ingroup Slicer_QtModules_DeformationFieldVisualizer
class Q_SLICER_QTMODULES_DEFORMATIONFIELDVISUALIZER_EXPORT
qSlicerDeformationFieldVisualizerModule
  : public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerDeformationFieldVisualizerModule(QObject *parent=0);
  virtual ~qSlicerDeformationFieldVisualizerModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);

  virtual QString helpText()const;
  virtual QString acknowledgementText()const;
  virtual QStringList contributors()const;

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
  QScopedPointer<qSlicerDeformationFieldVisualizerModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDeformationFieldVisualizerModule);
  Q_DISABLE_COPY(qSlicerDeformationFieldVisualizerModule);

};

#endif
