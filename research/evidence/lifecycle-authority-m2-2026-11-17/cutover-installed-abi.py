from pathlib import Path
import hashlib
import json
import mmap
import re
import struct

exe = Path(r'E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe')
src = Path('src/hooks/signatures/signature_definitions.hpp').read_text(encoding='utf-8')

def pattern(name, wild=()):
    if match := re.search(r'char ' + name + r'\[\] =\s*((?:"[^"]*"\s*)+);', src):
        tokens = ' '.join(re.findall(r'"([^"]*)"', match[1])).split()
    else:
        region = re.search(r'std::uint8_t ' + name + r'\[\] = \{(.*?)\};', src, re.S)[1]
        region = re.sub(r'//[^\n]*', '', region)
        tokens = re.findall(r'0x([0-9A-Fa-f]{2})', region)
    return re.compile(b''.join(b'.' if token == '??' or i in wild else re.escape(bytes([int(token, 16)]))
                              for i, token in enumerate(tokens)), re.DOTALL)

with exe.open('rb') as file, mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as image:
    pe = struct.unpack_from('<I', image, 0x3c)[0]
    assert image[pe:pe+4] == b'PE\x00\x00'
    machine, n = struct.unpack_from('<HH', image, pe + 4)
    assert machine == 0x8664
    section_table = pe + 24 + struct.unpack_from('<H', image, pe + 20)[0]
    sections = {}
    for i in range(n):
        entry = section_table + i * 40
        name, sv, rva, size, raw = struct.unpack_from('<8s4I', image, entry)
        sections[name.rstrip(b'\x00')] = (rva, raw, min(sv, size))
    text_rva, text_off, text_size = sections[b'.text']
    assert text_off + text_size <= len(image)

    anchors = {}
    for name, wild, delta in [
        ('CameraWriter', range(17, 21), 25),
        ('CameraWriterPostProcessCall', range(26, 30), 25),
        ('CinematicAspectSetter', (), 25),
        ('CinematicEnter', (), 8),
        ('CinematicExit', (), 5),
        ('CinematicExitIndexed', (), 10),
        ('DialogueBoundary', (), 6),
        ('PostExitBlendStartCapture', (), 5),
    ]:
        found = list(pattern(name, wild).finditer(image, text_off, text_off + text_size))
        anchors[name] = [{'rva': hex(text_rva + m.start() - text_off + delta),
                          'instruction_bytes': image[m.start()+delta:m.start()+delta+10].hex(' ')}
                         for m in found]

    def instruction(name, prefix):
        return len(anchors[name]) == 1 and anchors[name][0]['instruction_bytes'].startswith(prefix)

    checks = {
        'writer_movss_rbx_30_xmm0': instruction('CameraWriter', 'f3 0f 11 43 30'),
        'aspect_store_rax_254_ten_bytes': instruction('CinematicAspectSetter', 'c7 80 54 02 00 00 39 8e e3 3f'),
        'dialogue_xmm6_to_xmm1': instruction('DialogueBoundary', '0f 28 ce ff 90 08 06 00 00'),
        'enter_call': instruction('CinematicEnter', 'e8 '),
        'exit_call': instruction('CinematicExit', 'e8 ') or instruction('CinematicExitIndexed', 'e8 '),
        'armer_component_lea': instruction('PostExitBlendStartCapture', '48 8d 4e 28'),
    }
    print(json.dumps({'image': str(exe), 'sha256': hashlib.sha256(image).hexdigest(),
                      'text_size': text_size, 'anchors': anchors, 'checks': checks}, indent=2))
    if not all(checks.values()):
        raise SystemExit(1)
