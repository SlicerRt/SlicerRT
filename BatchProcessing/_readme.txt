BatchContourConversion
  Purpose:
    Convert all DICOM RTSS contours to labelmap and save them to disk
  Usage:
    [path/]Slicer.exe --no-main-window --python-script [path/]BatchContourConversion.py --input-folder input/folder/path --output-folder output/folder/path
    (Optionally use -i and -o instead of the long argument names)
  Notes:
    * The CT (or other anatomical) volume of the study needs to be present in the input folder so that the converter can use it as a reference.
    * Windows users need to be careful to use slash characters in the path of the python script. It may be needed to replace '\' in the command window auto-completed path names with '/' for the paths arguments of the script, because the Slicer launcher can only interpret this path format.
    * Output messages are not visible with current Slicer 4.4.0 installers (although they appear with locally built Slicer), so it will be hard to see where the script fails if it does not function properly for some reason. The workaround for this is to remove the sys.exit() statements from the script and run it *without* the --no-main-window switch. Then console output is available in the python interactor window.
