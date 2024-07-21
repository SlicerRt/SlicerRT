


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
#include <vtkMRMLObjectiveNode.h>

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


    // get reference Volume
    vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();
    if (!planNode || !referenceVolumeNode || !resultOptimizationVolumeNode)
    {
        QString errorMessage("Unable to access reference volume"); // needed? checked by abstract engine?
        qCritical() << Q_FUNC_INFO << ": " << errorMessage;
        return errorMessage;
    }


    Eigen::VectorXd totalDose;

    int numberOfBeams = planNode->GetNumberOfBeams();

    std::vector<std::string> beamNames;
    int tried_beam_index = 0;

    while (beamNames.size() < numberOfBeams) {
        vtkMRMLRTBeamNode* beam = planNode->GetBeamByNumber(tried_beam_index);
        if (beam != nullptr) {
            beamNames.push_back(beam->GetName());
        }
        tried_beam_index++;
    }


    // calculate dose for each beamNode and add to to total dose
    for (int i = 0; i < planNode->GetNumberOfBeams(); i++)
    {
        // get dose influence matrix
        vtkMRMLRTBeamNode* beamNode = planNode->GetBeamByName(beamNames[i]);
        if (!beamNode)
        {
            QString errorMessage("Invalid beam node");
            qCritical() << Q_FUNC_INFO << ": " << errorMessage;
            return errorMessage;
        }
        vtkMRMLRTBeamNode::DoseInfluenceMatrixType doseInfluenceMatrix = beamNode->GetDoseInfluenceMatrix();
        if (doseInfluenceMatrix.rows() == 0 || doseInfluenceMatrix.cols() == 0)
        {
            QString errorMessage("Dose influence matrix is empty");
            qCritical() << Q_FUNC_INFO << ": " << errorMessage;
            return errorMessage;
        }


        // multiply Dose matrix with vector (filled with ones)
        Eigen::VectorXd vector = Eigen::VectorXd::Ones(doseInfluenceMatrix.rows());
        Eigen::VectorXd dose = doseInfluenceMatrix * vector;

        // resize total Dose vector to 
        if (i == 0)
        {
            totalDose.resize(dose.size());
        }

        totalDose += dose;


        //std::cout << "DOSE INFO:" << std::endl;
        //std::cout << "size total dose: " << totalDose.size() << std::endl;
        //std::cout << "number of rows: " << doseInfluenceMatrix.rows() << std::endl;
        //std::cout << "number of cols: " << doseInfluenceMatrix.cols() << std::endl;
    }

    
   
    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetExtent(referenceVolumeNode->GetImageData()->GetExtent());
    imageData->SetSpacing(referenceVolumeNode->GetImageData()->GetSpacing());
    imageData->SetOrigin(referenceVolumeNode->GetImageData()->GetOrigin());
    imageData->AllocateScalars(VTK_FLOAT, 1);


    //std::cout << "\nnumber of Points in reference Volume: " << imageData->GetNumberOfPoints() << std::endl;


    if (imageData->GetNumberOfPoints() != totalDose.size())
    {
        QString errorMessage("Geometrical discrepancy between reference volume and dose");
        qCritical() << Q_FUNC_INFO << ": " << errorMessage;
        return errorMessage;
    }

    // fill voxels with total dose
    float* floatPtr = (float*)imageData->GetScalarPointer();

    int N_x = referenceVolumeNode->GetImageData()->GetDimensions()[0];
    int N_y = referenceVolumeNode->GetImageData()->GetDimensions()[1];
    int N_z = referenceVolumeNode->GetImageData()->GetDimensions()[2];

    for (int i = 0; i < imageData->GetNumberOfPoints(); ++i)
    {
        int x = i % N_x;
        int y = (i / N_x) % N_y;
        int z = i / (N_x * N_y);

        int i_D = y + N_y * (x + N_x * z);

        (*floatPtr) = totalDose[i_D];
        ++floatPtr;
    }

    // set image
    resultOptimizationVolumeNode->SetAndObserveImageData(imageData);
    resultOptimizationVolumeNode->CopyOrientation(referenceVolumeNode);

    std::string randomDoseNodeName = std::string(planNode->GetName()) + "_MockOptimizer";
    resultOptimizationVolumeNode->SetName(randomDoseNodeName.c_str());

  return QString();
}


//-----------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkMRMLObjectiveNode>> qSlicerMockPlanOptimizer::getAvailableObjectives()
{
    std::vector<vtkSmartPointer<vtkMRMLObjectiveNode>> objectives;
    //objectives.push_back("AnotherObjective1");
    //objectives.push_back("AnotherObjective2");
    //// Add more specific objectives as needed
    return objectives;
}