// Independent research relation check; no production imports or reducer.
// Finite domain: <= 10,000 alternatives; no I/O, callbacks, timers or threads.
// Latent alternatives are hypotheses, NOT observed reachable engine states.
function runAudit() {
  const worlds = {
    G: { mode: 'Gameplay', provenance: 'original', correction: 'GameplayHorPlus' },
    DR: { mode: 'Dialogue', provenance: 'original', correction: 'ReducedThenHorPlus' },
    DP: { mode: 'Dialogue', provenance: 'setter-prepared', correction: 'HorPlusWithoutSecondReduced' },
    CR: { mode: 'Cinematic', provenance: 'original', correction: 'CinematicMapping' },
    CP: { mode: 'Cinematic', provenance: 'cinematic-prepared', correction: 'RetainPreparedOrMappedContinuation' }
  };
  const cases = [
    ['writer_only_new_source', ['G','DR','DP','CR','CP'], 'No independent mode/provenance proof'],
    ['address_change_only', ['G','DR','DP','CR','CP'], 'Address change adds no engine mode fact'],
    ['dialogue_then_setter_silence', ['G','DR','DP'], 'Invisible termination is a hypothetical alternative, not a recorded event'],
    ['source_owned_native_recovery_proof', ['G'], 'Conditional on a validated source-bound terminal proof'],
    ['correlated_provider_original', ['DR'], 'Conditional on valid producer/setter/writer original evidence'],
    ['correlated_provider_setter_prepared', ['DP'], 'Conditional on valid prepared setter evidence'],
    ['cinematic_owner_without_input_origin', ['CR','CP'], 'Ownership alone does not establish original input'],
    ['cinematic_owner_with_corrected_witness', ['CP'], 'Conditional on compatible admitted witness'],
    ['candidate_cancel_only', ['G','DR','DP'], 'Cancelling a hypothesis adds no input provenance'],
    ['cancel_plus_independent_gameplay_proof', ['G'], 'The separate proof, not cancellation, narrows alternatives']
  ];
  const observations = cases.map(([name, candidates, assumption]) => {
    const required = [...new Set(candidates.map(k => worlds[k].correction))];
    return { name, latentAlternatives: candidates, requiredCorrections: required,
      uniqueCorrectionConditionalOnAssumptions: required.length === 1,
      commonSafeAction: 'PassThrough', assumption,
      engineReachabilityVerified: false };
  });
  // Permissions: baseline admission, fresh-native transform, store-confirmed
  // assertion, retirement of an old claim. Guards are declarative requirements
  // from design rev2, not a reimplementation of ProcessCameraWriter.
  function violations(f, p) {
    const v = [];
    if (p.admit && !(f.sameSource && f.nativeProof && f.original)) v.push('admit_without_native_source_proof');
    if (p.transform && !(f.sameSource && f.nativeProof && f.original)) v.push('fresh_transform_without_original_proof');
    if (p.applied && !f.storeObserved) v.push('intent_or_argument_is_not_store_confirmation');
    if (p.retire && !(f.sameScope && f.terminalProof)) v.push('retire_without_scoped_terminal_proof');
    return v;
  }
  let checked = 0, admissible = 0;
  for (let facts = 0; facts < 128; facts++) {
    const f = { sameSource: !!(facts&1), nativeProof: !!(facts&2), original: !!(facts&4),
      storeObserved: !!(facts&8), sameScope: !!(facts&16), terminalProof: !!(facts&32),
      cancelled: !!(facts&64) };
    for (let rights = 0; rights < 16; rights++) {
      if (++checked > 10000) throw new Error('finite budget exceeded');
      const p = { admit: !!(rights&1), transform: !!(rights&2), applied: !!(rights&4), retire: !!(rights&8) };
      if (violations(f,p).length === 0) admissible++;
    }
  }
  const none = { sameSource:false, nativeProof:false, original:false, storeObserved:false,
    sameScope:false, terminalProof:false, cancelled:false };
  const noRights = { admit:false, transform:false, applied:false, retire:false };
  const shortcuts = [
    ['cancel_implies_gameplay', {...none, sameSource:true, cancelled:true}, {...noRights, admit:true}],
    ['new_address_implies_gameplay', none, {...noRights, admit:true}],
    ['prepared_value_is_native', {...none, sameSource:true, nativeProof:true}, {...noRights, transform:true}],
    ['planned_output_is_applied', none, {...noRights, applied:true}],
    ['foreign_mismatch_retires_handoff', {...none, terminalProof:true}, {...noRights, retire:true}]
  ].map(([name,facts,rights]) => ({ name, violations: violations(facts,rights) }));
  // Show the known stale-entry error without copying production transitions.
  const completion = { entryOwner:'Recovery', terminalProof:true, committedOwner:'Gameplay' };
  const snapshotFirstBaselineAllowed = completion.entryOwner === 'Gameplay';
  const finalWorkingStateBaselineAllowed = completion.terminalProof && completion.committedOwner === 'Gameplay';
  return {
    kind:'bounded_independent_constraint_and_observability_check',
    productionImported:false, nativeCallbacksInvoked:false, combinations:checked,
    admissiblePermissionTuples:admissible, rejectedPermissionTuples:checked-admissible,
    observations, shortcutCounterexamples:shortcuts,
    completionOrdering:{ snapshotFirstBaselineAllowed, finalWorkingStateBaselineAllowed,
      conditionalOnValidNativeTerminalProof:true },
    limits:[ 'No engine reachability proof for latent alternatives',
      'No independent proof that current hooks supply each named proof',
      'No full T01-T36 transition implementation or model check',
      'PassThrough is intervention safety, not correct FOV availability',
      'Single-snapshot ambiguity is not identical complete native history' ]
  };
}
