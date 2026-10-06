"""Generate explicitly synthetic SMBIOS records for firmware regression tests."""
from pathlib import Path
import struct
root=Path(__file__).resolve().parent.parent
out=root/'vm-tests'/'fixtures';out.mkdir(parents=True,exist_ok=True)
def dimm(handle,size,kind=0x22,speed=6000,extended=None):
 b=bytearray(92);b[0]=17;b[1]=92;struct.pack_into('<H',b,2,handle)
 struct.pack_into('<H',b,12,size);b[18]=kind
 struct.pack_into('<H',b,21,min(speed,65535));struct.pack_into('<H',b,32,min(speed,65535))
 if extended is not None:struct.pack_into('<I',b,28,extended)
 if speed>=65535:struct.pack_into('<I',b,88,speed)
 return bytes(b)+b'\0\0'
# One file may include multiple Type 17 records: replaces default generated type.
(out/'two-dimms.bin').write_bytes(dimm(0x1100,2048)+dimm(0x1101,2048))
(out/'unknown-memory.bin').write_bytes(dimm(0x1100,0xffff))
(out/'extended-memory.bin').write_bytes(dimm(0x1100,0x7fff,extended=32768))
(out/'short-record.bin').write_bytes(b'\x11\x03\x00\x11\0\0')
print(out)
