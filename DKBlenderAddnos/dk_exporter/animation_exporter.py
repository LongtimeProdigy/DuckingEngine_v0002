"""DuckingEngine animation exporter (.xml)."""

import math

import bpy
from bpy_extras.io_utils import ExportHelper

from .common import (
    DKExportMixin, conversion_matrix, decompose_transform, export_options,
    ordered_bones, resolve_armature, scene_state,
)
from .format import animation_bytes, sample_frames


def collect_animation(context, armature, unit_scale=1.0):
    bones = ordered_bones(armature)
    conversion = conversion_matrix(context.scene, unit_scale)
    inverse_conversion = conversion.inverted()
    scene = context.scene
    frames = sample_frames(scene.frame_start, scene.frame_end, scene.render.fps / scene.render.fps_base)
    tracks = {bone.name: [] for bone in bones}
    rotations = {}
    with scene_state(context, armature, "POSE"):
        for frame in frames:
            whole = math.floor(frame)
            scene.frame_set(whole, subframe=frame - whole)
            evaluated = armature.evaluated_get(context.evaluated_depsgraph_get())
            global_matrices = {
                bone.name: conversion @ evaluated.matrix_world @ evaluated.pose.bones[bone.name].matrix @ inverse_conversion
                for bone in bones
            }
            for bone in bones:
                matrix = global_matrices[bone.name]
                if bone.parent is not None:
                    matrix = global_matrices[bone.parent.name].inverted() @ matrix
                values, rotations[bone.name] = decompose_transform(matrix, f"{bone.name} at frame {frame:g}", rotations.get(bone.name))
                tracks[bone.name].append(values)
    return list(tracks.items())


def export_animation_data(context, unit_scale=1.0):
    return animation_bytes(collect_animation(context, resolve_armature(context), unit_scale))


@export_options()
class DK_OT_export_animation(DKExportMixin, bpy.types.Operator, ExportHelper):
    """Sample the scene frame range at the engine's fixed 30 FPS"""
    bl_idname = "export_scene.dk_animation"
    bl_label = "DuckingEngine Animation"
    filename_ext = ".xml"

    def build_outputs(self, context):
        return [(self.filepath, export_animation_data(context, self.unit_scale))]
