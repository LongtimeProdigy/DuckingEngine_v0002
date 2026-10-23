"""DuckingEngine skinned mesh exporter (.dc)."""

import bpy
from bpy_extras.io_utils import ExportHelper

from .common import DKExportMixin, export_options, selected_meshes, resolve_armature
from .format import mesh_bytes
from .mesh import collect_meshes


def export_mesh_data(context, unit_scale=1.0, split_materials=False, apply_modifiers=True):
    meshes = selected_meshes(context)
    armature = resolve_armature(context, meshes)
    data = collect_meshes(context, meshes, armature, unit_scale, split_materials, apply_modifiers)
    return mesh_bytes(data, skinned=True), data


@export_options(mesh=True)
class DK_OT_export_skinned(DKExportMixin, bpy.types.Operator, ExportHelper):
    """Export selected meshes in the DuckingEngine four-influence skin layout"""
    bl_idname = "export_scene.dk_skinned_mesh"
    bl_label = "DuckingEngine Skinned Mesh"
    filename_ext = ".dc"

    def build_outputs(self, context):
        data, _ = export_mesh_data(context, self.unit_scale, self.split_materials, self.apply_modifiers)
        return [(self.filepath, data)]
