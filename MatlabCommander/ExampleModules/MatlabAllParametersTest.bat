@set MODULE_NAME=MatlabAllParametersTest

@echo off

if "%1"=="--xml" goto :print_xml
goto :forward_parameters

:print_xml
type %MODULE_NAME%.xml
goto :end

:forward_parameters
set SLICER_LAUNCHER="f:\S4D\Slicer-build\Slicer.exe"
rem set SLICER_LAUNCHER=%SLICER_HOME%/Slicer.exe
%SLICER_LAUNCHER% --launcher-no-splash --launch f:\devel\SlicerRtExtension-win64dbg\inner-build\lib\Slicer-4.2\cli-modules\Debug\MatlabCommander.exe --call-matlab-function %MODULE_NAME% %*
goto :end

:end
