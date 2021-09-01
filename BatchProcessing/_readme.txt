BatchStructureSetConversion
  Purpose:
    Convert all DICOM RTSS structures to labelmap and save them to disk
  Usage:
    [path/]Slicer.exe --no-main-window --python-script [path/]BatchStructureSetConversion.py --input-folder input/folder/path --output-folder output/folder/path
    (Optionally use -i and -o instead of the long argument names)
    Additional arguments:
      --ref-dicom-folder (-r): Folder containing reference anatomy DICOM image series, if stored outside the input study
      --use-ref-image (-u): Use anatomy image as reference when converting structure set to labelmap
      --exist-db (-x): Process an existing database instead of importing data in a new one (in this case --input-folder is a database and not a folder containing DICOM data)
      --export-images (-m): Export all image data alongside the labelmaps to NRRD
      --export-surfaces (-s): Export surface mesh representation of segments
      --show-python-console (-c): display an interactive Python console for troubleshooting, it displays all messages and prevents the application from exiting when the processing is completed.

  Notes:
    * The CT (or other anatomical) volume of the study needs to be present in the input folder so that the converter can use it as a reference, unless --ref-dicom-folder is specified
    * Windows users need to be careful to use slash characters in the path of the python script. It may be needed to replace '\' in the command window auto-completed path names with '/' for the paths arguments of the script, because the Slicer launcher can only interpret this path format.
    * On Windows, output messages are not visible in the console, specify -c option to show a console window where the messages are displayed.
