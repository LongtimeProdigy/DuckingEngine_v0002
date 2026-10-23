"""DuckingEngine static mesh exporter (.dm)."""

import bpy
from bpy_extras.io_utils import ExportHelper

from .common import DKExportMixin, export_options, selected_meshes
from .format import mesh_bytes
from .mesh import collect_meshes


def export_mesh_data(context, unit_scale=1.0, split_materials=False, apply_modifiers=True):
    meshes = selected_meshes(context)
    armature = None
    data = collect_meshes(context, meshes, armature, unit_scale, split_materials, apply_modifiers)
    return mesh_bytes(data, skinned=False), data


@export_options(mesh=True)
class DK_OT_export_static(DKExportMixin, bpy.types.Operator, ExportHelper):
    """Export selected meshes in the DuckingEngine static vertex layout"""
    bl_idname = "export_scene.dk_static_mesh"
    bl_label = "DuckingEngine Static Mesh"
    filename_ext = ".dm"

    def build_outputs(self, context):
        data, _ = export_mesh_data(context, self.unit_scale, self.split_materials, self.apply_modifiers)
        return [(self.filepath, data)]
