void U2ShadowCases()
{
    Memory initialCamera;
    Setup(initialCamera);
    using diagnostics::EvaluateU2Shadow;
    diagnostics::U2ShadowInput input{true,true,true,true,126.87f,70.0f,98.435f,126.87f,32.0f/9.0f};
    const auto midpoint=EvaluateU2Shadow(input);
    Check(midpoint.available && std::string_view(midpoint.status)=="conditional-affine-hypothesis",
        "Shadow asserted verified branch or rejected affine hypothesis");
    Near(midpoint.weight,0.5f,"Transformed-start weight");
    Near(midpoint.hypothetical,(126.87f+camera::HorPlus(70,32.0f/9.0f,16.0f/9.0f))*0.5f,
        "Shadow reapplied HorPlus to transformed start");
    input.sample=input.start;
    Near(EvaluateU2Shadow(input).hypothetical,input.correctedEndpoint,"Start continuity");
    input.sample=input.target;
    Check(std::fabs(EvaluateU2Shadow(input).hypothetical-108.941f)<0.01f,"Target HorPlus");
    diagnostics::U2ShadowInput enterInput{true,true,true,true,70.0f,126.87f,
        70.0f,118.55f,32.0f/9.0f,126.87f};
    const auto enterStart=EvaluateU2Shadow(enterInput);
    Check(enterStart.available && std::fabs(enterStart.hypothetical-118.55f)<0.01f &&
        std::fabs(enterStart.correctedTarget-126.87f)<0.001f,
        "Cinematic ENTER shadow maps native start from the prior policy output to its corrected target");
    enterInput.sample=126.87f;
    const auto enterEnd=EvaluateU2Shadow(enterInput);
    Check(enterEnd.available && std::fabs(enterEnd.hypothetical-126.87f)<0.01f,
        "Cinematic ENTER shadow preserves the corrected target endpoint");
    input.start=70; input.target=90.3115f; input.sample=80;
    Check(EvaluateU2Shadow(input).available,"Native-start hypothesis unavailable");
    for(unsigned i=0;i<4;++i) {
        auto invalid=input;
        if(i==0) invalid.enabled=false;
        if(i==1) invalid.matchedActivation=false;
        if(i==2) invalid.receiverMatches=false;
        if(i==3) invalid.policyEligible=false;
        Check(!EvaluateU2Shadow(invalid).available,"Missing authority promoted to shadow evidence");
    }
    input.start=input.target;
    Check(!EvaluateU2Shadow(input).available,"Degenerate endpoints divided");
    input.start=70;input.sample=130;
    Check(!EvaluateU2Shadow(input).available,"Foreign sample clamped into shadow");
    gameplay::HorPlusRecoverySample recovery{};
    recovery.source=recovery.validatedSource=1; recovery.inputFov=115;
    recovery.cachedCinematicFov=126.87f;recovery.exitNativeTarget=90.3115f;
    recovery.aspect=recovery.policyAspect=32.0f/9.0f;
    recovery.policyAspectValid=recovery.cameraReadable=true;recovery.flags=4;
    Check(std::string_view(diagnostics::U2ShadowWriterAction(recovery,CoordinatorState::CinematicExiting,0.02f))=="would-remap",
        "Shadow concealed downstream remapping hazard");
    Check(std::string_view(diagnostics::U2ShadowWriterAction(recovery,CoordinatorState::Gameplay,0.02f))=="not-recovery",
        "Gameplay control mislabeled recovery");

    Memory code;
    auto* p=code.bytes+64;
    const std::uint8_t signature[]{0x48,0x8D,0x74,0x24,0x20,0x48,0x89,0xF2,0xE8,0,0,0,0,
        0x48,0x8B,0x0E,0x48,0x85,0xC9,0x74,0x0D,0xF3,0x0F,0x10,0x0D,0,0,0,0,0xE8,0,0,0,0};
    std::memcpy(p,signature,sizeof(signature));
    const auto rel=[&](unsigned offset,unsigned end,unsigned target){
        const auto d=static_cast<std::int32_t>((code.bytes+target)-(p+end));
        std::memcpy(p+offset,&d,4);
    };
    rel(9,13,512); rel(25,29,1024); rel(30,34,768);
    code.Put<float>(1024,70);
    const hooks::ExecutableSpan text{code.bytes,1000}, data{code.bytes+1024,64};
    auto found=hooks::ResolveBlendProducer(text,data,code.bytes+768);
    Check(found.matches==1 && found.returnAddress==reinterpret_cast<std::uintptr_t>(p+34),"Structural producer rejected");
    code.Put<float>(1024,83);
    Check(hooks::ResolveBlendProducer(text,data,code.bytes+768).returnAddress==found.returnAddress,
        "Producer classification depends on literal FOV");
    Check(!hooks::ResolveBlendProducer(text,data,code.bytes+769).returnAddress,"Foreign wrapper accepted");
    Check(!hooks::ResolveBlendProducer({code.bytes,97},data,code.bytes+768).returnAddress,"Truncated span accepted");
    std::memcpy(code.bytes+128,p,34);
    Check(!hooks::ResolveBlendProducer(text,data,code.bytes+768).returnAddress,"Duplicate producer accepted");

    diagnostics::ManualU2Controls controls;
    Check(controls.Initialize(GetCurrentProcessId()),"Control event initialization failed");
    wchar_t name[96]{};swprintf_s(name,L"Local\\STALKER2CameraTweaks.U2Control.%lu",GetCurrentProcessId());
    const auto signal=OpenEventW(EVENT_MODIFY_STATE,FALSE,name);
    Check(signal && !controls.Consume(),"Unsignaled control consumed");
    for(unsigned i=1;i<=3;++i) {
        Check(SetEvent(signal),"Control signal failed");
        Check(controls.Consume()==(i<=2 ? i : 0),"Control exceeded two-phase budget");
    }
    CloseHandle(signal);controls.Close();
    const auto leaked=OpenEventW(EVENT_MODIFY_STATE,FALSE,name);
    if(leaked) CloseHandle(leaked);
    Check(!leaked,"Control handle leaked");
    diagnostics::DialogueBoundaryObservationGate gate;
    gate.Arm(0);
    diagnostics::DialogueBoundaryObservation control{};
    control.callbackFov=70;
    Check(gate.Observe(true,control).has_value(),"Gameplay-only generation zero not observed");
    for(unsigned i=1;i<=5;++i) {
        control.callbackFov=70+0.12f*static_cast<float>(i);
        static_cast<void>(gate.Observe(true,control));
    }
    Check(gate.Emitted()==2,"Slow cumulative FOV change was suppressed forever");
    gate.Arm(0,diagnostics::DialogueBoundaryObservationGate::Clock::now()-std::chrono::seconds(21));
    Check(!gate.Observe(true,control),"Expired control emitted");

    // Existing actual native adapters verify register/memory pass-through ON/OFF.
    PostExitBlendObservationCases();
    Memory cameraObject,blendObject,outputObject;
    g_runtime.postExitBlendLayoutAvailable=true;
    g_runtime.postExitBlendObservation.Begin(42,cameraObject.Address(),90.3115f);
    g_runtime.postExitBlendActivationObservation.Begin(42);
    g_coordinator=CoordinatorState::CinematicExiting;
    g_runtimeGameplayEnabled=true;g_runtimeCinematicsEnabled=true;
    g_activeCinematicSettingsEpoch=g_cinematicsSettingsEpoch.load();
    g_runtimeGameplayMode=config::GameplayMode::RuntimeCombine;
    g_exitCinematicTransformedFov=126.87f;
    cinematics::CinematicWriterAssociationSnapshot owner{};
    owner.generationActive=owner.associationValid=true;owner.generation=42;
    owner.effectiveSettings.enabled=true;
    owner.aspectStoreTargetObject=owner.associatedWriterRsi=cameraObject.Address();
    owner.lastAdmittedOutput={true,42,1,cameraObject.Address(),outputObject.Address()};
    Check(g_cinematicRecoveryOwner.Capture(owner,42),"Shadow recovery fixture owner invalid");
    g_cinematicWriterAssociation.ObserveWriter(CoordinatorState::CinematicExiting,
        cameraObject.Address(),outputObject.Address(),100,32.0f/9.0f,true,4,true);
    blendObject.Put<float>(0x58,126.87f);blendObject.Put<float>(0x54,70);
    cameraObject.Put<float>(0x230,100);
    diagnostics::BlendStartCapture start{};
    start.component=blendObject.Address();start.receiver=cameraObject.Address();
    start.start=126.87f;start.target=70;
    start.gameplaySettingsEpoch=g_gameplaySettingsEpoch;
    start.cinematicSettingsEpoch=g_cinematicsSettingsEpoch;
    start.shadowAspect=32.0f/9.0f;
    Check(g_runtime.postExitBlendActivationObservation.ObserveStartCapture(start)!=nullptr,"Shadow activation not captured");
    auto context=Context(blendObject,outputObject,100);
    context.rsi=blendObject.Address()+0x28;context.rcx=cameraObject.Address();
    context.xmm6.f32[0]=context.xmm1.f32[0]=100;
    const auto contextBefore=context;
    {
        auto lock=g_cameraCorrectionGate.Acquire();
        PostExitBlendCallbackCapture capture(context);
        Check(capture.sample.shadow.available==diagnosticMode,"Actual adapter shadow availability incorrect");
    }
    UnchangedContext(contextBefore,context);
    Near(cameraObject.Get<float>(0x230),100,"Shadow wrote native camera");
    ++g_gameplaySettingsEpoch;
    {
        auto lock=g_cameraCorrectionGate.Acquire();
        PostExitBlendCallbackCapture capture(context);
        Check(!capture.sample.shadow.available,"Settings change retained shadow mapping");
    }
    g_runtime.postExitBlendObservation.End();
    g_runtime.postExitBlendActivationObservation.Retire();
    g_runtime.postExitBlendLayoutAvailable=false;
}
