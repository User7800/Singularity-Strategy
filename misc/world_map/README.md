# World Map Generation Utilities

# Requirements
- bitarray
- numpy
- pillow
- rasterio

# Setup and use
```shell
cd misc/world_map
python -m venv ./venv
source ./venv/bin/activate
pip install -r requirements.txt
```
After setup, you can use `gen_world_bitmap.py` to generate a file, `out`, a bitmap representation of the land and the water.

Once you have a bitmap file, you can use `bitmap_to_image.py` to convert the bitmap to an image.

```shell
source ./venv/bin/activate
python gen_world_bitmap.py
python bitmap_to_image.py
```

# TODO:
The method for generating this map is very primitive, it just iterates over latitude and longitude points and checks to see if that particular point is land or water.

This is pretty close, but would be better if we checked a square area, and then assigned it based on which was more prominent in the square.