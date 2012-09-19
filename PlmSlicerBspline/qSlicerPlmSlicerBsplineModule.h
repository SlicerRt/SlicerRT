/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef __qSlicerPlmSlicerBsplineModule_h
#define __qSlicerPlmSlicerBsplineModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerPlmSlicerBsplineModuleExport.h"

class qSlicerPlmSlicerBsplineModulePrivate;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_PLMSLICERBSPLINE_EXPORT qSlicerPlmSlicerBsplineModule :
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerPlmSlicerBsplineModule(QObject *parent=0);
  virtual ~qSlicerPlmSlicerBsplineModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);
  
  /// Help to use the module
  virtual QString helpText()const;

  /// Return acknowledgements
  virtual QString acknowledgementText()const;
  
  /// Set the category for this Module
  virtual QStringList categories()const;

  /// Return a custom icon for the module
  virtual QIcon icon()const;

protected:

  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerPlmSlicerBsplineModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlmSlicerBsplineModule);
  Q_DISABLE_COPY(qSlicerPlmSlicerBsplineModule);

};

#endif
