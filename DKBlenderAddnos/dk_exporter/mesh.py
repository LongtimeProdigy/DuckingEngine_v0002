"""Shared static/skinned geometry extraction using Blender 5.2+ corner data."""

import math

from .common import conversion_matrix, ordered_bones, scene_state
from .format import ExportError, SubMesh, skin_weights


def collect_meshes(context, meshes, armature=None, unit_scale=1.0, split_materials=False, apply_modifiers=True):
    conversion = conversion_matrix(context.scene, unit_scale)
    bone_indices = {bone.name: index for index, bone in enumerate(ordered_bones(armature)) if bone.use_deform} if armature else {}
    result = []
    with scene_state(context, armature, "REST" if armature else None, reference=armature is not None):
        depsgraph = context.evaluated_depsgraph_get()
        for obj in sorted(meshes, key=lambda item: item.name):
            evaluated = obj.evaluated_get(depsgraph)
            owner = evaluated if apply_modifiers else obj
            mesh = owner.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
            try:
                if mesh is None or not mesh.polygons:
                    raise ExportError(f"{obj.name}: mesh has no faces")
                mesh.calc_loop_triangles()
                transform = conversion @ evaluated.matrix_world
                linear = transform.to_3x3()
                if not math.isfinite(linear.determinant()) or linear.determinant() == 0:
                    raise ExportError(f"{obj.name}: singular object transform")
                normal_transform = linear.inverted().transposed()
                reverse_winding = linear.determinant() < 0
                uv_layer = mesh.uv_layers.active
                corner_normals = mesh.corner_normals
                groups = {group.index: group.name for group in obj.vertex_groups}
                weights = {}
                buckets = {}
                for triangle in mesh.loop_triangles:
                    material = triangle.material_index if split_materials else 0
                    if material not in buckets:
                        buckets[material] = ([], [], {})
                    vertices, indices, unique = buckets[material]
                    loops = list(triangle.loops)
                    if reverse_winding:
                        loops[1], loops[2] = loops[2], loops[1]
                    for loop_index in loops:
                        source_index = mesh.loops[loop_index].vertex_index
                        vertex = mesh.vertices[source_index]
                        position = transform @ vertex.co
                        source_normal = corner_normals[loop_index].vector
                        normal = normal_transform @ source_normal
                        if normal.length_squared < 1e-20:
                            raise ExportError(f"{obj.name}: degenerate corner normal")
                        normal.normalize()
                        if uv_layer is None:
                            uv = (0.0, 0.0)
                        else:
                            value = uv_layer.uv[loop_index].vector
                            uv = (value.x, 1.0 - value.y)
                        record = (*position, *normal, *uv)
                        if armature is not None:
                            if source_index not in weights:
                                influences = [(bone_indices[groups[group.group]], group.weight)
                                              for group in vertex.groups if groups.get(group.group) in bone_indices]
                                try:
                                    weights[source_index] = skin_weights(influences)
                                except ExportError as error:
                                    raise ExportError(f"{obj.name}, vertex {source_index}: {error}") from error
                            bone_ids, bone_weights = weights[source_index]
                            record += (*bone_ids, *bone_weights)
                        # Include source index so disconnected vertices with different skinning stay independent.
                        key = (source_index, record)
                        if key not in unique:
                            unique[key] = len(vertices)
                            vertices.append(record)
                        indices.append(unique[key])
                for material, (vertices, indices, _) in sorted(buckets.items()):
                    result.append(SubMesh(f"{obj.name}:{material}", vertices, indices))
            finally:
                owner.to_mesh_clear()
    return result
