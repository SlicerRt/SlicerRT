function MatlabAllParametersTest(args)

inputImgFilename=getarg(args,'',1);
outputImgFilename=getarg(args,'',2);
threshold=str2num(getarg(args,'threshold'));

[inputImgVoxels,inputImgMeta]=nrrdread(inputImgFilename);

inputImgVoxels=int16((double(inputImgVoxels)>threshold)*100+50);

nrrdwrite(outputImgFilename,inputImgVoxels,inputImgMeta);

