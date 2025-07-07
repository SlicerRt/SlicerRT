


// Optimizer includes
#include "qSlicerMockPlanOptimizer.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// Segmentations includes
#include "vtkOrientedImageData.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"

// Objectives includes
#include "qSlicerSquaredDeviationObjective.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"
#include <vtkMRMLRTObjectiveNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>

// Slicer includes
#include <vtkSlicerVersionConfigure.h>
#include <vtkSlicerVolumesLogic.h>
#include <vtkImageReslice.h>

// Qt includes
#include <QDebug>
#include "qSlicerApplication.h"


//----------------------------------------------------------------------------
qSlicerMockPlanOptimizer::qSlicerMockPlanOptimizer(QObject* parent)
    : qSlicerAbstractPlanOptimizer(parent)
{
    this->m_Name = QString("Mock Optimizer");
}

//----------------------------------------------------------------------------
qSlicerMockPlanOptimizer::~qSlicerMockPlanOptimizer() = default;

//---------------------------------------------------------------------------  
QString qSlicerMockPlanOptimizer::optimizePlanUsingOptimizer(vtkMRMLRTPlanNode* planNode, std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> objectives, vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode)  
{  
   // Get reference Volume  
   vtkMRMLScalarVolumeNode* referenceVolumeNode = planNode->GetReferenceVolumeNode();  
   if (!planNode || !referenceVolumeNode || !resultOptimizationVolumeNode)  
   {  
       QString errorMessage("Unable to access reference volume");  
       qCritical() << Q_FUNC_INFO << ": " << errorMessage;  
       return errorMessage;  
   }  

   // Create total dose image data from reference volume
   vtkImageData* totalDoseImageData = vtkImageData::New();
   totalDoseImageData->CopyStructure(referenceVolumeNode->GetImageData());
   totalDoseImageData->AllocateScalars(VTK_FLOAT, 1);

   // Get beam names
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

   // calculate dose for each beamNode and add to total dose
   for (int i = 0; i < planNode->GetNumberOfBeams(); i++)  
   {  
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

       // Multipy dose influence matrix with uniform fluence
       Eigen::VectorXd vector = Eigen::VectorXd::Ones(doseInfluenceMatrix.rows());  
       Eigen::VectorXd dose = doseInfluenceMatrix * vector;

       // Get dose grid dimensions & spacing
       double doseGridDim[3];
	   beamNode->GetDoseGridDim(doseGridDim);

       double doseGridSpacing[3];
       beamNode->GetDoseGridSpacing(doseGridSpacing);

       bool resample = true;
	   if (doseGridDim[0] <= 0 || doseGridDim[1] <= 0 || doseGridDim[2] <= 0) {
		   // If dose grid dimensions are not set, use reference volume dimensions
		   qWarning() << Q_FUNC_INFO << ": Dose grid dimensions are not set for beam" << beamNode->GetName();
		   qWarning() << Q_FUNC_INFO << ": Using reference volume dimensions instead.";
           doseGridDim[0] = referenceVolumeNode->GetImageData()->GetDimensions()[0];
		   doseGridDim[1] = referenceVolumeNode->GetImageData()->GetDimensions()[1];
		   doseGridDim[2] = referenceVolumeNode->GetImageData()->GetDimensions()[2];
           resample = false;
       }

       if (doseGridSpacing[0] <= 0 || doseGridSpacing[1] <= 0 || doseGridSpacing[2] <= 0) {
           // If dose grid spacing is not set, use reference volume spacing
           qWarning() << Q_FUNC_INFO << ": Dose grid spacing not set for beam" << beamNode->GetName();
           qWarning() << Q_FUNC_INFO << ": Using reference volume spacing instead.";
           referenceVolumeNode->GetImageData()->GetSpacing(doseGridSpacing);
           resample = false;
       }

       // Create volumeNode for dose of beam
       int N_x = static_cast<int>(doseGridDim[0]);
       int N_y = static_cast<int>(doseGridDim[1]);
       int N_z = static_cast<int>(doseGridDim[2]);
       vtkImageData* doseImageData = vtkImageData::New();
       doseImageData->SetDimensions(N_x, N_y, N_z);
       doseImageData->SetSpacing(doseGridSpacing);
       doseImageData->AllocateScalars(VTK_FLOAT, 1);
          
       // Fill with dose (according to doseGridDim)
       float* vtkPtr = static_cast<float*>(doseImageData->GetScalarPointer());
       for (int i = 0; i < dose.size(); ++i) {
	       vtkPtr[i] = static_cast<float>(dose[i]);
       }

	   // Resample image if necessary and get dosPtr
       float* dosePtr;
       if (resample) {
           // Create reslice filter for interpolation
           vtkSmartPointer<vtkImageReslice> resliceFilter = vtkSmartPointer<vtkImageReslice>::New();
           resliceFilter->SetInputData(doseImageData);
           resliceFilter->SetOutputSpacing(referenceVolumeNode->GetSpacing());
           resliceFilter->SetOutputExtent(referenceVolumeNode->GetImageData()->GetExtent());
           resliceFilter->SetInterpolationModeToLinear();
           resliceFilter->Update();

           // Get resampled dose image
           vtkSmartPointer<vtkImageData> resampledDoseImageData = vtkSmartPointer<vtkImageData>::New();
           resampledDoseImageData->DeepCopy(resliceFilter->GetOutput());
           if (!resampledDoseImageData || resampledDoseImageData->GetNumberOfPoints() == 0)
           {
               QString errorMessage("Dose resampling failed or resulted in empty data");
               qCritical() << Q_FUNC_INFO << ": " << errorMessage;
               return errorMessage;
           }
           if (resampledDoseImageData->GetNumberOfPoints() != totalDoseImageData->GetNumberOfPoints())
           {
               QString errorMessage("Number of points in resampled dose and total dose don't match! (Should match referenceVolume.)");
               qCritical() << Q_FUNC_INFO << ": " << errorMessage;
               return errorMessage;
           }
           // Add resampled dose to total dose
           float* totalDosePtr = static_cast<float*>(totalDoseImageData->GetScalarPointer());
           float* dosePtr = static_cast<float*>(resampledDoseImageData->GetScalarPointer());
           if (!totalDosePtr || !dosePtr)
           {
               QString errorMessage("Invalid pointer in total or resampled beam dose data!");
               qCritical() << Q_FUNC_INFO << ": " << errorMessage;
               return errorMessage;
           }
           for (int i = 0; i < totalDoseImageData->GetNumberOfPoints(); ++i) {
               totalDosePtr[i] += dosePtr[i];
           }
       }
	   else {
		   // No resampling needed, use original dose image data
		   if (doseImageData->GetNumberOfPoints() != totalDoseImageData->GetNumberOfPoints())
		   {
			   QString errorMessage("Number of points in dose and total dose don't match! (Should match referenceVolume.)");
			   qCritical() << Q_FUNC_INFO << ": " << errorMessage;
			   return errorMessage;
		   }
           // Add dose to total dose
           float* totalDosePtr = static_cast<float*>(totalDoseImageData->GetScalarPointer());
		   float* dosePtr = static_cast<float*>(doseImageData->GetScalarPointer());
           if (!totalDosePtr || !dosePtr)
           {
               QString errorMessage("Invalid pointer in total dose or beam dose data!");
               qCritical() << Q_FUNC_INFO << ": " << errorMessage;
               return errorMessage;
           }
           for (int i = 0; i < totalDoseImageData->GetNumberOfPoints(); ++i) {
               totalDosePtr[i] += dosePtr[i];
           }
	   }

	   // Clean up
	   doseImageData->Delete();
   }
   
   // Set image
   resultOptimizationVolumeNode->SetAndObserveImageData(totalDoseImageData);
   resultOptimizationVolumeNode->CopyOrientation(referenceVolumeNode);

   std::string randomDoseNodeName = std::string(planNode->GetName()) + "_MockOptimizer";
   resultOptimizationVolumeNode->SetName(randomDoseNodeName.c_str());

   return QString();
}

//-----------------------------------------------------------------------------
void qSlicerMockPlanOptimizer::setAvailableObjectives()
{
 //   qSlicerSquaredDeviationObjective* squaredDeviationObjective1 = new qSlicerSquaredDeviationObjective();
	//qSlicerSquaredDeviationObjective* squaredDeviationObjective2 = new qSlicerSquaredDeviationObjective();

	//std::vector<qSlicerAbstractObjective*> objectives;

	//objectives.push_back(squaredDeviationObjective1);
	//objectives.push_back(squaredDeviationObjective2);

	//this->availableObjectives = objectives;
}