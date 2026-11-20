import plistlib
import subprocess
import sys
from pathlib import Path

root = Path(sys.argv[1])
cpu = sys.argv[2]
for folder, extension in (('VST3', 'vst3'), ('AU', 'component'), ('Standalone', 'app')):
    bundle = root / folder / ('Skeuomorphic Space Panner.' + extension)
    contents = bundle / 'Contents'
    with (contents / 'Info.plist').open('rb') as f:
        metadata = plistlib.load(f)
    if metadata.get('CFBundleShortVersionString') != '1.1.0':
        raise SystemExit(f'Versione inattesa: {bundle}')
    if metadata.get('CFBundleIdentifier') != 'com.gett1.skeuomorphicspacepanner':
        raise SystemExit(f'Bundle identifier inatteso: {bundle}')
    executable = contents / 'MacOS' / metadata['CFBundleExecutable']
    archs = subprocess.check_output(['lipo', str(executable), '-archs'], text=True).split()
    if archs != [cpu]:
        raise SystemExit(f'Architetture inattese per {bundle}: {archs}')
    if folder == 'AU':
        keys = ('type', 'manufacturer', 'subtype')
        component = metadata['AudioComponents'][0]
        if tuple(component[k] for k in keys) != ('aufx', 'Gett', 'Spnr'):
            raise SystemExit('Identità AU diversa dalla 1.0.0')
    if folder == 'VST3':
        import re
        manifest = (contents / 'Resources' / 'moduleinfo.json').read_text()
        ids = set(re.findall(r'"CID"\s*:\s*"([^"]+)"', manifest))
        if ids != {'ABCDEF019182FAEB4765747453706E72', 'ABCDEF011234ABCD4765747453706E72'}:
            raise SystemExit(f'Class ID VST3 inattesi: {ids}')
    print(f'OK {folder} 1.1.0 {cpu}')
