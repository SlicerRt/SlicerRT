function outputParams=MatlabModuleTemplate(inputParams)

%% Getting inputs
%
% Named parameters: inputParams.paramName
% Unnamed parameters: inputParams.unnamed(1), inputParams.unnamed(2), ...
%
% For integer, integer-vector, integer-enumeration, float, float-vector, float-enumeration, double, double-vector, double-enumeration, point, region, string, string-enum:
%   paramValue=inputParams.paramName;
% For boolean:
%   paramValue=isfield(inputParams,'paramName');
% For string-vector:
%   paramValue=strsplit(inputParams.paramName,',');
% For file:
%   f=fopen(inputParams.paramName, ...);
%   ...
%   fclose(f);
% For directory:
%   ???
% For image:
%   img=nrrdread(inputParams.paramName);
% Geometry:
%   geom=stlread(inputParams.paramName);
%

%% Writing outputs
%
% For integer, integer-vector, integer-enumeration, float, float-vector, float-enumeration, double, double-vector, double-enumeration, point, region, string, string-enum, boolean:
%   outputParams.paramName=paramValue;
% For string-vector:
%   outputParams.paramName=strjoin(paramValue,',');
% For file:
%   f=fopen(inputParams.paramName, ...);
%   ...
%   fclose(f);
% For directory:
%   ???
% For image (note that for processing you may need to convert the image type to double and it is advisable to convert back to the original data type before writing):
%   nrrdwrite(inputParams.paramName, img);
% Geometry:
%   stlwrite(inputParams.paramName, geom);
%
% If there are no output parameters:
%   outputParams=[];

%% Example

img=cli_imageread(inputParams.unnamed{1});

outputParams.min=min(min(min(img.pixelData)));
outputParams.max=max(max(max(img.pixelData)));

img.pixelData=(double(img.pixelData)>inputParams.threshold)*100+50;

cli_imagewrite(inputParams.unnamed{2}, img);
