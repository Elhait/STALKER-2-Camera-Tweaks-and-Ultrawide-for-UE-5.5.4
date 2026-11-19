"""Read-only, image-pinned scan for scalar MOVSS stores at candidate FOV offsets."""
import bisect
import hashlib
import mmap
import struct
import sys
import zipfile
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")

exe = Path(r"E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe")
expected = "2bd3f115b959a8eee7e5c4b42b0142dd0025bd437e1014f92dfc60b9afee5479"
decoder_root = Path(r"E:\Work\Slaker2 mods\02-Research\evidence\post-exit-dataflow-2026-11-11\decoder")
wheel = Path(r"E:\Work\Slaker2 mods\Instruments\ghidra_12.1.3_PUBLIC\Ghidra\Debug\Debugger-agent-dbgeng\pypkg\dist\capstone-5.0.6-py3-none-win_amd64.whl")
if not (decoder_root / "capstone" / "__init__.py").exists():
    with zipfile.ZipFile(wheel) as archive:
        for name in archive.namelist():
            if name.startswith("capstone/") and ".." not in Path(name).parts:
                archive.extract(name, decoder_root)
sys.path.insert(0, str(decoder_root))
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86_const import X86_OP_MEM


def scan(data, text_raw, text_rva, text_size, base, ranges, starts, decoder, displacement):
    needle = b"\xf3\x0f\x11"
    count = 0
    position = 0
    while position < text_size:
        position = data.find(needle, text_raw + position, text_raw + text_size)
        if position < 0:
            break
        match = position
        offset = match - text_raw
        position = match + 1
        if offset + 8 > text_size:
            continue
        modrm = data[match + 3]
        mod = modrm >> 6
        rm = modrm & 0x07
        if mod == 1:
            disp = struct.unpack_from("<b", data, match + 4)[0]
        elif mod == 2:
            disp_offset = 5 if rm == 4 else 4
            disp = struct.unpack_from("<i", data, match + disp_offset)[0]
        else:
            continue
        if disp != displacement:
            continue
        candidate_rva = text_rva + offset
        index = bisect.bisect_right(starts, candidate_rva) - 1
        function = ranges[index] if index >= 0 and ranges[index][0] <= candidate_rva < ranges[index][1] else None
        decoded = []
        fingerprints = set()
        if function and (displacement == 0x230 or candidate_rva == 0x9cd2eb):
            function_raw = text_raw + function[0] - text_rva
            function_size = function[1] - function[0]
            decoded = list(decoder.disasm(data[function_raw:function_raw + function_size],
                base + function[0]))
            for item in decoded:
                for operand in item.operands:
                    if operand.type == X86_OP_MEM and operand.mem.index == 0:
                        fingerprints.add(operand.mem.disp)
            ins = next((item for item in decoded
                if item.address <= base + candidate_rva < item.address + item.size and
                b"\xf3\x0f\x11" in item.bytes and item.mnemonic == "movss" and
                len(item.operands) >= 2 and item.operands[0].type == X86_OP_MEM and
                item.operands[0].mem.disp == displacement), None)
            if not ins:
                continue
            rva = ins.address - base
        else:
            instruction_offset = offset - 1 if offset > 0 and 0x40 <= data[match - 1] <= 0x4F else offset
            instruction_file_offset = text_raw + instruction_offset
            rva = text_rva + instruction_offset
            instructions = list(decoder.disasm(
                data[instruction_file_offset:instruction_file_offset + 9], base + rva, count=1))
            if not instructions or instructions[0].mnemonic != "movss":
                continue
            ins = instructions[0]
            if len(ins.operands) < 2 or ins.operands[0].type != X86_OP_MEM:
                continue
            if ins.operands[0].mem.disp != displacement:
                continue
        if displacement == 0x230 and (ins.op_str.startswith("dword ptr [rsp") or
                ins.op_str.startswith("dword ptr [rbp")):
            continue
        owner = f"{function[0]:#x}-{function[1]:#x}" if function else "unknown"
        if displacement == 0x230 or rva == 0x9cd2eb:
            related = ",".join(f"{value:#x}" for value in (0x234, 0x254, 0x259) if value in fingerprints) or "none"
            print(f"STORE offset={displacement:#x} rva={rva:#x} function={owner} relatedOffsets={related} instruction={ins.mnemonic} {ins.op_str}")
        if displacement == 0x230 and function:
            pivot = next((i for i, item in enumerate(decoded) if item.address == base + rva), -1)
            if pivot >= 0:
                first = max(0, pivot - 12)
                last = min(len(decoded), pivot + 16)
                for item in decoded[first:last]:
                    mark = ">" if item.address == base + rva else " "
                    print(f"  {mark} {item.address - base:#x}: {item.mnemonic} {item.op_str}")
        count += 1
    label = "DECODED_STORE_COUNT" if displacement == 0x230 else "BYTE_PATTERN_CANDIDATE_COUNT"
    print(f"{label} offset={displacement:#x} count={count}")


with exe.open("rb") as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
    digest = hashlib.sha256(data).hexdigest()
    print(f"EXE={exe}")
    print(f"SHA256={digest}")
    if digest != expected:
        raise SystemExit("IMAGE_IDENTITY_MISMATCH")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    section_count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    base = struct.unpack_from("<Q", data, optional + 24)[0]
    sections = []
    for i in range(section_count):
        row = optional + optional_size + i * 40
        name = data[row:row + 8].rstrip(b"\0").decode("ascii", errors="replace")
        virtual_size, rva, raw_size, raw = struct.unpack_from("<IIII", data, row + 8)
        sections.append((name, rva, virtual_size, raw, raw_size))
    text = next(item for item in sections if item[0] == ".text")
    pdata = next(item for item in sections if item[0] == ".pdata")
    ranges = []
    for offset in range(pdata[3], pdata[3] + pdata[4], 12):
        begin, end, _ = struct.unpack_from("<III", data, offset)
        if begin and end > begin:
            ranges.append((begin, end))
    ranges.sort()
    starts = [item[0] for item in ranges]
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    print(f"IMAGE_BASE={base:#x} TEXT_RVA={text[1]:#x} TEXT_SIZE={text[2]:#x}")
    for displacement in (0x230, 0x30):
        scan(data, text[3], text[1], text[4], base, ranges, starts, decoder, displacement)
