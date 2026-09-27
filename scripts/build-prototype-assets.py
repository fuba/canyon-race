"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>."""
import unreal as u
import sys

assets = u.AssetToolsHelpers.get_asset_tools()
mel = u.MaterialEditingLibrary

def material(name):
    path = '/Game/Canyon/Materials/' + name
    existing = u.load_asset(path)
    if existing:
        return existing, False
    return assets.create_asset(name, '/Game/Canyon/Materials', u.Material, u.MaterialFactoryNew()), True

def expr(mat, cls, x=0, y=0):
    return mel.create_material_expression(mat, cls, x, y)

def wire(a, output, b, input_name):
    names = mel.get_material_expression_input_names(b)
    if input_name not in names and len(names) == 1:
        input_name = names[0]
    assert mel.connect_material_expressions(a, output, b, input_name), (b.get_name(), list(names), input_name)

def scalar(mat, value):
    e = expr(mat, u.MaterialExpressionConstant)
    e.set_editor_property('r', value)
    return e

rock, new = material('M_Sandstone')
rebuild = new or '--rebuild-materials' in sys.argv
if rebuild and not new:
    mel.delete_all_material_expressions(rock)
if rebuild:
    vertex = expr(rock, u.MaterialExpressionVertexColor, -700, 0)
    position = expr(rock, u.MaterialExpressionWorldPosition, -1200, 200)
    mask = expr(rock, u.MaterialExpressionComponentMask, -1000, 200)
    mask.set_editor_property('r', False)
    mask.set_editor_property('g', False)
    mask.set_editor_property('b', True)
    mask.set_editor_property('a', False)
    wire(position, '', mask, 'Input')
    bands = expr(rock, u.MaterialExpressionMultiply, -800, 200)
    bands.set_editor_property('const_b', .0018)
    wire(mask, '', bands, 'A')
    sine = expr(rock, u.MaterialExpressionSine, -650, 200)
    wire(bands, '', sine, 'Input')
    modulation = expr(rock, u.MaterialExpressionMultiply, -500, 200)
    modulation.set_editor_property('const_b', .12)
    wire(sine, '', modulation, 'A')
    bias = expr(rock, u.MaterialExpressionAdd, -350, 200)
    bias.set_editor_property('const_b', .8)
    wire(modulation, '', bias, 'A')
    noise = expr(rock, u.MaterialExpressionNoise, -650, 500)
    noise.set_editor_property('scale', .005)
    noise.set_editor_property('levels', 3)
    noise.set_editor_property('quality', 1)
    noise.set_editor_property('output_min', .8)
    noise.set_editor_property('output_max', 1.15)
    # An unconnected Noise position uses absolute world position (UE 5.8 default).
    variation = expr(rock, u.MaterialExpressionMultiply, -150, 250)
    wire(bias, '', variation, 'A'); wire(noise, '', variation, 'B')
    color = expr(rock, u.MaterialExpressionMultiply, 0, 0)
    wire(vertex, '', color, 'A'); wire(variation, '', color, 'B')
    mel.connect_material_property(color, '', u.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(rock, .88), '', u.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(scalar(rock, .22), '', u.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(rock)
    u.EditorAssetLibrary.save_loaded_asset(rock)

accent, new = material('M_Accent')
if new:
    tint=expr(accent,u.MaterialExpressionVectorParameter)
    tint.set_editor_property('parameter_name','Tint')
    tint.set_editor_property('default_value',u.LinearColor(.02,.8,1,1))
    mel.connect_material_property(tint,'',u.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(tint,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(scalar(accent,.4),'',u.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(accent)
    u.EditorAssetLibrary.save_loaded_asset(accent)

level='/Game/Canyon/Maps/L_Horseshoe'
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
if not u.EditorAssetLibrary.does_asset_exist(level):
    assert levels.new_level(level)
    actors=u.get_editor_subsystem(u.EditorActorSubsystem)
    sun=actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,80000),u.Rotator(-28,-35,0))
    sun.set_actor_label('Late afternoon sun')
    light=sun.get_component_by_class(u.DirectionalLightComponent)
    light.set_editor_property('mobility',u.ComponentMobility.MOVABLE)
    light.set_editor_property('intensity',6.)
    light.set_editor_property('atmosphere_sun_light',True)
    light.set_editor_property('dynamic_shadow_distance_movable_light',30000.)
    light.set_editor_property('dynamic_shadow_cascades',3)
    actors.spawn_actor_from_class(u.SkyAtmosphere,u.Vector(0,0,0))
    sky=actors.spawn_actor_from_class(u.SkyLight,u.Vector(0,0,50000))
    skycomp=sky.get_component_by_class(u.SkyLightComponent)
    skycomp.set_editor_property('mobility',u.ComponentMobility.MOVABLE)
    skycomp.set_editor_property('intensity',1.)
    skycomp.set_editor_property('real_time_capture',True)
    fog=actors.spawn_actor_from_class(u.ExponentialHeightFog,u.Vector(0,0,-5000))
    fogcomp=fog.get_component_by_class(u.ExponentialHeightFogComponent)
    fogcomp.set_editor_property('fog_density',.007)
    fogcomp.set_editor_property('fog_height_falloff',.12)
    actors.spawn_actor_from_class(u.PlayerStart,u.Vector(-123600,-177600,2000))
    assert levels.save_current_level()
else:
    levels.load_level(level)

# Forward rendering needs an explicit sky surface. This inexpensive gradient is a prototype sky.
sky_mat, new = material('M_PrototypeSky')
if new:
    sky_mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    sky_mat.set_editor_property('two_sided',True)
    sky_mat.set_editor_property('is_sky',True)
    pos=expr(sky_mat,u.MaterialExpressionWorldPosition)
    normal=expr(sky_mat,u.MaterialExpressionNormalize)
    wire(pos,'',normal,'')
    z=expr(sky_mat,u.MaterialExpressionComponentMask)
    z.set_editor_property('r',False); z.set_editor_property('g',False)
    z.set_editor_property('b',True); z.set_editor_property('a',False)
    wire(normal,'',z,'')
    clamp=expr(sky_mat,u.MaterialExpressionClamp)
    wire(z,'',clamp,'')
    low=expr(sky_mat,u.MaterialExpressionConstant3Vector)
    low.set_editor_property('constant',u.LinearColor(.38,.57,.72,1))
    high=expr(sky_mat,u.MaterialExpressionConstant3Vector)
    high.set_editor_property('constant',u.LinearColor(.035,.18,.48,1))
    blend=expr(sky_mat,u.MaterialExpressionLinearInterpolate)
    wire(low,'',blend,'A'); wire(high,'',blend,'B'); wire(clamp,'',blend,'Alpha')
    mel.connect_material_property(blend,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(sky_mat)
    u.EditorAssetLibrary.save_loaded_asset(sky_mat)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
domes=[a for a in actors.get_all_level_actors() if a.get_actor_label()=='Prototype sky dome']
if not domes:
    dome=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,0))
    dome.set_actor_label('Prototype sky dome')
    comp=dome.get_component_by_class(u.StaticMeshComponent)
    comp.set_static_mesh(u.load_asset('/Engine/BasicShapes/Sphere'))
    comp.set_material(0,sky_mat)
    comp.set_editor_property('cast_shadow',False)
    dome.set_actor_scale3d(u.Vector(20000,20000,20000))
else:
    dome=domes[0]
comp=dome.get_component_by_class(u.StaticMeshComponent)
comp.set_collision_profile_name('NoCollision')
dome.set_actor_enable_collision(False)
assert levels.save_current_level()
u.log('CANYON_ASSETS_READY')
