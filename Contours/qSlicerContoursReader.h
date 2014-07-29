/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Adam Rankin, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerContoursReader_h
#define __qSlicerContoursReader_h

// SlicerQt includes
#include "qSlicerFileReader.h"
class qSlicerContoursReaderPrivate;

// Slicer includes
class vtkSlicerContoursModuleLogic;

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Contours
class qSlicerContoursReader
  : public qSlicerFileReader
{
  Q_OBJECT
public:
  typedef qSlicerFileReader Superclass;
  qSlicerContoursReader(vtkSlicerContoursModuleLogic* contoursLogic = 0, QObject* parent = 0);
  virtual ~qSlicerContoursReader();

  void setContoursLogic(vtkSlicerContoursModuleLogic* contoursLogic);
  vtkSlicerContoursModuleLogic* contoursLogic()const;

  virtual QString description()const;
  virtual IOFileType fileType()const;
  virtual QStringList extensions()const;

  virtual bool load(const IOProperties& properties);

protected:
  QScopedPointer<qSlicerContoursReaderPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerContoursReader);
  Q_DISABLE_COPY(qSlicerContoursReader);
};

#endif
