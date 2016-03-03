The dictionaries within this directory are taken from OpenOffice

The create_includes script needs xxd and zoepfli to create 2 types of includes. The includes can
be directly used within your program, by including the header.

The 2 types of includes are
- one version of the file with an added null-terminator to use as a string and create a string stream from for loading
- one version compressed using zoepfli (a high compression gzip program) to load using a gzip read stream (e.g. from Boost)

I will always include all the files in here. The script is just for the ones that need to add further dictionaries


Versions:

de_DE: 2015-12-28
en_GB: 2016-03-01
en_US: 2016-03-01
