
//DeformationFieldVisualizer logic
#include "vtkSlicerDeformationFieldVisualizerLogic.h"
#include "vtkMRMLDeformationFieldVisualizerParametersNode.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLVectorVolumeNode.h>

// VTK includes
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

// ITK includes
#if ITK_VERSION_MAJOR > 3
  #include "itkFactoryRegistration.h"
#endif

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//#include <omp.h>

//-----------------------------------------------------------------------------
int qSlicerDeformationFieldVisualizerTest(int argc, char *argv[])
{
	#if ITK_VERSION_MAJOR > 3
	itk::itkFactoryRegistration();
	#endif

/*

	const char *dataPath;
    dataPath = argv[1];
	
	const char *temporarySceneFileName;
    temporarySceneFileName = argv[2];
	
	//Create scene
	vtkSmartPointer<vtkMRMLScene> scene = vtkSmartPointer<vtkMRMLScene>::New();

	vtksys::SystemTools::RemoveFile(temporarySceneFileName); //Remove scene file of same name
	scene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
	scene->SetURL(temporarySceneFileName);
	scene->Commit();
	
	//Add Vector Volume Node for Deformation Field
	vtkSmartPointer<vtkMRMLVectorVolumeNode> inputVolumeNode = vtkSmartPointer<vtkMRMLVectorVolumeNode>::New();
	scene->AddNode(inputVolumeNode);

	//Load test file (.mha)
	std::string inputFilename;
	inputFilename = std::string(dataPath) + "/brain.mha";
	
	if (!vtksys::SystemTools::FileExists(inputFilename.c_str())){
		std::cerr << "The input file " << inputFilename << " could not be found." << std::endl;
		return EXIT_FAILURE;
	}
		
	//Add storage node
	vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> inputVolumeArchetypeStorageNode = vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
	inputVolumeArchetypeStorageNode->SetFileName(inputFilename.c_str());
	scene->AddNode(inputVolumeArchetypeStorageNode);
	inputVolumeNode->SetAndObserveStorageNodeID(inputVolumeArchetypeStorageNode->GetID());

	//Read data from reference node
	if (!inputVolumeArchetypeStorageNode->ReadData(inputVolumeNode)){
		scene->Commit();
		std::cerr << "Reading dose volume from file '" << inputFilename << "' failed!" << std::endl;
		return EXIT_FAILURE;
	}
	
	//Set up parameter node
	vtkSmartPointer<vtkMRMLDeformationFieldVisualizerParametersNode> pNode = vtkSmartPointer<vtkMRMLDeformationFieldVisualizerParametersNode>::New();
	pNode->SetAndObserveInputVolumeNodeID(inputVolumeNode->GetID());
	pNode->SetGridDensity(1);
	scene->AddNode(pNode);
	
	//Rebuild imagedata and prepare for test
	double origin[3];
	double spacing[3];
	int dimensions[3];
	
	inputVolumeNode->GetImageData()->GetPointData()->SetActiveVectors("ImageScalars");
	inputVolumeNode->GetOrigin(origin);
	inputVolumeNode->GetSpacing(spacing);
	
	vtkSmartPointer<vtkImageData> field = vtkSmartPointer<vtkImageData>::New();
	field = inputVolumeNode->GetImageData();
	field->SetOrigin(origin);
	field->SetSpacing(spacing);

	//Initialize logic node
	vtkSmartPointer<vtkSlicerDeformationFieldVisualizerLogic> deformationFieldVisualizerLogic = vtkSmartPointer<vtkSlicerDeformationFieldVisualizerLogic>::New();
	deformationFieldVisualizerLogic->SetMRMLScene(scene);
	deformationFieldVisualizerLogic->SetAndObserveParameterNode(pNode);

	
	//Glyph
	deformationFieldVisualizerLogic->glyphVisualization(field,0);

	//Grid
	deformationFieldVisualizerLogic->gridVisualization(field);
	
	//Block
	deformationFieldVisualizerLogic->blockVisualization(field);
	
	//Contour
	deformationFieldVisualizerLogic->contourVisualization(field);
	
//*/

	return EXIT_SUCCESS;
}
