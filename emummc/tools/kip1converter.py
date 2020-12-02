# Modified kip1 conversion script, originally by jakibaki
# Used for dev purposes, will be replaced in the future

from struct import pack, unpack
from sys import argv

f = open(argv[1], "rb")

header_start = f.read(0x20)

section_names = [".text", ".rodata", ".data", ".bss"]

sections = []
for i in range(6):
    section_bytes = f.read(0x10)
    section = {}

    if i < len(section_names):
        section["Name"] = section_names[i]

    section["OutOffset"], section["DecompressedSize"], section["CompressedSize"], section["Attribute"] = unpack("IIII", section_bytes)
    sections.append(section)
    print(section)

assert (sections[3]["OutOffset"] + sections[3]["DecompressedSize"]) % 0x1000 == 0

kernel_caps = []
for i in range(0x20):
    val, = unpack("I", f.read(4))
    kernel_caps.append(val)

f.seek(0x100)

for i in range(3):
    section = sections[i]
    section["Buffer"] = f.read(section["DecompressedSize"])
print(f.read())

f.close()

f = open(argv[2], "wb")

for i in range(3):
    section = sections[i]
    f.seek(section["OutOffset"])
    f.write(section["Buffer"])
    
f.seek(sections[3]["OutOffset"])
f.write(b'\0' * sections[3]["DecompressedSize"])

caps = open("emummc.caps", "wb")
for i in range(0x20):
    caps.write(pack("I", kernel_caps[i]))
