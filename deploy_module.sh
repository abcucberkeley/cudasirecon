#!/bin/bash
set -euo pipefail

SRC=/global/home/users/matthewmueller/miniconda3/envs/sim2Beam3D
DST=/global/home/groups/software/rocky-8.x86_64/modules/cudasirecon/2Beam3D
PATCHELF=/global/home/groups/software/rocky-8.x86_64/modules/patchelf/0.17.2/bin/patchelf

# Spack-installed boost 1.83.0 (same thing `module load boost/1.83.0` provides).
# We bake this into the rpath so cudasirecon finds libboost_*.so.1.83.0 without
# the modulefile having to chain-load the boost module. Chain-loading caused
# intermittent "Non-zero status returned" errors on some users' shells.
BOOST_LIB=/global/software/rocky-8.x86_64/gcc/linux-rocky8-x86_64/gcc-8.5.0/boost-1.83.0-ybfoz6jm3k4hax5jn365lrvrpdytjobv/lib

# Only the project's own binaries go into the module's bin/. The conda env
# drops in dozens of unrelated tools (tclsh, python helpers, etc.) that we
# don't want to ship as part of the cudasirecon module.
BIN_WHITELIST=(cudasirecon makeotf otfviewer)

echo "=== Syncing bin/ (whitelist), lib/, include/ ==="
mkdir -p "$DST/bin"
for b in "${BIN_WHITELIST[@]}"; do
  if [[ -f "$SRC/bin/$b" ]]; then
    rsync -aP "$SRC/bin/$b" "$DST/bin/"
  else
    echo "  warning: $SRC/bin/$b not found, skipping" >&2
  fi
done
# Exclude ncurses/readline from the shipped lib/. cudasirecon doesn't link
# against them (verified via ldd), but conda builds them with the terminfo
# database path baked in to $CONDA_PREFIX/share/terminfo -- which isn't part
# of the module. If we ship these libs, LD_LIBRARY_PATH hijacks the system's
# libtinfo.so.6 for any ncurses user (htop, vim, less, tmux, ...) and they
# all fail with: Error opening terminal: xterm-256color.
LIB_EXCLUDES=(
  'libncurses*' 'libncursesw*'
  'libtinfo*'  'libtinfow*'
  'libreadline*'
  'libform*'   'libformw*'
  'libmenu*'   'libmenuw*'
  'libpanel*'  'libpanelw*'
  'libhistory*'
  'terminfo'   # in case conda also dropped a share/terminfo into lib/
)
RSYNC_EXCLUDES=()
for e in "${LIB_EXCLUDES[@]}"; do RSYNC_EXCLUDES+=(--exclude="$e"); done

rsync -aP "${RSYNC_EXCLUDES[@]}" "$SRC/lib" "$SRC/include" "$DST/"

echo ""
echo "=== Pruning any pre-existing ncurses/readline libs from $DST/lib ==="
for e in "${LIB_EXCLUDES[@]}"; do
  find "$DST/lib" -maxdepth 1 -name "$e" -print -delete 2>/dev/null || true
done

echo ""
echo "=== Patching RPATH ==="

# Binary: look for libs in ../lib, then the system boost/1.83.0 install.
$PATCHELF --force-rpath --set-rpath "\$ORIGIN/../lib:$BOOST_LIB" "$DST/bin/cudasirecon"
echo "  cudasirecon -> \$ORIGIN/../lib:$BOOST_LIB"

# Shared lib: look for libs in its own directory, then the system boost/1.83.0.
$PATCHELF --force-rpath --set-rpath "\$ORIGIN:$BOOST_LIB" "$DST/lib/libcudasirecon.so"
echo "  libcudasirecon.so -> \$ORIGIN:$BOOST_LIB"

echo ""
echo "=== Verifying ==="
readelf -d "$DST/bin/cudasirecon" 2>/dev/null | grep RPATH
readelf -d "$DST/lib/libcudasirecon.so" 2>/dev/null | grep RPATH

echo ""
echo "Done."
