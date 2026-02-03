# ski-vis

* Authors: Audrey Au , Ziv Dreyfuss

* The geoSkiViz folder is the geotiff and OpenStreetMap pre-processing folder. It contains a "geo" directory which has the python script to produce the  "piste_coords_mammoth_v3.txt" piste file and the "geo_test11.ply". It also contains a Docker file to run the Python code via Docker. See separate README in geoSkiViz folder for directions on how to accomplish this. Any additional PLY / piste files must be manually moved to the scivis/data/scalar_data/ folder.

* The scivis folder contains the C++ code to actually produce the ski visualization based on the "piste_coords_mammoth_v3.txt" piste file. Instructions to build this repo are contained in the scivis folder. A command line argument must be used pointing to the data/scalar_data/geo_test11.ply file (using the entire file path). The "geo_test9.ply" file provides more detail at the expense of a longer render time.

* Compiling the C++ code on a Mac may require uncommenting "#define" (lines 11 - 14) and "#undef" (lines 21 - 24) in the project2.h file
