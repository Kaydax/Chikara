#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in float inNoteStart;
layout(location = 1) in float inNoteEnd;
layout(location = 2) in int inNoteKey;
layout(location = 3) in uint inNoteColor;

layout(location = 0) out float outNoteStart;
layout(location = 1) out float outNoteEnd;
layout(location = 2) out int outNoteKey;
layout(location = 3) out uint outNoteColor;

// basically just a point passthrough for the geometry shader
// there has to be a better way to do this
void main() {
    outNoteStart = inNoteStart;
    outNoteEnd = inNoteEnd;
    outNoteKey = inNoteKey;
    outNoteColor = inNoteColor;
}
