


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

    // ToDo: check if pyRadPlanEngine's dose calculation works with mock optimizer


    Eigen::VectorXd totalDose;

    // calculate dose for each beamNode and add to to total dose
    for (int i = 1; i <= planNode->GetNumberOfBeams(); i++)
    {
 
        // get dose influence matrix
        vtkMRMLRTBeamNode* beamNode = planNode->GetBeamByNumber(i);
        vtkMRMLRTBeamNode::DoseInfluenceMatrixType doseInfluenceMatrix = beamNode->GetDoseInfluenceMatrix();

        // multiply Dose matrix with vector (filled with ones)
        Eigen::VectorXd vector = Eigen::VectorXd::Ones(doseInfluenceMatrix.rows()); // sparse: column-major -> vector length: number of rows in sparse (= number of voxels)
        Eigen::VectorXd dose = doseInfluenceMatrix * vector;

        // resize total Dose vector to 
        if (i == 1)
        {
            totalDose.resize(dose.size());
        }

        totalDose += dose;
    }

    // get reference Volume
    vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();

    // fill voxels with total dose
    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetExtent(referenceVolumeNode->GetImageData()->GetExtent());
    imageData->SetSpacing(referenceVolumeNode->GetImageData()->GetSpacing());
    imageData->SetOrigin(referenceVolumeNode->GetImageData()->GetOrigin());
    imageData->AllocateScalars(VTK_FLOAT, 1);

    float* floatPtr = (float*)imageData->GetScalarPointer();

    for (int i = 0; i < imageData->GetNumberOfPoints(); ++i)
    {
        (*floatPtr) = totalDose[i];
        ++floatPtr;
    }

    // set image
    resultOptimizationVolumeNode->SetAndObserveImageData(imageData);
    resultOptimizationVolumeNode->CopyOrientation(referenceVolumeNode);

    std::string randomDoseNodeName = std::string(planNode->GetName()) + "_MockOptimizer";
    resultOptimizationVolumeNode->SetName(randomDoseNodeName.c_str());


    // ERRORs to include:
    // - dose size doesn't match


  return QString();
}