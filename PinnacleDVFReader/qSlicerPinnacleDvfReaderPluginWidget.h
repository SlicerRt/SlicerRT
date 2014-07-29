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

#ifndef __qSlicerPinnacleDvfReaderPluginWidget_h
#define __qSlicerPinnacleDvfReaderPluginWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// PinnacleDvfReader includes
#include "qSlicerPinnacleDvfReaderModuleExport.h"

class qSlicerPinnacleDvfReaderPluginWidgetPrivate;

/// \ingroup SlicerRt_QtModules_PinnacleDvfReader
class Q_SLICER_PINNACLEDVFREADER_EXPORT qSlicerPinnacleDvfReaderPluginWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerPinnacleDvfReaderPluginWidget(QWidget *parent=0);
  virtual ~qSlicerPinnacleDvfReaderPluginWidget();

protected:
  QScopedPointer<qSlicerPinnacleDvfReaderPluginWidgetPrivate> d_ptr;
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerPinnacleDvfReaderPluginWidget);
  Q_DISABLE_COPY(qSlicerPinnacleDvfReaderPluginWidget);
};

#endif
