#!/bin/bash

outdir="$1"
if [ -z "$outdir" ]; then
  outdir="."
fi

/usr/local/bin/glslc shader.vert -o "$outdir/vert.spv"
/usr/local/bin/glslc shader.frag -o "$outdir/frag.spv"
