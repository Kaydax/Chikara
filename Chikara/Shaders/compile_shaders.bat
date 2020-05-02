glslc notes.vert -o notes_v.spv
glslc notes.frag -o notes_f.spv
bin2c notes_v.spv notes_v.h notes_v
bin2c notes_f.spv notes_f.h notes_f