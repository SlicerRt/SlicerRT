function MatlabModuleTemplate(args)

% Get inputs

inputImgFilename=getarg(args,'',1);
[inputImgVoxels,inputImgMeta]=nrrdread(inputImgFilename);

threshold=str2num(getarg(args,'threshold'));

% Process

imgMin=min(min(min(inputImgVoxels)));
imgMax=max(max(max(inputImgVoxels)));

% Convert to floating point pixel type
originalType=class(inputImgVoxels);
inputImgVoxels=double(inputImgVoxels);
% Modify the data
outputImgVoxels=(inputImgVoxels>threshold)*100+50;
% Convert back to original pixel type
outputImgVoxels=cast(outputImgVoxels, originalType);

% Set outputs

outputImgFilename=getarg(args,'',2);
nrrdwrite(outputImgFilename,outputImgVoxels,inputImgMeta);

setarg(args,'min',imgMin)
setarg(args,'max',imgMax)
