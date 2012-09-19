/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include <QtPlugin>

#include "qSlicerPlmSlicerBsplineModule.h"
#include "qSlicerPlmSlicerBsplineModuleWidget.h"
#include "vtkSlicerPlmSlicerBsplineLogic.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerPlmSlicerBsplineModule, qSlicerPlmSlicerBsplineModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPlmSlicerBsplineModulePrivate
{
public:
  qSlicerPlmSlicerBsplineModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerPlmSlicerBsplineModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerPlmSlicerBsplineModulePrivate::qSlicerPlmSlicerBsplineModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPlmSlicerBsplineModule methods

//-----------------------------------------------------------------------------
qSlicerPlmSlicerBsplineModule::qSlicerPlmSlicerBsplineModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlmSlicerBsplineModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPlmSlicerBsplineModule::~qSlicerPlmSlicerBsplineModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerPlmSlicerBsplineModule::helpText()const
{
  return "This PlmSlicerBspline module illustrates how a loadable module should "
      "be implemented.";
}

//-----------------------------------------------------------------------------
QString qSlicerPlmSlicerBsplineModule::acknowledgementText()const
{
  return "This work was supported by ...";
}

//-----------------------------------------------------------------------------
QIcon qSlicerPlmSlicerBsplineModule::icon()const
{
  return QIcon(":/Icons/PlmSlicerBspline.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlmSlicerBsplineModule::categories()const
{
  return QStringList() << "Plastimatch";
}

//-----------------------------------------------------------------------------
void qSlicerPlmSlicerBsplineModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerPlmSlicerBsplineModule::createWidgetRepresentation()
{
  return new qSlicerPlmSlicerBsplineModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPlmSlicerBsplineModule::createLogic()
{
  return vtkSlicerPlmSlicerBsplineLogic::New();
}
