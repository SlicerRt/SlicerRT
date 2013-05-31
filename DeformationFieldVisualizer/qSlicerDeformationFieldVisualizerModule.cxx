/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
  
  This file was originally developed by Franklin King, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Qt includes
#include <QtPlugin>

// DeformationFieldVisualizer Logic includes
#include <vtkSlicerDeformationFieldVisualizerLogic.h>

// DeformationFieldVisualizer includes
#include "qSlicerDeformationFieldVisualizerModule.h"
#include "qSlicerDeformationFieldVisualizerModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDeformationFieldVisualizerModule, qSlicerDeformationFieldVisualizerModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DeformationFieldVisualizer
class qSlicerDeformationFieldVisualizerModulePrivate
{
public:
  qSlicerDeformationFieldVisualizerModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDeformationFieldVisualizerModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDeformationFieldVisualizerModulePrivate
::qSlicerDeformationFieldVisualizerModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDeformationFieldVisualizerModule methods

//-----------------------------------------------------------------------------
qSlicerDeformationFieldVisualizerModule
::qSlicerDeformationFieldVisualizerModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDeformationFieldVisualizerModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDeformationFieldVisualizerModule::~qSlicerDeformationFieldVisualizerModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDeformationFieldVisualizerModule::helpText()const
{
  return "TODO: This is a placeholder help text.";
}

//-----------------------------------------------------------------------------
QString qSlicerDeformationFieldVisualizerModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDeformationFieldVisualizerModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Franklin King") << QString("Andras Lasso") << QString("Csaba Pinter");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerDeformationFieldVisualizerModule::icon()const
{
  return QIcon(":/Icons/DeformationFieldVisualizer.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerDeformationFieldVisualizerModule::categories() const
{
  return QStringList() << "Registration";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDeformationFieldVisualizerModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDeformationFieldVisualizerModule
::createWidgetRepresentation()
{
  return new qSlicerDeformationFieldVisualizerModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDeformationFieldVisualizerModule::createLogic()
{
  return vtkSlicerDeformationFieldVisualizerLogic::New();
}
