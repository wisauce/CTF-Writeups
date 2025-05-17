from PIL import Image

# Buka gambar
img = Image.open("location.png")
width, height = img.size

# Buat gambar baru untuk hasil perbaikan
fixed_img = Image.new("RGB", (width, height))

for y in range(height):
    # Hitung jumlah shift (misalnya 5 piksel per baris)
    shift = (y * 5) % width
    # Ambil satu baris
    row = img.crop((0, y, width, y + 1))
    # Geser ke kiri (kebalikan dari distorsi)
    fixed_row = Image.new("RGB", (width, 1))
    fixed_row.paste(row.crop((shift, 0, width, 1)), (0, 0))
    fixed_row.paste(row.crop((0, 0, shift, 1)), (width - shift, 0))
    # Tempel ke gambar baru
    fixed_img.paste(fixed_row, (0, y))

# Simpan hasilnya
fixed_img.save("fixed.png")