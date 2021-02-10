/*==============================================================================

  Program: 3D Slicer, Plastimatch

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.
  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/


// ITK includes
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkMetaImageIO.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkThresholdImageFilter.h>
#include <itkPluginUtilities.h>

// Plastimatch includes
#include <plmreconstruct_config.h>
#include <drr_options.h>
#include <drr.h>
#include <threading.h>

#include "plastimatch_slicer_drrCLP.h"

// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()
//
namespace
{

int DoSetupDRR( int argc, char * argv[], Drr_options& options ) throw( std::string )
{
  PARSE_ARGS;

  // Single image mode, no geometry
  options.have_angle_diff = 0;
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
int DoIt( int argc, char * argv[], Drr_options& options, TPixel ) throw( std::string, itk::ExceptionObject )
{
  PARSE_ARGS;

  typedef TPixel InputPixelType; // CT pixel type (short)
  const unsigned int Dimension = 3;

  // CT image type and reader
  using InputImageType = itk::Image< InputPixelType, Dimension >;
  using InputReaderType = itk::ImageFileReader< InputImageType >;
  // Read CT data
  typename InputReaderType::Pointer inputReader = InputReaderType::New();
  inputReader->SetFileName( inputVolume.c_str() );
  inputReader->Update();

  // Apply threshold filter if HU thresholdBelow is higher than -1000
  typename InputImageType::Pointer inputImagePointer = inputReader->GetOutput();
  using ThresholdFilterType = itk::ThresholdImageFilter< InputImageType >;
  typename ThresholdFilterType::Pointer thresholdFilter = ThresholdFilterType::New();

  if (thresholdBelow > -1000)
  {
    thresholdFilter->SetOutsideValue(static_cast< TPixel >(-1000.));
    thresholdFilter->ThresholdBelow(thresholdBelow);
    thresholdFilter->SetInput(inputImagePointer);

    try
    {
      thresholdFilter->Update();
    }
    catch ( itk::ExceptionObject& excep )
    {
      throw;
    }
    inputImagePointer = thresholdFilter->GetOutput();
  }

  // Save CT data as MetaImageHeader (*.mha)
  using InputWriterType = itk::ImageFileWriter< InputImageType >;
  typename InputWriterType::Pointer inputWriter = InputWriterType::New();

  // Input and output files options
  std::size_t found = inputVolume.find_last_of("/\\");
  std::string mhdFilename;
  if (found < inputVolume.size() && options.output_format == OUTPUT_FORMAT_RAW)
  {
    std::string tmpDir = inputVolume.substr( 0, found + 1);
    options.input_file = tmpDir + "inputVolume.mha";
    options.output_file = tmpDir + "outputVolume.raw";
    mhdFilename = tmpDir + "outputVolume.mhd";
  }
  else if (found == inputVolume.size() || found == std::string::npos)
  {
    throw std::string("Unable to find directory name");
  }

  inputWriter->SetFileName(options.input_file.c_str());
  inputWriter->SetInput(inputImagePointer);
  try
  {
    inputWriter->Update();
  }
  catch ( itk::ExceptionObject& excep )
  {
    throw;
  }

  // Compute DRR image if everything is OK
  drr_compute(&options);

  // Create mhd file for raw file loading
  if (!mhdFilename.empty())
  {
    // Plastimatch DRR pixel type (float)
    typedef float PlmDrrPixelType;

    size_t imageSize = options.image_resolution[0] * options.image_resolution[1] * sizeof(PlmDrrPixelType);
    size_t rawSize = 0;

    // check that raw file is exists and has a proper size 
    std::ifstream ifs( options.output_file.c_str(), std::ifstream::binary);
    if (ifs)
    {
      // get length of file
      ifs.seekg( 0, ifs.end);
      rawSize = ifs.tellg();
      ifs.close();
    }

    if (rawSize == imageSize)
    {
      // setup meta file info for raw file
      std::ofstream ofs(mhdFilename.c_str());
      ofs << "NDims = " << Dimension << "\n";
      ofs << "DimSize = " << options.image_resolution[0] << " " << options.image_resolution[1] << " 1\n"; // x (columns), y (rows), 1
      float imageSpacing_[2] = {
        options.image_size[0] / float(options.detector_resolution[0]),
        options.image_size[1] / float(options.detector_resolution[1])
      };
      ofs << "ElementSpacing = " << imageSpacing_[0] << " " << imageSpacing_[1] << " 1\n"; // x (columns), y (rows), 1
      ofs << "Position = 0 0 0\n";
      ofs << "BinaryData = True\n";
      ofs << "ElementByteOrderMSB = False\n";
      ofs << "ElementType = MET_FLOAT\n";
      ofs << "ElementDataFile = outputVolume.raw\n";
      ofs.close();
    }
    else
    {
      throw std::string("Raw file and DRR image has different sizes");
    }

    using PlmDrrImageType = itk::Image< PlmDrrPixelType, Dimension >;

    // read mhd file
    using PlmDrrReaderType = itk::ImageFileReader< PlmDrrImageType >;
    PlmDrrReaderType::Pointer drrReader = PlmDrrReaderType::New();
    drrReader->SetFileName(mhdFilename.c_str());

    // Transform DRR image (range, invert)
    // rescale to autoscale range
    using RescaleFilterType = itk::RescaleIntensityImageFilter< PlmDrrImageType, PlmDrrImageType >;
    RescaleFilterType::Pointer rescale = RescaleFilterType::New();
    rescale->SetOutputMinimum(options.autoscale_range[0]);
    rescale->SetOutputMaximum(options.autoscale_range[1]);
    rescale->SetInput(drrReader->GetOutput());

    // write data into Slicer
    using WriterType = itk::ImageFileWriter< PlmDrrImageType >;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( outputVolume.c_str() );
    writer->SetUseCompression(1);

    if (invertIntensity)
    {
      // invert
      using InvertFilterType = itk::InvertIntensityImageFilter< PlmDrrImageType, PlmDrrImageType >;
      InvertFilterType::Pointer invert = InvertFilterType::New();
      invert->SetInput(rescale->GetOutput());
      invert->SetMaximum(options.autoscale_range[0]);

      // inverted input
      writer->SetInput( invert->GetOutput() );

      try
      {
        writer->Update();
      }
      catch ( itk::ExceptionObject& excep )
      {
        throw;
      }
    }
    else
    {
      writer->SetInput( rescale->GetOutput() );

      try
      {
        writer->Update();
      }
      catch ( itk::ExceptionObject& excep )
      {
        throw;
      }
    }
  }
  return EXIT_SUCCESS;
}

} // end of anonymous namespace

int main( int argc, char * argv[] )
{
  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType     pixelType;
  itk::ImageIOBase::IOComponentType componentType;

  Drr_options drrOptions;

  try
  {
    // Since there are no conditional flags, EVERY options must be 
    // explicitly defined and checked.
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
