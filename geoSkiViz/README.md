# geoSkiViz
Project for CS453/553

## Docker usage
* Run `docker build . -t geotiff`
* Run `docker run -v "<pathToDir>/geoSkiViz/geo:/usr/src/geo"  -it geotiff /bin/bash`
* Now that you're inside the container run `python3 geotiff.py`
