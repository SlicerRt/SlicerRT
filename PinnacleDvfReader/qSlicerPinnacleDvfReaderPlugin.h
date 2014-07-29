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

#ifndef __qSlicerPinnacleDvfReaderPlugin
#define __qSlicerPinnacleDvfReaderPlugin

// SlicerQt includes
#include "qSlicerFileReader.h"

class qSlicerPinnacleDvfReaderPluginPrivate;
class vtkSlicerPinnacleDvfReaderLogic;

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDvfReader
class qSlicerPinnacleDvfReaderPlugin
  : public qSlicerFileReader
{
  Q_OBJECT

public:
  typedef qSlicerFileReader Superclass;
  qSlicerPinnacleDvfReaderPlugin(QObject* parent = 0);
  qSlicerPinnacleDvfReaderPlugin(vtkSlicerPinnacleDvfReaderLogic* logic, QObject* parent = 0);
  virtual ~qSlicerPinnacleDvfReaderPlugin();

  vtkSlicerPinnacleDvfReaderLogic* logic()const;
  void setLogic(vtkSlicerPinnacleDvfReaderLogic* logic);

  virtual QString description()const;
  virtual IOFileType fileType()const;
  virtual QStringList extensions()const;
  virtual qSlicerIOOptions* options()const;
  virtual bool load(const IOProperties& properties);

protected:
  QScopedPointer<qSlicerPinnacleDvfReaderPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPinnacleDvfReaderPlugin);
  Q_DISABLE_COPY(qSlicerPinnacleDvfReaderPlugin);
};

#endif
