"""Reimport only generated meshes without touching the material graphs."""
from pathlib import Path
import unreal as u
ROOT=Path(__file__).resolve().parents[1]
AT=u.AssetToolsHelpers.get_asset_tools()
mapping={'Hull':'M_Hull','Trim':'M_Trim','Accent':'M_Paint','Light':'M_Light','Screen':'M_Screen','Rock':'M_Sandstone'}
for name in ['SM_Cockpit','SM_Rock0','SM_Rock1','SM_Rock2']:
    opt=u.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False
    opt.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;opt.automated_import_should_detect_type=False
    opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=False
    task=u.AssetImportTask();task.filename=str(ROOT/'data/art/generated'/(name+'.obj'))
    task.destination_path='/Game/Canyon/Models';task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=True;task.options=opt
    AT.import_asset_tasks([task]);assert task.imported_object_paths
    mesh=u.load_asset(task.imported_object_paths[0])
    for i,s in enumerate(mesh.static_materials):
        mesh.set_material(i,u.load_asset('/Game/Canyon/Materials/'+mapping.get(str(s.material_slot_name),'M_Hull')))
    u.EditorAssetLibrary.save_loaded_asset(mesh)
    u.log('MODEL_READY '+name+' '+str(mesh.get_bounds()))
