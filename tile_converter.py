from PIL import Image

def process_rgb(color):
    scaled = [color[i] >> 3 for i in range(3)]
    color = scaled[0] | (scaled[1] << 5) | (scaled[2] << 10)
    return "0x" + hex(color)[2:].rjust(4, "0")

im = Image.open("tiles.png").convert("RGB")
assert im.size[0] % 8 == 0 and im.size[1] % 8 == 0

colors = sorted(im.getcolors(), key=lambda x: x[0], reverse=True)
colormap = {color[1]: i for i, color in enumerate(colors)}
gba_colors = {i: process_rgb(color) for color, i in colormap.items()}

data = im.get_flattened_data()
tilemap = []
end = False
for ty in range(im.size[1] // 8):
    for tx in range(im.size[0] // 8):
        tile = []
        for y in range(8):
            row = ""
            for x in range(8):
                color = colormap[data[(ty * 8 + y) * im.size[0] + tx * 8 + x]]
                row = hex(color)[2:].upper() + row
            tile.append("0x" + row[4:])
            tile.append("0x" + row[:4])
        if tilemap and tile == tilemap[0]:
            end = True
            break
        tilemap.append(tile)
    if end:
        break

with open("tiles.c", "w+") as f:
    f.write("#include \"tiles.h\"\n\n")
    f.write("const unsigned short palette[] = {\n    ")
    f.write(", ".join(gba_colors.values()) + "\n};\n")
    f.write(f"const int palette_count = {len(colormap)};\n\n")
    f.write("const unsigned short tiles[] = {\n")
    for tile in tilemap:
        f.write("    " + ", ".join(tile[:8]) + ",\n")
        f.write("    " + ", ".join(tile[8:]) + ",\n")
    f.write("};\n")
    f.write(f"const int tile_count = {len(tilemap) * 16};\n")
