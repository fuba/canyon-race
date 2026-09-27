"""Small shader edits that preserve existing graphs and references."""
import unreal as u
m=u.load_asset('/Game/Canyon/Materials/M_Sandstone')
m.set_editor_property('used_with_instanced_static_meshes',True)
for e in u.MaterialEditingLibrary.get_material_expressions(m):
    if isinstance(e,u.MaterialExpressionCustom):
        code=e.get_editor_property('code')
        if 'float flat=' in code and 'g=dot(g' not in code:
            code=code.replace('float flat=','g=dot(g,float3(.3,.59,.11))*float3(1.3,.96,.65);\nfloat flat=')
            e.set_editor_property('code',code)
u.MaterialEditingLibrary.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)
m=u.load_asset('/Game/Canyon/Materials/M_River')
for e in u.MaterialEditingLibrary.get_material_expressions(m):
    if isinstance(e,u.MaterialExpressionCustom):
        code=e.get_editor_property('code').replace('.06*sin','.028*sin').replace('.025*sin','.012*sin').replace('.055*cos','.026*cos')
        e.set_editor_property('code',code)
u.MaterialEditingLibrary.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)
u.log('CANYON_POLISH_READY')
