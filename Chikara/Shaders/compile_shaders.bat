glslc notes.vert -o notes_v.spv
glslc notes.frag -o notes_f.spv
glslc notes.geom -o notes_g.spv
bin2c notes_v.spv notes_v.h notes_v
bin2c notes_f.spv notes_f.h notes_f
bin2c notes_g.spv notes_g.h notes_g