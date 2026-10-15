"""Validate the actual drumlogue header and ABI of the compiled ELF32 unit."""
import hashlib
import pathlib
import struct
import sys

path = pathlib.Path(sys.argv[1])
data = path.read_bytes()
assert data[:6] == b'\x7fELF\x01\x01', 'Expected little-endian ELF32'
h = struct.unpack_from('<HHIIIIIHHHHHH', data, 16)
assert h[0] == 3 and h[1] == 40, 'Expected ARM shared object'
assert h[6] & 0x400, 'Expected hard-float ABI'
sections = [struct.unpack_from('<IIIIIIIIII', data, h[5]+i*h[10]) for i in range(h[11])]
s = sections[h[12]]
names = data[s[4]:s[4]+s[5]]
unit = next(s for s in sections if names[s[0]:].split(b'\0')[0] == b'.unit_header')
u = data[unit[4]:unit[4]+unit[5]]
size, target, api, dev, uid, version = struct.unpack_from('<IHIIII', u)
name = u[22:36].split(b'\0')[0].decode()
presets, params = struct.unpack_from('<II', u, 36)
assert size == 596 and len(u) == 596, 'Header layout does not match drumlogue SDK'
assert target == 0x405 and api == 0x20000, 'Wrong platform, module, or API'
assert name == 'JerzyGen' and dev == 0x4A424B31 and uid == 0x47454E31
assert params == 20 and presets == 0 and version == 0x10002
print(f'Validated {name}: ARM hard-float, drumlogue synth API 2.0, {params} parameters, {len(data)} bytes')
print('SHA256:', hashlib.sha256(data).hexdigest())
