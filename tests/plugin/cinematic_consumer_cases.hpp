void ConsumerFixtureBegin(SafetyHookContext& context)
{
    if (!diagnostics::Enabled()) return;
    g_consumerInvocations.Begin({++g_runtime.consumerInvocationSequence,
        g_cinematicSelectionGeneration.load(), static_cast<std::uintptr_t>(context.rsp),
        0,0,0, GetCurrentThreadId(), context.xmm0.f32[0], context.xmm0.f32[0]});
}

void CinematicConsumerImageCase()
{
    // SEC_IMAGE_NO_EXECUTE maps bytes/sections only; no entry point, TLS or imports execute.
    struct Mapping {
        HANDLE file{INVALID_HANDLE_VALUE}, mapping{}; void* view{};
        ~Mapping() { if(view) UnmapViewOfFile(view); if(mapping) CloseHandle(mapping);
            if(file!=INVALID_HANDLE_VALUE) CloseHandle(file); }
    } image;
    image.file=CreateFileW(L"E:\\Steam\\steamapps\\common\\S.T.A.L.K.E.R. 2 Heart of Chornobyl\\Stalker2\\Binaries\\Win64\\Stalker2-Win64-Shipping.exe",
        GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
    Check(image.file!=INVALID_HANDLE_VALUE,"Read-only image file open failed");
    image.mapping=CreateFileMappingW(image.file,nullptr,PAGE_READONLY|SEC_IMAGE_NO_EXECUTE,0,0,nullptr);
    Check(image.mapping!=nullptr,"Non-executable image mapping failed");
    image.view=MapViewOfFile(image.mapping,FILE_MAP_READ,0,0,0);
    Check(image.view!=nullptr,"Read-only image view failed");
    hooks::ExecutableSpan span{};
    Check(hooks::FindSectionSpan(image.view,".text",span),"Image .text absent");
    const auto enters=hooks::FindAll(image.view,hooks::signatures::CinematicEnter);
    Check(enters.size()==1,"Image ENTER signature not unique");
    const auto sites=hooks::ResolveCinematicConsumerSites(enters.front()+8,span);
    Check(static_cast<bool>(sites),"Actual current image consumer contract rejected");
    hooks::ExecutableSpan data{};
    Check(hooks::FindSectionSpan(image.view,".rdata",data),"Image .rdata absent");
    hooks::ExecutableSpan writableData{};
    Check(hooks::FindSectionSpan(image.view,".data",writableData),"Image .data absent");
    const auto producer=hooks::ResolveBlendProducer(span,data,sites.wrapper,writableData);
    Check(producer.returnAddress && producer.matches==1,"Actual image producer path unresolved");
    std::cout << "producer_return_rva=0x" << std::hex
        <<producer.returnAddress-reinterpret_cast<std::uintptr_t>(image.view)
        <<std::dec<<" operand_observed="<<producer.operand<<" classification=structural\n";
    for(unsigned i=0;i<4;++i) std::cout<<"consumer_site="<<i<<" rva=0x"<<std::hex
        <<reinterpret_cast<std::uintptr_t>(sites.points[i])-reinterpret_cast<std::uintptr_t>(image.view)
        <<std::dec<<'\n';
}

void CinematicConsumerCases()
{
    diagnostics::ManualFovCapture control;
    Check(control.Initialize(GetCurrentProcessId()), "Manual event creation failed");
    Check(!control.Consume(), "Unsignalled event armed capture");
    diagnostics::ManualFovCapture duplicate;
    Check(!duplicate.Initialize(GetCurrentProcessId()), "Existing named event accepted as new owner");
    const auto signal = OpenEventW(EVENT_MODIFY_STATE, FALSE, control.Name());
    Check(signal && SetEvent(signal), "Manual user signal failed");
    Check(control.Consume() && control.Spent() && !control.Consume(), "Capture request was not consumed exactly once");
    Check(SetEvent(signal) && !control.Consume(), "Spent capture was rearmed");
    CloseHandle(signal); control.Close();
    const auto leaked = OpenEventW(EVENT_MODIFY_STATE, FALSE, control.Name());
    Check(!leaked, "Manual event ownership leaked after cleanup");

    diagnostics::ConsumerInvocationStack stack;
    diagnostics::ConsumerInvocation first{1,2,0x1000,0,0,0,7,90,90};
    Check(stack.Begin(first), "Invocation begin failed");
    auto mismatch = stack.Finish(0x1008,7,2);
    Check(mismatch.Outcome() == diagnostics::ConsumerOutcome::OutcomeUnobserved && !stack.Top(),
        "Stack mismatch did not retire evidence as OutcomeUnobserved");
    for (unsigned i=0; i<8; ++i) Check(stack.Begin(first), "In-bound invocation rejected");
    Check(!stack.Begin(first) && !stack.Top(), "Invocation overflow retained stale evidence");
    first.resolverSeen=true; first.provider=1; first.startSeen=true;
    stack.Begin(first);
    Check(stack.Finish(first.stack,first.thread,first.generation).Outcome() ==
        diagnostics::ConsumerOutcome::InitializerObservationReached, "Start evidence was not represented");

    diagnostics::ConsumerInvocationStack nested;
    diagnostics::ConsumerInvocation outer{101,4,0x2000,0,0,0,7,90,126.87f};
    diagnostics::ConsumerInvocation inner{102,4,0x3000,0x10,0,0,7,70,108.941f};
    inner.resolverSeen = inner.epilogueSeen = true;
    inner.equal = true;
    inner.comparisonSeen = true;
    Check(nested.Begin(outer) && nested.Begin(inner), "Nested invocation setup failed");
    const auto innerResult = nested.Finish(inner.stack,inner.thread,inner.generation);
    Check(innerResult.id == inner.id && innerResult.Outcome() ==
        diagnostics::ConsumerOutcome::ExistingTargetMatched && nested.Top() &&
        nested.Top()->id == outer.id, "Nested completion corrupted the outer invocation");
    const auto outerResult = nested.Finish(outer.stack,outer.thread,outer.generation);
    Check(outerResult.id == outer.id && outerResult.Outcome() ==
        diagnostics::ConsumerOutcome::OutcomeUnobserved && !nested.Top(),
        "Outer invocation inherited nested evidence");

    Check(nested.Begin(outer), "Stale-generation setup failed");
    const auto stale = nested.Finish(outer.stack,outer.thread,outer.generation + 1);
    Check(stale.Outcome() == diagnostics::ConsumerOutcome::OutcomeUnobserved && !nested.Top(),
        "Stale generation retained consumer outcome evidence");

    std::barrier tlsReady(2);
    std::array<std::uint64_t,2> tlsIds{};
    std::array<bool,2> tlsIsolated{};
    std::thread tlsA([&] {
        g_consumerInvocations.Clear();
        g_consumerInvocations.Begin({201,5,0x4000,0,0,0,GetCurrentThreadId(),90,90});
        tlsReady.arrive_and_wait();
        tlsIds[0] = g_consumerInvocations.Top() ? g_consumerInvocations.Top()->id : 0;
        tlsIsolated[0] = tlsIds[0] == 201;
        g_consumerInvocations.Clear();
    });
    std::thread tlsB([&] {
        g_consumerInvocations.Clear();
        g_consumerInvocations.Begin({202,5,0x5000,0,0,0,GetCurrentThreadId(),70,70});
        tlsReady.arrive_and_wait();
        tlsIds[1] = g_consumerInvocations.Top() ? g_consumerInvocations.Top()->id : 0;
        tlsIsolated[1] = tlsIds[1] == 202;
        g_consumerInvocations.Clear();
    });
    tlsA.join(); tlsB.join();
    Check(tlsIsolated[0] && tlsIsolated[1], "Interleaved native threads shared invocation frames");
    if (!diagnosticMode) return;

    const auto previousGeneration = g_cinematicSelectionGeneration.load();
    g_cinematicSelectionGeneration = 41;
    g_runtime.consumerObservationAvailable = true;
    for (unsigned path=0; path<8; ++path) {
        Memory code;
        std::array<std::uint8_t,0x3000> provider{};
        std::array<std::uint8_t,128> component{};
        std::array<std::uint8_t,16> resolvedObject{};
        const auto returned = (path==0 || path==7) ? std::uintptr_t{} : reinterpret_cast<std::uintptr_t>(resolvedObject.data());
        const std::uint32_t flags = path==1 ? (1u<<30) : 0;
        std::memcpy(resolvedObject.data()+8,&flags,4);
        const auto componentAddress = path==7 ? reinterpret_cast<std::uintptr_t>(provider.data()) :
            path>=4 ? reinterpret_cast<std::uintptr_t>(component.data()) : 0;
        std::memcpy(provider.data()+0x2CF8,&componentAddress,8);
        const float savedTarget = path==4 ? 90.0f : path==6 ? NAN : 70.0f;
        std::memcpy(component.data()+0x54,&savedTarget,4);
        const auto emit = [&](unsigned at, std::initializer_list<std::uint8_t> bytes) {
            std::copy(bytes.begin(),bytes.end(),code.bytes+at);
        };
        const auto call = [&](unsigned at,unsigned target) {
            code.bytes[at]=0xE8; code.Put<std::int32_t>(at+1, static_cast<std::int32_t>(target)-at-5);
        };
        const auto branch = [&](unsigned at,std::uint8_t op,unsigned target) {
            emit(at,{0x0F,op}); code.Put<std::int32_t>(at+2,static_cast<std::int32_t>(target)-at-6);
        };
        emit(0,{0x48,0x83,0xEC,0x28,0x0F,0x28,0xC1}); call(7,512);
        emit(12,{0x48,0x89,0xF1}); call(15,2300); emit(20,{0x48,0x83,0xC4,0x28,0xC3});
        emit(512,{0x56,0x48,0x83,0xEC,0x30,0x0F,0x29,0x74,0x24,0x20,0x0F,0x28,0xF0,0x48,0x89,0xCE});
        emit(528,{0x48,0x89,0xF1,0x0F,0x28,0xCE,0x0F,0x28,0x74,0x24,0x20,0x48,0x83,0xC4,0x30,0x5E,0xE9});
        code.Put<std::int32_t>(545,1024-549);
        constexpr unsigned w=1024;
        emit(w,{0x56,0x57,0x53,0x48,0x83,0xEC,0x30,0x0F,0x29,0x74,0x24,0x20,0x0F,0x28,0xF1,0x48,0x89,0xCE});
        call(w+18,2048); emit(w+23,{0x48,0x85,0xC0}); branch(w+26,0x84,w+108);
        emit(w+32,{0x8B,0x48,0x08,0x0F,0xBA,0xE1,0x1E}); branch(w+39,0x82,w+108);
        emit(w+45,{0x48,0x8B,0x8E,0xF8,0x2C,0,0,0x48,0x85,0xC9}); branch(w+55,0x85,w+96);
        call(w+61,2080); emit(w+66,{0x84,0xC0}); branch(w+68,0x84,w+108);
        emit(w+74,{0x0F,0x28,0xCE,0x0F,0x28,0x74,0x24,0x20,0x48,0x83,0xC4,0x30,0x5B,0x5F,0x5E,0xE9});
        code.Put<std::int32_t>(w+90,2200-(w+94)); code.bytes[2200]=0xC3;
        emit(w+96,{0xF3,0x0F,0x10,0x41,0x54,0x0F,0x2E,0xC6,0x75,0xE0,0x7A,0xDE,
            0x0F,0x28,0x74,0x24,0x20,0x48,0x83,0xC4,0x30,0x5B,0x5F,0x5E,0xC3});
        emit(2048,{0x48,0xB8}); code.Put<std::uintptr_t>(2050,returned); code.bytes[2058]=0xC3;
        emit(2080,{0xB0,static_cast<std::uint8_t>(path==3),0xC3}); code.bytes[2300]=0xC3;
        const hooks::ExecutableSpan span{code.bytes,4096};
        const auto sites=hooks::ResolveCinematicConsumerSites(code.bytes+7,span);
        Check(sites && sites.points[0]==code.bytes+12 && sites.points[1]==code.bytes+w+23 &&
            sites.points[2]==code.bytes+w+66 && sites.points[3]==code.bytes+w+108,
            "Linked consumer resolver did not identify fixture instruction boundaries");
        Check(!hooks::ResolveCinematicConsumerSites(code.bytes+7,{code.bytes,540}),
            "Truncated consumer span accepted");
        const auto beforeProvider=provider;
        const auto beforeComponent=component;
        code.Protect(PAGE_EXECUTE_READ);
        Check(FlushInstructionCache(GetCurrentProcess(),code.bytes,4096), "Consumer fixture cache flush");
        std::cerr << "consumer_fixture path=" << path << " stage=unhooked\n";
        reinterpret_cast<void(*)(void*,float)>(code.bytes)(provider.data(),90.0f);
        auto begin=safetyhook::MidHook::create(code.bytes+7,ConsumerFixtureBegin);
        std::array<safetyhook::MidHook,4> probes;
        const std::array<safetyhook::MidHookFn,4> callbacks{TraceCinematicConsumer<0>,TraceCinematicConsumer<1>,
            TraceCinematicConsumer<2>,TraceCinematicConsumer<3>};
        for(unsigned i=0;i<4;++i){
            auto hook=safetyhook::MidHook::create(sites.points[i],callbacks[i]);
            Check(static_cast<bool>(hook),"Consumer fixture hook installation failed"); probes[i]=std::move(*hook);
        }
        Check(static_cast<bool>(begin),"Consumer fixture ENTER hook failed");
        g_runtime.postExitBlendObservation.Begin(41,0,NAN);
        const auto logStart=observedLogs.size();
        std::cerr << "consumer_fixture path=" << path << " stage=hooked\n";
        reinterpret_cast<void(*)(void*,float)>(code.bytes)(provider.data(),90.0f);
        const auto expected=(path==0 || path==7) ? "outcome=ProviderNull" : path==1 ? "outcome=ResolverObjectFlagRejected" : path==2 ? "outcome=CreationGuardRejected" :
            path==4 ? "outcome=ExistingTargetMatched" : "outcome=OutcomeUnobserved";
        Check(std::any_of(observedLogs.begin()+logStart,observedLogs.end(),[&](const auto& line){
            return line.find(expected)!=std::string::npos && line.find("actual_return=1")!=std::string::npos;
        }),"Executed native consumer path lost or misclassified its actual outcome");
        Check(provider==beforeProvider && component==beforeComponent,"Consumer observer mutated native memory");
        for(auto& hook:probes) hook.reset(); begin->reset();
    }
    g_runtime.consumerObservationAvailable=false;
    g_runtime.postExitBlendObservation.End(); g_consumerInvocations.Clear();
    g_cinematicSelectionGeneration=previousGeneration;
}
