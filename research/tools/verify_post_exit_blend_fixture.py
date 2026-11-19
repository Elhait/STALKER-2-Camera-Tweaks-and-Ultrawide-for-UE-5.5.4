"""Verify recorded evidence integrity, not camera behavior or a future fix.

Standard-library only; never launches a native tool, loads an ASI or writes a log.
"""
import argparse
import copy
from datetime import datetime
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import re


def require(condition, message):
    if not condition:
        raise ValueError(message)


def verify(fixture, raw):
    require(fixture['schema_version'] == 1, 'unsupported fixture schema')
    require(fixture['kind'] == 'recorded_observation_not_behavior_oracle', 'wrong evidence kind')
    require(fixture['future_expected_output_curve'] is None, 'unapproved output oracle')
    capture = fixture['capture']
    require(hashlib.sha256(raw).hexdigest().upper() == capture['sha256'], 'source hash mismatch')
    lines = raw.decode('utf-8-sig').splitlines()
    require(len(raw) == capture['size_bytes'] and len(lines) == capture['line_count'], 'source size mismatch')
    identity = re.search(r'modSha256=([A-F0-9]{64}) gameSha256=([A-F0-9]{64})', lines[0])
    require(identity is not None, 'runtime identity missing')
    require(identity.groups() == (capture['mod_sha256'], capture['game_sha256']), 'artifact identity mismatch')

    def parse(number):
        require(1 <= number <= len(lines), 'source line out of bounds')
        line = lines[number - 1]
        return {'line': number, 'timestamp': line[1:24],
                'marker': line.split('] ', 1)[-1].split(' ', 1)[0],
                'fields': dict(re.findall(r'(\w+)=([^\s]+)', line))}

    by_line = {}
    for record in fixture['records']:
        number = record['line']
        require(number not in by_line and record == parse(number), f'record altered: line {number}')
        by_line[number] = record
    require(len(by_line) == 66, 'canonical observation selection changed')
    tokens = fixture['opaque_tokens']
    camera = tokens['receiver']
    pairs = fixture['correlated_problem_samples']
    require(len(pairs) == 7, 'seven sampled correlations required')
    previous = 0
    for pair in pairs:
        blend = by_line[pair['blend_line']]
        writer = by_line[pair['writer_line']]
        b, w = blend['fields'], writer['fields']
        require(blend['marker'] == 'POST_EXIT_BLEND_SAMPLE' and
                writer['marker'] == 'POST_EXIT_BLEND_WRITER', 'wrong correlation boundary')
        require(previous < blend['line'] < writer['line'], 'correlation ordering changed')
        previous = writer['line']
        require(b['context'] == w['context'] == tokens['problem_blend'] and
                b['receiver'] == w['camera'] == camera, 'camera/context correlation mismatch')
        require(b['generation'] == w['generation'] == '2' and
                b['serial'] == w['candidate_serial'] and w['same_thread'] == '1', 'episode/serial/thread mismatch')
        require(b['readable_mask'] == '31' and b['start'] == '70' and
                b['target'] == '90.3115', 'start/target/readability mismatch')
        values = [b[k] for k in ('raw_xmm6', 'raw_xmm1', 'setter_argument')]
        values += [w[k] for k in ('raw_blend', 'setter_argument', 'writer_field', 'writer_input', 'writer_callback_output')]
        require(len({Decimal(value) for value in values}) == 1 and
                Decimal(w['input_minus_setter']) == 0, 'logged value chain changed')
        require(b['exit_attribution'] == w['exit_attribution'] == 'unknown' and
                b['after_setter'].removesuffix('.') ==
                w['after_setter'].removesuffix('.') == 'unobserved', 'unsupported evidence promotion')

    entry = fixture['problem_entry']
    summary = by_line[entry['cinematic_summary_line']]['fields']
    first = by_line[entry['first_writer_line']]['fields']
    require(summary['source'] == first['source'] == camera and
            summary['input_min'] == summary['input_max'] == '70' and
            summary['last_result'] == '108.941' and
            first['fov_input'] == first['fov_output'] == '70', 'cinematic/first-writer boundary changed')

    boundary = fixture['resumption_boundary']
    before = by_line[boundary['previous_logged_writer_line']]
    resumed = by_line[boundary['resume_writer_line']]
    a, b = before['fields'], resumed['fields']
    prior_runtime = [i for i in range(1, resumed['line'])
                     if 'RUNTIME_COMBINE ' in lines[i - 1] and f'source={camera} ' in lines[i - 1]]
    require(prior_runtime[-1] == before['line'], 'boundary is not adjacent logged writer evidence')
    for key, value in [('input_before', a['fov_input']), ('input_at_resume', b['fov_input']),
                       ('output_before', a['fov_output']), ('output_at_resume', b['fov_output'])]:
        require(Decimal(str(boundary[key])) == Decimal(value), f'boundary value changed: {key}')
    difference = Decimal(b['fov_output']) - Decimal(a['fov_output'])
    require(difference == Decimal(str(boundary['logged_output_difference_degrees'])), 'output difference changed')
    delta = datetime.fromisoformat(resumed['timestamp']) - datetime.fromisoformat(before['timestamp'])
    require(delta.total_seconds() * 1000 == boundary['logged_time_delta_ms'], 'logged interval changed')
    exact = by_line[boundary['exact_native_target_sample_line']]
    require(exact['fields']['context'] == tokens['problem_blend'] and
            exact['fields']['raw_xmm6'] == exact['fields']['target'] == '90.3115' and
            resumed['line'] < boundary['mod_completion_line'] < exact['line'], 'endpoint/completion ordering changed')

    witness = fixture['overlap_witness']
    low = by_line[witness['other_sample_before_line']]
    middle = by_line[witness['constant_sample_line']]
    high = by_line[witness['other_sample_after_line']]
    require(low['line'] < middle['line'] < high['line'], 'interleaving order changed')
    require(low['fields']['context'] == high['fields']['context'] == tokens['descending_blend'] and
            middle['fields']['context'] == tokens['constant_blend'] and
            tokens['constant_blend'] != tokens['descending_blend'], 'distinct contexts missing')
    require(all(x['fields']['receiver'] == camera for x in (low, middle, high)), 'interleaving camera mismatch')
    require(len({x['fields']['thread'] for x in (low, middle, high)}) == 1, 'interleaving thread mismatch')
    require(low['fields']['target'] == high['fields']['target'] == '70' and
            middle['fields']['raw_xmm6'] == middle['fields']['target'] == '90.3115' and
            middle['fields']['receiver_field_before_setter'] == low['fields']['raw_xmm6'], 'overlap witness changed')
    return {'evidence_integrity': 'PASS', 'records': len(by_line), 'sample_pairs': len(pairs),
            'logged_output_difference_degrees': str(difference), 'camera_fix_tested': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('fixture', type=Path)
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    require(args.fixture.stat().st_size < 256 * 1024, 'fixture exceeds bounded input size')
    fixture = json.loads(args.fixture.read_text(encoding='utf-8'))
    parent = args.fixture.resolve().parent
    source = (parent / fixture['capture']['file']).resolve()
    require(source.parent == parent and source.stat().st_size < 2 * 1024 * 1024, 'invalid/oversized source')
    raw = source.read_bytes()
    result = verify(fixture, raw)
    if args.self_test:
        for mutation in ('source_hash', 'sample', 'boundary', 'oracle'):
            altered = copy.deepcopy(fixture)
            if mutation == 'source_hash': altered['capture']['sha256'] = '0' * 64
            if mutation == 'sample': altered['correlated_problem_samples'][0]['writer_line'] = 802
            if mutation == 'boundary': altered['resumption_boundary']['previous_logged_writer_line'] = 952
            if mutation == 'oracle': altered['future_expected_output_curve'] = 'invented'
            try:
                verify(altered, raw)
            except ValueError:
                continue
            raise ValueError(f'negative integrity case was accepted: {mutation}')
        result['negative_integrity_cases'] = 4
    print(json.dumps(result))


if __name__ == '__main__':
    main()
