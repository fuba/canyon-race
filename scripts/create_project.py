"""Materialize the installed Epic VR template and its declared shared packs."""
import json
import re
import shutil
import uuid
from pathlib import Path

root = Path(__file__).resolve().parents[1]
engine = Path('E:/Epic Games/UE_5.8')
template = engine / 'Templates/TP_VirtualRealityBP'
project = root / 'CanyonRace'
if project.exists():
    raise SystemExit('CanyonRace already exists; refusing to overwrite')
(project / 'Config').mkdir(parents=True)
for filename in ['DefaultEditor.ini', 'DefaultEngine.ini', 'DefaultGame.ini', 'DefaultInput.ini']:
    text = (template / 'Config' / filename).read_text(encoding='utf-8-sig')
    text = text.replace('TP_VirtualRealityBP', 'CanyonRace')
    if filename == 'DefaultGame.ini':
        text = re.sub(r'ProjectID=\w+', 'ProjectID=' + uuid.uuid4().hex.upper(), text)
        text += '\nProjectName=CanyonRace\n'
    if filename == 'DefaultEngine.ini':
        # Keep PCVR configuration; these template sections only concern other targets.
        text = re.sub(r'(?ms)^\[/Script/(AndroidRuntimeSettings|AndroidFileServerEditor)[^\n]*\n.*?(?=^\[|\Z)', '', text)
        text = text.replace('r.ReflectionMethod=1', 'r.ReflectionMethod=0')
        text = text.replace('DefaultGraphicsRHI=DefaultGraphicsRHI_Default', 'DefaultGraphicsRHI=DefaultGraphicsRHI_DX12')
    (project / 'Config' / filename).write_text(text, encoding='utf-8')
defs = (template / 'Config/TemplateDefs.ini').read_text(encoding='utf-8-sig')
packs = re.findall(r'SharedContentPacks=\(MountName="([^"]+)",DetailLevels=\("([^"]+)"\)\)', defs)
for name, detail in packs:
    source = engine / 'Templates/TemplateResources' / detail / name / 'Content'
    shutil.copytree(source, project / 'Content' / name)
descriptor = json.loads((template / 'TP_VirtualRealityBP.uproject').read_text(encoding='utf-8-sig'))
descriptor['EngineAssociation'] = '5.8'
descriptor['Description'] = 'PCVR canyon racing development baseline; official XR template, not a finished racing game.'
descriptor['TargetPlatforms'] = ['Windows']
descriptor['DisableEnginePluginsByDefault'] = True
descriptor['Plugins'] = [dict(Name=name, Enabled=True) for name in
                         ['OpenXR', 'OpenXREyeTracker', 'OpenXRHandTracking',
                          'EnhancedInput', 'Niagara', 'XInputDevice', 'PythonScriptPlugin',
                          'AnimationData', 'ControlRig', 'IKRig']]
(project / 'CanyonRace.uproject').write_text(json.dumps(descriptor, indent=2), encoding='utf-8')
print(json.dumps(dict(project=str(project), copied_shared_packs=packs), indent=2))
