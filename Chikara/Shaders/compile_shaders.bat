glslc triangle.vert -o vert.spv
glslc triangle.frag -o frag.spv
bin2c vert.spv vert.h vert_spv
bin2c frag.spv frag.h frag_spv