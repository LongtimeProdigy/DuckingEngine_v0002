"""Shared Blender 5.2+ context, transforms and exporter options."""

from contextlib import contextmanager
import math

from bpy.props import BoolProperty, FloatProperty, StringProperty
from mathutils import Matrix

from .format import ExportError, MAX_BONES, write_file

AXIS = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))


def conversion_matrix(scene, unit_scale=1.0):
    """Blender world coordinates -> engine world coordinates in meters."""
    scale = scene.unit_settings.scale_length * unit_scale
    if not math.isfinite(scale) or scale <= 0:
        raise ExportError("Unit scale must be finite and positive")
    return Matrix.Diagonal((scale, scale, scale, 1.0)) @ AXIS


def selected_meshes(context):
    if context.mode != "OBJECT":
        raise ExportError("Switch to Object Mode before exporting")
    meshes = sorted((obj for obj in context.selected_objects if obj.type == "MESH"), key=lambda obj: obj.name)
    if not meshes:
        raise ExportError("Select at least one mesh object")
    return meshes


def resolve_armature(context, meshes=()):
    if context.mode != "OBJECT":
        raise ExportError("Switch to Object Mode before exporting")
    candidates = set()
    for obj in meshes:
        modifiers = [m for m in obj.modifiers if m.type == "ARMATURE" and m.show_viewport]
        if len(modifiers) != 1 or modifiers[0].object is None:
            raise ExportError(f"{obj.name}: exactly one enabled Armature modifier is required")
        modifier = modifiers[0]
        if modifier.use_deform_preserve_volume or modifier.use_bone_envelopes or not modifier.use_vertex_groups:
            raise ExportError(f"{obj.name}: use vertex-group linear skinning; disable envelopes and Preserve Volume")
        if modifier.vertex_group:
            raise ExportError(f"{obj.name}: an Armature modifier mask cannot be represented by the engine")
        candidates.add(modifier.object)
    if not meshes:
        candidates.update(obj for obj in context.selected_objects if obj.type == "ARMATURE")
        active = context.view_layer.objects.active
        if not candidates and active is not None and active.type == "MESH":
            rig = active.find_armature()
            if rig is not None:
                candidates.add(rig)
    if len(candidates) != 1:
        raise ExportError("Select one armature, or meshes bound to the same armature")
    return candidates.pop()


def ordered_bones(armature):
    bones = []

    def visit(bone):
        bones.append(bone)
        for child in sorted(bone.children, key=lambda item: item.name):
            visit(child)

    for root in sorted((bone for bone in armature.data.bones if bone.parent is None), key=lambda item: item.name):
        visit(root)
    if not 0 < len(bones) <= MAX_BONES:
        raise ExportError(f"{armature.name}: {len(bones)} bones; engine limit is {MAX_BONES}")
    if any(bone.use_deform and bone.bbone_segments > 1 for bone in bones):
        raise ExportError("Segmented B-Bone deformation cannot be represented by the engine")
    return bones


@contextmanager
def scene_state(context, armature=None, pose_position=None, reference=False):
    scene = context.scene
    frame, subframe = scene.frame_current, scene.frame_subframe
    old_pose = armature.data.pose_position if armature is not None else None
    try:
        if armature is not None and pose_position is not None:
            armature.data.pose_position = pose_position
        if reference:
            scene.frame_set(scene.frame_start, subframe=0.0)
        context.view_layer.update()
        yield
    finally:
        if armature is not None:
            armature.data.pose_position = old_pose
        scene.frame_set(frame, subframe=subframe)
        context.view_layer.update()


def decompose_transform(matrix, label, previous_rotation=None):
    position, rotation, scale = matrix.decompose()
    if min(abs(value) for value in scale) < 1e-10:
        raise ExportError(f"{label}: singular bone scale")
    reconstructed = Matrix.LocRotScale(position, rotation, scale)
    error = max(abs(matrix[row][column] - reconstructed[row][column])
                for row in range(4) for column in range(4))
    magnitude = max(1.0, max(abs(matrix[row][column]) for row in range(3) for column in range(3)))
    if not math.isfinite(error) or error > 2e-5 * magnitude:
        raise ExportError(f"{label}: shear cannot be stored as engine TRS")
    if previous_rotation is not None and rotation.dot(previous_rotation) < 0:
        rotation.negate()
    values = (tuple(position), (rotation.x, rotation.y, rotation.z, rotation.w), tuple(scale))
    return values, rotation


class DKExportMixin:
    bl_options = {"REGISTER"}

    def execute(self, context):
        try:
            outputs = self.build_outputs(context)
            for path, data in outputs:
                write_file(path, data)
        except (ExportError, OSError, RuntimeError, ValueError) as error:
            self.report({"ERROR"}, str(error))
            return {"CANCELLED"}
        self.report({"INFO"}, f"Exported {len(outputs)} DuckingEngine file(s)")
        return {"FINISHED"}


def export_options(*, mesh=False):
    """Attach shared Blender RNA properties to each exporter class."""
    def decorate(operator):
        annotations = dict(operator.__dict__.get("__annotations__", {}))
        annotations.update({
            "filter_glob": StringProperty(default=f"*{operator.filename_ext}", options={"HIDDEN"}),
            "unit_scale": FloatProperty(name="Unit Multiplier", default=1.0, min=0.000001,
                                        description="Multiply scene unit scale; engine distances are meters"),
        })
        if mesh:
            annotations.update({
                "split_materials": BoolProperty(name="Split by Material", default=False,
                                                description="Create a submesh per used material slot; match ModelProperty order"),
                "apply_modifiers": BoolProperty(name="Apply Modifiers", default=True,
                                                description="Evaluate modifiers on a temporary mesh; skinned meshes use rest pose"),
            })
        operator.__annotations__ = annotations
        return operator
    return decorate
