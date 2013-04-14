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
