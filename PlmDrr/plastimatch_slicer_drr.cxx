/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// SlicerRT includes
#include <vtkSlicerRtCommon.h>
#include <vtkSlicerPlanarImageModuleLogic.h>
#include <vtkMRMLPlanarImageNode.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLLinearTransformNode.h>

// SlicerRT MRML includes
#include <vtkMRMLRTBeamNode.h>
#include <vtkMRMLRTPlanNode.h>

// VTK includes
#include <vtkTransform.h>
#include <vtkGeneralTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// ITK includes
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkMetaImageIO.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkPluginUtilities.h>

#include "plastimatch_slicer_drrCLP.h"

// Plastimatch includes
#include "plmreconstruct_config.h"
#include "drr_options.h"
#include "drr.h"
#include "plm_math.h"
#include "threading.h"

// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()
//
namespace
{

int DoSetupDRR( int argc, char * argv[], Drr_options& options) throw(std::string)
{
  PARSE_ARGS;

  // Single image mode, no geometry
  options.have_angle_diff = false;
  options.num_angles = 1;
  options.start_angle = 0.;
  options.geometry_only = 0;

  // Imager MUST be with image window and image center and image resolution
  options.have_image_center = 1;
  options.have_image_window = useImageWindow;
  options.detector_resolution[0] = imagerResolution[0]; // columns
  options.detector_resolution[1] = imagerResolution[1]; // rows
  options.image_size[0] = imagerResolution[0] * imagerSpacing[0]; // columns
  options.image_size[1] = imagerResolution[1] * imagerSpacing[1]; // rows
  if (useImageWindow)
  {
    options.image_window[0] = std::max<int>( 0, imageWindow[0]); // start column
    options.image_window[1] = std::min<int>( imagerResolution[0] - 1, imageWindow[2]); // end column
    options.image_window[2] = std::max<int>( 0, imageWindow[1]); // start row
    options.image_window[3] = std::min<int>( imagerResolution[1] - 1, imageWindow[3]); // end row
  }
  else
  {
    options.image_window[0] = 0; // start column
    options.image_window[1] = imagerResolution[0] - 1; // end column
    options.image_window[2] = 0; // start row
    options.image_window[3] = imagerResolution[1] - 1; // end row
  }
  options.image_center[0] = options.image_window[0] + (options.image_window[1] - options.image_window[0]) / 2.f; // column
  options.image_center[1] = options.image_window[2] + (options.image_window[3] - options.image_window[2]) / 2.f; // row
  options.image_resolution[0] = options.image_window[1] - options.image_window[0] + 1; // columns
  options.image_resolution[1] = options.image_window[3] - options.image_window[2] + 1; // rows

  // geometry
  // vup and normal vectors
  options.vup[0] = viewUpVector[0];
  options.vup[1] = viewUpVector[1];
  options.vup[2] = viewUpVector[2];

  options.have_nrm = 1;
  options.nrm[0] = normalVector[0];
  options.nrm[1] = normalVector[1];
  options.nrm[2] = normalVector[2];

  if (sourceImagerDistance < sourceAxisDistance)
  {
    throw std::string("SID is less than SAD");
  }
  options.sad = sourceAxisDistance;
  options.sid = sourceImagerDistance;

  // Isocenter DICOM (LPS) position
  options.isocenter[0] = isocenterPosition[0];
  options.isocenter[1] = isocenterPosition[1];
  options.isocenter[2] = isocenterPosition[2];

  // intensity
  options.exponential_mapping = static_cast<int>(exponentialMapping);
  options.autoscale = autoscale;
  if (autoscaleRange[0] >= autoscaleRange[1])
  {
    throw std::string("Autoscale range is wrong");
  }
  options.autoscale_range[0] = autoscaleRange[0];
  options.autoscale_range[1] = autoscaleRange[1];

  // processing
  options.threading = threading_parse(threading);

  if (!huconversion.compare("preprocess"))
  {
    options.hu_conversion = PREPROCESS_CONVERSION;
  }
  else if (!huconversion.compare("inline"))
  {
    options.hu_conversion = INLINE_CONVERSION;
  }
  else if (!huconversion.compare("none"))
  {
    options.hu_conversion = NO_CONVERSION;
  }
  else
  {
    throw std::string("Wrong huconversion value");
  }

  if (!algorithm.compare("exact"))
  {
    options.algorithm = DRR_ALGORITHM_EXACT;
  }
  else if (!algorithm.compare("uniform"))
  {
    options.algorithm = DRR_ALGORITHM_UNIFORM;
  }
  else
  {
    throw std::string("Wrong algorithm value");
  }

  if (!outputFormat.compare("pgm"))
  {
    options.output_format = OUTPUT_FORMAT_PGM;
  }
  else if (!outputFormat.compare("pfm"))
  {
    options.output_format = OUTPUT_FORMAT_PFM;
  }
  else if (!outputFormat.compare("raw"))
  {
    options.output_format = OUTPUT_FORMAT_RAW;
  }
  else
  {
    throw std::string("Wrong output file format");
  }

  return EXIT_SUCCESS;
}

template <typename TPixel>
int DoIt( int argc, char * argv[], Drr_options& options, TPixel ) throw(itk::ExceptionObject)
{
  PARSE_ARGS;

  using InputPixelType = TPixel; // CT pixel type (short)
  const unsigned int Dimension = 3;

  // CT image type and reader
  using InputImageType = itk::Image< InputPixelType, Dimension>;
  using InputReaderType = itk::ImageFileReader<InputImageType>;
  
  typename InputReaderType::Pointer inputReader = InputReaderType::New();
  inputReader->SetFileName( inputVolume.c_str() );

  // MetaImageHeader temporary writer
  using InputWriterType = itk::ImageFileWriter<InputImageType>;
  typename InputWriterType::Pointer inputWriter = InputWriterType::New();

  // Input and output files options
  std::string mhdFilename;
  std::size_t found = inputVolume.find_last_of("/\\");
  if (found < inputVolume.size() - 1)
  {
    std::string tmpDir = inputVolume.substr( 0, found + 1);
    options.input_file = tmpDir + "inputVolume.mha";
    options.output_file = tmpDir + "outputVolume.raw";
    mhdFilename = tmpDir + "outputVolume.mhd";

    // setup meta file info for raw file
    std::ofstream ofs(mhdFilename.c_str());
    ofs << "NDims = " << Dimension << "\n";
    ofs << "DimSize = " << options.image_resolution[0] << " " << options.image_resolution[1] << " 1\n"; // x (columns), y (rows), 1
//    float spacingColumns = options.image_size[0] / float(options.detector_resolution[0]);
//    float spacingRows = options.image_size[1] / float(options.detector_resolution[1]);
    ofs << "ElementSpacing = " << imagerSpacing[0] << " " << imagerSpacing[1] << " 1\n"; // x (columns), y (rows), 1
    ofs << "Position = 0 0 0\n";
    ofs << "BinaryData = True\n";
    ofs << "ElementByteOrderMSB = False\n";
    ofs << "ElementType = MET_LONG\n";
    ofs << "ElementDataFile = outputVolume.raw\n";
    ofs.close();
  }
  else if (found == inputVolume.size() or found == std::string::npos)
  {
    throw std::string("Unable to find directory name");
  }

  inputWriter->SetFileName(options.input_file.c_str());
  inputWriter->SetInput(inputReader->GetOutput());
  try
  {
    inputWriter->Update();
  }
  catch ( itk::ExceptionObject & excep )
  {
    throw;
  }

  // Compute DRR image if everything is OK
  drr_compute(&options);

  // Plastimatch DRR pixel type (long)
  using PlmDrrPixelType = signed long int;
  using PlmDrrImageType = itk::Image< PlmDrrPixelType, Dimension >;
  using PlmDrrReaderType = itk::ImageFileReader< PlmDrrImageType >;

  // read mhd file
  typename PlmDrrReaderType::Pointer drrReader = PlmDrrReaderType::New();
  drrReader->SetFileName(mhdFilename.c_str());

  // rescale to autoscale range
  using RescaleFilterType = itk::RescaleIntensityImageFilter< PlmDrrImageType, PlmDrrImageType >;
  typename RescaleFilterType::Pointer rescale = RescaleFilterType::New();
  rescale->SetOutputMinimum(options.autoscale_range[0]);
  rescale->SetOutputMaximum(options.autoscale_range[1]);
  rescale->SetInput(drrReader->GetOutput());

  // invert (optional?)
  using InvertFilterType = itk::InvertIntensityImageFilter< PlmDrrImageType, PlmDrrImageType >;
  InvertFilterType::Pointer invert = InvertFilterType::New();
  invert->SetInput(rescale->GetOutput());
  invert->SetMaximum(options.autoscale_range[0]);

  // write data into Slicer
  using WriterType = itk::ImageFileWriter< PlmDrrImageType >;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputVolume.c_str() );
  writer->SetInput( invert->GetOutput() );
//  writer->SetUseCompression(1);

  try
  {
    writer->Update();
  }
  catch ( itk::ExceptionObject & excep )
  {
    throw;
  }

  return EXIT_SUCCESS;
}

} // end of anonymous namespace

int main( int argc, char * argv[] )
{
  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType     pixelType;
  itk::ImageIOBase::IOComponentType componentType;

  try
  {
    // Since there are no conditional flags, EVERY options must be 
    // explicitly defined and checked.
    Drr_options drrOptions;
    DoSetupDRR( argc, argv, drrOptions);

    itk::GetImageType(inputVolume, pixelType, componentType);

    // This filter handles all types on input, but only produces
    // signed types
    switch( componentType )
    {
    case itk::ImageIOBase::UCHAR:
      return DoIt( argc, argv, drrOptions, static_cast<unsigned char>(0) );
      break;
    case itk::ImageIOBase::CHAR:
      return DoIt( argc, argv, drrOptions, static_cast<signed char>(0) );
      break;
    case itk::ImageIOBase::USHORT:
      return DoIt( argc, argv, drrOptions, static_cast<unsigned short>(0) );
      break;
    case itk::ImageIOBase::SHORT: // CT data, everything else is a mistake
      return DoIt( argc, argv, drrOptions, static_cast<short>(0) );
      break;
    case itk::ImageIOBase::UINT:
      return DoIt( argc, argv, drrOptions, static_cast<unsigned int>(0) );
      break;
    case itk::ImageIOBase::INT:
      return DoIt( argc, argv, drrOptions, static_cast<int>(0) );
      break;
    case itk::ImageIOBase::ULONG:
      return DoIt( argc, argv, drrOptions, static_cast<unsigned long>(0) );
      break;
    case itk::ImageIOBase::LONG:
      return DoIt( argc, argv, drrOptions, static_cast<long>(0) );
      break;
    case itk::ImageIOBase::FLOAT:
      return DoIt( argc, argv, drrOptions, static_cast<float>(0) );
      break;
    case itk::ImageIOBase::DOUBLE:
      return DoIt( argc, argv, drrOptions, static_cast<double>(0) );
      break;
    case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
    default:
      std::cerr << "Unknown input image pixel component type: ";
      std::cerr << itk::ImageIOBase::GetComponentTypeAsString( componentType );
      std::cerr << std::endl;
      return EXIT_FAILURE;
      break;
    }
  }
  catch( itk::ExceptionObject & excep )
  {
    std::cerr << argv[0] << ": ITK exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
  }
  catch( const std::exception& ex )
  {
    std::cerr << argv[0] << ": std exception caught !" << std::endl;
    std::cerr << "Error message: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
  catch( const std::string& errorString )
  {
    std::cerr << argv[0] << ": exception message caught !" << std::endl;
    std::cerr << "Error message: " << errorString << std::endl;
    return EXIT_FAILURE;
  }
  catch( ... )
  {
    std::cerr << "Disaster! Unknown exception caught!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
