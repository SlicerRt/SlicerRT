


// Optimizer includes
#include "qSlicerMockPlanOptimizer.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// Segmentations includes
#include "vtkOrientedImageData.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>

// Slicer includes
#include <vtkSlicerVersionConfigure.h>

// Qt includes
#include <QDebug>


//----------------------------------------------------------------------------
qSlicerMockPlanOptimizer::qSlicerMockPlanOptimizer(QObject* parent)
    : qSlicerAbstractPlanOptimizer(parent)
{
    this->m_Name = QString("Mock Optimizer");
}

//----------------------------------------------------------------------------
qSlicerMockPlanOptimizer::~qSlicerMockPlanOptimizer() = default;

//---------------------------------------------------------------------------
QString qSlicerMockPlanOptimizer::optimizePlanUsingOptimizer(vtkMRMLRTPlanNode* planNode, vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode)
{
    // TODO: access dose matrix from planNode --> multiply by vector (filled with ones)

    int numberOfBeams = planNode->GetNumberOfBeams();

    std::cout << "Number of Beams : " << numberOfBeams << std::endl;


    for (int i = 1; i < numberOfBeams; i++) {
        vtkMRMLRTBeamNode* beamNode = planNode->GetBeamByNumber(1);
        std::cout << beamNode->GetName() << std::endl;
    }
    
    // doesn't seem to be called when optimizing (button)

  return QString();
}

