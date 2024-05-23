#!/bin/bash

output_dir="$1"
if [ -z "$output_dir" ]; then
  output_dir="."
fi

/usr/local/bin/glslc shader.vert -o "$output_dir/vert.spv"
/usr/local/bin/glslc shader.frag -o "$output_dir/frag.spv"
