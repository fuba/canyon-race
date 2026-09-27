"""Prepare runtime data and project settings without changing the original USGS files."""
import json
from pathlib import Path
import numpy as np
import rasterio

root = Path(__file__).resolve().parents[1]
project = root / 'CanyonRace'
out = project / 'Content/Canyon/Data'
out.mkdir(parents=True, exist_ok=True)
with rasterio.open(root / 'data/terrain/horseshoe_bend_dem.tif') as src:
    height = src.read(1)
assert height.shape == (1009, 1009) and np.isfinite(height).all()
datum = float(height.min())
sample = ((height[::2, ::2] - datum) * 100).astype('<f4')
assert sample.shape == (505, 505)
sample.tofile(out / 'terrain.f32')
# Hand-selected centerline samples from the USGS hillshade; +X east, +Y south.
pixels = [(195,60),(195,115),(170,165),(125,208),(85,250),(66,290),
          (83,327),(135,353),(200,365),(270,384),(329,414),(374,455),
          (396,500),(385,541),(350,571),(298,585),(244,580),(189,564),
          (130,543),(67,524)]
rows = []
for x, y in pixels:
    assert height[y,x] - datum < 25, (x,y,float(height[y,x]-datum))
    rows.append(f'{x*400-201600},{y*400-201600}')
(out / 'route.csv').write_text('\n'.join(rows)+'\n',encoding='utf-8')
(out / 'provenance.json').write_text(json.dumps({
    'source': 'USGS 3DEP', 'source_metadata': '../../../../data/terrain/horseshoe_bend.json',
    'source_url':'https://elevation.nationalmap.gov/arcgis/rest/services/3DEPElevation/ImageServer',
    'grid_size':505,'spacing_m':8,'datum_m':datum,
    'format':'little-endian float32 centimeters above datum; rows south, columns east',
    'note':'8 m runtime sampling is not native source accuracy. Procedural mesh prototype, not a Landscape import.'
},indent=2),encoding='utf-8')
descriptor = json.loads((project/'CanyonRace.uproject').read_text(encoding='utf-8'))
descriptor['Modules'] = [{'Name':'CanyonRace','Type':'Runtime','LoadingPhase':'PostConfigInit'}]
descriptor['Description'] = 'PCVR hover racing prototype on USGS canyon terrain.'
for plugin in ['ProceduralMeshComponent','EditorScriptingUtilities','XRBase']:
    if not any(p['Name']==plugin for p in descriptor['Plugins']):
        descriptor['Plugins'].append({'Name':plugin,'Enabled':True})
(project/'CanyonRace.uproject').write_text(json.dumps(descriptor,indent=2),encoding='utf-8')
engine = project/'Config/DefaultEngine.ini'
text = engine.read_text(encoding='utf-8')
text = text.replace('/Game/XRFramework/Blueprints/BP_XRGameMode.BP_XRGameMode_C','/Script/CanyonRace.CanyonGameMode')
text = text.replace('/Game/XRFramework/Levels/L_XRTemplate.L_XRTemplate','/Game/Canyon/Maps/L_Horseshoe.L_Horseshoe')
engine.write_text(text,encoding='utf-8')
game = project/'Config/DefaultGame.ini'
text=game.read_text(encoding='utf-8').replace('bStartInVR=True','bStartInVR=False')
if 'DirectoriesToAlwaysStageAsUFS=(Path="Canyon/Data")' not in text:
    text+='\n[/Script/UnrealEd.ProjectPackagingSettings]\n+DirectoriesToAlwaysStageAsUFS=(Path="Canyon/Data")\n+MapsToCook=(FilePath="/Game/Canyon/Maps/L_Horseshoe")\n'
text=text.replace('DirectoriesToAlwaysCook=(Path="/Game/Canyon/Materials")','DirectoriesToAlwaysCook=(Path="/Game/Canyon")')
if 'DirectoriesToAlwaysCook=(Path="/Game/Canyon")' not in text:
    text+='\n[/Script/UnrealEd.ProjectPackagingSettings]\n+DirectoriesToAlwaysCook=(Path="/Game/Canyon")\n'
game.write_text(text,encoding='utf-8')
input_file=project/'Config/DefaultInput.ini'
text=input_file.read_text(encoding='utf-8')
text=text.replace('/Script/EnhancedInput.EnhancedPlayerInput','/Script/Engine.PlayerInput')
text=text.replace('/Script/EnhancedInput.EnhancedInputComponent','/Script/Engine.InputComponent')
if 'AxisName="CanyonThrottle"' not in text:
    text+='\n[/Script/Engine.InputSettings]\nbUseMouseForTouch=False\n'
    for axis, key, scale in [
        ('Throttle','W',1),('Throttle','Gamepad_RightTriggerAxis',1),
        ('Throttle','ValveIndex_Right_Trigger_Axis',1),('Throttle','OculusTouch_Right_Trigger_Axis',1),
        ('Steer','A',-1),('Steer','D',1),('Steer','Gamepad_LeftX',1),
        ('Steer','ValveIndex_Left_Thumbstick_X',1),('Steer','OculusTouch_Left_Thumbstick_X',1),
        ('Brake','S',1),('Brake','Gamepad_LeftTriggerAxis',1),
        ('Brake','ValveIndex_Left_Trigger_Axis',1),('Brake','OculusTouch_Left_Trigger_Axis',1)]:
        text+=f'+AxisMappings=(AxisName="Canyon{axis}",Scale={scale}.000000,Key={key})\n'
    for action,key in [('Boost','LeftShift'),('Boost','Gamepad_FaceButton_Bottom'),
                       ('Restart','R'),('Restart','Gamepad_Special_Right'),
                       ('Recover','BackSpace'),('Recover','Gamepad_FaceButton_Top'),
                       ('Recenter','F'),('Quit','Escape')]:
        text+=f'+ActionMappings=(ActionName="Canyon{action}",Key={key})\n'
input_file.write_text(text,encoding='utf-8')
print(json.dumps({'runtime_grid':sample.shape,'gates':len(rows),'datum_m':datum,
                  'route_length_m':float(sum(np.linalg.norm(np.subtract(b,a))*4 for a,b in zip(pixels,pixels[1:])))}))
