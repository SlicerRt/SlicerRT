function nrrdwrite(outputFilename, pixelData, metaData)
% Function for writing an array struct to an NRRD file

% open file for writing
fid=fopen(outputFilename, 'w');
if(fid<=0) 
  fprintf('Could not open file: %s\n', outputFilename);
end

fprintf(fid,'NRRD0004\n');
fprintf(fid,'# Complete NRRD file format specification at:\n');
fprintf(fid,'# http://teem.sourceforge.net/nrrd/format.html\n');

metaDataCellArr = struct2cell(metaData);

% get string names for each field in the meta data
fields = fieldnames(metaData);

% Print the header data to the output file
for i=1:numel(fields)
  % Cannot use spaces in field names, they were replace by underscore, restore them to spaces here
  field=strrep(fields{i},'_',' ');
  fprintf(fid,field);
  writeDataByType(fid,metaDataCellArr{i});
end

fprintf(fid,'\n');

% Convert pixel data from MATLAB's order to NRRD order
pixelData = permute(pixelData, [2 1 3]);
% Write pixel data
fwrite(fid, pixelData,class(pixelData));

fclose('all');

function writeDataByType(fid, data)
% Function that writes the header data to file based on the type of data
%
% params: - fid of file to write to
%         - data from header to write

  %write the fields out depending on their type
  if ischar(data)
    fprintf(fid,': %s\n',data);  
  else
    fprintf(fid,': ');
    fprintf(fid,'%d ',data);  
    fprintf(fid,'\n');
  end

