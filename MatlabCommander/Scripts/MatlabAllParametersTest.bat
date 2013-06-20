@set MODULE_NAME=MatlabAllParametersTest

@echo off
if "%1"=="--xml" goto :print_xml

rem Forward parameters to the Matlab CLI
%SLICER_HOME%/Slicer.exe --launcher-no-splash --launch f:\devel\SlicerRtExtension-win64dbg\inner-build\lib\Slicer-4.2\cli-modules\Debug\MatlabCommander.exe --call-matlab-function %MODULE_NAME% %*
goto :end

rem Print CLI descriptor XML
:print_xml
type %MODULE_NAME%.xml
goto :end

:end
