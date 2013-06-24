function cli_imagewrite(outputFilename, img)
% Function for writing pixel and meta data struct to a NRRD file

% Open file for writing
fid=fopen(outputFilename, 'w');
if(fid<=0) 
  fprintf('Could not open file: %s\n', outputFilename);
end

fprintf(fid,'NRRD0004\n');
fprintf(fid,'# Complete NRRD file format specification at:\n');
fprintf(fid,'# http://teem.sourceforge.net/nrrd/format.html\n');

metaDataCellArr = struct2cell(img.metaData);

% Get string names for each field in the meta data
fields = fieldnames(img.metaData);

% Print the header data to the output file
for i=1:numel(fields)
  % Cannot use spaces in field names, they were replace by underscore, restore them to spaces here
  field=strrep(fields{i},'_',' ');
  fprintf(fid,field);
  writeDataByType(fid,metaDataCellArr{i});
end

fprintf(fid,'\n');

% Convert pixel data from MATLAB's order to NRRD order
img.pixelData = permute(img.pixelData, [2 1 3]);
% Write pixel data
if (isfield(img.metaData,'type'))
  % data type is specified in the metadata => convert the pixel data to that
  datatype = getDatatype(img.metaData.type);
  img.pixelData=cast(img.pixelData, datatype);
end
fwrite(fid, img.pixelData,class(img.pixelData));

fclose('all');

function writeDataByType(fid, data)
% Function that writes the header data to file based on the type of data
% params: - fid of file to write to
%         - data from header to write
  if ischar(data)
    fprintf(fid,': %s\n',data);  
  else
    fprintf(fid,': ');
    fprintf(fid,'%d ',data);  
    fprintf(fid,'\n');
  end

function datatype = getDatatype(metaType)
% Determine the datatype from the type string of the metadata
switch (metaType)
 case {'signed char', 'int8', 'int8_t'}
  datatype = 'int8';  
 case {'uchar', 'unsigned char', 'uint8', 'uint8_t'}
  datatype = 'uint8';
 case {'short', 'short int', 'signed short', 'signed short int', ...
       'int16', 'int16_t'}
  datatype = 'int16';
 case {'ushort', 'unsigned short', 'unsigned short int', 'uint16', ...
       'uint16_t'}
  datatype = 'uint16';
 case {'int', 'signed int', 'int32', 'int32_t'}
  datatype = 'int32';
 case {'uint', 'unsigned int', 'uint32', 'uint32_t'}
  datatype = 'uint32';
 case {'longlong', 'long long', 'long long int', 'signed long long', ...
       'signed long long int', 'int64', 'int64_t'}
  datatype = 'int64';
 case {'ulonglong', 'unsigned long long', 'unsigned long long int', ...
       'uint64', 'uint64_t'}
  datatype = 'uint64';
 case {'float'}
  datatype = 'single';
 case {'double'}
  datatype = 'double';
 otherwise
  assert(false, 'Unknown datatype')
end 
