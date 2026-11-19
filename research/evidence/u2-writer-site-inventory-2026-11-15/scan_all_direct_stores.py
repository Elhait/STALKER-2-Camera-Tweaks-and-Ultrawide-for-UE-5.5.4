"""Bounded read-only scan of decoded .text function stores at camera field offsets."""
import hashlib
import mmap
import struct
import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
sys.path.insert(0, r"E:\Work\Slaker2 mods\02-Research\evidence\post-exit-dataflow-2026-11-11\decoder")
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86_const import X86_OP_MEM, X86_REG_RSP

exe = Path(r"E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe")
expected = "2bd3f115b959a8eee7e5c4b42b0142dd0025bd437e1014f92dfc60b9afee5479"


def raw_offset(sections, rva):
    for _, start, virtual_size, raw, raw_size in sections:
        if start <= rva < start + raw_size:
            return raw + rva - start
    return None


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
        if begin and end > begin and text[1] <= begin < end <= text[1] + text[2]:
            ranges.append((begin, end))
    ranges.sort()
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    hits = {0x230: [], 0x30: []}
    seen = {0x230: set(), 0x30: set()}
    decoded_functions = 0
    decoded_instructions = 0
    stack_stores_skipped = 0
    for begin, end in ranges:
        raw = raw_offset(sections, begin)
        if raw is None:
            continue
        size = end - begin
        instructions = list(decoder.disasm(data[raw:raw + size], base + begin))
        if not instructions:
            continue
        decoded_functions += 1
        decoded_instructions += len(instructions)
        offsets = set()
        for ins in instructions:
            for operand in ins.operands:
                if operand.type == X86_OP_MEM and operand.mem.index == 0:
                    offsets.add(operand.mem.disp)
        fingerprints = {value for value in (0x234, 0x254, 0x259) if value in offsets}
        for ins in instructions:
            if not ins.operands or ins.operands[0].type != X86_OP_MEM or not (ins.operands[0].access & 2):
                continue
            operand = ins.operands[0]
            memory = operand.mem
            width = operand.size
            if memory.index != 0:
                continue
            if memory.base == X86_REG_RSP:
                stack_stores_skipped += 1
                continue
            start = memory.disp
            for field_offset in (0x230, 0x30):
                if start <= field_offset < start + width:
                    if ins.address in seen[field_offset]:
                        continue
                    seen[field_offset].add(ins.address)
                    hits[field_offset].append((ins.address - base, start, width,
                        ins.mnemonic, ins.op_str, begin, fingerprints))
    print(f"IMAGE_BASE={base:#x} TEXT_RVA={text[1]:#x} TEXT_SIZE={text[2]:#x}")
    print(f"DECODED_FUNCTION_RANGES={decoded_functions} DECODED_INSTRUCTIONS={decoded_instructions} STACK_STORES_SKIPPED={stack_stores_skipped}")
    for field_offset in (0x230, 0x30):
        rows = sorted(hits[field_offset])
        exact = [row for row in rows if row[1] == field_offset]
        overlap = [row for row in rows if row[1] != field_offset]
        print(f"FIELD_OFFSET={field_offset:#x} DIRECT_WRITES={len(rows)} EXACT_OFFSET={len(exact)} OVERLAPPING_WRITES={len(overlap)}")
        for rva, store_offset, width, mnemonic, op_str, function, fingerprints in rows:
            if field_offset == 0x30 and rva != 0x9cd2eb:
                continue
            flags = ",".join(f"{value:#x}" for value in sorted(fingerprints)) or "none"
            print(f"WRITE rva={rva:#x} function={function:#x} storeOffset={store_offset:#x} width={width} fingerprint={flags} insn={mnemonic} {op_str}")
