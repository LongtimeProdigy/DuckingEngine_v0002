"""DuckingEngine skeleton exporter (.xml)."""

import bpy
from bpy_extras.io_utils import ExportHelper

from .common import (
    DKExportMixin, conversion_matrix, decompose_transform, export_options,
    ordered_bones, resolve_armature, scene_state,
)
from .format import skeleton_bytes


def collect_skeleton(context, armature, unit_scale=1.0):
    bones = ordered_bones(armature)
    conversion = conversion_matrix(context.scene, unit_scale)
    inverse_conversion = conversion.inverted()
    result = []
    with scene_state(context, armature, "REST", reference=True):
        evaluated = armature.evaluated_get(context.evaluated_depsgraph_get())
        global_matrices = {bone.name: conversion @ evaluated.matrix_world @ bone.matrix_local @ inverse_conversion
                           for bone in bones}
        for bone in bones:
            matrix = global_matrices[bone.name]
            parent = bone.parent.name if bone.parent else None
            if parent is not None:
                matrix = global_matrices[parent].inverted() @ matrix
            values, _ = decompose_transform(matrix, bone.name)
            result.append((bone.name, parent, values))
    return result


def export_skeleton_data(context, unit_scale=1.0):
    return skeleton_bytes(collect_skeleton(context, resolve_armature(context), unit_scale))


@export_options()
class DK_OT_export_skeleton(DKExportMixin, bpy.types.Operator, ExportHelper):
    """Export armature rest transforms with stable parent-before-child indices"""
    bl_idname = "export_scene.dk_skeleton"
    bl_label = "DuckingEngine Skeleton"
    filename_ext = ".xml"

    def build_outputs(self, context):
        return [(self.filepath, export_skeleton_data(context, self.unit_scale))]
