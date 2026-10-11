// Offline evidence only. Map file bytes as data; never call game code or Initialize.
// Reuse the existing fixture's actual runtime adapters and controlled dependencies.
#define main N06ExistingAdapterMain
#include "../../../../../tests/plugin/runtime_native_adapter_harness.cpp"
#undef main

#include <filesystem>
#include <fstream>
#include <iomanip>

namespace {
    struct Image {
        std::vector<std::uint8_t> file;
        std::uint8_t* base{};
        IMAGE_NT_HEADERS64* nt{};
        explicit Image(const char* path) {
            std::ifstream stream(std::filesystem::path(path), std::ios::binary | std::ios::ate);
            Check(stream.good(), "Cannot read game image");
            const auto size = stream.tellg();
            Check(size > 0, "Empty game image");
            file.resize(static_cast<std::size_t>(size));
            stream.seekg(0); stream.read(reinterpret_cast<char*>(file.data()), size);
            Check(stream.good() && file.size() >= sizeof(IMAGE_DOS_HEADER), "Incomplete game image");
            const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
            Check(dos->e_magic == IMAGE_DOS_SIGNATURE && dos->e_lfanew >= 0 &&
                static_cast<std::size_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) <= file.size(), "Invalid PE header");
            const auto* headers = reinterpret_cast<const IMAGE_NT_HEADERS64*>(file.data() + dos->e_lfanew);
            Check(headers->Signature == IMAGE_NT_SIGNATURE && headers->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 &&
                headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC, "Not AMD64 PE32+");
            Check(headers->OptionalHeader.SizeOfHeaders <= file.size() &&
                headers->OptionalHeader.SizeOfHeaders <= headers->OptionalHeader.SizeOfImage, "Invalid header span");
            const auto sectionEnd = reinterpret_cast<const std::uint8_t*>(IMAGE_FIRST_SECTION(headers) + headers->FileHeader.NumberOfSections);
            Check(sectionEnd <= file.data() + headers->OptionalHeader.SizeOfHeaders, "Invalid section table");
            base = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, headers->OptionalHeader.SizeOfImage,
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
            Check(base != nullptr, "Cannot allocate offline image");
            std::memcpy(base, file.data(), headers->OptionalHeader.SizeOfHeaders);
            nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
            const auto* section = IMAGE_FIRST_SECTION(headers);
            for (unsigned i = 0; i < headers->FileHeader.NumberOfSections; ++i, ++section) {
                Check(static_cast<std::uint64_t>(section->PointerToRawData) + section->SizeOfRawData <= file.size() &&
                    static_cast<std::uint64_t>(section->VirtualAddress) + (std::max)(section->SizeOfRawData, section->Misc.VirtualSize)
                        <= headers->OptionalHeader.SizeOfImage, "Invalid section range");
                std::memcpy(base + section->VirtualAddress, file.data() + section->PointerToRawData, section->SizeOfRawData);
                std::cout << "SECTION name=" << std::string(reinterpret_cast<const char*>(section->Name), strnlen(reinterpret_cast<const char*>(section->Name), 8))
                    << " rva=0x" << std::hex << section->VirtualAddress << " virtual_size=0x" << section->Misc.VirtualSize
                    << " raw_offset=0x" << section->PointerToRawData << " raw_size=0x" << section->SizeOfRawData
                    << " characteristics=0x" << section->Characteristics << std::dec << '\n';
            }
            DWORD old{};
            Check(VirtualProtect(base, headers->OptionalHeader.SizeOfImage, PAGE_READONLY, &old) != 0, "Protect image data");
            section = IMAGE_FIRST_SECTION(headers);
            for (unsigned i = 0; i < headers->FileHeader.NumberOfSections; ++i, ++section)
                if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                    Check(VirtualProtect(base + section->VirtualAddress, (std::max)(section->SizeOfRawData, section->Misc.VirtualSize),
                        PAGE_EXECUTE_READ, &old) != 0, "Protect offline executable section");
        }
        ~Image() { if (base) VirtualFree(base, 0, MEM_RELEASE); }
        std::uint64_t Rva(const void* address) const { return static_cast<const std::uint8_t*>(address) - base; }
    };

    void Hex(const std::uint8_t* data, std::size_t size) {
        for (std::size_t i = 0; i < size; ++i)
            std::cout << (i ? " " : "") << std::hex << std::setfill('0') << std::setw(2) << unsigned(data[i]);
        std::cout << std::dec;
    }
    ZydisDecodedInstruction Decode(const std::uint8_t* data, std::size_t size, ZydisDecodedOperand* operands) {
        ZydisDecoder decoder{}; ZydisDecodedInstruction instruction{};
        Check(ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)) &&
            ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, data, size, &instruction, operands)), "Decode failed");
        return instruction;
    }
    void Dump(const Image& image, const std::uint8_t* start, std::size_t size) {
        ZydisFormatter formatter{};
        Check(ZYAN_SUCCESS(ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL)), "Formatter failed");
        std::size_t offset = 0;
        while (offset < size) {
            ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
            const auto instruction = Decode(start + offset, size - offset, operands);
            char text[256]{};
            Check(ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&formatter, &instruction, operands,
                instruction.operand_count_visible, text, sizeof(text), image.nt->OptionalHeader.ImageBase + image.Rva(start + offset), nullptr)), "Format failed");
            std::cout << "  RVA=0x" << std::hex << image.Rva(start + offset) << std::dec << " bytes=";
            Hex(start + offset, instruction.length); std::cout << " | " << text << '\n';
            offset += instruction.length;
        }
    }
    void Noop(SafetyHookContext&) noexcept {}
    __declspec(noinline) void Destination() noexcept {}
    void Site(const Image& image, const char* name, std::uint8_t* target, safetyhook::MidHookFn callback) {
        Check(target != nullptr, "Missing hook target");
        std::cout << "HOOK " << name << " rva=0x" << std::hex << image.Rva(target) << std::dec << '\n';
        auto mid = safetyhook::MidHook::create(target, callback, safetyhook::MidHook::StartDisabled);
        Check(mid.has_value() && !mid->enabled(), "Disabled actual mid-hook construction failed");
        const auto original = mid->original_bytes();
        Check(std::memcmp(target, original.data(), original.size()) == 0, "Disabled mid-hook changed original image");
        std::cout << "  actual_callback_bound=yes enabled=no displaced_length=" << original.size() << " original=";
        Hex(original.data(), original.size()); std::cout << '\n';
        Dump(image, target, original.size());
        mid->reset();

        // Inspect bundled relocation directly. This hook is also NEVER enabled.
        auto in = safetyhook::InlineHook::create(target, &Destination, safetyhook::InlineHook::StartDisabled);
        Check(in.has_value() && !in->enabled(), "Disabled inline relocation construction failed");
        Check(in->original_bytes() == original, "Inline/mid displaced lengths differ");
        auto* relocated = in->trampoline().data();
        std::size_t oldOffset = 0, newOffset = 0;
        while (oldOffset < original.size()) {
            ZydisDecodedOperand before[ZYDIS_MAX_OPERAND_COUNT]{}, after[ZYDIS_MAX_OPERAND_COUNT]{};
            const auto a = Decode(target + oldOffset, original.size() - oldOffset, before);
            const auto b = Decode(relocated + newOffset, in->trampoline().size() - newOffset, after);
            Check(a.mnemonic == b.mnemonic, "Relocation changed mnemonic");
            for (unsigned i = 0; i < a.operand_count_visible; ++i) {
                if ((before[i].type == ZYDIS_OPERAND_TYPE_IMMEDIATE && before[i].imm.is_relative) ||
                    (before[i].type == ZYDIS_OPERAND_TYPE_MEMORY && before[i].mem.base == ZYDIS_REGISTER_RIP)) {
                    ZyanU64 first{}, second{};
                    Check(ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&a, &before[i], reinterpret_cast<std::uintptr_t>(target + oldOffset), &first)) &&
                        ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&b, &after[i], reinterpret_cast<std::uintptr_t>(relocated + newOffset), &second)) &&
                        first == second, "Relocation changed absolute branch/data target");
                    std::cout << "  relocated_reference old_offset=" << oldOffset << " new_offset=" << newOffset
                        << " target_rva=0x" << std::hex << image.Rva(reinterpret_cast<void*>(first)) << std::dec << " equal=yes\n";
                }
            }
            oldOffset += a.length; newOffset += b.length;
        }
        ZydisDecodedOperand continuation[ZYDIS_MAX_OPERAND_COUNT]{};
        const auto jmp = Decode(relocated + newOffset, in->trampoline().size() - newOffset, continuation);
        ZyanU64 destination{};
        Check(jmp.mnemonic == ZYDIS_MNEMONIC_JMP && ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&jmp, &continuation[0],
            reinterpret_cast<std::uintptr_t>(relocated + newOffset), &destination)) &&
            destination == reinterpret_cast<std::uintptr_t>(target + original.size()), "Relocated fallthrough changed");
        std::cout << "  relocated_continuation_rva=0x" << std::hex << image.Rva(target + original.size()) << std::dec
            << " structural_equal=yes executed=no\n";
        in->reset();
        Check(std::memcmp(target, original.data(), original.size()) == 0, "Inspection mutated offline image");
    }
}

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "Expected game executable path");
        Memory source; Setup(source);
        Image image(argv[1]); g_executable = reinterpret_cast<HMODULE>(image.base);
        constexpr char16_t engineText[] = u"Unreal Engine 5.5.4";
        const auto* needle = reinterpret_cast<const std::uint8_t*>(engineText);
        const auto engine = std::search(image.file.begin(), image.file.end(), needle, needle + sizeof(engineText));
        std::cout << "ENGINE_LITERAL exact_utf16=Unreal_Engine_5.5.4 found=" << (engine != image.file.end());
        if(engine != image.file.end()) std::cout << " file_offset=0x" << std::hex << (engine-image.file.begin()) << std::dec;
        std::cout << '\n';
        const auto version = ZydisGetVersion();
        std::cout << "ZYDIS version=" << ZYDIS_VERSION_MAJOR(version) << '.' << ZYDIS_VERSION_MINOR(version)
            << '.' << ZYDIS_VERSION_PATCH(version) << " packed=0x" << std::hex << version
            << " PE preferred_base=0x" << image.nt->OptionalHeader.ImageBase << " size=0x" << image.nt->OptionalHeader.SizeOfImage
            << " timestamp=0x" << image.nt->FileHeader.TimeDateStamp << std::dec << '\n';
        const auto writer = gameplay::ResolveCameraWriter(image.base);
        std::cout << "RESOLVER CameraWriter matches=" << writer.matches << " accepted=" << (writer.writeAddress != nullptr) << '\n';
        Check(writer.writeAddress != nullptr, "Actual Gameplay resolver refused image");
        std::uint8_t *aspect{}, *enter{}, *exit{};
        Check(ResolveCinematicAspectStore(aspect), "Actual cinematic aspect resolver refused image");
        Check(ResolveCinematicFovCallsites(enter, exit), "Actual cinematic FOV resolver refused image");
        const char* names[] = { "CinematicAspectSetter", "CinematicEnter", "CinematicExit", "CinematicExitIndexed", "DialogueBoundary", "ZoomIn", "ZoomOut" };
        const char* patterns[] = { CinematicAspectSetter, CinematicEnter, CinematicExit, CinematicExitIndexed, DialogueBoundary, ZoomIn, ZoomOut };
        for (unsigned i = 0; i < std::size(names); ++i) {
            const auto matches = hooks::FindAll(image.base, patterns[i]);
            std::cout << "SIGNATURE " << names[i] << " count=" << matches.size();
            for (auto* match : matches) std::cout << " match_rva=0x" << std::hex << image.Rva(match) << std::dec;
            std::cout << '\n';
        }
        std::cout << "CINEMATIC indexed=" << g_cinematicExitIndexed.load() << " consumer_rva=0x" << std::hex
            << image.Rva(hooks::validation::ResolveRel32CallTarget(enter, {image.base, image.nt->OptionalHeader.SizeOfImage})) << std::dec << '\n';
        std::cout << "ORIGINAL_CONTEXT GameplayWriter\n"; Dump(image, writer.writeAddress - hooks::signatures::FovWriteOffsetInCameraWriter, sizeof(hooks::signatures::CameraWriter));
        std::cout << "ORIGINAL_CONTEXT CinematicAspect\n"; Dump(image, aspect - 0x19, 35);
        std::cout << "ORIGINAL_CONTEXT CinematicENTER\n"; Dump(image, enter - 8, 13);
        std::cout << "ORIGINAL_CONTEXT CinematicEXITIndexed\n"; Dump(image, exit - 10, 15);
        Site(image, "GameplayWriter", writer.writeAddress, SafeMidHookEntry<&ReplayManualTransition>);
        Site(image, "CinematicAspect", aspect, SafeMidHookEntry<&ApplyCinematicAspectStore>);
        Site(image, "CinematicENTER", enter, SafeMidHookEntry<&TraceCinematicEnter>);
        Site(image, "CinematicEXIT", exit, SafeMidHookEntry<&TraceCinematicExit>);
        const auto dialogue = hooks::FindAll(image.base, DialogueBoundary);
        Check(dialogue.size() == 1, "Dialogue signature not unique");
        constexpr std::uint8_t indirect[] = {0xFF,0x90,0x08,0x06,0x00,0x00};
        Check(std::memcmp(dialogue[0] + kDialogueBoundaryHookOffset, indirect, sizeof(indirect)) == 0, "Dialogue byte check refused");
        std::cout << "ORIGINAL_CONTEXT Dialogue\n"; Dump(image, dialogue[0], 15);
        Site(image, "DialogueBoundary", dialogue[0] + kDialogueBoundaryHookOffset, SafeMidHookEntry<&TraceDialogueBoundary>);
        const auto zoomIn = hooks::FindAll(image.base, ZoomIn), zoomOut = hooks::FindAll(image.base, ZoomOut);
        Check(zoomIn.size() == 1 && zoomOut.size() == 1, "Zoom signatures not unique");
        constexpr std::uint8_t compare[] = {0x0F,0x2E,0xC8};
        for (auto* match : {zoomIn[0], zoomOut[0]})
            Check(std::memcmp(match + kZoomTransitionPostLoadOffset, compare, sizeof(compare)) == 0, "Zoom byte check refused");
        std::cout << "ZOOM hash_gate_expected=" << kZoomTransitionGameSha256 << '\n';
        std::cout << "ORIGINAL_CONTEXT ZoomIN\n"; Dump(image, zoomIn[0], 16);
        std::cout << "ORIGINAL_CONTEXT ZoomOUT\n"; Dump(image, zoomOut[0], 16);
        Site(image, "ZoomIN", zoomIn[0] + kZoomTransitionPostLoadOffset, SafeMidHookEntry<&TraceZoomIn>);
        Site(image, "ZoomOUT", zoomOut[0] + kZoomTransitionPostLoadOffset, SafeMidHookEntry<&TraceZoomOut>);
        for (const auto& line : observedLogs) std::cout << "ACTUAL_RESOLVER_LOG " << line << '\n';
        std::cout << "N06_STATIC_IMAGE_PROBE PASS: actual resolvers, exact game bytes, disabled hook creation and structural relocation; game code never executed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "N06_STATIC_IMAGE_PROBE FAIL: " << error.what() << '\n'; return 1; }
}
