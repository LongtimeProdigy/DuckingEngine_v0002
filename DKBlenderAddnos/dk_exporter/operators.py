"""Register the individual exporters and their File > Export menu entries."""

import bpy

from .animation_exporter import DK_OT_export_animation
from .character_exporter import DK_OT_export_character
from .skeleton_exporter import DK_OT_export_skeleton
from .skinned_mesh_exporter import DK_OT_export_skinned
from .static_mesh_exporter import DK_OT_export_static

CLASSES = (DK_OT_export_static, DK_OT_export_skinned, DK_OT_export_skeleton,
           DK_OT_export_animation, DK_OT_export_character)


def menu_export(self, context):
    self.layout.separator()
    for operator in CLASSES:
        self.layout.operator(operator.bl_idname, text=f"{operator.bl_label} ({operator.filename_ext})")


def register():
    for operator in CLASSES:
        bpy.utils.register_class(operator)
    bpy.types.TOPBAR_MT_file_export.append(menu_export)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(menu_export)
    for operator in reversed(CLASSES):
        bpy.utils.unregister_class(operator)
