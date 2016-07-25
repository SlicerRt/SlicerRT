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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerDoseEngineLogic_h
#define __qSlicerDoseEngineLogic_h

#include "qSlicerExternalBeamPlanningDoseEnginesExport.h"

// Qt includes
#include "QObject"

class vtkMRMLRTPlanNode;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract dose calculation algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class for specific dose engine plugins
class Q_SLICER_EXTERNALBEAMPLANNING_DOSE_ENGINES_EXPORT qSlicerDoseEngineLogic : public QObject
{
  Q_OBJECT

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerDoseEngineLogic(QObject* parent=NULL);
  /// Destructor
  virtual ~qSlicerDoseEngineLogic();

public:
  /// Calculate dose for a plan
  Q_INVOKABLE QString calculateDose(vtkMRMLRTPlanNode* planNode);

  /// Accumulate per-beam dose volumes for each beam under given plan. The accumulated
  /// total dose is 
  Q_INVOKABLE QString createAccumulatedDose(vtkMRMLRTPlanNode* planNode);

  /// Remove MRML nodes created by dose calculation for the current RT plan,
  /// such as apertures, range compensators, and doses
  Q_INVOKABLE void removeIntermediateResults(vtkMRMLRTPlanNode* planNode);

signals:
  /// Signals for dose calculation progress update
  /// \param progress Value between 0 and 1
  void progressUpdated(double progress);

private:
  Q_DISABLE_COPY(qSlicerDoseEngineLogic);
};

#endif
