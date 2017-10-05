// module includes
#include "vtkPolyDataDistanceHistogramFilter.h"

// vtk includes
#include <vtkMRMLScene.h>
#include <vtkTable.h>
#include <vtkSphereSource.h>


//-----------------------------------------------------------------------------
int vtkPolyDataDistanceHistogramFilterTest( int vtkNotUsed( argc ), char* vtkNotUsed( argv )[] )
{
  std::ostream& outputStream = std::cout;
  std::ostream& errorStream = std::cerr;
  
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
  polyDataDistanceHistogramFilter->SetSamplingDistance( 0.01 );
  polyDataDistanceHistogramFilter->SetHistogramMinimum( -0.5 );
  polyDataDistanceHistogramFilter->SetHistogramMaximum( 0.5 );
  polyDataDistanceHistogramFilter->SetHistogramSpacing( 0.05 );
  polyDataDistanceHistogramFilter->Update();

  // "ground truth" may be a bit misleading. These values were collected
  // by running this code through the Python interactor on Oct 5 2017.
  const int numGroundTruthFrequencies = 20;
  int groundTruthFrequencies[ numGroundTruthFrequencies ] = { 94, 330, 136, 240, 180, 172, 640, 392, 576, 58534, 97079, 3660, 2258, 2840, 2024, 1882, 1736, 971, 1214, 702 };
  vtkTable* histogramTable = polyDataDistanceHistogramFilter->GetOutputHistogram();

  if ( histogramTable == NULL )
  {
    errorStream << "Histogram is null!" << std::endl;
    return EXIT_FAILURE;
  }

  bool mismatchFound = false;
  for ( int i = 1; i < numGroundTruthFrequencies; i++ )
  {
    int computedFrequecy = histogramTable->GetValue( i, 1 ).ToInt();
    outputStream << "Comparing " << computedFrequecy << " and " << groundTruthFrequencies[ i ] << std::endl;
    if ( computedFrequecy != groundTruthFrequencies[ i ] )
    {
      mismatchFound = true;
    }
  }

  if ( mismatchFound )
  {
    errorStream << "Mismatch found!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
