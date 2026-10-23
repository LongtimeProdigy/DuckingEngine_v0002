"""The existing DuckingEngine wire formats; no Blender dependency."""

from dataclasses import dataclass
import math
import os
from pathlib import Path
import struct
import tempfile
import xml.etree.ElementTree as ET

MAX_BONES = 256
MAX_INFLUENCES = 4
INVALID_BONE = 0xFFFFFFFF
ANIMATION_FPS = 30
STATIC_VERTEX = struct.Struct("<8f")
SKINNED_VERTEX = struct.Struct("<8f4I4f")
UINT32 = struct.Struct("<I")


class ExportError(ValueError):
    pass


@dataclass
class SubMesh:
    name: str
    vertices: list
    indices: list


def skin_weights(influences):
    """Keep the largest four bone weights, merge duplicates, then normalize."""
    merged = {}
    for index, weight in influences:
        if not 0 <= index < MAX_BONES:
            raise ExportError(f"Bone index {index} exceeds the {MAX_BONES}-bone limit")
        if not math.isfinite(weight) or weight < 0:
            raise ExportError("Bone weights must be finite and nonnegative")
        if weight > 0:
            merged[index] = merged.get(index, 0.0) + weight
    kept = sorted(merged.items(), key=lambda item: (-item[1], item[0]))[:MAX_INFLUENCES]
    total = sum(weight for _, weight in kept)
    if not math.isfinite(total) or total <= 0:
        raise ExportError("Vertex has no positive deform-bone weights")
    indices = [index for index, _ in kept]
    weights = [weight / total for _, weight in kept]
    indices.extend([INVALID_BONE] * (MAX_INFLUENCES - len(indices)))
    weights.extend([0.0] * (MAX_INFLUENCES - len(weights)))
    return tuple(indices), tuple(weights)


def mesh_bytes(submeshes, skinned=False):
    if not submeshes:
        raise ExportError("Select at least one nonempty mesh")
    vertex_format = SKINNED_VERTEX if skinned else STATIC_VERTEX
    parts = [UINT32.pack(len(submeshes))]
    for mesh in submeshes:
        if not mesh.vertices or not mesh.indices or len(mesh.indices) % 3:
            raise ExportError(f"{mesh.name}: expected nonempty triangle geometry")
        if any(index < 0 or index >= len(mesh.vertices) for index in mesh.indices):
            raise ExportError(f"{mesh.name}: triangle index is outside the vertex buffer")
        parts.append(UINT32.pack(len(mesh.indices)))
        parts.append(struct.pack(f"<{len(mesh.indices)}I", *mesh.indices))
        parts.append(UINT32.pack(len(mesh.vertices)))
        for vertex in mesh.vertices:
            if any(not math.isfinite(value) for value in vertex):
                raise ExportError(f"{mesh.name}: vertex contains NaN or infinity")
            parts.append(vertex_format.pack(*vertex))
    return b"".join(parts)


def sample_frames(start, end, source_fps):
    if end < start or not math.isfinite(source_fps) or source_fps <= 0:
        raise ExportError("Invalid animation frame range or frame rate")
    # The engine displays each exported sample for 1/30 s, including the last.
    count = max(1, math.ceil((end - start + 1) * ANIMATION_FPS / source_fps - 1e-9))
    return [min(end, start + index * source_fps / ANIMATION_FPS) for index in range(count)]


def _transform(parent, values):
    for name, value in zip(("Position", "Rotation", "Scale"), values):
        if any(not math.isfinite(component) for component in value):
            raise ExportError("Transform contains NaN or infinity")
        ET.SubElement(parent, name).text = " ".join(format(component, ".9g") for component in value)


def _xml(root):
    ET.indent(root, space="\t")
    return ET.tostring(root, encoding="utf-8", short_empty_elements=False) + b"\n"


def skeleton_bytes(bones):
    root = ET.Element("Skeleton", Count=str(len(bones)))
    seen = set()
    for name, parent, transform in bones:
        if name in seen or (parent is not None and parent not in seen):
            raise ExportError("Skeleton requires unique names and parent-before-child order")
        attributes = {"Name": name}
        if parent is not None:
            attributes["Parent"] = parent
        _transform(ET.SubElement(root, "Bone", attributes), transform)
        seen.add(name)
    if not 0 < len(seen) <= MAX_BONES:
        raise ExportError(f"Skeleton must contain 1..{MAX_BONES} bones")
    return _xml(root)


def animation_bytes(tracks):
    if not tracks or len(tracks) > MAX_BONES:
        raise ExportError(f"Animation must contain 1..{MAX_BONES} bone tracks")
    frame_count = len(tracks[0][1])
    if frame_count == 0:
        raise ExportError("Animation has no frames")
    root = ET.Element("Animation", FrameCount=str(frame_count))
    names = set()
    for name, transforms in tracks:
        if name in names or len(transforms) != frame_count:
            raise ExportError("Animation requires unique bones and equal track lengths")
        names.add(name)
        bone = ET.SubElement(root, "Bone", Name=name)
        for index, transform in enumerate(transforms):
            _transform(ET.SubElement(bone, "Transform", Index=str(index)), transform)
    return _xml(root)


def write_file(path, data):
    """Validate first, then replace the destination with a complete file."""
    path = Path(path)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=f".{path.name}.", delete=False) as stream:
            temporary = stream.name
            stream.write(data)
        os.replace(temporary, path)
        temporary = None
    finally:
        if temporary is not None:
            os.unlink(temporary)
