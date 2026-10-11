#pragma once
#include <algorithm>
#include <span>
#include <vector>

// Offline executable fixtures for the exact Zoom load/compare/branch windows
// in game image 61BC...D56B. The runner verifies these byte arrays against the
// executable before building or running this harness. Only the code after the
// original branch destinations is a synthetic endpoint marker.
namespace {
    struct ZoomFixtureExpectation {
        std::uintptr_t rax{};
        std::uintptr_t rsi{};
        std::uintptr_t rcx{};
        std::uintptr_t rdx{};
        std::uintptr_t rspAtEntry{};
        std::uintptr_t rspAtExit{};
        float xmm0{};
        float xmm1{};
        std::uint32_t branch{};
    };

    constexpr std::array<std::uint8_t, 22> kZoomInGameBytes{
        0xF3,0x0F,0x10,0x40,0x4C,             // movss xmm0,[rax+4Ch]
        0xF3,0x0F,0x10,0x8E,0x38,0x01,0,0,  // movss xmm1,[rsi+138h]
        0x0F,0x2E,0xC8,                       // ucomiss xmm1,xmm0
        0x0F,0x87,0xC7,0x01,0,0               // jnbe 0x602C49
    };
    constexpr std::array<std::uint8_t, 18> kZoomOutGameBytes{
        0xF3,0x0F,0x10,0x40,0x50,             // movss xmm0,[rax+50h]
        0xF3,0x0F,0x10,0x8E,0x3C,0x01,0,0,  // movss xmm1,[rsi+13Ch]
        0x0F,0x2E,0xC8,                       // ucomiss xmm1,xmm0
        0x77,0x15                             // ja 0x602C39
    };

    constexpr std::size_t kZoomInCompareOffset = 13;
    constexpr std::size_t kZoomOutCompareOffset = 13;
    constexpr std::size_t kEpilogueOffset = 0x300;
    constexpr std::size_t kBranchMarkerOffset = 0x200;
    constexpr std::size_t kRaxOffset = 0x208;
    constexpr std::size_t kRsiOffset = 0x210;
    constexpr std::size_t kRcxOffset = 0x218;
    constexpr std::size_t kRdxOffset = 0x220;
    constexpr std::size_t kXmm0Offset = 0x228;
    constexpr std::size_t kXmm1Offset = 0x22C;
    constexpr std::size_t kRspExitOffset = 0x230;
    constexpr std::size_t kRspEntryOffset = 0x238;
    constexpr std::size_t kRflagsOffset = 0x240;
    constexpr std::size_t kGprBeforeOffset = 0x248;
    constexpr std::size_t kGprAfterOffset = 0x500;
    constexpr std::size_t kSimdBeforeOffset = 0x300;
    constexpr std::size_t kSimdAfterOffset = 0x400;
    constexpr std::array<unsigned, 11> kOtherGpr{3,5,7,8,9,10,11,12,13,14,15};

    void WriteBytes(std::vector<std::uint8_t>& code, std::size_t offset,
        std::span<const std::uint8_t> bytes) {
        Check(offset + bytes.size() <= code.size(), "Fixture instruction exceeds code page");
        std::copy(bytes.begin(), bytes.end(), code.begin() + offset);
    }
    void WriteBytes(std::vector<std::uint8_t>& code, std::size_t offset,
        std::initializer_list<std::uint8_t> bytes) {
        WriteBytes(code, offset, std::span<const std::uint8_t>(bytes.begin(), bytes.size()));
    }
    void WriteDisp32(std::vector<std::uint8_t>& code, std::size_t offset, std::uint32_t value) {
        Check(offset + sizeof(value) <= code.size(), "Fixture displacement exceeds code page");
        std::memcpy(code.data() + offset, &value, sizeof(value));
    }
    void EmitGprStore(std::vector<std::uint8_t>& code, std::size_t& cursor,
        unsigned reg, std::uint32_t displacement) {
        const std::uint8_t rex = reg >= 8 ? 0x4C : 0x48;
        const std::uint8_t modrm = static_cast<std::uint8_t>(0x82 | ((reg & 7) << 3));
        WriteBytes(code, cursor, {rex,0x89,modrm,0,0,0,0});
        WriteDisp32(code, cursor + 3, displacement);
        cursor += 7;
    }
    void EmitXmmStore(std::vector<std::uint8_t>& code, std::size_t& cursor,
        unsigned reg, std::uint32_t displacement) {
        if (reg >= 8) code[cursor++] = 0x44; // REX.R extends the XMM register number.
        WriteBytes(code, cursor, {0x0F,0x11,
            static_cast<std::uint8_t>(0x82 | ((reg & 7) << 3)),0,0,0,0});
        WriteDisp32(code, cursor + 3, displacement);
        cursor += 7;
    }

    std::size_t RelativeBranchDestination(std::span<const std::uint8_t> bytes,
        std::size_t compareOffset, std::size_t branchOffset, std::size_t branchSize) {
        std::int32_t displacement{};
        if (branchSize == 4) {
            Check(branchOffset + 6 <= bytes.size(), "Zoom IN branch bytes are truncated");
            std::memcpy(&displacement, bytes.data() + branchOffset + 2, sizeof(displacement));
        } else {
            Check(branchSize == 1 && branchOffset + 2 <= bytes.size(),
                "Unsupported game-derived Zoom branch encoding");
            displacement = static_cast<std::int8_t>(bytes[branchOffset + 1]);
        }
        const auto instructionLength = branchSize == 4 ? std::size_t{6} : std::size_t{2};
        const auto target = static_cast<std::int64_t>(compareOffset + branchOffset + instructionLength) + displacement;
        Check(target >= 0 && target < 4096, "Relocated fixture branch target is outside its page");
        return static_cast<std::size_t>(target);
    }

    void WriteContinuationMarker(std::vector<std::uint8_t>& code,
        std::size_t offset, std::uint32_t marker) {
        // Synthetic, bounded endpoint replacing the engine body at the exact
        // game branch/fallthrough destination. The source branch itself stays
        // byte-identical and SafetyHook relocates it during execution.
        WriteBytes(code, offset, {
            0xC6,0x82,0x00,0x02,0x00,0x00,static_cast<std::uint8_t>(marker), // mov byte [rdx+200h], marker
            0x9C,                               // pushfq: capture native compare flags
            0x8F,0x82,0x40,0x02,0x00,0x00,    // pop qword [rdx+240h], preserves flags
            0xE9,0,0,0,0
        });
        const auto jumpEnd = offset + 19;
        const auto relative = static_cast<std::int32_t>(kEpilogueOffset - jumpEnd);
        std::memcpy(code.data() + offset + 15, &relative, sizeof(relative));
    }

    std::vector<std::uint8_t> BuildZoomFixture(
        std::span<const std::uint8_t> gameBytes, bool entering,
        std::size_t& hookOffset, std::size_t& takenOffset, std::size_t& fallthroughOffset) {
        std::vector<std::uint8_t> code(4096, 0x90);
        std::size_t cursor{};
        auto emit = [&](std::initializer_list<std::uint8_t> bytes) {
            WriteBytes(code, cursor, bytes);
            cursor += bytes.size();
        };
        auto emitSpan = [&](std::span<const std::uint8_t> bytes) {
            WriteBytes(code, cursor, bytes);
            cursor += bytes.size();
        };

        emit({0x56});                         // push rsi; preserve Windows nonvolatile RSI
        emit({0x4C,0x89,0xC6});               // mov rsi,r8 (synthetic fixture setup)
        emit({0x48,0x89,0xC8});               // mov rax,rcx (synthetic fixture setup)
        emit({0x48,0x89,0xA2,0x38,0x02,0,0});// save post-prologue RSP in result
        const auto loadLength = entering ? kZoomInCompareOffset : kZoomOutCompareOffset;
        emitSpan(gameBytes.first(loadLength));
        for (unsigned reg = 0; reg < 16; ++reg)
            EmitXmmStore(code, cursor, reg, static_cast<std::uint32_t>(kSimdBeforeOffset + reg * 16));
        for (std::size_t i = 0; i < kOtherGpr.size(); ++i)
            EmitGprStore(code, cursor, kOtherGpr[i],
                static_cast<std::uint32_t>(kGprBeforeOffset + i * sizeof(std::uint64_t)));
        hookOffset = cursor;
        emitSpan(gameBytes.subspan(loadLength));

        const auto branchOffset = std::size_t{3}; // UCOMISS is followed by the conditional branch
        const auto branchSize = entering ? std::size_t{4} : std::size_t{1};
        takenOffset = RelativeBranchDestination(gameBytes.subspan(loadLength), 0,
            branchOffset, branchSize) + hookOffset;
        fallthroughOffset = hookOffset + gameBytes.size() - loadLength;
        Check(takenOffset != fallthroughOffset, "Zoom branch target aliases fallthrough");
        Check(takenOffset - hookOffset == (entering ? 0x1D0u : 0x1Au),
            "Game-derived Zoom branch destination differs from the verified image");
        WriteContinuationMarker(code, fallthroughOffset, 0);
        WriteContinuationMarker(code, takenOffset, 1);

        cursor = kEpilogueOffset;
        emit({0x48,0x89,0x82,0x08,0x02,0,0}); // save RAX
        emit({0x48,0x89,0xB2,0x10,0x02,0,0}); // save RSI at the hook continuation
        emit({0x48,0x89,0x8A,0x18,0x02,0,0}); // save RCX
        emit({0x48,0x89,0x92,0x20,0x02,0,0}); // save RDX
        emit({0xF3,0x0F,0x11,0x82,0x28,0x02,0,0}); // save XMM0 low lane
        emit({0xF3,0x0F,0x11,0x8A,0x2C,0x02,0,0}); // save XMM1 low lane
        emit({0x48,0x89,0xA2,0x30,0x02,0,0}); // save RSP before epilogue
        for (unsigned reg = 0; reg < 16; ++reg)
            EmitXmmStore(code, cursor, reg, static_cast<std::uint32_t>(kSimdAfterOffset + reg * 16));
        for (std::size_t i = 0; i < kOtherGpr.size(); ++i)
            EmitGprStore(code, cursor, kOtherGpr[i],
                static_cast<std::uint32_t>(kGprAfterOffset + i * sizeof(std::uint64_t)));
        emit({0x5E,0xC3});                    // pop rsi; ret
        return code;
    }

    std::uint64_t ReadResult64(const Memory& output, std::size_t offset) {
        return output.Get<std::uint64_t>(offset);
    }

    void RunZoomGameDerivedFixture(bool entering) {
        Memory raxSource, rsiSource, output;
        Setup(raxSource);
        const auto gameBytes = entering
            ? std::span<const std::uint8_t>(kZoomInGameBytes)
            : std::span<const std::uint8_t>(kZoomOutGameBytes);
        const auto compareOffset = entering ? kZoomInCompareOffset : kZoomOutCompareOffset;
        std::size_t hookOffset{}, takenOffset{}, fallthroughOffset{};
        const auto codeBytes = BuildZoomFixture(gameBytes, entering,
            hookOffset, takenOffset, fallthroughOffset);
        auto* code = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, codeBytes.size(),
            MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
        Check(code != nullptr, "Could not allocate isolated Zoom fixture code");
        std::memcpy(code, codeBytes.data(), codeBytes.size());
        FlushInstructionCache(GetCurrentProcess(), code, codeBytes.size());

        const auto original = entering
            ? std::span<const std::uint8_t>(kZoomInGameBytes)
            : std::span<const std::uint8_t>(kZoomOutGameBytes);
        Check(std::memcmp(code + hookOffset, original.data() + compareOffset,
            original.size() - compareOffset) == 0,
            "Displaced bytes differ from the game-derived compare/branch window");
        using Entry = void(*)(void*, void*, void*);
        const auto entry = reinterpret_cast<Entry>(code);
        const auto callback = entering ? SafeMidHookEntry<&TraceZoomIn> : SafeMidHookEntry<&TraceZoomOut>;
        auto created = safetyhook::MidHook::create(code + hookOffset, callback,
            safetyhook::MidHook::StartDisabled);
        Check(created.has_value(), "Game-derived Zoom SafetyHook creation failed");
        auto hook = std::move(*created);
        Check(!hook.enabled(), "Game-derived Zoom hook was not created disabled");
        Check(hook.enable().has_value(), "Game-derived Zoom hook enable failed");

        constexpr std::uint32_t canary = 0xA5A5A5A5;
        output.Put(0x1FC, canary);
        output.Put(0x2FC, canary);
        output.Put(0x5FC, canary);
        output.Put(0x610, canary);

        SafetyHookContext invalid{};
        invalid.rsi = rsiSource.Address();
        invalid.rax = 0;
        if (entering) SafeMidHookEntry<&TraceZoomIn>(invalid);
        else SafeMidHookEntry<&TraceZoomOut>(invalid);

        const auto sourceAddress = raxSource.Address();
        const auto rsiAddress = rsiSource.Address();
        const auto initialize = [&](float value0, float value1) {
            raxSource.Put(0x4C, entering ? value0 : 0.0f);
            raxSource.Put(0x50, entering ? 0.0f : value0);
            rsiSource.Put(0x138, entering ? value1 : 0.0f);
            rsiSource.Put(0x13C, entering ? 0.0f : value1);
            output.Put<std::uint8_t>(kBranchMarkerOffset, 0xFF);
        };
        const auto executeAndCheck = [&](float value0, float value1, bool taken) {
            initialize(value0, value1);
            entry(reinterpret_cast<void*>(sourceAddress),
                reinterpret_cast<void*>(output.Address()), reinterpret_cast<void*>(rsiAddress));
            Check(output.Get<std::uint8_t>(kBranchMarkerOffset) == (taken ? 1u : 0u),
                "Relocated native Zoom branch reached the wrong continuation");
            Check(ReadResult64(output, kRaxOffset) == sourceAddress &&
                ReadResult64(output, kRsiOffset) == rsiAddress &&
                ReadResult64(output, kRcxOffset) == sourceAddress &&
                ReadResult64(output, kRdxOffset) == output.Address(),
                "Native Zoom callback/continuation corrupted a general register");
            Check(output.Get<std::uintptr_t>(kRspEntryOffset) ==
                output.Get<std::uintptr_t>(kRspExitOffset),
                "Native Zoom continuation changed stack depth");
            const auto flags = output.Get<std::uint64_t>(kRflagsOffset);
            const auto expectedFlags = static_cast<std::uint64_t>(taken ? 0 : 1);
            Check((flags & 0x895u) == expectedFlags,
                "Native Zoom continuation did not preserve UCOMISS branch flags");
            Near(output.Get<float>(kXmm0Offset), value0,
                "Native Zoom callback changed XMM0 input");
            Near(output.Get<float>(kXmm1Offset), value1,
                "Native Zoom callback changed XMM1 input");
            for (unsigned reg = 0; reg < 16; ++reg)
                Check(std::memcmp(output.bytes + kSimdBeforeOffset + reg * 16,
                    output.bytes + kSimdAfterOffset + reg * 16, 16) == 0,
                    "Native Zoom callback changed an XMM register");
            for (std::size_t i = 0; i < kOtherGpr.size(); ++i)
                Check(ReadResult64(output, kGprBeforeOffset + i * sizeof(std::uint64_t)) ==
                    ReadResult64(output, kGprAfterOffset + i * sizeof(std::uint64_t)),
                    "Native Zoom callback changed an unrelated general register");
            Check(output.Get<std::uint32_t>(0x1FC) == canary &&
                output.Get<std::uint32_t>(0x2FC) == canary &&
                output.Get<std::uint32_t>(0x5FC) == canary &&
                output.Get<std::uint32_t>(0x610) == canary,
                "Native Zoom replay wrote outside its result record");
        };

        if (entering) {
            executeAndCheck(2.0f, 1.0f, false);
            executeAndCheck(1.0f, 2.0f, true);
        } else {
            executeAndCheck(1.0f, 2.0f, true);
            executeAndCheck(2.0f, 1.0f, false);
        }
        Check(g_runtime.snapshotZoomObservationValid.load(std::memory_order_acquire) &&
            g_runtime.snapshotZoomSource.load(std::memory_order_acquire) == rsiAddress,
            "Production Zoom callback did not publish the game-derived RSI source");
        Check(g_runtime.snapshotZoomDirection.load(std::memory_order_acquire) == (entering ? 1 : 2),
            "Production Zoom callback published the wrong direction");
        Check(g_runtime.snapshotZoomPrimary.load(std::memory_order_acquire) ==
                raxSource.Get<float>(0x4C) &&
            g_runtime.snapshotZoomSecondary.load(std::memory_order_acquire) ==
                raxSource.Get<float>(0x50),
            "Production Zoom callback observations differ from the native RAX fields");

        Check(hook.disable().has_value(), "Game-derived Zoom hook disable failed");
        Check(std::memcmp(code + hookOffset, original.data() + compareOffset,
            original.size() - compareOffset) == 0,
            "SafetyHook did not restore the exact game-derived instruction bytes");
        hook.reset();
        VirtualFree(code, 0, MEM_RELEASE);
    }
}
