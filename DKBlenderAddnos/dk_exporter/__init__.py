bl_info = {
    "name": "DuckingEngine Exporters",
    "author": "DuckingEngine",
    "version": (1, 1, 0),
    "blender": (5, 2, 0),
    "location": "File > Export > DuckingEngine",
    "description": "Export static/skinned meshes, rest skeletons and sampled animation",
    "category": "Import-Export",
}


def register():
    import bpy
    if bpy.app.version < bl_info["blender"]:
        raise RuntimeError("DuckingEngine Exporters requires Blender 5.2 or later")
    from . import operators
    operators.register()


def unregister():
    from . import operators
    operators.unregister()
