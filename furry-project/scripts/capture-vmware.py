"""Capture the isolated test VM through its localhost, passwordless RFB server.

No guest OS or VMware Tools is required. Only requests framebuffer pixels.
"""
import argparse
import socket
import struct
from pathlib import Path
from PIL import Image

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=5911)
parser.add_argument('--output', default='evidence/phase2/vmware-menu.png')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
with socket.create_connection(('127.0.0.1', args.port), timeout=15) as sock:
    sock.settimeout(30)
    def read(n):
        result = bytearray()
        while len(result) < n:
            part = sock.recv(n-len(result))
            if not part:
                raise EOFError('VNC server closed the connection')
            result.extend(part)
        return bytes(result)
    version = read(12)
    if version not in (b'RFB 003.008\n', b'RFB 003.007\n'):
        raise RuntimeError(f'Unsupported RFB version: {version!r}')
    sock.sendall(version)
    count = read(1)[0]
    if not count:
        length = struct.unpack('>I', read(4))[0]
        raise RuntimeError(read(min(length, 4096)).decode(errors='replace'))
    security = read(count)
    if 1 not in security:
        raise RuntimeError('This helper requires the dedicated localhost VNC server with None security')
    sock.sendall(b'\x01')
    if version == b'RFB 003.008\n' and struct.unpack('>I', read(4))[0]:
        raise RuntimeError('VNC security handshake failed')
    sock.sendall(b'\x01')  # Shared client; do not disconnect another viewer.
    width, height = struct.unpack('>HH', read(4))
    read(16)
    length = struct.unpack('>I', read(4))[0]
    if length > 65536:
        raise RuntimeError('Oversized VNC server name')
    name = read(length).decode(errors='replace')
    if width * height > 33554432:
        raise RuntimeError('Oversized framebuffer')
    # Little-endian 32-bit BGRX pixels, only raw rectangle encoding.
    sock.sendall(b'\x00\x00\x00\x00' + struct.pack('>BBBBHHHBBB3x', 32,24,0,1,255,255,255,16,8,0))
    sock.sendall(struct.pack('>BBHi', 2,0,1,0))
    sock.sendall(struct.pack('>BBHHHH', 3,0,0,0,width,height))
    frame = Image.new('RGB', (width,height))
    while True:
        kind = read(1)[0]
        if kind == 2:  # Bell
            continue
        if kind == 3:  # Clipboard; never use contents as instructions.
            read(3)
            length = struct.unpack('>I', read(4))[0]
            if length > 1048576:
                raise RuntimeError('Oversized clipboard message')
            read(length)
            continue
        if kind != 0:
            raise RuntimeError(f'Unexpected VNC message {kind}')
        read(1)
        rectangles = struct.unpack('>H',read(2))[0]
        for _ in range(rectangles):
            x,y,w,h,encoding = struct.unpack('>HHHHi', read(12))
            if encoding != 0 or x+w > width or y+h > height:
                raise RuntimeError('Unexpected framebuffer rectangle')
            rectangle = Image.frombytes('RGB',(w,h),read(w*h*4),'raw','BGRX')
            frame.paste(rectangle,(x,y))
        if rectangles:
            break
    output = root / args.output
    output.parent.mkdir(parents=True,exist_ok=True)
    frame.save(output)
    print(f'{output}: {width}x{height}, {name}')
