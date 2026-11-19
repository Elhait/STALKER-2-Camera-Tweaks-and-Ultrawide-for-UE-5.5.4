// Isolated M1 relation. No production imports, runtime callbacks, memory or IO.
// Native proof and native causal binding are explicit evidence inputs; this
// module never infers either from pointer/value equality.
function createAuthority(initialState) {
  const clone = value => JSON.parse(JSON.stringify(value));
  const state = clone(initialState || {
    revision: 0, settingsEpoch: 0, claims: {}, baseline: null, armings: {},
    nextArmingId: 1, effects: {}, nextEffectId: 1
  });
  function accept(event) {
    const w = clone(state);
    const decision = {accepted:false, action:'NoChange', correction:'PassThrough',
      effectId:null, reason:'guard-rejected'};
    const claim = event.scope ? w.claims[event.scope] : null;
    const commit = () => { w.revision=state.revision+1; Object.keys(state).forEach(k=>delete state[k]); Object.assign(state,w); };
    switch(event.type) {
      case 'ArmingAccepted': {
        if(!event.nativeCallerValidated || !event.scope) break;
        const id=w.nextArmingId++;
        if(claim && claim.activeArmingId) {
          const old=w.armings[claim.activeArmingId];
          if(old) old.status='superseded';
        }
        w.armings[id]={id,scope:event.scope,component:event.component||0,
          receiver:event.receiver||0,thread:event.thread||0,
          cinematicGeneration:event.cinematicGeneration||0,
          providerGeneration:event.providerGeneration||0,
          nativeStart:event.nativeStart,nativeTarget:event.nativeTarget,
          status:'armed',setterMatched:false,writerConfirmed:false};
        w.claims[event.scope]={...(claim||{}),activeArmingId:id};
        decision.accepted=true; decision.action='ArmingCommitted';
        decision.armingEventId=id; decision.reason='new-accepted-arming-event';
        commit(); return {decision,state:clone(state)};
      }
      case 'SetterEvidence':
      case 'WriterEvidence': {
        const arm=w.armings[event.armingEventId];
        // This event ID must be carried by a verified causal binding. Matching
        // the compatibility tuple alone is intentionally insufficient.
        if(!arm || arm.status!=='armed' || !event.causalBindingVerified ||
          arm.scope!==event.scope || arm.component!==event.component ||
          arm.receiver!==event.receiver || arm.thread!==event.thread ||
          arm.cinematicGeneration!==event.cinematicGeneration ||
          arm.providerGeneration!==event.providerGeneration) break;
        if(event.type==='SetterEvidence') {
          if(!event.nativeSetterBoundary || !event.argumentReadable) break;
          arm.setterMatched=true; decision.action='SetterBoundToArming';
        } else {
          if(!arm.setterMatched || !event.writerBoundary || !event.originalInputProvenance) break;
          arm.writerConfirmed=true; decision.action='WriterBoundToArming';
        }
        decision.accepted=true; decision.armingEventId=arm.id;
        decision.reason='causal-arming-binding'; commit();
        return {decision,state:clone(state)};
      }
      case 'GameplayAdmission':
        if(!event.nativeGameplayProof || !event.originalInputProvenance ||
          !event.scope || event.settingsEpoch!==w.settingsEpoch ||
          event.providerQuarantine) break;
        w.baseline={scope:event.scope,settingsEpoch:w.settingsEpoch,
          value:event.nativeFov,provenance:'native-gameplay',trust:'current'};
        decision.accepted=true; decision.action='GameplayAdmitted';
        decision.reason='source-scoped-native-gameplay-proof';
        commit(); return {decision,state:clone(state)};
      case 'EffectRequested':
        if(!event.committedPermission || !event.boundary || !event.scope) break;
        const id=w.nextEffectId++;
        w.effects[id]={id,scope:event.scope,boundary:event.boundary,
          dependencyRevision:w.revision,status:'requested'};
        decision.accepted=true; decision.action='EffectRequested';
        decision.effectId=id; decision.reason='committed-per-call-permission';
        commit(); return {decision,state:clone(state)};
      case 'EffectAttempted':
        // Effect result is a new fact. Failure cannot roll back unrelated
        // committed ownership and never fabricates Applied.
        const effect=w.effects[event.effectId];
        if(!effect || effect.status!=='requested' ||
          effect.scope!==event.scope || effect.boundary!==event.boundary ||
          effect.dependencyRevision!==event.dependencyRevision ||
          !['applied','refused','partial','unknown'].includes(event.result)) break;
        effect.status=event.result; decision.accepted=true;
        decision.action=event.result==='applied'?'AppliedAtNamedBoundary':
          event.result==='refused'?'EffectRefused':
          event.result==='partial'?'EffectPartial':'EffectUnknown';
        decision.reason='correlated-effect-result'; commit();
        return {decision,state:clone(state)};
      case 'SettingsRevoked':
        w.settingsEpoch++;
        for(const effect of Object.values(w.effects))
          if(effect.status==='requested') effect.status='revoked';
        decision.accepted=true; decision.action='PermissionsRevoked';
        decision.reason='settings-epoch-advanced'; commit();
        return {decision,state:clone(state)};
      default: break;
    }
    return {decision,state:clone(state)};
  }
  return {accept,read:()=>clone(state)};
}

function runM1Regressions() {
  const failures=[]; let checks=0;
  const check=(condition,name,actual)=>{checks++;if(!condition)failures.push({name,actual});};
  const auth=createAuthority();
  const tuple={scope:'camera-A',component:11,receiver:22,thread:33,
    cinematicGeneration:4,providerGeneration:8};
  const a1=auth.accept({type:'ArmingAccepted',nativeCallerValidated:true,...tuple,nativeStart:70,nativeTarget:90}).decision;
  const a2=auth.accept({type:'ArmingAccepted',nativeCallerValidated:true,...tuple,nativeStart:90,nativeTarget:80}).decision;
  check(a1.armingEventId!==a2.armingEventId,'rearm_gets_new_authority_identity',[a1,a2]);
  check(auth.read().armings[a1.armingEventId].status==='superseded','old_arm_retired_atomically',auth.read().armings);
  const oldSetter=auth.accept({type:'SetterEvidence',...tuple,armingEventId:a1.armingEventId,
    causalBindingVerified:true,nativeSetterBoundary:true,argumentReadable:true}).decision;
  check(!oldSetter.accepted,'late_old_setter_rejected',oldSetter);
  const tupleOnly=auth.accept({type:'SetterEvidence',...tuple,armingEventId:a2.armingEventId,
    causalBindingVerified:false,nativeSetterBoundary:true,argumentReadable:true}).decision;
  check(!tupleOnly.accepted,'same_tuple_does_not_prove_causal_binding',tupleOnly);
  const setter=auth.accept({type:'SetterEvidence',...tuple,armingEventId:a2.armingEventId,
    causalBindingVerified:true,nativeSetterBoundary:true,argumentReadable:true}).decision;
  check(setter.accepted,'verified_setter_binds_current_arm',setter);
  const writer=auth.accept({type:'WriterEvidence',...tuple,armingEventId:a2.armingEventId,
    causalBindingVerified:true,writerBoundary:true,originalInputProvenance:true}).decision;
  check(writer.accepted,'writer_requires_and_uses_same_arm',writer);

  const effects=createAuthority();
  effects.accept({type:'ArmingAccepted',nativeCallerValidated:true,...tuple});
  const requested=effects.accept({type:'EffectRequested',committedPermission:true,
    boundary:'camera-store',scope:'camera-A'}).decision;
  check(requested.accepted,'effect_request_is_correlated',requested);
  const ownerBefore=effects.read().claims['camera-A'].activeArmingId;
  const revoked=effects.accept({type:'SettingsRevoked'}).decision;
  const failed=effects.accept({type:'EffectAttempted',effectId:requested.effectId,
    scope:'camera-A',boundary:'camera-store',dependencyRevision:1,result:'refused'}).decision;
  check(revoked.accepted&& !failed.accepted,'revoked_effect_cannot_be_applied',failed);
  check(effects.read().claims['camera-A'].activeArmingId===ownerBefore,'effect_failure_keeps_independent_owner',effects.read().claims);
  check(effects.read().effects[requested.effectId].status==='revoked','no_fabricated_applied_after_revoke',effects.read().effects);

  for(const result of ['refused','partial','unknown','applied']) {
    const local=createAuthority();
    const req=local.accept({type:'EffectRequested',committedPermission:true,
      boundary:'output-post-store',scope:'camera-B'}).decision;
    const out=local.accept({type:'EffectAttempted',effectId:req.effectId,
      scope:'camera-B',boundary:'output-post-store',dependencyRevision:0,result}).decision;
    check(out.accepted,'named_effect_result_'+result,out);
    check((out.action==='AppliedAtNamedBoundary')===(result==='applied'),
      'applied_only_on_correlated_store_'+result,out);
  }
  return {checks,failures,passed:failures.length===0};
}
