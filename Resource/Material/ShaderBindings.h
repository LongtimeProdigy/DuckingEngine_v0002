#ifndef DK_SHADER_BINDINGS_H
#define DK_SHADER_BINDINGS_H

// CPU descriptor tables and HLSL bindless arrays must use the same register spaces.
#define DK_BINDLESS_TEXTURE_SPACE 10
#define DK_BINDLESS_VERTEX_SPACE 11
#define DK_BINDLESS_INDEX_SPACE 12
#define DK_BINDLESS_MATERIAL_SPACE 13
#define DK_BINDLESS_TEXTURE3D_SPACE 14

#define DK_REGISTER_SPACE_IMPL(index) space##index
#define DK_REGISTER_SPACE(index) DK_REGISTER_SPACE_IMPL(index)

#endif
