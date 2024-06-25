#!/bin/bash

outdir="$1"
if [ -z "$outdir" ]; then
  outdir="."
fi

glslc shader.vert -o "$outdir/vert.spv"
glslc shader.frag -o "$outdir/frag.spv"
