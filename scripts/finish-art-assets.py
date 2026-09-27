"""Import CC0 photo textures and original meshes/audio; rebuild the presentation assets."""
from pathlib import Path
import unreal as u
ROOT=Path(__file__).resolve().parents[1]
AT=u.AssetToolsHelpers.get_asset_tools(); M=u.MaterialEditingLibrary
def load(name): return u.load_asset('/Game/Canyon/'+name)
def imported(file,folder,name,options=None):
    task=u.AssetImportTask(); task.filename=str(file); task.destination_path='/Game/Canyon/'+folder
    task.destination_name=name; task.automated=True; task.replace_existing=True; task.save=True
    if options: task.options=options
    existing=u.load_asset(task.destination_path+'/'+name)
    if existing and folder!='Models': return existing
    AT.import_asset_tasks([task])
    assert task.imported_object_paths, str(file)
    asset=u.load_asset(task.imported_object_paths[0]); u.log(f'IMPORTED {asset.get_path_name()}')
    return asset
tex={}
for prefix in ['rock_face','rocky_terrain_02']:
    for kind in ['color','normal','arm']:
        name=prefix+'_'+kind
        t=imported(ROOT/'data/art'/(name+'.jpg'),'Textures',name)
        if kind!='color': t.set_editor_property('srgb',False)
        if kind=='normal': t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
        if kind=='arm': t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS)
        u.EditorAssetLibrary.save_loaded_asset(t); tex[name]=t
sky=imported(ROOT/'data/art/kloppenheim_06_puresky_sky.hdr','Textures','SkyHDR')

def mat(name):
    m=load('Materials/'+name) or AT.create_asset(name,'/Game/Canyon/Materials',u.Material,u.MaterialFactoryNew())
    M.delete_all_material_expressions(m); return m
def ex(m,cls,**props):
    e=M.create_material_expression(m,cls)
    for k,v in props.items(): e.set_editor_property(k,v)
    return e
def wire(a,b,p=''): assert M.connect_material_expressions(a,'',b,p)
def prop(e,p): assert M.connect_material_property(e,'',p)
def scalar(m,x): return ex(m,u.MaterialExpressionConstant,r=x)
def vec(m,r,g,b): return ex(m,u.MaterialExpressionConstant3Vector,constant=u.LinearColor(r,g,b,1))
def custom(m,code,inputs,output=u.CustomMaterialOutputType.CMOT_FLOAT3):
    e=ex(m,u.MaterialExpressionCustom,code=code,output_type=output)
    pins=[]
    for k in inputs:
        pin=u.CustomInput();pin.set_editor_property('input_name',k);pins.append(pin)
    e.set_editor_property('inputs',pins)
    for name,node in inputs.items(): wire(node,e,name)
    return e
def save(m):
    M.layout_material_expressions(m); M.recompile_material(m); u.EditorAssetLibrary.save_loaded_asset(m)

rock=mat('M_Sandstone'); rock.set_editor_property('tangent_space_normal',False)
rock.set_editor_property('used_with_instanced_static_meshes',True)
p=ex(rock,u.MaterialExpressionWorldPosition); n=ex(rock,u.MaterialExpressionVertexNormalWS)
def obj(t): return ex(rock,u.MaterialExpressionTextureObject,texture=tex[t])
color=custom(rock,r'''
float3 w=pow(abs(N),4); w/=max(dot(w,1),.0001);
float3 q=P/1800;
float3 c=Texture2DSample(C,CSampler,q.yz).rgb*w.x+Texture2DSample(C,CSampler,q.xz).rgb*w.y+Texture2DSample(C,CSampler,q.xy).rgb*w.z;
float3 g=Texture2DSample(G,GSampler,P.xy/1200).rgb;
g=dot(g,float3(.3,.59,.11))*float3(1.3,.96,.65);
float flat=smoothstep(.73,.96,N.z);
float macro=.88+.12*sin(P.z*.0018+sin(P.x*.00011)*2)+.08*sin(P.x*.00027+P.y*.00019);
return lerp(c*float3(1.18,.90,.68),g*float3(1.12,.91,.71),flat)*macro;
''',{'P':p,'N':n,'C':obj('rock_face_color'),'G':obj('rocky_terrain_02_color')})
prop(color,u.MaterialProperty.MP_BASE_COLOR)
normal=custom(rock,r'''
float3 w=pow(abs(N),4); w/=max(dot(w,1),.0001); float3 q=P/1800;
float2 nx=Texture2DSample(T,TSampler,q.yz).rg*2-1;
float2 ny=Texture2DSample(T,TSampler,q.xz).rg*2-1;
float2 nz=Texture2DSample(T,TSampler,q.xy).rg*2-1;
float2 ng=Texture2DSample(G,GSampler,P.xy/1200).rg*2-1;
nz=lerp(nz,ng,smoothstep(.73,.96,N.z));
float3 perturb=float3(0,nx.x,nx.y)*w.x+float3(ny.x,0,ny.y)*w.y+float3(nz.x,nz.y,0)*w.z;
return normalize(N+perturb*.45);
''',{'P':p,'N':n,'T':obj('rock_face_normal'),'G':obj('rocky_terrain_02_normal')})
prop(normal,u.MaterialProperty.MP_NORMAL)
rough=custom(rock,'return clamp(Texture2DSample(T,TSampler,P.xy/1800).g,.65,.98);',{'P':p,'T':obj('rock_face_arm')},u.CustomMaterialOutputType.CMOT_FLOAT1)
prop(rough,u.MaterialProperty.MP_ROUGHNESS); prop(scalar(rock,.22),u.MaterialProperty.MP_SPECULAR); save(rock)

water=mat('M_River');water.set_editor_property('tangent_space_normal',False)
p=ex(water,u.MaterialExpressionWorldPosition); t=ex(water,u.MaterialExpressionTime)
wn=custom(water,'return normalize(float3(.028*sin(P.x*.021+P.y*.015+T*1.8)+.012*sin(P.x*.045-T*2),.026*cos(P.y*.018+T*1.4),1));',{'P':p,'T':t})
prop(wn,u.MaterialProperty.MP_NORMAL); prop(vec(water,.012,.065,.052),u.MaterialProperty.MP_BASE_COLOR)
prop(scalar(water,.22),u.MaterialProperty.MP_ROUGHNESS); prop(scalar(water,.65),u.MaterialProperty.MP_SPECULAR)
save(water)

sm=mat('M_PrototypeSky');sm.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
sm.set_editor_property('two_sided',True);sm.set_editor_property('is_sky',True)
pos=ex(sm,u.MaterialExpressionWorldPosition);norm=ex(sm,u.MaterialExpressionNormalize);wire(pos,norm,'')
sample=ex(sm,u.MaterialExpressionTextureSampleParameterCube,parameter_name='Environment',texture=sky,sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
wire(norm,sample,'UVs');prop(sample,u.MaterialProperty.MP_EMISSIVE_COLOR);save(sm)

for name,color,metal,rough,emission in [
 ('M_Hull',(.055,.075,.085),.7,.32,False),('M_Trim',(.012,.016,.02),.3,.6,False),
 ('M_Paint',(.68,.17,.018),.45,.3,False),('M_Light',(.025,.8,1),.2,.3,True),
 ('M_Screen',(.002,.005,.008),.1,.35,False)]:
    # These materials can be rooted by the native pawn's default mesh during commandlets.
    # They are constant paint finishes; preserve them on repeat runs.
    if load('Materials/'+name): continue
    m=mat(name); c=vec(m,*color); prop(c,u.MaterialProperty.MP_BASE_COLOR)
    prop(scalar(m,metal),u.MaterialProperty.MP_METALLIC);prop(scalar(m,rough),u.MaterialProperty.MP_ROUGHNESS)
    if emission: prop(c,u.MaterialProperty.MP_EMISSIVE_COLOR)
    save(m)

for name in ['SM_Cockpit','SM_Rock0','SM_Rock1','SM_Rock2']:
    opt=u.FbxImportUI(); opt.set_editor_property('import_mesh',True)
    opt.set_editor_property('import_materials',False);opt.set_editor_property('import_textures',False)
    opt.set_editor_property('mesh_type_to_import',u.FBXImportType.FBXIT_STATIC_MESH)
    opt.set_editor_property('automated_import_should_detect_type',False)
    data=opt.static_mesh_import_data
    data.set_editor_property('combine_meshes',True);data.set_editor_property('auto_generate_collision',False)
    mesh=imported(ROOT/'data/art/generated'/(name+'.obj'),'Models',name,opt)
    slots=mesh.get_editor_property('static_materials')
    mapping={'Hull':'M_Hull','Trim':'M_Trim','Accent':'M_Paint','Light':'M_Light','Screen':'M_Screen','Rock':'M_Sandstone'}
    for i,s in enumerate(slots):
        label=str(s.material_slot_name)
        mesh.set_material(i,load('Materials/'+mapping.get(label,'M_Hull')))
    u.log(f'MESH_BOUNDS {name} {mesh.get_bounds()} SLOTS {[str(s.material_slot_name) for s in slots]}')
    u.EditorAssetLibrary.save_loaded_asset(mesh)
for name in ['Engine','Impact','Gate']:
    sound=imported(ROOT/'data/art/generated'/(name+'.wav'),'Audio',name)
    sound.set_editor_property('looping',name=='Engine');u.EditorAssetLibrary.save_loaded_asset(sound)

levels=u.get_editor_subsystem(u.LevelEditorSubsystem);levels.load_level('/Game/Canyon/Maps/L_Horseshoe')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
for a in actors.get_all_level_actors():
    if isinstance(a,u.SkyLight):
        c=a.get_component_by_class(u.SkyLightComponent)
        c.set_editor_property('real_time_capture',False)
        c.set_editor_property('source_type',u.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
        c.set_editor_property('cubemap',sky);c.set_editor_property('intensity',.7)
    if isinstance(a,u.DirectionalLight):
        c=a.get_component_by_class(u.DirectionalLightComponent)
        c.set_editor_property('intensity',4.)
        c.set_editor_property('light_color',u.Color(255,235,210,255))
        c.set_editor_property('dynamic_shadow_distance_movable_light',60000.)
        c.set_editor_property('dynamic_shadow_cascades',4)
    if a.get_actor_label()=='Prototype sky dome':
        c=a.get_component_by_class(u.StaticMeshComponent);c.set_material(0,sm)
        c.set_collision_profile_name('NoCollision');a.set_actor_enable_collision(False)
assert levels.save_current_level()
u.log('CANYON_ART_READY')
