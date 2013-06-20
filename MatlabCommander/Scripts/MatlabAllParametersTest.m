function MatlabAllParametersTest(args)

% Get inputs

inputImgFilename=getarg(args,'',1);
[inputImgVoxels,inputImgMeta]=nrrdread(inputImgFilename);

threshold=str2num(getarg(args,'threshold'));

% Process

imgMin=min(min(min(inputImgVoxels)));
imgMax=max(max(max(inputImgVoxels)));

inputImgVoxels=int16((double(inputImgVoxels)>threshold)*100+50);

% Set outputs

outputImgFilename=getarg(args,'',2);
nrrdwrite(outputImgFilename,inputImgVoxels,inputImgMeta);

setarg(args,'min',imgMin)
setarg(args,'max',imgMax)
