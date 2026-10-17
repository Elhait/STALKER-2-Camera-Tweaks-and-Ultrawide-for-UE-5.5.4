"""Prepare byte-exact, non-loadable evidence; never rewrite or load an ASI.

Run under the project process-tree supervisor, with a 60-second bound.
One output directory is created exclusively; existing evidence is never replaced.
These section samples are for diagnostic review, not alternative mod builds.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import zipfile

sys.dont_write_bytecode = True

from inspect_asi_security import ROOT, LIMIT, inspect, read


def sha(data):
    return hashlib.sha256(data).hexdigest()


def archive_asi(path):
    with zipfile.ZipFile(path) as archive:
        info = archive.getinfo('STALKER2CameraTweaks.asi')
        if info.file_size > LIMIT or info.flag_bits & 1:
            raise ValueError('Unbounded or encrypted ASI input')
        return archive.read(info)


def samples(data):
    pe = inspect(data)
    result = {}
    for name in ('.text', '.rdata', '.rsrc'):
        matches = [s for s in pe['sections'] if s['name'] == name]
        if len(matches) != 1:
            raise ValueError('Missing or duplicate section: ' + name)
        section = matches[0]
        offset, size = section['offset'], section['size']
        if offset < 0 or offset + size > len(data):
            raise ValueError('Section outside file')
        blob = data[offset:offset + size]
        result[name] = (blob, dict(offset=offset, rva=section['rva'],
                                   size=size, sha256=sha(blob)))
    return pe, result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    artifact_root = (ROOT / 'build-artifacts').resolve()
    if artifact_root not in output.parents:
        raise ValueError('Output must be a new directory below build-artifacts')
    subjects = {
        'v3_1_0': archive_asi(ROOT / 'release-assets/STALKER2CameraTweaks-UE5.5.4-v3.1.0.zip'),
        'v3_2_0_published': archive_asi(ROOT / 'release-assets/STALKER2CameraTweaks-UE5.5.4-v3.2.0.zip'),
        'current': read(ROOT / 'STALKER2CameraTweaks.asi'),
    }
    # Parse all inputs before creating any output. No DLL loader is invoked.
    parsed = {label: samples(data) for label, data in subjects.items()}
    output.mkdir(parents=True, exist_ok=False)
    manifest = {'scope': 'Raw PE-section evidence, not executable diagnostic builds',
                'subjects': {}, 'interpretation': [
                    'Do not install, load, execute or distribute these samples as a mod.',
                    'No antivirus verdict is obtained by this tool; no upload is performed.',
                    'A matching positive sample narrows bytes recognized by that scanner.',
                    'A negative raw sample does not clear its subsystem: PE context is absent.',
                    'A section can contain several subsystems; this is not function attribution.',
                    'The v3.1 clean observation concerns its ZIP, not an independently scanned ASI.',
                ]}
    for label, (pe, sections) in parsed.items():
        target = output / label
        target.mkdir()
        entries = {}
        for name, (blob, metadata) in sections.items():
            filename = name.lstrip('.') + '.bin'
            with (target / filename).open('xb') as handle:
                handle.write(blob)
            if sha(read(target / filename)) != metadata['sha256']:
                raise ValueError('Evidence verification failed')
            entries[name] = dict(file=f'{label}/{filename}', **metadata)
        manifest['subjects'][label] = dict(
            source_sha256=pe['sha256'], source_size=pe['size'], sections=entries,
            imports=pe['imports'], resources=pe['resources'])
    old_resources = {tuple(r['path']): r for r in parsed['v3_1_0'][0]['resources']}
    manifest['resource_comparison'] = [
        dict(path=r['path'], size=r['size'], sha256=r['sha256'],
             identical_to_v3_1=(old_resources.get(tuple(r['path']), {}).get('sha256') == r['sha256']))
        for r in parsed['current'][0]['resources']]
    with (output / 'manifest.json').open('x', encoding='utf-8') as handle:
        json.dump(manifest, handle, indent=2)
    print(json.dumps(dict(output=str(output), samples=9,
                          sources={k: sha(v) for k, v in subjects.items()},
                          unchanged_resources=sum(r['identical_to_v3_1'] for r in manifest['resource_comparison']),
                          total_resources=len(manifest['resource_comparison']))))


if __name__ == '__main__':
    main()
