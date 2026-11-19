// Research-only abstract transition relation. No production imports or I/O.
// Proof flags are symbolic assumptions, not native discoveries.
function runTransitionStudy() {
  const clone = x => JSON.parse(JSON.stringify(x));
  const ids = Array.from({length:36},(_,i)=>'T'+String(i+1).padStart(2,'0'));
  const fresh = () => ({c:0,bound:false,admitted:false,d:'none',r:'none',h:'none',
    consumed:false,policy:0,p:'unknown',w:'none',result:'unavailable',pending:null,observed:{},rg:0,hKey:null});
  const initial = () => ({ready:false,on:[true,true,true],selection:0,ge:0,b:null,
    scopes:[fresh(),fresh()]});
  const coverage = Object.fromEntries(ids.map(id=>[id,{accepted:0,rejected:0,aborted:0,observation:0,inactive:0}]));
  const limitations = {
    T01:'Resource startup is abstracted to admission ready; thread lifecycle excluded',
    T03:'Symbolic reference permission, not FOV formula or selector order implementation',
    T04:'Witness compatibility supplied as symbolic evidence',
    T05:'No real aspect-store instruction or memory alias model',
    T07:'Mapping/progress accepted as symbolic inTrajectory; no float interpolation',
    T11:'Replacement vs interleaving is explicit input, not inferred from new pointer',
    T13:'nativeProof is assumed source-bound; availability not proven',
    T15:'AspectRecalculation shares terminal abstraction; physical aspect effects excluded',
    T17:'Physical atomic handoff represented by correlated intent/result',
    T18:'Component/thread/generation compatibility represented by exact flag',
    T19:'Native matching guard represented by exact flag, not trajectory arithmetic',
    T20:'Trajectory and writer confirmation abstracted to exact/inTrajectory',
    T24:'Descent/stability abstracted to descent/conflict flags',
    T26:'Terminal proof assumed; setter silence cannot synthesize it',
    T27:'Only explicit scoped incompatibility, no numeric FOV-jump detector',
    T28:'Native proof assumed; no numerical baseline',
    T29:'Relevant Gameplay setting invalidates reference; no physical pending-enable sequence',
    T30:'Enable/policy abstracted; activation policy latch retained',
    T31:'Named argument/store/partial/refused results, no native memory effects',
    T32:'Observation-only; zoom numerical/neutral admission not implemented',
    T33:'Failure injection at seven logical precommit boundaries, not every C++ return',
    T34:'Logical permission revoke only, not platform drain/join',
    T35:'Inactive: no production callers found in current inventory',
    T36:'Excluded legacy/experimental profile: reject, not a simulated implementation'
  };
  function step(before,e,fault='',record=false) {
    let s=clone(before), q=s.scopes[e.src||0], status='rejected';
    const old=before.scopes[e.src||0], src=e.src||0;
    const out={kind:'pass',effect:false,baseline:'retain',applied:false,source:src,mapping:'none'};
    const accept=()=>status='accepted';
    const hasB=()=>s.b && s.b.src===src && s.b.epoch===s.ge && s.b.trust!=='invalid';
    const native=()=>e.nativeProof===true && e.original===true;
    const matches=()=>e.exact===true&&q.hKey!==null&&
      (e.component===undefined||e.component===q.hKey.component)&&
      (e.thread===undefined||e.thread===q.hKey.thread)&&
      (e.hGen===undefined||e.hGen===q.hKey.gen);
    const invalidate=()=>{if(s.b && s.b.src===src)s.b.trust='invalid';};
    const drop=()=>{q.d='none';q.h='none';q.hKey=null;q.consumed=false;q.r='none';q.pending=null;};
    const request=kind=>{
      out.kind=kind;
      out.mapping=q.p==='prepared'?(kind==='dialogue'?'HorPlusOnly':'PreservePrepared'):
        kind==='dialogue'?'ReducedThenHorPlus':kind==='gameplay'?'GameplayHorPlus':'CinematicMapping';
      const enabled=kind==='gameplay'?s.on[0]:kind==='cinematic'?s.on[1]:s.on[2];
      out.effect=s.ready && enabled;
      // pending is an observation correlation record, NEVER an executable right.
      // Executable right exists only in out.effect of this invocation.
      if(out.effect){q.pending={token:e.token||'call',c:q.c,ge:s.ge,kind};q.observed={};}
    };
    if(e.id==='T01'){s.ready=true;accept();}
    else if(e.id==='T34'){s.ready=false;s.b=null;s.scopes=[fresh(),fresh()];accept();}
    else if(e.id==='T35'||e.id==='T36'){status='inactive';}
    else if(!s.ready){status='rejected';}
    else switch(e.id) {
      case 'T02': if(e.valid){q.c=e.gen||1;q.bound=false;q.admitted=false;drop();if(hasB())s.b.trust='retained';accept();}break;
      case 'T03': if(q.c){out.kind=hasB()?'baseline-reference':'authored-fallback';accept();}break;
      case 'T04': if(q.c&&e.exact){q.w=e.prepared?'prepared':'original';accept();}break;
      case 'T05': if(q.c&&e.exact){q.bound=true;accept();}else if(q.c&&e.conflict){q.bound=false;q.admitted=false;accept();}break;
      case 'T06': if(q.c&&q.bound&&e.exact){q.admitted=true;q.p=e.prepared?'prepared':e.original?'original':'unknown';if(q.p!=='unknown')request('cinematic');accept();}break;
      case 'T07': if(q.c&&q.admitted&&e.inTrajectory){q.w=e.prepared?'prepared':'original';request('cinematic');accept();}break;
      case 'T08': if(q.c&&q.admitted&&e.exact){q.rg=q.c;q.c=0;q.bound=false;q.admitted=false;q.r='pending';q.pending=null;accept();}break;
      case 'T09': if(q.c&&!q.admitted&&e.exact){q.c=0;q.bound=false;q.r='none';q.pending=null;accept();}break;
      case 'T10': break;
      case 'T11': if(e.replacement){s.scopes[src]=fresh();q=s.scopes[src];if(s.b)s.b.trust='invalid';accept();}else{status='observation';}break;
      case 'T12': break; // foreign writer does not retire an existing owner
      case 'T13': case 'T15':
        if(q.r==='pending'&&e.exact&&native()) {
          q.r='complete';q.p='original';accept();
          if(e.transfer && (q.h==='confirmed'||q.h==='matched')) {
            q.h='transferred';q.consumed=true;q.d='active';q.p=e.prepared?'prepared':'original';request('dialogue');
          }else if(q.h==='armed'||q.h==='matched'||q.h==='confirmed') {out.baseline='exclude';}
          else if(fault==='stale-entry') {out.baseline='exclude';}
          else {q.d='none';s.b={src,epoch:s.ge,trust:'current'};out.baseline='update';request('gameplay');}
        }break;
      case 'T14': if(q.r==='pending'&&e.ownerMissing){q.r='none';q.pending=null;out.baseline='exclude';accept();}break;
      case 'T16': if(q.r==='pending'&&e.exact){if(e.terminalProof)q.r='complete';accept();}break;
      case 'T17': if(q.r==='pending'&&e.exact){request('cinematic');accept();}break;
      case 'T18': if(e.exact&&(q.r==='pending'||q.r==='complete')&&!q.c){q.h='armed';q.hKey={component:src+1,thread:src,gen:q.rg};q.consumed=false;q.policy=s.selection;accept();}break;
      case 'T19': if(q.h==='armed'&&matches()){q.h='matched';accept();}else if(fault==='mismatch-reset'){q.h='none';}break;
      case 'T20': if(q.h==='matched'&&matches()&&e.inTrajectory){q.h='confirmed';request('dialogue');accept();}break;
      case 'T21': if(q.h==='confirmed'&&matches()&&!q.consumed){q.r='complete';q.h='transferred';q.consumed=true;q.d='active';if(hasB())s.b.trust='retained';request('dialogue');accept();}break;
      case 'T22': if(q.d==='active'&&e.exact){q.p=e.prepared?'prepared':'original';if(e.argumentPrepared)q.result='argument';accept();}break;
      case 'T23': if(!q.c&&q.d==='none'&&e.valid){q.d='candidate';accept();}break;
      case 'T24': if(q.d==='candidate'&&e.exact){
        if(e.conflict){q.d='none';if(fault==='cancel-native'){q.p='original';s.b={src,epoch:s.ge,trust:'current'};out.baseline='update';}}
        else if(e.descent){q.d='active';q.policy=s.selection;if(hasB())s.b.trust='retained';request('dialogue');}accept();}break;
      case 'T25': if(q.d==='candidate'&&q.r!=='pending'&&!['armed','matched','confirmed'].includes(q.h)&&hasB()&&e.exact&&native()){q.d='none';q.p='original';s.b.trust='current';out.baseline='update';request('gameplay');accept();}break;
      case 'T26': if(q.d==='active'||q.d==='exiting'||q.d==='rearm'){
        if(e.terminalProof&&e.exact){q.d='none';q.h='none';q.pending=null;accept();}
        else if(e.ascent&&e.exact){q.d='rearm';accept();}
      }break;
      case 'T27': if(e.exact&&e.conflict){drop();q.p='unknown';accept();}break;
      case 'T28': if(native()&&!q.c&&q.r!=='pending'&&q.d!=='active'&&q.d!=='rearm'&&
        (fault==='quarantine-bypass'||!['armed','matched','confirmed'].includes(q.h))){
        const foreignVeto=fault==='global-candidate'&&s.scopes.some((x,i)=>i!==src&&x.d==='candidate');
        if(!foreignVeto){q.p='original';s.b={src,epoch:s.ge,trust:'current'};out.baseline='update';request('gameplay');accept();}
      }break;
      case 'T29': if(e.relevant){s.ge=1-s.ge;if(s.b)s.b.trust='invalid';for(const x of s.scopes)x.pending=null;}if(e.disable)s.on[0]=false;if(e.enable)s.on[0]=true;accept();break;
      case 'T30': if(e.policyChange)s.selection=1-s.selection;if(e.disable){s.on[1]=false;s.on[2]=false;for(const x of s.scopes)x.pending=null;}if(e.enable){s.on[1]=true;s.on[2]=true;}accept();break;
      case 'T31': if(q.pending&&e.exact&&q.pending.token===(e.token||'call')&&q.pending.c===q.c&&q.pending.ge===s.ge){
        q.result=e.result||'unavailable';out.applied=q.result==='store';
        q.observed[e.boundary||'post-store']=q.result;accept();
      } else if(fault==='late-resurrection'&&e.result==='store'){q.result='store';out.applied=true;}break;
      case 'T32': status='observation';break;
      case 'T33': break;
    }
    // Seven abstract logical cuts: staged changes must never escape before commit.
    if(e.abortAt!==undefined){
      if(fault==='partial-commit'&&e.abortAt>=2){/* deliberate mutation retains W */}
      else s=clone(before);
      out.effect=false;out.kind='pass';out.mapping='none';out.baseline='retain';out.applied=false;status='aborted';
    }
    if(record)coverage[e.id][status]++;
    return {s,out,status,old};
  }
  function check(before,e,r) {
    const {s,out,status}=r, src=e.src||0, q=s.scopes[src], old=before.scopes[src], errors=[];
    const assert=(ok,name)=>{if(!ok)errors.push(name);};
    if(status==='aborted')assert(JSON.stringify(s)===JSON.stringify(before),'no_partial_commit');
    if(out.effect)assert(s.ready&&s.on[out.kind==='gameplay'?0:out.kind==='cinematic'?1:2],'effect_gate');
    if(out.effect&&q.p==='prepared')assert(out.mapping!=='ReducedThenHorPlus'&&out.mapping!=='GameplayHorPlus','no_repeat_original_mapping');
    if(out.baseline==='update')assert(e.nativeProof&&e.original&&q.p==='original'&&!q.c&&q.d!=='active'&&q.d!=='rearm','native_baseline_only');
    if(out.baseline==='update')assert(q.r!=='pending'&&!['armed','matched','confirmed'].includes(q.h),'no_baseline_during_quarantine');
    if(out.applied)assert(e.id==='T31'&&e.exact&&old.pending&&old.pending.token===(e.token||'call')&&old.pending.c===old.c&&old.pending.ge===before.ge,'correlated_effect_only');
    if(e.id==='T19'&&!e.exact)assert(q.h===old.h,'foreign_mismatch_retains_handoff');
    if(['T19','T20','T21'].includes(e.id)&&old.hKey&&(
      (e.component!==undefined&&e.component!==old.hKey.component)||
      (e.thread!==undefined&&e.thread!==old.hKey.thread)||
      (e.hGen!==undefined&&e.hGen!==old.hKey.gen)))assert(JSON.stringify(q)===JSON.stringify(old)&&!out.effect,'provider_identity_guard');
    if(e.id==='T24'&&e.conflict)assert(JSON.stringify(s.b)===JSON.stringify(before.b)&&q.p===old.p,'cancel_not_native_proof');
    if((e.id==='T13'||e.id==='T15')&&old.r==='pending'&&e.exact&&e.nativeProof&&e.original&&!e.transfer&&old.h==='none'&&status!=='aborted')assert(out.baseline==='update'&&q.r==='complete','completion_no_repair_writer');
    if(e.id==='T28'&&e.nativeProof&&e.original&&!old.c&&old.r!=='pending'&&old.d!=='active'&&old.d!=='rearm'&&!['armed','matched','confirmed'].includes(old.h)&&before.ready&&status!=='aborted')assert(out.baseline==='update','foreign_candidate_no_veto');
    if(e.id==='T30'&&e.policyChange)assert(q.policy===old.policy,'activation_policy_latched');
    if((e.id==='T13'||e.id==='T15')&&e.transfer&&(old.h==='matched'||old.h==='confirmed')&&status==='accepted')assert(out.baseline!=='update'&&out.kind==='dialogue','transfer_has_no_intermediate_gameplay');
    if((e.id==='T13'||e.id==='T15')&&['armed','matched','confirmed'].includes(old.h)&&status==='accepted')assert(out.baseline!=='update','provider_quarantine_not_gameplay');
    assert(!(q.c&&(q.d==='active'||q.d==='rearm')),'no_same_scope_dual_owner');
    if(e.id!=='T01'&&e.id!=='T29'&&e.id!=='T30'&&e.id!=='T34')assert(JSON.stringify(s.scopes[1-src])===JSON.stringify(before.scopes[1-src]),'no_foreign_scope_mutation');
    return errors;
  }
  const event=(id,src=0,extra={})=>({id,src,...extra});
  const open=event('T01'), native=src=>event('T28',src,{nativeProof:true,original:true});
  const alphabet=[];
  for(let src=0;src<2;src++)alphabet.push(native(src),
    event('T02',src,{valid:true}),event('T05',src,{exact:true}),
    event('T06',src,{exact:true,original:true}),event('T08',src,{exact:true}),
    event('T09',src,{exact:true}),event('T13',src,{exact:true,nativeProof:true,original:true}),
    event('T13',src,{exact:true,nativeProof:true,original:true,transfer:true}),
    event('T18',src,{exact:true}),event('T19',src,{exact:true}),event('T19',src,{exact:false}),
    event('T20',src,{exact:true,inTrajectory:true}),event('T21',src,{exact:true}),
    event('T23',src,{valid:true}),event('T24',src,{exact:true,conflict:true}),
    event('T24',src,{exact:true,descent:true}),event('T26',src,{exact:true,terminalProof:true}),
    event('T11',src,{replacement:true}),event('T31',src,{exact:true,result:'store'}));
  alphabet.push(event('T29',0,{relevant:true}),event('T30',0,{disable:true}),event('T30',0,{enable:true}),event('T30',0,{policyChange:true}));
  // Deterministic fixtures seed each documented family, with positive/negative guards.
  const seeds=[initial()];
  let seed=step(initial(),open).s;seeds.push(seed);
  seed=step(seed,native(0)).s;seeds.push(seed);
  for(const e of [event('T02',0,{valid:true}),event('T05',0,{exact:true}),event('T06',0,{exact:true,original:true}),event('T08',0,{exact:true}),event('T18',0,{exact:true}),event('T19',0,{exact:true}),event('T20',0,{exact:true,inTrajectory:true}),event('T21',0,{exact:true})]){seed=step(seed,e).s;seeds.push(seed);}
  for(const d of ['none','candidate','active','rearm']){const x=clone(seeds[2]);x.scopes[0].d=d;seeds.push(x);}
  const positive={valid:true,exact:true,original:true,nativeProof:true,inTrajectory:true,descent:true,terminalProof:true,ownerMissing:true,relevant:true,policyChange:true,result:'store'};
  let fixtureChecks=0;const fixtureFailures=[];
  for(const s of seeds)for(const id of ids)for(const extra of [positive,{}, {...positive,conflict:true}, {...positive,prepared:true},
    {...positive,transfer:true}, {...positive,ascent:true,terminalProof:false},
    {...positive,result:'refused'}, {...positive,result:'partial'}, {...positive,result:'unavailable'}, {...positive,result:'argument'},
    {...positive,token:'foreign'}, {...positive,disable:true}, {...positive,enable:true}, {...positive,replacement:true}]){
    const e=event(id,0,extra),r=step(s,e,'',true);fixtureChecks++;const errors=check(s,e,r);if(errors.length)fixtureFailures.push({id,errors});
  }
  let identityChecks=0;
  for(const s of seeds)for(const id of ['T19','T20','T21'])for(const mismatch of [{component:9},{thread:9},{hGen:9}]){
    const e=event(id,0,{...positive,...mismatch}),r=step(s,e,'',true);identityChecks++;
    const errors=check(s,e,r);if(errors.length)fixtureFailures.push({id,mismatch,errors});
  }
  // Early abort cuts on all groups and fixtures, before publication/effects.
  let abortChecks=0;
  for(const s of seeds)for(const id of ids)for(let cut=0;cut<7;cut++){
    const e=event(id,0,{...positive,abortAt:cut}),r=step(s,e,'',true);abortChecks++;
    const errors=check(s,e,r);if(errors.length)fixtureFailures.push({id,cut,errors});
  }
  function search(fault,depthLimit,nodeLimit,edgeLimit,startState=null,domain=alphabet){
    const start=startState?clone(startState):step(initial(),open).s, queue=[{s:start,path:[]}], seen=new Set([JSON.stringify(start)]);
    let edges=0,deepest=0;const began=Date.now();
    for(let head=0;head<queue.length;head++){
      const n=queue[head];deepest=Math.max(deepest,n.path.length);if(n.path.length===depthLimit)continue;
      for(const base of domain){
        const variants=fault==='partial-commit'?[base,{...base,abortAt:2}]:[base];
        for(const e of variants){
          if(++edges>edgeLimit||seen.size>=nodeLimit||Date.now()-began>15000)return {fault,completeToDepth:false,depthLimit,deepest,states:seen.size,edges,limit:'explicit_budget',counterexample:null};
          const r=step(n.s,e,fault,!fault), errors=check(n.s,e,r),path=n.path.concat(e);
          if(errors.length)return {fault,completeToDepth:false,depthLimit,deepest,states:seen.size,edges,counterexample:{errors,path,shortestWithinAlphabet:true}};
          const key=JSON.stringify(r.s);if(!seen.has(key)){seen.add(key);queue.push({s:r.s,path});}
        }
      }
    }
    return {fault,completeToDepth:true,depthLimit,deepest,states:seen.size,edges,counterexample:null};
  }
  const bfs=search('',6,35000,600000);
  const seededBfs=seeds.slice(1,11).map((s,index)=>({seedIndex:index+1,initialState:s,
    result:search('',3,12000,100000,s)}));
  // All 36 inventory families participate in a separate sequence alphabet.
  // Invalid-only/observation/inactive IDs retain their explicit semantics.
  const fullAlphabet=ids.flatMap(id=>[event(id,0,positive),event(id,1,positive),event(id)]);
  for(const id of ['T04','T06','T07','T22'])fullAlphabet.push(event(id,0,{...positive,prepared:true}));
  for(const result of ['refused','partial','unavailable','argument'])fullAlphabet.push(event('T31',0,{...positive,result}));
  fullAlphabet.push(event('T13',0,{...positive,transfer:true}),event('T26',0,{exact:true,ascent:true}),
    event('T19',0,{...positive,component:9}),event('T29',0,{disable:true}),event('T30',0,{disable:true}));
  const allFamilyBfs=search('',3,35000,600000,null,fullAlphabet);
  const allFamilySeededBfs=seeds.slice(1,11).map((s,index)=>({seedIndex:index+1,
    result:search('',2,12000,100000,s,fullAlphabet)}));
  const faults=['stale-entry','cancel-native','mismatch-reset','global-candidate','late-resurrection','partial-commit'].map(f=>search(f,8,12000,180000));
  faults.push({...search('quarantine-bypass',3,12000,100000,seeds[7]),seedIndex:7,
    initialState:seeds[7]});
  // Long deterministic streams complement shallow BFS; they are not exhaustive.
  const streams=[];
  for(const domain of [alphabet,fullAlphabet])for(const fixedSeed of [1,7372,13472,26108]){
    let rng=fixedSeed>>>0,s=step(initial(),open).s,fail=null;
    for(let i=0;i<1200;i++){
      rng=(Math.imul(rng,1664525)+1013904223)>>>0;
      const e=domain[rng%domain.length],r=step(s,e,'',true),errors=check(s,e,r);
      if(errors.length){fail={step:i,event:e,errors};break;}s=r.s;
    }streams.push({domain:domain===alphabet?'focused':'all-families',seed:fixedSeed,steps:1200,failure:fail});
  }
  const scenarios=[];
  function scenario(name,events,assertions){
    let s=initial();const trace=[];let failure=null;
    for(let i=0;i<events.length;i++){
      const e=events[i],r=step(s,e,'',true),errors=check(s,e,r);
      const wanted=assertions[i];if(wanted&&!wanted(r))errors.push('scenario_expected_state');
      trace.push({event:e,status:r.status,decision:r.out,d:r.s.scopes[e.src||0].d,
        recovery:r.s.scopes[e.src||0].r,handoff:r.s.scopes[e.src||0].h,baseline:r.s.b});
      if(errors.length){failure={index:i,errors};break;}s=r.s;
    }scenarios.push({name,steps:trace.length,failure,trace});
  }
  const en=g=>event('T02',0,{valid:true,gen:g}),bind=event('T05',0,{exact:true}),
    write=event('T06',0,{exact:true,original:true}),exit=event('T08',0,{exact:true}),
    recovery=event('T13',0,{exact:true,nativeProof:true,original:true});
  const long=[open,native(0),en(1),event('T03'),bind,write,exit,
    event('T18',0,{exact:true}),event('T19',0,{exact:true}),event('T20',0,{exact:true,inTrajectory:true}),
    {...recovery,transfer:true},event('T22',0,{exact:true,prepared:true,argumentPrepared:true}),
    en(2),event('T03'),event('T04',0,{exact:true,prepared:true}),bind,
    event('T06',0,{exact:true,prepared:true}),event('T07',0,{inTrajectory:true,prepared:true}),exit,recovery,
    event('T23',0,{valid:true}),event('T25',0,{exact:true,nativeProof:true,original:true}),
    event('T23',0,{valid:true}),event('T24',0,{exact:true,descent:true}),
    event('T26',0,{exact:true,ascent:true}),event('T26',0,{exact:true,terminalProof:true}),native(0),
    en(3),bind,write,exit,recovery];
  scenario('three_cinematics_provider_dialogue_ordinary_dialogue_no_load',long,{
    10:r=>r.out.kind==='dialogue'&&r.out.baseline!=='update',
    13:r=>r.out.kind==='baseline-reference'&&r.s.b.trust==='retained',
    16:r=>r.out.mapping==='PreservePrepared',
    19:r=>r.s.b.trust==='current'&&r.out.baseline==='update',
    21:r=>r.s.scopes[0].d==='none',31:r=>r.out.baseline==='update'});
  for(const result of ['refused','partial','unavailable','argument','store']){
    scenario('effect_'+result,[open,native(0),event('T31',0,{exact:true,result})],{
      2:r=>r.out.applied===(result==='store')&&r.s.b.trust==='current'});
  }
  scenario('revoke_then_late_result',[open,native(0),event('T30',0,{disable:true}),
    event('T29',0,{relevant:true}),event('T31',0,{exact:true,result:'store'})],{
    4:r=>!r.out.applied&&r.status==='rejected'});
  scenario('foreign_candidate_independent_source',[open,event('T23',0,{valid:true}),native(1)],{
    2:r=>r.out.baseline==='update'&&r.s.scopes[0].d==='candidate'&&r.s.b.src===1});
  scenario('replacement_no_automatic_admission',[open,native(0),event('T11',1,{replacement:true}),
    event('T28',1,{original:true})],{2:r=>r.s.b.trust==='invalid',3:r=>r.out.baseline!=='update'});
  scenario('N_minus_1_result_and_named_boundaries',[open,{...native(0),token:'N-1'},
    {...native(0),token:'N'},event('T31',0,{exact:true,token:'N-1',result:'store'}),
    event('T31',0,{exact:true,token:'N',result:'store',boundary:'post-store'}),
    event('T31',0,{exact:true,token:'N',result:'store',boundary:'post-helper'})],{
      3:r=>r.status==='rejected'&&!r.out.applied,
      5:r=>r.s.scopes[0].observed['post-store']==='store'&&r.s.scopes[0].observed['post-helper']==='store'});
  return {kind:'independent_abstract_transition_relation',productionImports:false,
    scope:{sources:2,sourceAllocationReuse:false,numericalFov:false,proofs:'symbolic',nativeReachability:false,fullCppEarlyReturns:false},
    limitations,coverage,fixtureChecks,identityChecks,abortChecks,fixtureFailures,bfs,seededBfs,
    allFamilyBfs,allFamilySeededBfs,faults,streams,scenarios,
    conclusion:'Bounded model result only; no production, hook sufficiency or universal engine liveness proof'};
}
