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

#ifndef __qSlicerPinnacleDvfReaderOptionsWidget_h
#define __qSlicerPinnacleDvfReaderOptionsWidget_h

// CTK includes
#include <ctkPimpl.h>

// SlicerQt includes
#include "qSlicerIOOptionsWidget.h"

// PinnacleDvfReader includes
#include "qSlicerPinnacleDvfReaderModuleExport.h"

class qSlicerPinnacleDvfReaderOptionsWidgetPrivate;

/// \ingroup SlicerRt_QtModules_PinnacleDvfReader
class Q_SLICER_PINNACLEDVFREADER_EXPORT qSlicerPinnacleDvfReaderOptionsWidget :
  public qSlicerIOOptionsWidget
{
  Q_OBJECT
public:
  typedef qSlicerIOOptionsWidget Superclass;
  qSlicerPinnacleDvfReaderOptionsWidget(QWidget *parent=0);
  virtual ~qSlicerPinnacleDvfReaderOptionsWidget();


protected slots:
  void updateProperties();

private:
  Q_DECLARE_PRIVATE_D(qGetPtrHelper(qSlicerIOOptions::d_ptr), qSlicerPinnacleDvfReaderOptionsWidget);
  //Q_DECLARE_PRIVATE(qSlicerPinnacleDvfReaderOptionsWidget);
  Q_DISABLE_COPY(qSlicerPinnacleDvfReaderOptionsWidget);
};

#endif
