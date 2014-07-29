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

#ifndef __qSlicerPinnacleDvfReaderModule_h
#define __qSlicerPinnacleDvfReaderModule_h

// SlicerQt includes
#include "qSlicerLoadableModule.h"

// PinnacleDvfReader includes
#include "qSlicerPinnacleDvfReaderModuleExport.h"

class qSlicerPinnacleDvfReaderModulePrivate;

/// \ingroup SlicerRt_QtModules_PinnacleDvfReader
class Q_SLICER_PINNACLEDVFREADER_EXPORT qSlicerPinnacleDvfReaderModule:
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);

public:

  typedef qSlicerLoadableModule Superclass;
  qSlicerPinnacleDvfReaderModule(QObject *parent=0);
  virtual ~qSlicerPinnacleDvfReaderModule();

  virtual QString helpText()const;
  virtual QString acknowledgementText()const;
  virtual QStringList contributors()const;
  virtual QStringList categories()const;
  qSlicerGetTitleMacro(QTMODULE_TITLE);

  // Makes the module hidden
  virtual bool isHidden()const { return true; };

protected:
  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation* createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerPinnacleDvfReaderModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPinnacleDvfReaderModule);
  Q_DISABLE_COPY(qSlicerPinnacleDvfReaderModule);
};

#endif
