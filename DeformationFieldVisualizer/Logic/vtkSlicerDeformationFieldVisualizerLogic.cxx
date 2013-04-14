
// DeformationFieldVisualizer Logic includes
#include "vtkSlicerDeformationFieldVisualizerLogic.h"
#include "vtkMRMLDeformationFieldVisualizerParametersNode.h"

// MRML includes
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLSliceNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkVectorNorm.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkGeneralTransform.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>

//Glyph VTK includes
#include "vtkDFVGlyph3D.h"
#include <vtkArrowSource.h>
#include <vtkConeSource.h>
#include <vtkSphereSource.h>

//Grid VTK includes
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkLine.h>
#include <vtkWarpVector.h>
//#include <vtkTubeFilter.h>

//Block VTK includes
#include <vtkGeometryFilter.h>
#include <vtkVectorDot.h>
#include <vtkPolyDataNormals.h>

//Contour VTK includes
#include <vtkMarchingCubes.h>

//Glyph Slice VTK includes
#include <vtkGlyphSource2D.h>
#include <vtkRibbonFilter.h>
//#include <vtkImageReslice.h>
//#include <vtkImageResample.h>

//Grid Slice VTK includes
#include <vtkAppendPolyData.h>

//ITK includes
//#include <itkVectorResampleImageFilter.h>
//#include <itkVTKImageToImageFilter.h>

// STD includes
#include <cassert>
#include <math.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDeformationFieldVisualizerLogic);

//----------------------------------------------------------------------------
vtkSlicerDeformationFieldVisualizerLogic::vtkSlicerDeformationFieldVisualizerLogic(){
	this->ParameterNode = NULL;
	transformField = vtkSmartPointer<vtkImageData>::New();
	option = 0;
}

//----------------------------------------------------------------------------
vtkSlicerDeformationFieldVisualizerLogic::~vtkSlicerDeformationFieldVisualizerLogic(){
	vtkSetAndObserveMRMLNodeMacro(this->ParameterNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::PrintSelf(ostream& os, vtkIndent indent){
	this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::SetAndObserveParameterNode(vtkMRMLDeformationFieldVisualizerParametersNode *node){
	vtkSetAndObserveMRMLNodeMacro(this->ParameterNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene){
	vtkNew<vtkIntArray> events;
	events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
	events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
	//events->InsertNextValue(vtkMRMLScene::EndImportEvent);
	//events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
	events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
	this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::RegisterNodes(){
	vtkMRMLScene* scene = this->GetMRMLScene();
	assert(scene != 0);
	
	scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDeformationFieldVisualizerParametersNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::UpdateFromMRMLScene(){
  assert(this->GetMRMLScene() != 0);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node){
	if (!node || !this->GetMRMLScene()){
		return;
	}

	if (node->IsA("vtkMRMLVectorVolumeNode") ||
		node->IsA("vtkMRMLLinearTransformNode") || 
		node->IsA("vtkMRMLGridTransformNode") || 
		node->IsA("vtkMRMLBSplineTransformNode") || 
		node->IsA("vtkMRMLDeformationFieldVisualizerParametersNode")){
		this->Modified();
	}
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node){
	if (!node || !this->GetMRMLScene()){
		return;
	}

	if (node->IsA("vtkMRMLVectorVolumeNode") ||
		node->IsA("vtkMRMLLinearTransformNode") || 
		node->IsA("vtkMRMLGridTransformNode") || 
		node->IsA("vtkMRMLBSplineTransformNode") || 
		node->IsA("vtkMRMLDeformationFieldVisualizerParametersNode")){
		this->Modified();
	}
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::OnMRMLSceneEndImport(){
	//Select parameter node if it exists
	vtkSmartPointer<vtkMRMLDeformationFieldVisualizerParametersNode> paramNode = NULL;
	vtkSmartPointer<vtkMRMLNode> node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLDeformationFieldVisualizerParametersNode");
	if (node){
		paramNode = vtkMRMLDeformationFieldVisualizerParametersNode::SafeDownCast(node);
		vtkSetAndObserveMRMLNodeMacro(this->ParameterNode, paramNode);
	}
}

//---------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::OnMRMLSceneEndClose(){
	this->Modified();
}

//*
// Issue when input data is changed without modifying node
// Will either remake or account for scenario some other way
//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::generateTransformField(){
	vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNode = vtkMRMLTransformNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetInputVolumeNodeID()));
	if (inputVolumeNode == NULL){return;}
	
	vtkSmartPointer<vtkMRMLVolumeNode> referenceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetReferenceVolumeNodeID()));
	if (referenceVolumeNode == NULL){return;}
	
	double origin[3];
	double spacing[3];
	double dirs[3][3];
	int extent[6];		
	vtkSmartPointer<vtkImageData> field = vtkSmartPointer<vtkImageData>::New();
	
	referenceVolumeNode->GetImageData()->GetPointData()->SetActiveVectors("ImageScalars");
	referenceVolumeNode->GetOrigin(origin);
	referenceVolumeNode->GetSpacing(spacing);
	referenceVolumeNode->GetImageData()->GetExtent(extent);
	referenceVolumeNode->GetIJKToRASDirections(dirs);
	vtkSmartPointer<vtkMatrix4x4> IJKToRAS = vtkSmartPointer<vtkMatrix4x4>::New();	
	referenceVolumeNode->GetIJKToRASMatrix(IJKToRAS);
	
	field->DeepCopy(referenceVolumeNode->GetImageData());
	field->SetScalarTypeToFloat();
	field->SetNumberOfScalarComponents(3);
	field->AllocateScalars();
	field->SetSpacing(spacing);
	field->GetPointData()->SetActiveVectors("ImageScalars");
	field->Update();
	
	float point1[3];
	float point2[3];

	vtkSmartPointer<vtkGeneralTransform> inputTransform = inputVolumeNode->GetTransformToParent();
	float *ptr = (float *)field->GetPointData()->GetScalars()->GetVoidPointer(0);
	for(int k = 0; k < extent[5]+1; k++){
		for(int j = 0; j < extent[3]+1; j++){
			for(int i = 0; i < extent[1]+1; i++){
				point1[0] = i*IJKToRAS->GetElement(0,0) + j*IJKToRAS->GetElement(0,1) + k*IJKToRAS->GetElement(0,2) + IJKToRAS->GetElement(0,3);
				point1[1] = i*IJKToRAS->GetElement(1,0) + j*IJKToRAS->GetElement(1,1) + k*IJKToRAS->GetElement(1,2) + IJKToRAS->GetElement(1,3);
				point1[2] = i*IJKToRAS->GetElement(2,0) + j*IJKToRAS->GetElement(2,1) + k*IJKToRAS->GetElement(2,2) + IJKToRAS->GetElement(2,3);
				
				inputTransform->TransformPoint(point1, point2);
				
				ptr[(i + j*(extent[1]+1) + k*(extent[1]+1)*(extent[3]+1))*3] = point2[0] - point1[0];
				ptr[(i + j*(extent[1]+1) + k*(extent[1]+1)*(extent[3]+1))*3 + 1] = point2[1] - point1[1];
				ptr[(i + j*(extent[1]+1) + k*(extent[1]+1)*(extent[3]+1))*3 + 2] = point2[2] - point1[2];
			}
		}
	}
	
	vtkSmartPointer<vtkMatrix4x4> invertDirs = vtkSmartPointer<vtkMatrix4x4>::New();
	invertDirs->Identity();
	int row, col;
	for (row=0; row<3; row++){
		for (col=0; col<3; col++){
			invertDirs->SetElement(row, col, dirs[row][col]);
		}
		invertDirs->SetElement(row, 3, 0);
	}
	invertDirs->Invert();
	
	double x,y,z;
	for(int i = 0; i < field->GetPointData()->GetScalars()->GetNumberOfTuples()*3; i+=3){
		x = ptr[i];
		y = ptr[i+1];
		z = ptr[i+2];			
		ptr[i] = x*invertDirs->GetElement(0,0) + y*invertDirs->GetElement(0,1) + z*invertDirs->GetElement(0,2);
		ptr[i+1] = x*invertDirs->GetElement(1,0) + y*invertDirs->GetElement(1,1) + z*invertDirs->GetElement(1,2);
		ptr[i+2] = x*invertDirs->GetElement(2,0) + y*invertDirs->GetElement(2,1) + z*invertDirs->GetElement(2,2);		
	}
	
	this->transformField = field;
}
//*/


//----------------------------------------------------------------------------
double vtkSlicerDeformationFieldVisualizerLogic::GetFieldMaxNorm(){
	return this->transformField->GetPointData()->GetArray(0)->GetMaxNorm();
}


//----------------------------------------------------------------------------
void vtkSlicerDeformationFieldVisualizerLogic::createVisualization(int option){
	if (!this->ParameterNode || !this->GetMRMLScene()){
		return;
	}

	//this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 
	
	//Initialize output
	vtkSmartPointer<vtkMRMLModelNode> outputModelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetOutputModelNodeID()));
	outputModelNode->SetScene(this->GetMRMLScene());
	//add model node checks! (Check for other types of models and etc.) (vtkErrorMacro)
	if (outputModelNode == NULL){
		return;
	}

	double origin[3];
	double spacing[3];
	double dirs[3][3];
	vtkSmartPointer<vtkImageData> field = vtkSmartPointer<vtkImageData>::New();
	
	//Initialize input
	if (strcmp((this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetInputVolumeNodeID()))->GetClassName(), "vtkMRMLVectorVolumeNode") == 0){
	
		vtkSmartPointer<vtkMRMLVectorVolumeNode> inputVolumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetInputVolumeNodeID()));
		if (inputVolumeNode == NULL){return;}
		
		//Set to something more solid than ImageScalars
		inputVolumeNode->GetImageData()->GetPointData()->SetActiveVectors("ImageScalars");
		inputVolumeNode->GetOrigin(origin);
		inputVolumeNode->GetSpacing(spacing);
		inputVolumeNode->GetIJKToRASDirections(dirs);
		field->DeepCopy(inputVolumeNode->GetImageData());
		field->SetSpacing(spacing);
		
		vtkSmartPointer<vtkMatrix4x4> invertDirs = vtkSmartPointer<vtkMatrix4x4>::New();
		invertDirs->Identity();
		int row, col;
		for (row=0; row<3; row++){
			for (col=0; col<3; col++){
				invertDirs->SetElement(row, col, dirs[row][col]);
			}
			invertDirs->SetElement(row, 3, 0);
		}
		invertDirs->Invert();
		
		double x,y,z;
		float *ptr = (float *)field->GetPointData()->GetScalars()->GetVoidPointer(0);
		for(int i = 0; i < field->GetPointData()->GetScalars()->GetNumberOfTuples()*3; i+=3){
			x = ptr[i];
			y = ptr[i+1];
			z = ptr[i+2];			
			ptr[i] = x*invertDirs->GetElement(0,0) + y*invertDirs->GetElement(0,1) + z*invertDirs->GetElement(0,2);
			ptr[i+1] = x*invertDirs->GetElement(1,0) + y*invertDirs->GetElement(1,1) + z*invertDirs->GetElement(1,2);
			ptr[i+2] = x*invertDirs->GetElement(2,0) + y*invertDirs->GetElement(2,1) + z*invertDirs->GetElement(2,2);		
		}
	}		
	else if (strcmp((this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetInputVolumeNodeID()))->GetClassName(), "vtkMRMLLinearTransformNode") == 0 || 
	strcmp((this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetInputVolumeNodeID()))->GetClassName(), "vtkMRMLBSplineTransformNode") == 0 ||
	strcmp((this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetInputVolumeNodeID()))->GetClassName(), "vtkMRMLGridTransformNode") == 0){
		vtkSmartPointer<vtkMRMLVolumeNode> referenceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetReferenceVolumeNodeID()));
		if (referenceVolumeNode == NULL){return;}	
	
		referenceVolumeNode->GetOrigin(origin);
		referenceVolumeNode->GetIJKToRASDirections(dirs);
		vtkSmartPointer<vtkMatrix4x4> IJKToRAS = vtkSmartPointer<vtkMatrix4x4>::New();	
		referenceVolumeNode->GetIJKToRASMatrix(IJKToRAS);		
		
		field = this->transformField;
	}
	else{
		std::cout<<"Invalid Node"<<std::endl;
		return;
	}

	//Create IJKToRAS Matrix without spacing; spacing will be added to imagedata directly to avoid warping geometry
	vtkSmartPointer<vtkMatrix4x4> UnspacedIJKToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
	UnspacedIJKToRAS->Identity();
	int row, col;
	for (row=0; row<3; row++){
		for (col=0; col<3; col++){
			UnspacedIJKToRAS->SetElement(row, col, dirs[row][col]);
		}
		UnspacedIJKToRAS->SetElement(row, 3, origin[row]);
    }
	
	
	vtkSmartPointer<vtkTransform> unspacedTransformToRAS = vtkSmartPointer<vtkTransform>::New();
	unspacedTransformToRAS->SetMatrix(UnspacedIJKToRAS);
	unspacedTransformToRAS->PostMultiply();

	vtkSmartPointer<vtkTransformPolyDataFilter> polydataTransform = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	polydataTransform->SetTransform(unspacedTransformToRAS);
	
	
	//Output
	if (outputModelNode->GetModelDisplayNode()==NULL){
		vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
		displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
		outputModelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
	}
	
	//Glyphs
	if (option == 1){
		//outputModelNode->SetAndObservePolyData(glyphVisualization(field, this->ParameterNode->GetGlyphSourceOption()));
		polydataTransform->SetInput(glyphVisualization(field, this->ParameterNode->GetGlyphSourceOption()));
		outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
		outputModelNode->GetModelDisplayNode()->SetActiveScalarName("VectorMagnitude");
	}
	//Grid
	else if (option == 2){
		polydataTransform->SetInput(gridVisualization(field));
		outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
		outputModelNode->GetModelDisplayNode()->SetActiveScalarName("VectorMagnitude");
		outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
	}
	//Contour
	else if (option == 3){
		polydataTransform->SetInput(contourVisualization(field));
		outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
		outputModelNode->GetModelDisplayNode()->SetActiveScalarName("VectorMagnitude");
		outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
		outputModelNode->GetModelDisplayNode()->SetSliceIntersectionVisibility(1);
	}
	//Block
	else if (option == 4){
		polydataTransform->SetInput(blockVisualization(field));
		outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
		outputModelNode->GetModelDisplayNode()->SetActiveScalarName("VectorMagnitude");
		outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
	}
	//Glyph Slice
	else if (option == 5){
		if (vtkMRMLSliceNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetGlyphSliceNodeID())) == NULL){return;}
		polydataTransform->SetInput(glyphSliceVisualization(field, UnspacedIJKToRAS));
		outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
		outputModelNode->GetModelDisplayNode()->SetActiveScalarName("OriginalMagnitude");
		outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
		outputModelNode->GetModelDisplayNode()->SetSliceIntersectionVisibility(1);
	}
	//Grid Slice
	else if (option == 6){
		if (vtkMRMLSliceNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetGridSliceNodeID())) == NULL){return;}
		polydataTransform->SetInput(gridSliceVisualization(field, UnspacedIJKToRAS));
		outputModelNode->GetModelDisplayNode()->SetScalarVisibility(1);
		outputModelNode->GetModelDisplayNode()->SetActiveScalarName("VectorMagnitude");
		outputModelNode->GetModelDisplayNode()->SetBackfaceCulling(0);
		outputModelNode->GetModelDisplayNode()->SetSliceIntersectionVisibility(1);
	}
	polydataTransform->Update();
	
	outputModelNode->SetAndObservePolyData(polydataTransform->GetOutput());
	
	outputModelNode->SetHideFromEditors(0);
	outputModelNode->SetSelectable(1);
	outputModelNode->Modified();
	
	if (outputModelNode->GetModelDisplayNode()->GetColorNode()==NULL){
		vtkSmartPointer<vtkMRMLColorTableNode> colorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
		colorTableNode = vtkMRMLColorTableNode::SafeDownCast(this->GetMRMLScene()->AddNode(colorTableNode));
		
		colorTableNode->SetName("Deformation Field Colors");
		colorTableNode->SetAttribute("Category", "User Generated");
		colorTableNode->SetTypeToUser();
		colorTableNode->SetNumberOfColors(4);
		colorTableNode->GetLookupTable();
		colorTableNode->AddColor("negligible", 0.0, 0.0, 0.0, 0.2);
		colorTableNode->AddColor(       "low", 0.0, 1.0, 0.0, 1.0);
		colorTableNode->AddColor(    "medium", 1.0, 1.0, 0.0, 1.0);
		colorTableNode->AddColor(      "high", 1.0, 0.0, 0.0, 1.0);

		outputModelNode->GetModelDisplayNode()->SetAndObserveColorNodeID(colorTableNode->GetID());
	}
	vtkMRMLColorTableNode::SafeDownCast(outputModelNode->GetModelDisplayNode()->GetColorNode())->GetLookupTable()->SetTableRange(0,field->GetPointData()->GetArray(0)->GetMaxNorm());

	
	//this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);
}

//Glyph visualization
//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerDeformationFieldVisualizerLogic::glyphVisualization(vtkSmartPointer<vtkImageData> field, int sourceOption){
	vtkSmartPointer<vtkDFVGlyph3D> glyphFilter = vtkSmartPointer<vtkDFVGlyph3D>::New();
	glyphFilter->SetPointMax(this->ParameterNode->GetGlyphPointMax());
	glyphFilter->SetSeed(this->ParameterNode->GetGlyphSeed());
	glyphFilter->ScalingOn();
	glyphFilter->SetVectorModeToUseVector();
	glyphFilter->SetScaleModeToScaleByVector();
	glyphFilter->SetMagnitudeMax(this->ParameterNode->GetGlyphThresholdMax());
	glyphFilter->SetMagnitudeMin(this->ParameterNode->GetGlyphThresholdMin());
	glyphFilter->SetScaleFactor(this->ParameterNode->GetGlyphScale());
	glyphFilter->SetScaleDirectional(this->ParameterNode->GetGlyphScaleDirectional());
	glyphFilter->SetColorModeToColorByVector();

	if (sourceOption == 0){ //Arrows
		vtkSmartPointer<vtkArrowSource> arrowSource = vtkSmartPointer<vtkArrowSource>::New();
		arrowSource->SetTipLength(this->ParameterNode->GetGlyphArrowTipLength());
		arrowSource->SetTipRadius(this->ParameterNode->GetGlyphArrowTipRadius());
		arrowSource->SetTipResolution(this->ParameterNode->GetGlyphArrowResolution());
		arrowSource->SetShaftRadius(this->ParameterNode->GetGlyphArrowShaftRadius());
		arrowSource->SetShaftResolution(this->ParameterNode->GetGlyphArrowResolution());
		
		glyphFilter->OrientOn();
		glyphFilter->SetSourceConnection(arrowSource->GetOutputPort());
	}
	else if (sourceOption == 1){ //Cone
		vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
		coneSource->SetHeight(this->ParameterNode->GetGlyphConeHeight());
		coneSource->SetRadius(this->ParameterNode->GetGlyphConeRadius());
		coneSource->SetResolution(this->ParameterNode->GetGlyphConeResolution());
		
		glyphFilter->OrientOn();
		glyphFilter->SetSourceConnection(coneSource->GetOutputPort());
	}
	else if (sourceOption == 2){ //Sphere
		vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
		sphereSource->SetRadius(1);
		sphereSource->SetThetaResolution(this->ParameterNode->GetGlyphSphereResolution());
		sphereSource->SetPhiResolution(this->ParameterNode->GetGlyphSphereResolution());
		
		glyphFilter->OrientOn();
		glyphFilter->SetSourceConnection(sphereSource->GetOutputPort());
	}

	glyphFilter->SetInputConnection(field->GetProducerPort());

	return glyphFilter->GetOutput();
}


//Grid visualization
//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerDeformationFieldVisualizerLogic::gridVisualization(vtkSmartPointer<vtkImageData> field)
{
	double origin[3];
	field->GetOrigin(origin);
	double spacing[3];
	field->GetSpacing(spacing);
	int dimensions[3];
	int dimensions2[3];
	field->GetDimensions(dimensions);
	
	int GridDensity = this->ParameterNode->GetGridDensity();
	
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	points->SetNumberOfPoints(dimensions[0]*dimensions[1]*dimensions[2]);
	vtkSmartPointer<vtkCellArray> grid = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
	
	int i,j,k;
	for (k = 0; k < dimensions[2]; k++){
		for (j = 0; j < dimensions[1]; j++){
			for (i = 0; i < dimensions[0]; i++){
				points->SetPoint(i + j*dimensions[0] + k*dimensions[0]*dimensions[1],
					origin[0] + i + ((spacing[0]-1)*i),origin[1] + j + ((spacing[1]-1)*j),origin[2] + k + ((spacing[2]-1)*k));
			}
		}
	}
	
	for (k = 0; k < dimensions[2]; k++){
		for (j = 0; j < dimensions[1]; j++){
			for (i = 0; i < dimensions[0]; i++){
				if ((i!=0) && (j%GridDensity == 0) && (k%GridDensity == 0) && i<=((dimensions[0]-1)-(dimensions[0]-1)%GridDensity)){
					line->GetPointIds()->SetId(0, (i-1) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
					line->GetPointIds()->SetId(1, (i) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
					grid->InsertNextCell(line);
				}
				
				
				if ((j!=0) && (i%GridDensity == 0) && (k%GridDensity == 0) && j<=((dimensions[1]-1)-(dimensions[1]-1)%GridDensity)){
					line->GetPointIds()->SetId(0, ((j-1)*dimensions[0]+i) + (k*dimensions[0]*dimensions[1]));
					line->GetPointIds()->SetId(1, (j*dimensions[0]+i) + (k*dimensions[0]*dimensions[1]));
					grid->InsertNextCell(line);				
				}
				
				
				if ((k!=0) && (i%GridDensity == 0) && (j%GridDensity == 0) && k<=((dimensions[2]-1)-(dimensions[2]-1)%GridDensity)){
					line->GetPointIds()->SetId(0, (i + ((k-1)*dimensions[0]*dimensions[1])) + (j*dimensions[0]));
					line->GetPointIds()->SetId(1, (i + (k*dimensions[0]*dimensions[1])) + (j*dimensions[0]));
					grid->InsertNextCell(line);				
				}		
			}
		}
	}
	
	vtkSmartPointer<vtkFloatArray> array = vtkFloatArray::SafeDownCast(field->GetPointData()->GetVectors());
	
	vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
	norm->SetInputConnection(field->GetProducerPort());
	norm->Update();
	
	vtkSmartPointer<vtkFloatArray> vectorMagnitude = vtkFloatArray::SafeDownCast(norm->GetOutput()->GetPointData()->GetScalars());
	
	vtkSmartPointer<vtkPolyData> polygrid = vtkSmartPointer<vtkPolyData>::New();
	polygrid->SetPoints(points);
	polygrid->SetLines(grid);
	polygrid->GetPointData()->AddArray(array);
	polygrid->GetPointData()->SetActiveVectors(array->GetName());
	
	vtkSmartPointer<vtkWarpVector> warp = vtkSmartPointer<vtkWarpVector>::New();
	warp->SetInputConnection(polygrid->GetProducerPort());
	warp->SetScaleFactor(this->ParameterNode->GetGridScale());
	
	//Fix up with smart pointers later
	vtkSmartPointer<vtkPolyData> polyoutput = warp->GetPolyDataOutput();
	//vtkPolyData *polyoutput = tubefilter->GetOutput();
	polyoutput->Update();
	polyoutput->GetPointData()->AddArray(vectorMagnitude);
	vectorMagnitude->SetName("VectorMagnitude");
	
	return polyoutput;
}


//Block visualization
//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerDeformationFieldVisualizerLogic::blockVisualization(vtkSmartPointer<vtkImageData> field)
{
	vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
	norm->SetInputConnection(field->GetProducerPort());
	norm->Update();
	vtkSmartPointer<vtkFloatArray> vectorMagnitude = vtkFloatArray::SafeDownCast(norm->GetOutput()->GetPointData()->GetScalars());

	vtkSmartPointer<vtkWarpVector> warp = vtkSmartPointer<vtkWarpVector>::New();
	warp->SetInputConnection(field->GetProducerPort());
	warp->SetScaleFactor(this->ParameterNode->GetBlockScale());

	vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
	geometryFilter->SetInputConnection(warp->GetOutputPort());
	
	vtkSmartPointer<vtkPolyData> polyoutput = geometryFilter->GetOutput();
	polyoutput->Update();
	polyoutput->GetPointData()->AddArray(vectorMagnitude);
	vectorMagnitude->SetName("VectorMagnitude");
	
	//*
	if (this->ParameterNode->GetBlockDisplacementCheck()){
		vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
		normals->SetInput(polyoutput);	
		normals->Update();
		normals->GetOutput()->GetPointData()->SetVectors(field->GetPointData()->GetScalars());
		
		vtkSmartPointer<vtkVectorDot> dot = vtkSmartPointer<vtkVectorDot>::New();
		dot->SetInput(normals->GetOutput());
		dot->Update();
		vtkSmartPointer<vtkFloatArray> vectorDot = vtkFloatArray::SafeDownCast(dot->GetOutput()->GetPointData()->GetScalars());
		
		//normals->GetOutput()->GetPointData()->AddArray(vectorMagnitude);
		//vectorMagnitude->SetName("VectorMagnitude");
		normals->GetOutput()->GetPointData()->AddArray(vectorDot);
		vectorDot->SetName("VectorDot");
		
		return normals->GetOutput();
	}
	//*/
	return polyoutput;
}

//Isosurface visualization
//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerDeformationFieldVisualizerLogic::contourVisualization(vtkSmartPointer<vtkImageData> field)
{
	//Contour by vector magnitude
	vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
	norm->SetInputConnection(field->GetProducerPort());
	norm->Update();
	
	vtkSmartPointer<vtkMarchingCubes> iso = vtkSmartPointer<vtkMarchingCubes>::New();
    iso->SetInputConnection(norm->GetOutputPort());
	iso->ComputeScalarsOn();
	iso->ComputeNormalsOff();
	iso->ComputeGradientsOff();
    iso->GenerateValues(this->ParameterNode->GetContourNumber(), this->ParameterNode->GetContourMin(), this->ParameterNode->GetContourMax());
	iso->Update();
	
	iso->GetOutput()->GetPointData()->GetArray(0)->SetName("VectorMagnitude");
	return iso->GetOutput();
}

//Glyph Slice visualization
//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerDeformationFieldVisualizerLogic::glyphSliceVisualization(vtkSmartPointer<vtkImageData> field, vtkSmartPointer<vtkMatrix4x4> IJKToRASDirections)
{
	double origin[3];
	field->GetOrigin(origin);
	double spacing[3];
	field->GetSpacing(spacing);
	int dimensions[3];
	field->GetDimensions(dimensions);
	int angle = 0;
	vtkSmartPointer<vtkRibbonFilter> ribbon = vtkSmartPointer<vtkRibbonFilter>::New();
	float sliceNormal[3];
	float dot;
	double width = 1;
	
	vtkSmartPointer<vtkImageData> field2 = vtkSmartPointer<vtkImageData>::New();
	field2->DeepCopy(field);
	
	vtkSmartPointer<vtkMRMLSliceNode> sliceNode = NULL;
	vtkSmartPointer<vtkMRMLNode> node = this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetGlyphSliceNodeID());
	if (node){sliceNode = vtkMRMLSliceNode::SafeDownCast(node);}
	
	char* orientation = sliceNode->GetOrientationString();
	if (strcmp(orientation, "Axial") == 0){
		sliceNormal[0] = IJKToRASDirections->Element[0][2];
		sliceNormal[1] = IJKToRASDirections->Element[1][2];
		sliceNormal[2] = IJKToRASDirections->Element[2][2];
		width = spacing[2];
	}
	else if (strcmp(orientation, "Sagittal") == 0){
		sliceNormal[0] = IJKToRASDirections->Element[0][0];
		sliceNormal[1] = IJKToRASDirections->Element[1][0];
		sliceNormal[2] = IJKToRASDirections->Element[2][0];	
		width = spacing[0];		
	}
	else if (strcmp(orientation, "Coronal") == 0){
		sliceNormal[0] = IJKToRASDirections->Element[0][1];
		sliceNormal[1] = IJKToRASDirections->Element[1][1];
		sliceNormal[2] = IJKToRASDirections->Element[2][1];		
		width = spacing[1];
	}
	//Oblique
	else{
		sliceNormal[0] = IJKToRASDirections->Element[0][0]*sliceNode->GetSliceToRAS()->Element[0][2] + 
						 IJKToRASDirections->Element[0][1]*sliceNode->GetSliceToRAS()->Element[1][2] + 
						 IJKToRASDirections->Element[0][2]*sliceNode->GetSliceToRAS()->Element[2][2];
		sliceNormal[1] = IJKToRASDirections->Element[1][0]*sliceNode->GetSliceToRAS()->Element[0][2] + 
						 IJKToRASDirections->Element[1][1]*sliceNode->GetSliceToRAS()->Element[1][2] + 
						 IJKToRASDirections->Element[1][2]*sliceNode->GetSliceToRAS()->Element[2][2];
		sliceNormal[2] = IJKToRASDirections->Element[2][0]*sliceNode->GetSliceToRAS()->Element[0][2] + 
						 IJKToRASDirections->Element[2][1]*sliceNode->GetSliceToRAS()->Element[1][2] + 
						 IJKToRASDirections->Element[2][2]*sliceNode->GetSliceToRAS()->Element[2][2];
	}
	
	//Projection to slice plane
	float *ptr = (float *)field2->GetPointData()->GetScalars()->GetVoidPointer(0);
	for(int i = 0; i < field2->GetPointData()->GetScalars()->GetNumberOfTuples()*3; i+=3){
		dot = ptr[i]*sliceNormal[0] + ptr[i+1]*sliceNormal[1] + ptr[i+2]*sliceNormal[2];
		ptr[i] =  ptr[i] - dot*sliceNormal[0];
		ptr[i+1] =  ptr[i+1] - dot*sliceNormal[1];
		ptr[i+2] =  ptr[i+2] - dot*sliceNormal[2];		
	}
	
	vtkSmartPointer<vtkDataArray> projected = field2->GetPointData()->GetScalars();
	
	vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
	norm->SetInputConnection(field->GetProducerPort());
	norm->Update();
	vtkSmartPointer<vtkDataArray> vectorMagnitude = norm->GetOutput()->GetPointData()->GetScalars();
	
	field2->GetPointData()->AddArray(projected);
	field2->GetPointData()->GetArray(0)->SetName("Projected");	
	field2->GetPointData()->AddArray(vectorMagnitude);
	field2->GetPointData()->GetArray(1)->SetName("OriginalMagnitude");	
	
	ribbon->SetDefaultNormal(sliceNormal[0], sliceNormal[1], sliceNormal[2]);
	
	vtkSmartPointer<vtkTransform> rotateArrow = vtkSmartPointer<vtkTransform>::New();
	rotateArrow->RotateX(vtkMath::DegreesFromRadians(acos(abs(sliceNormal[2]))));
	
	vtkSmartPointer<vtkGlyphSource2D> arrow2DSource = vtkSmartPointer<vtkGlyphSource2D>::New();
	arrow2DSource->SetGlyphTypeToArrow();
	arrow2DSource->SetScale(1);
	arrow2DSource->SetFilled(0);
	
	vtkSmartPointer<vtkDFVGlyph3D> glyphFilter = vtkSmartPointer<vtkDFVGlyph3D>::New();
	glyphFilter->SetPointMax(this->ParameterNode->GetGlyphSlicePointMax());
	glyphFilter->SetSeed(this->ParameterNode->GetGlyphSliceSeed());
	glyphFilter->ScalingOn();
	glyphFilter->SetVectorModeToUseVector();
	glyphFilter->SetScaleModeToScaleByVector();
	glyphFilter->SetMagnitudeMax(this->ParameterNode->GetGlyphSliceThresholdMax());
	glyphFilter->SetMagnitudeMin(this->ParameterNode->GetGlyphSliceThresholdMin());
	glyphFilter->SetScaleFactor(this->ParameterNode->GetGlyphSliceScale());
	glyphFilter->SetScaleDirectional(false);
	glyphFilter->SetColorModeToColorByVector();
	glyphFilter->SetSourceTransform(rotateArrow);

	glyphFilter->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Projected");
	glyphFilter->SetInputArrayToProcess(3, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Original");
	
	glyphFilter->SetSourceConnection(arrow2DSource->GetOutputPort());
	glyphFilter->SetInputConnection(field2->GetProducerPort());
	glyphFilter->Update();
	
	ribbon->SetInputConnection(glyphFilter->GetOutputPort());
	ribbon->SetWidth(width/2);
	ribbon->SetAngle(90.0);
	ribbon->UseDefaultNormalOn();
	
	return ribbon->GetOutput();
}

//Grid Slice visualization
//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerDeformationFieldVisualizerLogic::gridSliceVisualization(vtkSmartPointer<vtkImageData> field, vtkSmartPointer<vtkMatrix4x4> IJKToRASDirections)
{
	float sliceNormal[3];
	float dot;
	vtkSmartPointer<vtkRibbonFilter> ribbon1 = vtkSmartPointer<vtkRibbonFilter>::New();
	vtkSmartPointer<vtkRibbonFilter> ribbon2 = vtkSmartPointer<vtkRibbonFilter>::New();

	int GridDensity = this->ParameterNode->GetGridSliceDensity();
	
	vtkSmartPointer<vtkImageData> field2 = vtkSmartPointer<vtkImageData>::New();
	field2->DeepCopy(field);
	
	double origin[3];
	field2->GetOrigin(origin);	
	double spacing[3];
	field2->GetSpacing(spacing);
	int dimensions[3];
	field2->GetDimensions(dimensions);
	
	float *ptr = (float *)field2->GetPointData()->GetScalars()->GetVoidPointer(0);
	
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	points->SetNumberOfPoints(dimensions[0]*dimensions[1]*dimensions[2]);
	vtkSmartPointer<vtkCellArray> grid = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
	
	vtkSmartPointer<vtkMRMLSliceNode> sliceNode = NULL;
	vtkSmartPointer<vtkMRMLNode> node = this->GetMRMLScene()->GetNodeByID(this->ParameterNode->GetGridSliceNodeID());
	if (node){sliceNode = vtkMRMLSliceNode::SafeDownCast(node);}

	double sliceSpacing[3];
	sliceNode->GetPrescribedSliceSpacing(sliceSpacing);
	double width = 1;
	
	//*
	//Need to ensure later that orientation of slice lines up with orientation of volume; no guarantee of that currently
	char* orientation = sliceNode->GetOrientationString();

		int i,j,k;
		for (k = 0; k < dimensions[2]; k++){
			for (j = 0; j < dimensions[1]; j++){
				for (i = 0; i < dimensions[0]; i++){
					points->SetPoint(i + j*dimensions[0] + k*dimensions[0]*dimensions[1],
						origin[0] + i + ((spacing[0]-1)*i),origin[1] + j + ((spacing[1]-1)*j),origin[2] + k + ((spacing[2]-1)*k));
				}
			}
		}
		
	if (strcmp(orientation, "Axial") == 0){
		sliceNormal[0] = IJKToRASDirections->Element[0][2];
		sliceNormal[1] = IJKToRASDirections->Element[1][2];
		sliceNormal[2] = IJKToRASDirections->Element[2][2];	
		
	}
	else if (strcmp(orientation, "Sagittal") == 0){
		sliceNormal[0] = IJKToRASDirections->Element[0][0];
		sliceNormal[1] = IJKToRASDirections->Element[1][0];
		sliceNormal[2] = IJKToRASDirections->Element[2][0];	
		
	}
	else if (strcmp(orientation, "Coronal") == 0){
		sliceNormal[0] = IJKToRASDirections->Element[0][1];
		sliceNormal[1] = IJKToRASDirections->Element[1][1];
		sliceNormal[2] = IJKToRASDirections->Element[2][1];	
		
	}
	else{
		//Reformat not supported at the moment
		sliceNormal[0] = 0;
		sliceNormal[1] = 0;
		sliceNormal[2] = 0;			
	}	

	std::cout<<sliceNormal[0]<<std::endl;
	std::cout<<sliceNormal[1]<<std::endl;
	std::cout<<sliceNormal[2]<<std::endl;
	
	if (abs(sliceNormal[0]) < 1.0e-15 && abs(sliceNormal[1]) < 1.0e-15 && abs(sliceNormal[2]) > 1.0e-15){
		width = spacing[2];
		for (k = 0; k < dimensions[2]; k++){
			for (j = 0; j < dimensions[1]; j++){
				for (i = 0; i < dimensions[0]; i++){
					if ((i!=0) && (j%GridDensity == 0) && i<=((dimensions[0]-1)-(dimensions[0]-1)%GridDensity)){
						line->GetPointIds()->SetId(0, (i-1) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
						line->GetPointIds()->SetId(1, (i) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
						grid->InsertNextCell(line);
					}
					
					
					if ((j!=0) && (i%GridDensity == 0) && j<=((dimensions[1]-1)-(dimensions[1]-1)%GridDensity)){
						line->GetPointIds()->SetId(0, ((j-1)*dimensions[0]+i) + (k*dimensions[0]*dimensions[1]));
						line->GetPointIds()->SetId(1, (j*dimensions[0]+i) + (k*dimensions[0]*dimensions[1]));
						grid->InsertNextCell(line);				
					}						
				}
			}
		}
	}
	else if (abs(sliceNormal[0]) > 1.0e-15 && abs(sliceNormal[1]) < 1.0e-15 && abs(sliceNormal[2]) < 1.0e-15){
		width = spacing[0];
		for (k = 0; k < dimensions[2]; k++){
			for (j = 0; j < dimensions[1]; j++){
				for (i = 0; i < dimensions[0]; i++){
					if ((j!=0) && (k%GridDensity == 0) && j<=((dimensions[1]-1)-(dimensions[1]-1)%GridDensity)){
						line->GetPointIds()->SetId(0, ((j-1)*dimensions[0]+i) + (k*dimensions[0]*dimensions[1]));
						line->GetPointIds()->SetId(1, (j*dimensions[0]+i) + (k*dimensions[0]*dimensions[1]));
						grid->InsertNextCell(line);				
					}
					
					if ((k!=0) && (j%GridDensity == 0) && k<=((dimensions[2]-1)-(dimensions[2]-1)%GridDensity)){
						line->GetPointIds()->SetId(0, (i + ((k-1)*dimensions[0]*dimensions[1])) + (j*dimensions[0]));
						line->GetPointIds()->SetId(1, (i + (k*dimensions[0]*dimensions[1])) + (j*dimensions[0]));
						grid->InsertNextCell(line);				
					}				
				}
			}
		}
	}
	else if (abs(sliceNormal[0]) < 1.0e-15 && abs(sliceNormal[1]) > 1.0e-15 && abs(sliceNormal[2] < 1.0e-15)){
		width = spacing[1];
		for (k = 0; k < dimensions[2]; k++){
			for (j = 0; j < dimensions[1]; j++){
				for (i = 0; i < dimensions[0]; i++){
					if ((i!=0) && (k%GridDensity == 0) && i<=((dimensions[0]-1)-(dimensions[0]-1)%GridDensity)){
						line->GetPointIds()->SetId(0, (i-1) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
						line->GetPointIds()->SetId(1, (i) + (j*dimensions[0]) + (k*dimensions[0]*dimensions[1]));
						grid->InsertNextCell(line);
					}
					
					if ((k!=0) && (i%GridDensity == 0) && k<=((dimensions[2]-1)-(dimensions[2]-1)%GridDensity)){
						line->GetPointIds()->SetId(0, (i + ((k-1)*dimensions[0]*dimensions[1])) + (j*dimensions[0]));
						line->GetPointIds()->SetId(1, (i + (k*dimensions[0]*dimensions[1])) + (j*dimensions[0]));
						grid->InsertNextCell(line);				
					}				
				}
			}
		}
	}		
	
	//Projection
	for(int i = 0; i < field2->GetPointData()->GetScalars()->GetNumberOfTuples()*3; i+=3){
		dot = ptr[i]*sliceNormal[0] + ptr[i+1]*sliceNormal[1] + ptr[i+2]*sliceNormal[2];
		ptr[i] =  ptr[i] - dot*sliceNormal[0];
		ptr[i+1] =  ptr[i+1] - dot*sliceNormal[1];
		ptr[i+2] =  ptr[i+2] - dot*sliceNormal[2];
	}
	ribbon1->SetDefaultNormal(sliceNormal[0], sliceNormal[1], sliceNormal[2]);
	ribbon2->SetDefaultNormal(sliceNormal[0], sliceNormal[1], sliceNormal[2]);


	vtkSmartPointer<vtkFloatArray> array = vtkFloatArray::SafeDownCast(field2->GetPointData()->GetScalars());

	vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
	norm->SetInputConnection(field->GetProducerPort());
	norm->Update();
	
	vtkSmartPointer<vtkFloatArray> vectorMagnitude = vtkFloatArray::SafeDownCast(norm->GetOutput()->GetPointData()->GetScalars());
	vectorMagnitude->SetName("VectorMagnitude");
	
	vtkSmartPointer<vtkPolyData> polygrid = vtkSmartPointer<vtkPolyData>::New();
	polygrid->SetPoints(points);
	polygrid->SetLines(grid);
	polygrid->GetPointData()->AddArray(array);
	polygrid->GetPointData()->SetActiveVectors(array->GetName());
	
	vtkSmartPointer<vtkWarpVector> warp = vtkSmartPointer<vtkWarpVector>::New();
	warp->SetInputConnection(polygrid->GetProducerPort());
	warp->SetScaleFactor(this->ParameterNode->GetGridSliceScale());
	
	vtkSmartPointer<vtkPolyData> polyoutput = vtkSmartPointer<vtkPolyData>::New();
	polyoutput = warp->GetPolyDataOutput();
	polyoutput->Update();
	polyoutput->GetPointData()->AddArray(vectorMagnitude);
	
	//vtkRibbonFilter
	ribbon1->SetInput(polyoutput);
	ribbon1->SetWidth(width/2);
	ribbon1->SetAngle(90);
	ribbon1->UseDefaultNormalOn();

	return ribbon1->GetOutput();
}


