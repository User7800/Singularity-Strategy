import numpy as np
import rasterio
import bitarray

# --------- CONFIG ----------
mask_tif = "land_water_mask.tif"  # GeoTIFF: land=1, water=0 (any 0/1 convention is fine)
lat_step = 0.18
lon_step = 0.18
lat_min, lat_max = -90.0, 90.0
lon_min, lon_max = -180.0, 180.0

# Assumes the raster is in EPSG:4326 (WGS84). If not, you must reproject.
# ---------------------------

with rasterio.open(mask_tif) as ds:
    # Read first band (adjust if your mask uses a different band/index)
    band = ds.read(1)
    nodata = ds.nodata

    def lonlat_to_value(lon, lat):
        # Convert lon/lat -> raster row/col
        row, col = ds.index(lon, lat)

        # Out of bounds => treat as "water" (or handle differently)
        if row < 0 or row >= ds.height or col < 0 or col >= ds.width:
            return 0

        v = band[row, col]
        if nodata is not None and v == nodata:
            return 0

        # If your raster stores land as 255/1/etc, normalize to 1/0:
        return 1 if v != 0 else 0

    #results = {}
    #results=[]
    results = bitarray.bitarray()
    #lats = np.arange(lat_min, lat_max + lat_step, lat_step)
    #lons = np.arange(lon_min, lon_max + lon_step, lon_step)

    #for lat in lats:
    #    for lon in lons:
    #for lon in lons:
    #    for lat in lats:
    for a in range(2000):
        for b in range(1000):
            lon = (0.18*a) - 180
            lat = (-0.18*b) + 90
            results.append(lonlat_to_value(lon, lat))


with open("out", "wb") as f:
    results.tofile(f)
print(results)
#with open(b, "w") as file:
#    pickle.dump(results, file)

# Example: print a few samples
#for i, ((lat, lon), is_land) in enumerate(results.items()):
#    print(f"{lat:.4f}, {lon:.4f} -> {'land' if is_land == 1 else 'water'}")
#    if i >= 10:
#        break

