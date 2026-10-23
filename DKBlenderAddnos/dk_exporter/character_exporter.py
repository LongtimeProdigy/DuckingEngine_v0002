"""Export matching character mesh, rest skeleton and animation together."""

from pathlib import Path

import bpy
from bpy_extras.io_utils import ExportHelper

from .animation_exporter import collect_animation
from .common import DKExportMixin, export_options, resolve_armature, selected_meshes
from .format import animation_bytes, mesh_bytes, skeleton_bytes
from .mesh import collect_meshes
from .skeleton_exporter import collect_skeleton


@export_options(mesh=True)
class DK_OT_export_character(DKExportMixin, bpy.types.Operator, ExportHelper):
    """Export matching mesh, rest skeleton and animation files together"""
    bl_idname = "export_scene.dk_character"
    bl_label = "DuckingEngine Character Bundle"
    filename_ext = ".dc"

    def build_outputs(self, context):
        meshes = selected_meshes(context)
        armature = resolve_armature(context, meshes)
        # Collect and validate all three before touching any output file.
        model = mesh_bytes(collect_meshes(context, meshes, armature, self.unit_scale,
                                         self.split_materials, self.apply_modifiers), skinned=True)
        skeleton = skeleton_bytes(collect_skeleton(context, armature, self.unit_scale))
        animation = animation_bytes(collect_animation(context, armature, self.unit_scale))
        path = Path(self.filepath)
        return [(path, model), (path.with_suffix(".skeleton.xml"), skeleton),
                (path.with_suffix(".animation.xml"), animation)]
