import bitarray
import numpy as np
from PIL import Image

W, H = 2000, 1000  # matches your loops: i in 0..1999, j in 0..999

with open("out", "rb") as f:
    b = bitarray.bitarray()
    b.fromfile(f)

# Expect exactly W*H bits
arr = np.fromiter(b, dtype=np.uint8).reshape((W, H))  # arr[x, y]

# Create RGB image: 0=water(blue), 1=land(green)
img = np.zeros((H, W, 3), dtype=np.uint8)  # (rows=y, cols=x)
img[arr.T == 0] = (0, 0, 255)              # blue
img[arr.T == 1] = (0, 255, 0)              # green

Image.fromarray(img).save("map.png")

