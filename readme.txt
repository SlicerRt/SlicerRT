Build and use DicomRtImport module:
* Make sure you have a built and working Slicer4
* Configure DicomRtImport directory with CMake
* Copy Slicer4 directory over your Slicer4 working copy
  Note: temporary solution until creating our Slicer4 branch
* Copy content of Slicer4-bin directory in your Slicer4 binary directory
* Build CTK using its superbuild
* Build the inner Slicer-build solution
* Build DicomRtImport solution
* Start Slicer4
* Add [DicomRtImport binary directory]\DicomRtImport\lib\Slicer-4.0\qt-loadable-modules\Debug directory to Slicer/Edit/Application Settings/Additional settings/Additional module paths list
* Restart Slicer4
* Load a DICOM Study containing DICOM RT in the DICOM module's DICOM database
* Select Study
* Click 'Load Selected Study in Slicer' button 