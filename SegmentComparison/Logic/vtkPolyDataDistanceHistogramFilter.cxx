#include "vtkPolyDataDistanceHistogramFilter.h"

// vtk includes
#include <vtkDataObject.h>
#include <vtkImageAccumulate.h>
#include <vtkImageData.h>
#include <vtkImplicitPolyDataDistance.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataPointSampler.h>
#include <vtkSmartPointer.h>
#include <vtkSortDataArray.h>
#include <vtkStreamingDemandDrivenPipeline.h>

vtkStandardNewMacro(vtkPolyDataDistanceHistogramFilter);

//----------------------------------------------------------------------------
const int vtkPolyDataDistanceHistogramFilter::INPUT_PORT_REFERENCE_POLYDATA = 0;
const int vtkPolyDataDistanceHistogramFilter::INPUT_PORT_COMPARE_POLYDATA = 1;
const int vtkPolyDataDistanceHistogramFilter::OUTPUT_PORT_HISTOGRAM = 0;
//const int vtkPolyDataDistanceHistogramFilter::OUTPUT_PORT_DISTANCES = 1;

//----------------------------------------------------------------------------
vtkPolyDataDistanceHistogramFilter::vtkPolyDataDistanceHistogramFilter()
  : OutputDistances(nullptr)
  , SamplePolyDataVertices(1)
  , SamplePolyDataEdges(0)
  , SamplePolyDataFaces(0)
  , SamplingDistance(0.01)
  , HistogramMinimum(-10.0)
  , HistogramMaximum(10.0)
  , HistogramSpacing(0.2)
{
  this->InputComparePolyData = vtkPolyData::New();
  this->InputReferencePolyData = vtkPolyData::New();
  this->OutputHistogram = vtkTable::New();
  this->OutputDistances = vtkDoubleArray::New();

  //this->SetNumberOfInputPorts(2);
  //this->SetNumberOfOutputPorts(1); // See below why not 2
}

//----------------------------------------------------------------------------
vtkPolyDataDistanceHistogramFilter::~vtkPolyDataDistanceHistogramFilter()
{
  if (this->InputComparePolyData)
  {
    this->InputComparePolyData->Delete();
    this->InputComparePolyData = nullptr;
  }
  if (this->InputReferencePolyData)
  {
    this->InputReferencePolyData->Delete();
    this->InputReferencePolyData = nullptr;
  }
  if (this->OutputHistogram)
  {
    this->OutputHistogram->Delete();
    this->OutputHistogram = nullptr;
  }
  if (this->OutputDistances)
  {
    this->OutputDistances->Delete();
    this->OutputDistances = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkPolyDataDistanceHistogramFilter::SetInputReferencePolyData(vtkPolyData* polyData)
{
  //this->SetInputDataObject(INPUT_PORT_REFERENCE_POLYDATA, polyData);
  this->InputReferencePolyData->DeepCopy(polyData);
}

//----------------------------------------------------------------------------
void vtkPolyDataDistanceHistogramFilter::SetInputComparePolyData(vtkPolyData* polyData)
{
  //this->SetInputDataObject(INPUT_PORT_COMPARE_POLYDATA, polyData);
  this->InputComparePolyData->DeepCopy(polyData);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataDistanceHistogramFilter::GetInputReferencePolyData()
{
  //return vtkPolyData::GetData(this->GetInputInformation(INPUT_PORT_REFERENCE_POLYDATA, 0));
  return this->InputReferencePolyData;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataDistanceHistogramFilter::GetInputComparePolyData()
{
  //return vtkPolyData::GetData(this->GetInputInformation(INPUT_PORT_COMPARE_POLYDATA, 0));
  return this->InputComparePolyData;
}

//----------------------------------------------------------------------------
vtkDoubleArray* vtkPolyDataDistanceHistogramFilter::GetOutputDistances()
{
  return this->OutputDistances;
}

//----------------------------------------------------------------------------
vtkTable* vtkPolyDataDistanceHistogramFilter::GetOutputHistogram()
{
  //return vtkTable::GetData(this->GetOutputInformation(OUTPUT_PORT_HISTOGRAM));
  return this->OutputHistogram;
}

//----------------------------------------------------------------------------
double vtkPolyDataDistanceHistogramFilter::GetMaximumHausdorffDistance()
{
  if (!this->OutputDistances)
  {
    vtkErrorMacro("GetMaximumHausdorffDistance: Output distances has not been created! Need to call Update after setting the inputs.");
    return 0.0;
  }

  return this->OutputDistances->GetMaxNorm();
}
  
//----------------------------------------------------------------------------
double vtkPolyDataDistanceHistogramFilter::GetAverageHausdorffDistance()
{
  if (!this->OutputDistances)
  {
    vtkErrorMacro("GetAverageHausdorffDistance: Output distances has not been created! Need to call Update after setting the inputs.");
    return 0.0;
  }

  double sum = 0.0;
  int numberOfDistances = this->OutputDistances->GetNumberOfValues();
  for (int distanceIndex=0; distanceIndex<numberOfDistances; ++distanceIndex)
  {
    sum += this->OutputDistances->GetValue(distanceIndex);
  }

  return sum / (double)numberOfDistances;
}
  
//----------------------------------------------------------------------------
double vtkPolyDataDistanceHistogramFilter::GetStandardDeviationHausdorffDistance()
{
  if (!this->OutputDistances)
  {
    vtkErrorMacro("GetStandardDeviationHausdorffDistance: Output distances has not been created! Need to call Update after setting the inputs.");
    return 0.0;
  }

  double averageDistance = this->GetAverageHausdorffDistance();
  double sumOfSquaredDifferencesFromAverage = 0.0;
  int numberOfDistances = this->OutputDistances->GetNumberOfValues();
  for (int distanceIndex=0; distanceIndex<numberOfDistances; ++distanceIndex)
  {
    double distance = this->OutputDistances->GetValue(distanceIndex);
    double differenceFromAverage = averageDistance - distance;
    double squaredDifferenceFromAverage = pow(differenceFromAverage, 2);
    sumOfSquaredDifferencesFromAverage += squaredDifferenceFromAverage;
  }

  double averageDifferenceFromAverage = sumOfSquaredDifferencesFromAverage / (double)numberOfDistances;
  double stdev = sqrt(averageDifferenceFromAverage);
  return stdev;
}
  
//----------------------------------------------------------------------------
double vtkPolyDataDistanceHistogramFilter::GetPercent95HausdorffDistance()
{
  if (!this->OutputDistances)
  {
    vtkErrorMacro("GetPercent95HausdorffDistance: Output distances has not been created! Need to call Update after setting the inputs.");
    return 0.0;
  }

  return this->GetNthPercentileHausdorffDistance( 95.0 );
}

//----------------------------------------------------------------------------
double vtkPolyDataDistanceHistogramFilter::GetNthPercentileHausdorffDistance(double n)
{
  if (!this->OutputDistances)
  {
    vtkErrorMacro("GetPercentNthHausdorffDistance: Output distances has not been created! Need to call Update after setting the inputs.");
    return 0.0;
  }

  if (n < 0)
  {
    vtkErrorMacro("GetPercentNthHausdorffDistance: N " << n << " must be equal to or greater than 0. Returning 0.0.");
    return 0.0;
  }

  if (n > 100)
  {
    vtkErrorMacro("GetPercentNthHausdorffDistance: N " << n << " must be equal to or less than 100. Returning 0.0.");
    return 0.0;
  }

  vtkSmartPointer<vtkDoubleArray> sortedDistances = vtkSmartPointer<vtkDoubleArray>::New();
  sortedDistances->DeepCopy(this->OutputDistances);

  vtkSmartPointer<vtkSortDataArray> sortDataArray = vtkSmartPointer<vtkSortDataArray>::New();
  sortDataArray->Sort(sortedDistances);

  int nthPercentileIndex = vtkMath::Round( (n/ 100) * (sortedDistances->GetNumberOfValues() - 1) );
  double percentileNthDistance = sortedDistances->GetValue( nthPercentileIndex );
  return percentileNthDistance;
}

//----------------------------------------------------------------------------
//int vtkPolyDataDistanceHistogramFilter::FillInputPortInformation(int, vtkInformation *info)
//{
//  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
//  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
//  return 1;
//}

//----------------------------------------------------------------------------
//   vtkDoubleArray is not specified in vtkDataObjectTypes so it does not seem to be supported as an algorithm output.
//   So this algorithm is made one-output and can get the double array separately
//   Since this is a subclass of vtkTableAlgorithm, and the only output is now vtkTable,
//   we do not need to specify the output port information.
//----------------------------------------------------------------------------
//int vtkPolyDataDistanceHistogramFilter::FillOutputPortInformation(int port, vtkInformation* info)
//{
//  if (port == OUTPUT_PORT_HISTOGRAM)
//  {
//    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
//    return 1;
//  }
//  else if (port == OUTPUT_PORT_DISTANCES)
//  {
//    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDoubleArray");
//    return 1;
//  }
//  return 0;
//}

//----------------------------------------------------------------------------
void vtkPolyDataDistanceHistogramFilter::ComputeDistances(vtkPolyData* referencePolyData, vtkPolyData* comparePolyData, vtkDoubleArray* distanceArray)
{
  //TODO: Revise the role of reference and compare

  // generate the points at which to sample the distance
  vtkSmartPointer<vtkPolyDataPointSampler> pointSampler = vtkSmartPointer<vtkPolyDataPointSampler>::New();
  pointSampler->SetGenerateVertexPoints(this->SamplePolyDataVertices);
  pointSampler->SetGenerateEdgePoints(this->SamplePolyDataEdges);
  pointSampler->SetGenerateInteriorPoints(this->SamplePolyDataFaces);
  pointSampler->SetDistance(this->SamplingDistance);
  pointSampler->SetInputData(comparePolyData);
  pointSampler->Update();  
  vtkPoints* samplingPoints = pointSampler->GetOutput()->GetPoints();
  
  // generate the distance field
  vtkSmartPointer<vtkImplicitPolyDataDistance> distanceField = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
  distanceField->SetInput(referencePolyData);
  
  int numPoints = samplingPoints->GetNumberOfPoints();
  for (int i = 0; i < numPoints; i++)
  {
    double samplePoint[3];
    samplingPoints->GetPoint(i,samplePoint);
    double newDistance = distanceField->EvaluateFunction(samplePoint);
    distanceArray->InsertNextTuple1(newDistance);
  }
}


//----------------------------------------------------------------------------
// DO NOT run anything in this function within the pipeline. This function
// itself uses the vtk pipeline, which results in an unstable "mini-pipeline"
// which can behave unpredictably in VTK.
//----------------------------------------------------------------------------
void vtkPolyDataDistanceHistogramFilter::Update()
{
  // get the input data objects
  vtkPolyData* inputPolyDataReference = this->GetInputReferencePolyData();
  vtkPolyData* inputPolyDataCompare = this->GetInputComparePolyData();

  vtkSmartPointer<vtkDoubleArray> distances = vtkSmartPointer<vtkDoubleArray>::New(); // hold the distances in this array until we copy to the output
  distances->SetName("Distances");
  this->ComputeDistances(inputPolyDataReference, inputPolyDataCompare, distances);
  
  // copy the distances into a dummy image
  vtkSmartPointer<vtkImageData> dummyImage = vtkSmartPointer<vtkImageData>::New();
  int numberOfDoubleValues = distances->GetNumberOfTuples();
  dummyImage->SetDimensions(numberOfDoubleValues,1,1);
  dummyImage->AllocateScalars(VTK_DOUBLE,1);
  double* distancesPointer = static_cast<double*>(distances->GetVoidPointer(0));
  double* dummyImagePtr = static_cast<double*>(dummyImage->GetScalarPointer());
  for (long i=0; i < numberOfDoubleValues; ++i)
  {
    (*dummyImagePtr) = (*distancesPointer);
    ++distancesPointer;
    ++dummyImagePtr;
  }

  // set up the image accumulator for building the histogram
  vtkSmartPointer<vtkImageAccumulate> imageAccumulator = vtkSmartPointer<vtkImageAccumulate>::New();
  imageAccumulator->SetInputData(dummyImage);
  double histogramBinMinimumArray[3] = {this->HistogramMinimum, 0.0, 0.0}; // the first component contains the minimum, other values are unused
  imageAccumulator->SetComponentOrigin(histogramBinMinimumArray);
  double histogramBinSpacingArray[3] = {this->HistogramSpacing, 1.0, 1.0}; // the first component contains the spacing, other values are unused
  imageAccumulator->SetComponentSpacing(histogramBinSpacingArray);
  int histogramBinExtent = vtkMath::Ceil((this->HistogramMaximum - this->HistogramMinimum) / this->HistogramSpacing);
  int histogramBinExtentArray[6] = {0, histogramBinExtent, 0, 0, 0, 0}; // the first and second values contain the extent, others are unused
  imageAccumulator->SetComponentExtent(histogramBinExtentArray);
  imageAccumulator->Update();

  // create the bin array
  vtkSmartPointer<vtkDoubleArray> bins = vtkSmartPointer<vtkDoubleArray>::New();
  bins->SetName("Bins");
  for (int i = 0; i <= histogramBinExtent; i++)
  {
    double newBinValue = this->HistogramMinimum + (i * this->HistogramSpacing);
    bins->InsertNextTuple1(newBinValue);
  }

  // create the frequencies array
  vtkImageData* frequencyImage = imageAccumulator->GetOutput();
  vtkSmartPointer<vtkIntArray> frequencies = vtkSmartPointer<vtkIntArray>::New();
  frequencies->SetName("Frequencies");
  for (int i = 0; i <= histogramBinExtent; i++)
  {
    int newFrequencyValue = (int)(frequencyImage->GetScalarComponentAsDouble(i,0,0,0));
    frequencies->InsertNextTuple1(newFrequencyValue);
  }

  // combine the bins and frequencies into a histogram
  vtkSmartPointer<vtkTable> histogram = vtkSmartPointer<vtkTable>::New();
  histogram->AddColumn(bins);
  histogram->AddColumn(frequencies);

  // output the distances
  //vtkInformation* outputInfoDistances = outputVector->GetInformationObject(OUTPUT_PORT_DISTANCES);
  //vtkDoubleArray* outputDistances = vtkDoubleArray::SafeDownCast(outputInfoHistogram->Get(vtkDataObject::DATA_OBJECT()));
  //outputDistances->DeepCopy(distances);
  this->OutputDistances->DeepCopy(distances);

  // output the histogram
  this->OutputHistogram->DeepCopy(histogram);
}
