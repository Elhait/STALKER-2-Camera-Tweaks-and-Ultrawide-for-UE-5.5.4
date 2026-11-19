# T01–T36: coverage of the research abstraction

Research relation calls, not production code branch coverage. `exact`, `valid`,
`nativeProof` and `original` are symbolic preconditions, not engine observations.
All families enter the all-family sequence alphabet. T35/T36 are explicitly
inactive/excluded; T10/T12/T33 reject and T32 observes rather than owning lifecycle.

| ID | Accepted | Rejected | Aborted cuts | Observation | Inactive | Abstraction / unverified detail |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| T01 | 1058 | 0 | 105 | 0 | 0 | Resource startup is abstracted to admission ready; thread lifecycle excluded |
| T02 | 8541 | 403 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T03 | 197 | 854 | 105 | 0 | 0 | Symbolic reference permission, not FOV formula or selector order implementation |
| T04 | 197 | 1154 | 105 | 0 | 0 | Witness compatibility supplied as symbolic evidence |
| T05 | 1994 | 6916 | 105 | 0 | 0 | No real aspect-store instruction or memory alias model |
| T06 | 650 | 8513 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T07 | 51 | 1287 | 105 | 0 | 0 | Mapping/progress accepted as symbolic inTrajectory; no float interpolation |
| T08 | 206 | 8685 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T09 | 1760 | 7130 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T10 | 0 | 1056 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T11 | 7866 | 113 | 105 | 932 | 0 | Replacement vs interleaving is explicit input, not inferred from new pointer |
| T12 | 0 | 1063 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T13 | 689 | 16333 | 105 | 0 | 0 | nativeProof is assumed source-bound; availability not proven |
| T14 | 103 | 951 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T15 | 103 | 960 | 105 | 0 | 0 | AspectRecalculation shares terminal abstraction; physical aspect effects excluded |
| T16 | 103 | 957 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T17 | 103 | 960 | 105 | 0 | 0 | Physical atomic handoff represented by correlated intent/result |
| T18 | 553 | 8341 | 105 | 0 | 0 | Component/thread/generation compatibility represented by exact flag |
| T19 | 133 | 16898 | 105 | 0 | 0 | Native matching guard represented by exact flag, not trajectory arithmetic |
| T20 | 90 | 8900 | 105 | 0 | 0 | Trajectory and writer confirmation abstracted to exact/inTrajectory |
| T21 | 106 | 8840 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T22 | 64 | 1285 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T23 | 4422 | 4472 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T24 | 2663 | 14105 | 105 | 0 | 0 | Descent/stability abstracted to descent/conflict flags |
| T25 | 17 | 1032 | 105 | 0 | 0 | Symbolic scoped transition; native guard extraction and C++ implementation unverified |
| T26 | 817 | 8358 | 105 | 0 | 0 | Terminal proof assumed; setter silence cannot synthesize it |
| T27 | 14 | 1044 | 105 | 0 | 0 | Only explicit scoped incompatibility, no numeric FOV-jump detector |
| T28 | 5373 | 3540 | 105 | 0 | 0 | Native proof assumed; no numerical baseline |
| T29 | 5112 | 154 | 105 | 0 | 0 | Relevant Gameplay setting invalidates reference; no physical pending-enable sequence |
| T30 | 12950 | 136 | 105 | 0 | 0 | Enable/policy abstracted; activation policy latch retained |
| T31 | 2535 | 7458 | 105 | 0 | 0 | Named argument/store/partial/refused results, no native memory effects |
| T32 | 0 | 120 | 105 | 934 | 0 | Observation-only; zoom numerical/neutral admission not implemented |
| T33 | 0 | 1049 | 105 | 0 | 0 | Failure injection at seven logical precommit boundaries, not every C++ return |
| T34 | 1047 | 0 | 105 | 0 | 0 | Logical permission revoke only, not platform drain/join |
| T35 | 0 | 0 | 105 | 0 | 1076 | Inactive: no production callers found in current inventory |
| T36 | 0 | 0 | 105 | 0 | 1052 | Excluded legacy/experimental profile: reject, not a simulated implementation |
