"""Prepare and revalidate an isolated iOS app copy with the freshly built mod.

Only JSON audit reports are retained. The supplied IPA and bundle stay unchanged;
this is a compatibility gate, not a signed/distributable application packager.
"""
import argparse
import json
from pathlib import Path
import plistlib
import subprocess
import tempfile
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--symgen', type=Path, required=True)
    parser.add_argument('--ipa', type=Path, required=True)
    parser.add_argument('--bundle', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='dawnlight-ios-check-') as tmp:
        binary, module = Path(tmp)/'Dusklight', Path(tmp)/'dawnlight.so'
        with zipfile.ZipFile(args.ipa) as ipa:
            infos = [n for n in ipa.namelist() if n.startswith('Payload/')
                     and n.count('/') == 2 and n.endswith('.app/Info.plist')]
            if len(infos) != 1:
                raise RuntimeError('Expected exactly one main app in the IPA')
            info = plistlib.loads(ipa.read(infos[0]))
            entry = infos[0].rsplit('/', 1)[0]+'/'+info['CFBundleExecutable']
            binary.write_bytes(ipa.read(entry))
        with zipfile.ZipFile(args.bundle) as bundle:
            module.write_bytes(bundle.read('lib/ios-arm64/mod.so'))
        for name, check in [('before', True), ('prepare', False), ('after', True)]:
            report = args.output/(name+'.json')
            command = [str(args.symgen.resolve()), 'prepatch', '--binary', str(binary),
                       '--report', str(report), str(module)]
            if check:
                command.append('--check')
            subprocess.run(command, check=True)
            data = json.loads(report.read_text())
            declarations = data['declarations']
            if not declarations or any(d.get('failure') for d in declarations):
                raise RuntimeError(f'{name}: missing or rejected hook declarations')
            if name == 'after' and any(not d.get('gateway_vmaddr') for d in declarations):
                raise RuntimeError('Prepared hook lacks a validated gateway')
            print(f'{name}: {len(declarations)} hook declarations validated', flush=True)


if __name__ == '__main__':
    main()
