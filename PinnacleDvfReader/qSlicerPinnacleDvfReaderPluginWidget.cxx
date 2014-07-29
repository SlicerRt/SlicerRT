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

/// PinnacleDvfReader includes
#include "qSlicerPinnacleDvfReaderPluginWidget.h"
#include "ui_qSlicerPinnacleDvfReaderPluginWidget.h"
#include "vtkSlicerPinnacleDvfReaderLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDvfReader
class qSlicerPinnacleDvfReaderPluginWidgetPrivate: public Ui_qSlicerPinnacleDvfReaderPluginWidget
{
public:
  qSlicerPinnacleDvfReaderPluginWidgetPrivate();
};

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderPluginWidgetPrivate::qSlicerPinnacleDvfReaderPluginWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderPluginWidget::qSlicerPinnacleDvfReaderPluginWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerPinnacleDvfReaderPluginWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderPluginWidget::~qSlicerPinnacleDvfReaderPluginWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerPinnacleDvfReaderPluginWidget::setup()
{
  Q_D(qSlicerPinnacleDvfReaderPluginWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
