/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

#ifndef __qSlicerPinnacleDVFReaderPlugin
#define __qSlicerPinnacleDVFReaderPlugin

// SlicerQt includes
#include "qSlicerFileReader.h"

class qSlicerPinnacleDVFReaderPluginPrivate;
class vtkSlicerPinnacleDVFReaderLogic;

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDVFReader
class qSlicerPinnacleDVFReaderPlugin
  : public qSlicerFileReader
{
  Q_OBJECT

public:
  typedef qSlicerFileReader Superclass;
  qSlicerPinnacleDVFReaderPlugin(QObject* parent = 0);
  qSlicerPinnacleDVFReaderPlugin(vtkSlicerPinnacleDVFReaderLogic* logic, QObject* parent = 0);
  virtual ~qSlicerPinnacleDVFReaderPlugin();

  vtkSlicerPinnacleDVFReaderLogic* logic()const;
  void setLogic(vtkSlicerPinnacleDVFReaderLogic* logic);

  virtual QString description()const;
  virtual IOFileType fileType()const;
  virtual QStringList extensions()const;
  virtual qSlicerIOOptions* options()const;
  virtual bool load(const IOProperties& properties);

protected:
  QScopedPointer<qSlicerPinnacleDVFReaderPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPinnacleDVFReaderPlugin);
  Q_DISABLE_COPY(qSlicerPinnacleDVFReaderPlugin);
};

#endif
