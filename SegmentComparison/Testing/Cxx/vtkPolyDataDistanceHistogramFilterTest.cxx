// Module includes
#include "vtkPolyDataDistanceHistogramFilter.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// Slicer includes
#include "vtkMRMLScene.h"
#include "qSlicerCoreApplication.h"

// VTK includes
#include <vtkDelimitedTextWriter.h>
#include <vtkDoubleArray.h>
#include <vtkSphereSource.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>

//-----------------------------------------------------------------------------
int vtkPolyDataDistanceHistogramFilterTest( int argc, char* argv[] )
{
  std::ostream& outputStream = std::cout;
  std::ostream& errorStream = std::cerr;

  int argIndex = 1;
  const char *rawDistancesFilename = nullptr;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-RawDistancesPath") == 0)
    {
      rawDistancesFilename = argv[argIndex+1];
      outputStream << "Raw distances file name: " << rawDistancesFilename << std::endl;
      argIndex += 2;
    }
    else
    {
      rawDistancesFilename = "";
    }
  }
  else
  {
    std::cerr << "Missing arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *histogramFilename = nullptr;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-HistogramPath") == 0)
    {
      histogramFilename = argv[argIndex+1];
      outputStream << "Histogram file name: " << histogramFilename << std::endl;
      argIndex += 2;
    }
    else
    {
      histogramFilename = "";
    }
  }
  else
  {
    std::cerr << "Missing arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  vtkSmartPointer< vtkSphereSource > sphereSource1 = vtkSmartPointer< vtkSphereSource >::New();
  sphereSource1->SetRadius( 1.0 );
  double center1[ 3 ] = { 0, 0, 0 };
  sphereSource1->SetCenter( center1 );
  sphereSource1->Update();

  vtkSmartPointer< vtkSphereSource > sphereSource2 = vtkSmartPointer< vtkSphereSource >::New();
  sphereSource2->SetRadius( 1.0 );
  double center2[ 3 ] = { 0.5, 0, 0 };
  sphereSource2->SetCenter( center2 );
  sphereSource2->Update();

  vtkSmartPointer< vtkPolyDataDistanceHistogramFilter > polyDataDistanceHistogramFilter = vtkSmartPointer< vtkPolyDataDistanceHistogramFilter >::New();
  polyDataDistanceHistogramFilter->SetInputReferencePolyData( sphereSource1->GetOutput() );
  polyDataDistanceHistogramFilter->SetInputComparePolyData( sphereSource2->GetOutput() );
  polyDataDistanceHistogramFilter->SetSamplePolyDataVertices( 1 );
  polyDataDistanceHistogramFilter->SetSamplePolyDataEdges( 1 );
  polyDataDistanceHistogramFilter->SetSamplePolyDataFaces( 1 );
  polyDataDistanceHistogramFilter->SetSamplingDistance( 0.025 );
  polyDataDistanceHistogramFilter->SetHistogramMinimum( -0.5 );
  polyDataDistanceHistogramFilter->SetHistogramMaximum( 0.5 );
  polyDataDistanceHistogramFilter->SetHistogramSpacing( 0.05 );
  polyDataDistanceHistogramFilter->Update();

  // Export distances to text file for comparison against python
  vtkDoubleArray* rawDistancesDoubleArray = polyDataDistanceHistogramFilter->GetOutputDistances();
  if ( rawDistancesDoubleArray == nullptr )
  {
    errorStream << "Distances are null. Aborting test." << std::endl;
    return EXIT_FAILURE;
  }

  vtkSmartPointer< vtkTable > rawDistancesInTable = vtkSmartPointer< vtkTable >::New();
  rawDistancesInTable->AddColumn( rawDistancesDoubleArray );

  outputStream << "Start setting up raw distance writer, destination: " << rawDistancesFilename << std::endl;
  vtkSmartPointer< vtkDelimitedTextWriter > rawDistancesWriter = vtkSmartPointer< vtkDelimitedTextWriter >::New();
  rawDistancesWriter->SetInputData( rawDistancesInTable );
  rawDistancesWriter->SetFileName( rawDistancesFilename );
  rawDistancesWriter->Write();

  // Export histogram
  vtkTable* histogramInTable = polyDataDistanceHistogramFilter->GetOutputHistogram();
  if ( histogramInTable == nullptr )
  {
    errorStream << "Histogram is null." << std::endl;
    return EXIT_FAILURE;
  }

  outputStream << "Start setting up histogram writer, destination: " << histogramFilename << std::endl;
  vtkSmartPointer< vtkDelimitedTextWriter > histogramWriter = vtkSmartPointer< vtkDelimitedTextWriter >::New();
  histogramWriter->SetInputData( histogramInTable );
  histogramWriter->SetFileName( histogramFilename );
  histogramWriter->Write();

  return EXIT_SUCCESS;
}
