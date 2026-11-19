"""Hash-pinned bounded scan for virtual calls through camera slot +0x628."""
import bisect
import hashlib
import mmap
import struct
import sys
from pathlib import Path

EVIDENCE_ROOT = Path(
    r"E:\Work\Slaker2 mods\02-Research\evidence\post-exit-dataflow-2026-11-11"
)
sys.path.insert(0, str(EVIDENCE_ROOT / "decoder"))
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_MEM

EXE = Path(
    r"E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl"
    r"\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe"
)
EXPECTED_SHA256 = "2bd3f115b959a8eee7e5c4b42b0142dd0025bd437e1014f92dfc60b9afee5479"
SLOT_DISPLACEMENT = 0x628
MAX_CANDIDATES = 256
MAX_FUNCTION_BYTES = 64 * 1024


def main():
    with EXE.open("rb") as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as image:
        digest = hashlib.sha256(image).hexdigest()
        if digest != EXPECTED_SHA256:
            raise SystemExit(f"IMAGE_HASH_MISMATCH {digest}")

        pe = struct.unpack_from("<I", image, 0x3C)[0]
        section_count = struct.unpack_from("<H", image, pe + 6)[0]
        optional_size = struct.unpack_from("<H", image, pe + 20)[0]
        optional = pe + 24
        image_base = struct.unpack_from("<Q", image, optional + 24)[0]
        sections = []
        for index in range(section_count):
            section = optional + optional_size + index * 40
            name = image[section:section + 8].rstrip(b"\0").decode("ascii")
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from(
                "<IIII", image, section + 8
            )
            sections.append((name, rva, virtual_size, raw_offset, raw_size))

        def raw_offset(rva):
            for _, start, _, raw, raw_size in sections:
                if start <= rva < start + raw_size:
                    return raw + rva - start
            raise ValueError(f"RVA outside raw section: {rva:#x}")

        text = next(section for section in sections if section[0] == ".text")
        text_rva, text_raw, text_size = text[1], text[3], text[4]
        exception_rva, exception_size = struct.unpack_from("<II", image, optional + 112 + 3 * 8)
        pdata = raw_offset(exception_rva)
        ranges = []
        for position in range(pdata, pdata + exception_size, 12):
            begin, end, _ = struct.unpack_from("<III", image, position)
            if begin and end > begin:
                ranges.append((begin, end))
        ranges.sort()
        starts = [start for start, _ in ranges]

        decoder = Cs(CS_ARCH_X86, CS_MODE_64)
        decoder.detail = True
        candidates = set()
        text_bytes = image[text_raw:text_raw + text_size]

        # CALL r/m64 with mod=10, reg=/2, disp32=+0x628. Search only this
        # vtable-slot access form, then validate instruction boundaries below.
        for modrm in range(0x90, 0x98):
            needle = b"\xff" + bytes((modrm,)) + struct.pack("<i", SLOT_DISPLACEMENT)
            cursor = 0
            while True:
                found = text_bytes.find(needle, cursor)
                if found < 0:
                    break
                cursor = found + 1
                instruction_start = found
                if found and 0x40 <= text_bytes[found - 1] <= 0x4F:
                    instruction_start -= 1
                candidates.add(text_rva + instruction_start)
                if len(candidates) > MAX_CANDIDATES:
                    raise SystemExit(f"CANDIDATE_LIMIT_EXCEEDED {MAX_CANDIDATES}")

        verified = []
        unbounded = []
        for call_rva in sorted(candidates):
            index = bisect.bisect_right(starts, call_rva) - 1
            if index < 0 or not ranges[index][0] <= call_rva < ranges[index][1]:
                unbounded.append(call_rva)
                continue
            begin, end = ranges[index]
            if end - begin > MAX_FUNCTION_BYTES:
                unbounded.append(call_rva)
                continue
            code = image[raw_offset(begin):raw_offset(begin) + end - begin]
            instructions = list(decoder.disasm(code, image_base + begin))
            match_index = next(
                (i for i, ins in enumerate(instructions) if ins.address - image_base == call_rva),
                None,
            )
            if match_index is None:
                continue
            ins = instructions[match_index]
            if ins.mnemonic != "call" or not ins.operands or ins.operands[0].type != CS_OP_MEM:
                continue
            if ins.operands[0].mem.disp != SLOT_DISPLACEMENT:
                continue
            verified.append((begin, end, instructions, match_index))

        print(f"SHA256 {digest} IMAGE_BASE {image_base:#x}")
        print(f"SLOT +0x{SLOT_DISPLACEMENT:x} candidates={len(candidates)} verified={len(verified)} unbounded={len(unbounded)}")
        output_accesses = 0
        output_accesses_before_setter = 0
        for begin, end, instructions, pivot in verified:
            call = instructions[pivot]
            out_register = None
            out_stack_offset = None
            for ins in reversed(instructions[max(0, pivot - 6):pivot]):
                if ins.operands and ins.reg_name(ins.operands[0].reg) == "r8":
                    if ins.mnemonic == "mov" and len(ins.operands) > 1 and ins.operands[1].type == 1:
                        out_register = ins.reg_name(ins.operands[1].reg)
                    elif ins.mnemonic == "lea" and len(ins.operands) > 1 and ins.operands[1].type == 3:
                        mem = ins.operands[1].mem
                        if ins.reg_name(mem.base) == "rsp" and mem.index == 0:
                            out_stack_offset = mem.disp
                    break
            accesses = []
            if out_register:
                for ins in instructions[pivot + 1:]:
                    if any(op.type == CS_OP_MEM and ins.reg_name(op.mem.base) == out_register
                            and op.mem.disp == 0x30 for op in ins.operands):
                        accesses.append(ins)
            if out_stack_offset is not None:
                target_disp = out_stack_offset + 0x30
                for ins in instructions[pivot + 1:]:
                    if any(op.type == CS_OP_MEM and ins.reg_name(op.mem.base) == "rsp"
                            and op.mem.index == 0 and op.mem.disp == target_disp
                            for op in ins.operands):
                        accesses.append(ins)
            output_accesses += len(accesses)
            if not accesses:
                continue
            print(f"\nOUTPUT_FOV_ACCESS caller={begin:#x} call={call.address - image_base:#x} output={out_register or ('rsp' + (f'{out_stack_offset:+#x}' if out_stack_offset is not None else '=unknown') )}")
            for access in accesses:
                print(f"  {access.address - image_base:08x} {access.mnemonic:8s} {access.op_str}")
                following = instructions[instructions.index(access) + 1:instructions.index(access) + 9]
                if any(ins.mnemonic == "call" and any(op.type == CS_OP_MEM and op.mem.disp == 0x608
                        for op in ins.operands) for ins in following):
                    output_accesses_before_setter += 1
                    print("    FOLLOWED_BY_VTABLE_SLOT_+0x608_WITHIN_8_INSNS=1")

        print(f"\nDIRECT_OUTPUT_FOV_ACCESSES={output_accesses} FOLLOWED_BY_SETTER_SLOT_+0x608={output_accesses_before_setter}")
        if unbounded:
            print("UNBOUNDED_CANDIDATE_RVAS " + " ".join(f"{rva:#x}" for rva in unbounded))


if __name__ == "__main__":
    main()
