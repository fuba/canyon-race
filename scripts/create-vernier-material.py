"""Create the small white thruster material once; do not rebuild existing materials."""
import unreal as u
path='/Game/Canyon/Materials/M_Vernier'
if not u.EditorAssetLibrary.does_asset_exist(path):
    m=u.AssetToolsHelpers.get_asset_tools().create_asset('M_Vernier','/Game/Canyon/Materials',u.Material,u.MaterialFactoryNew())
    m.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('two_sided',True)
    e=u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionVertexColor)
    assert u.MaterialEditingLibrary.connect_material_property(e,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    assert u.MaterialEditingLibrary.connect_material_property(e,'A',u.MaterialProperty.MP_OPACITY)
    u.MaterialEditingLibrary.recompile_material(m)
    u.EditorAssetLibrary.save_loaded_asset(m)
u.log('VERNIER_MATERIAL_READY '+path)
