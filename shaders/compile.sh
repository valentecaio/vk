#!/bin/bash

outdir="$1"
if [ -z "$outdir" ]; then
  outdir="."
fi

mkdir -p "$outdir"

for shader_file in *.vert *.frag; do
  output_file="$outdir/${shader_file}.spv"
  glslc "$shader_file" -o "$output_file"
  echo "Compiled $shader_file to $output_file"
done
