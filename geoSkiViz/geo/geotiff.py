import rasterio
from rasterio.enums import Resampling
from shapely.geometry import Polygon
import rasterio.mask
import csv
import numpy as np
import overpy


#---------------------------------------------------
# create ply file that contains 2 element types: vertex, face
# from rasterio object (opened geotiff file)
#---------------------------------------------------


def slice_topo(data, upperLat, leftLon, lowerLat, rightLon):

    return 0


#def create_ply(data, leftLon, lowerLat, rightLon, upperLat):
def create_ply(data):
    # upscale_factor = 1.0/3
    
    # elev_band  = data.read(
    #     out_shape=(
    #         data.count,
    #         int(data.height * upscale_factor),
    #         int(data.width * upscale_factor)
    #     ),
    #     resampling=Resampling.bilinear
    # )


    # slice1 = data.index(leftLon, lowerLat)
    # slice2 = data.index(rightLon, upperLat)

    elev_band = data.read(1)

    # elev_band = elev_band[slice1[0]:slice1[1], slice2[0]:slice2[1]]

    print(f"MIN VAL: {np.min(elev_band)}")
    print(f"MAX VAL: {np.max(elev_band)}")
    vertices = []
    faces = []

    num_verts = 0
    num_faces = 0

    width = elev_band.shape[0]
    height = elev_band.shape[1]

    print(f"width: {width} , height: {height}")

    # At 38 degrees North latitude (which passes through Stockton California and Charlottesville Virginia):
    # One degree of latitude equals approximately 364,000 feet (69 miles), one minute equals 6,068 feet (1.15 miles), and one-second equals 101 feet. 
    # One-degree of longitude equals 288,200 feet

    # one meter = 3.28084 feet
    for x in range(width):

        for y in range(height):

            lon, lat = data.xy(x, y)
            #vertex = [x, y, 0, 0, 0, 0, elev_band[x,y]]
            #vertex = [lon, lat, 0, 0, 0, 0, elev_band[x,y]]

            xCalc = abs( data.xy(x, y)[1] - data.xy(0, y)[1] ) * 288200.0

            yCalc = abs( data.xy(x, y)[0] - data.xy(x, 0)[0] ) * 364000.0


            vertex = [xCalc, yCalc, 0, 0, 0, 0, elev_band[x,y]*3.28084 ]

            vertices.append(vertex)

            num_verts+=1

            # index vertices for quads
            if x > 0 and y > 0:

                right_top = (x * height) + y 
                left_top = right_top - height 

                right_bottom = right_top - 1
                left_bottom = left_top - 1


                face = [4, left_bottom, left_top, right_top, right_bottom]


                num_faces+=1

                faces.append(face)

    print(f'num vertices {num_verts} - num faces {num_faces}')
    return vertices, faces



    def thing():
        ## query for all pistes
        q = f"""<osm-script>
                <query type="nwr">
                <bbox-query e="{xmax}" n="{ymax}" s="{ymin}" w="{xmin}"/>
                <has-kv k="piste:type" v="downhill"/>
                </query>
                <print/>
                </osm-script>"""
        api = overpy.Overpass()

        result = api.query(q)

        with open("piste_coords.txt", 'w') as f:
            for way in result.ways:

                # piste is named
                if res.tags.get("name", "") != "":

                    print(f"""Saving piste {res.tags.get("name", "")}""")

                    for nodes in way.get_nodes(resolve_missing=True):
                        0
                        #f.write(f"{} {}")


if __name__ == "__main__":


    # modify as needed for geotiff slicing - currently masking for everything but Mammoth mountain
    xmin, ymin = -119.06201, 37.61509
    xmax, ymax = -118.988889, 37.65753

    bbox = Polygon([(xmin, ymin), (xmax, ymin), (xmax, ymax), (xmin, ymax), (xmin, ymin)])
  

    with rasterio.open('USGS_1_n38w120_20210701.tif') as src:
        out_image, out_transform = rasterio.mask.mask(src, [bbox], crop=True, filled=False, all_touched=True) #filled
        out_meta = src.meta

        out_meta.update({"driver": "GTiff",
                     "height": out_image.shape[1],
                     "width": out_image.shape[2],
                     "transform": out_transform})

    with rasterio.open("mammoth-mtn.tif", "w", **out_meta) as dest:
        dest.write(out_image)
       
    scale_factor = (1.0/5)

    with rasterio.open('mammoth-mtn.tif') as clipped_src:

        print(f"mammoth-mtn bounds: {clipped_src.bounds}")
        out_meta_clipped = clipped_src.meta

        out_image_scld = clipped_src.read(
            out_shape=(
                clipped_src.count,
                int(clipped_src.height * scale_factor),
                int(clipped_src.width * scale_factor)
            ),
            resampling=Resampling.bilinear
        )

        transform_scald = clipped_src.transform * clipped_src.transform.scale(
            (clipped_src.width / out_image_scld.shape[-1]),
            (clipped_src.height / out_image_scld.shape[-2])
        )

        out_meta_clipped.update({"driver": "GTiff",
                     "height": out_image_scld.shape[1],
                     "width": out_image_scld.shape[2],
                     "transform": transform_scald})

    with rasterio.open("mammoth-mtn-scld.tif", "w", **out_meta_clipped) as dest:
        dest.write(out_image_scld)

    data = rasterio.open("mammoth-mtn-scld.tif")

    print(f"dataset bounds: {data.bounds}")

    final_bounds = data.bounds

    vertices, faces = create_ply(data)

    header = ("ply\n"
    "format ascii 1.0\n"
    "comment created by PolyEditor\n"
    "comment modified by pythonply\n"
    f"element vertex {len(vertices)}\n"
    "property float64 x\n"
    "property float64 y\n"
    "property float64 z\n"
    "property float64 vx\n"
    "property float64 vy\n"
    "property float64 vz\n"
    "property float64 s\n"
    f"element face {len(faces)}\n"
    "property list uint8 int32 vertex_indices\n"
    "end_header\n")

    print(f'writing {len(vertices)} vertices, {len(faces)} faces')

    with open("geo_test11.ply", 'w') as f:
        
        f.write(header)
        vertex_str = ""
        for v in vertices:
            line = ' '.join(map(str, v))
            line += '\n'
            vertex_str += line

        f.write(vertex_str)

        face_str = ""
        for face in faces:
            line = ' '.join(map(str, face))
            line += '\n'
            face_str += line

        f.write(face_str)


    ## 
    api = overpy.Overpass()

    q = f"""<osm-script>
                <query type="nwr">
                <bbox-query e="{xmax}" n="{ymax}" s="{ymin}" w="{xmin}"/>
                <has-kv k="piste:type" v="downhill"/>
                </query>
                <print/>
            </osm-script>"""

    result = api.query( q )

    nodes = []
    # Avalanche Chutes
    for res in result.ways:

        if res.tags.get("name", ""):
            print(res.tags.get("name", ""))
        #if res.tags.get("name", "") == "Avalanche Chutes":
        if res.tags.get("name", "") == "Hully Gully":
            nodes = res.get_nodes(resolve_missing=True)

            break

    with open("piste_coords_mammoth_v3.txt", "w") as f2:


        for res in result.ways:

            #if res.tags.get("name", "") in ("Hansel", "Clover Leaf"):
            if res.tags.get("name", ""):
                print(res.tags.get("name", ""))
                nodes = res.get_nodes(resolve_missing=True)

                f2.write(f"""{res.tags.get("name", "")}\n""")
                for node in nodes:
                    x = abs( float(node.lat) - float(data.xy(0, 0)[1]) ) * 288200.0
                    y = abs( float(node.lon) - float(data.xy(x, 0)[0]) ) * 364000.0
                    f2.write(f"{x} {y}\n")

                f2.write("\n") # line break between differen pistes

        f2.write("\n")

    ## 'aerialway'



