/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Paolo Zaffino, Universita' degli Studi
  "Magna Graecia" di Catanzaro and was supported through the Applied Cancer
  Research Unit program of Cancer Care Ontario with funds provided by the
  Natural Sciences and Engineering Research Council of Canada.

==========================================================================*/

#ifndef __qSlicerPlastimatchPyModule_h
#define __qSlicerPlastimatchPyModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerPlastimatchPyModuleExport.h"

class qSlicerPlastimatchPyModulePrivate;

/// \ingroup SlicerRt_QtModules_PlastimatchPy
class Q_SLICER_QTMODULES_PLASTIMATCHPY_EXPORT qSlicerPlastimatchPyModule : public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:
  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerPlastimatchPyModule(QObject *parent=0);
  virtual ~qSlicerPlastimatchPyModule();

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
  QScopedPointer<qSlicerPlastimatchPyModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlastimatchPyModule);
  Q_DISABLE_COPY(qSlicerPlastimatchPyModule);
};

#endif
