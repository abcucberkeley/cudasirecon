#!/bin/bash
set -euo pipefail

SRC=/global/home/users/matthewmueller/miniconda3/envs/sim2Beam3D
DST=/global/home/groups/software/rocky-8.x86_64/modules/cudasirecon/2Beam3D
PATCHELF=/global/home/groups/software/rocky-8.x86_64/modules/patchelf/0.17.2/bin/patchelf

echo "=== Syncing bin/, lib/, include/ ==="
rsync -aP "$SRC/bin" "$SRC/lib" "$SRC/include" "$DST/"

echo ""
echo "=== Patching RPATH ==="

# Binary: look for libs in ../lib relative to its own location
$PATCHELF --force-rpath --set-rpath '$ORIGIN/../lib' "$DST/bin/cudasirecon"
echo "  cudasirecon -> \$ORIGIN/../lib"

# Shared lib: look for libs in its own directory
$PATCHELF --force-rpath --set-rpath '$ORIGIN' "$DST/lib/libcudasirecon.so"
echo "  libcudasirecon.so -> \$ORIGIN"

echo ""
echo "=== Verifying ==="
readelf -d "$DST/bin/cudasirecon" 2>/dev/null | grep RPATH
readelf -d "$DST/lib/libcudasirecon.so" 2>/dev/null | grep RPATH

echo ""
echo "Done."
