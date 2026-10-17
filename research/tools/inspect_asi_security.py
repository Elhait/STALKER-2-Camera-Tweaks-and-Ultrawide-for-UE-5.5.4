"""Read-only, standard-library PE/ZIP inventory. Never loads an inspected DLL.

Usage: python research/tools/inspect_asi_security.py [path-to-additional-ASI]
Run under external supervision. Inputs are capped; resource/code bytes are hashed,
not executed. This structural inspection does not establish absence of malware.
"""
import collections
import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
LIMIT = 32 * 1024 * 1024


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    if path.stat().st_size > LIMIT:
        raise ValueError(f"Input exceeds 32 MiB: {path}")
    return path.read_bytes()


def inspect(data):
    def unpack(fmt, offset):
        return struct.unpack_from("<" + fmt, data, offset)

    pe = unpack("I", 60)[0]
    if data[:2] != b"MZ" or data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("Not a PE image")
    machine, count, timestamp, _, _, optional_size, flags = unpack("HHIIIHH", pe + 4)
    optional = pe + 24
    if machine != 0x8664 or unpack("H", optional)[0] != 0x20B or count > 96:
        raise ValueError("Expected bounded AMD64 PE32+ image")
    image_base = unpack("Q", optional + 24)[0]
    entry = unpack("I", optional + 16)[0]
    headers = unpack("I", optional + 60)[0]
    directories = [unpack("II", optional + 112 + i * 8) for i in range(16)]
    sections = []
    for index in range(count):
        offset = optional + optional_size + index * 40
        virtual_size, rva, size, raw = unpack("IIII", offset + 8)
        characteristics = unpack("I", offset + 36)[0]
        blob = data[raw:raw + size]
        if len(blob) != size:
            raise ValueError("Truncated section")
        entropy = -sum((n / len(blob)) * math.log2(n / len(blob))
                       for n in collections.Counter(blob).values()) if blob else 0
        sections.append(dict(name=data[offset:offset + 8].rstrip(b"\0").decode(),
                             rva=rva, virtual_size=virtual_size, offset=raw, size=size,
                             flags=hex(characteristics), entropy=round(entropy, 3),
                             execute=bool(characteristics & 0x20000000),
                             write=bool(characteristics & 0x80000000), sha256=digest(blob)))

    def file_offset(rva):
        if rva < headers:
            return rva
        for section in sections:
            if section['rva'] <= rva < section['rva'] + section['size']:
                return section['offset'] + rva - section['rva']
        raise ValueError(f"Unmapped RVA {rva:x}")

    def string(offset):
        end = data.find(b"\0", offset, offset + 8192)
        if end < offset:
            raise ValueError("Unterminated string")
        return data[offset:end].decode('ascii', 'replace')

    imports = {}
    if directories[1][0]:
        offset = file_offset(directories[1][0])
        for index in range(512):
            original, stamp, forwarder, name, thunk = unpack("IIIII", offset + index * 20)
            if not any((original, stamp, forwarder, name, thunk)):
                break
            names = []
            address = file_offset(original or thunk)
            for item in range(8192):
                value = unpack("Q", address + item * 8)[0]
                if not value:
                    break
                names.append("#" + str(value & 65535) if value >> 63 else
                             string(file_offset(value) + 2))
            imports[string(file_offset(name))] = names

    resources = []
    if directories[2][0]:
        resource_base = file_offset(directories[2][0])

        def walk(relative, path, seen):
            if len(path) >= 5 or relative in seen:
                raise ValueError("Invalid resource tree")
            offset = resource_base + relative
            named, numbered = unpack("HH", offset + 12)
            if named + numbered > 1024:
                raise ValueError("Resource tree exceeds budget")
            for index in range(named + numbered):
                name, target = unpack("II", offset + 16 + index * 8)
                if name & 0x80000000:
                    raise ValueError("Unexpected named resource in this release")
                current = path + [name]
                if target & 0x80000000:
                    walk(target & 0x7FFFFFFF, current, seen | {relative})
                else:
                    rva, size, _, _ = unpack("IIII", resource_base + target)
                    offset_data = file_offset(rva)
                    blob = data[offset_data:offset_data + size]
                    if len(blob) != size:
                        raise ValueError("Truncated resource")
                    resources.append(dict(path=current, size=size, sha256=digest(blob)))

        walk(0, [], set())

    tls = []
    if directories[9][0]:
        callback_table = unpack('Q', file_offset(directories[9][0]) + 24)[0]
        if callback_table:
            offset = file_offset(callback_table - image_base)
            for index in range(256):
                callback = unpack('Q', offset + index * 8)[0]
                if not callback:
                    break
                tls.append(hex(callback - image_base))
    strings = [m.group().decode('ascii') for m in re.finditer(rb'[\x20-\x7e]{6,}', data)]
    selected = sorted(set(s for s in strings if re.search(
        r'https?://|RtlDllShutdown|xinput|ShellExecute|powershell|cmd\.exe|\.pdb$', s, re.I)))
    end = max([headers] + [s['offset'] + s['size'] for s in sections])
    result = dict(size=len(data), sha256=digest(data), machine=hex(machine),
                  timestamp_utc=datetime.datetime.fromtimestamp(timestamp, datetime.timezone.utc).isoformat(),
                  characteristics=hex(flags), image_base=hex(image_base), entry_rva=hex(entry),
                  dll_characteristics=hex(unpack('H', optional + 70)[0]), sections=sections,
                  imports=imports, export_directory=directories[0],
                  certificate_directory=directories[4], delay_import_directory=directories[13],
                  tls_callbacks=tls, resources=resources, selected_strings=selected,
                  overlay_bytes=len(data) - end)
    if directories[10][0]:
        offset = file_offset(directories[10][0])
        result['load_config'] = dict(size=unpack('I', offset)[0],
            security_cookie_va=hex(unpack('Q', offset + 88)[0]),
            guard_flags=hex(unpack('I', offset + 144)[0]))
    return result


def main():
    result = dict(archives={}, binaries={})
    for version in ('3.1.0', '3.2.0'):
        path = ROOT / 'release-assets' / f'STALKER2CameraTweaks-UE5.5.4-v{version}.zip'
        raw = read(path)
        with zipfile.ZipFile(path) as archive:
            entries = archive.infolist()
            if len(entries) > 32 or sum(e.file_size for e in entries) > LIMIT:
                raise ValueError('Archive exceeds inspection budget')
            result['archives'][version] = dict(size=len(raw), sha256=digest(raw), entries=[
                dict(name=e.filename, size=e.file_size, sha256=digest(archive.read(e)),
                     encrypted=bool(e.flag_bits & 1)) for e in entries])
            result['binaries'][version] = inspect(archive.read('STALKER2CameraTweaks.asi'))
    for argument in sys.argv[1:]:
        result['binaries'][argument] = inspect(read(Path(argument)))
    known = {}
    ids = {name: int(number) for name, number in re.findall(r'#define\s+(\w+)\s+(\d+)',
        (ROOT / 'src/localization/localization_resource_ids.h').read_text())}
    for name, path in re.findall(r'(\w+)\s+RCDATA\s+"([^"]+)"',
            (ROOT / 'src/overlay/localization/localization_resources.rc').read_text()):
        known[ids[name]] = (path, digest(read(ROOT / path)))
    for binary in result['binaries'].values():
        binary['resource_source_matches'] = [dict(path=r['path'],
            source=known.get(r['path'][1], ('UNKNOWN', None))[0],
            identical=r['sha256'] == known.get(r['path'][1], ('UNKNOWN', None))[1])
            for r in binary['resources']]
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
