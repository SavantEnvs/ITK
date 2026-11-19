#!/usr/bin/env bash
#
# mayhem/build.sh — build ITK's file readers with ASan+UBSan+SanCov and the fuzz harnesses
# (mayhem/harnesses/*.cxx, one libFuzzer binary + one -standalone reproducer each), plus the
# functional selftest that mayhem/test.sh runs.
#
# ITK vendors every third-party library it needs (zlib, libtiff, expat, HDF5, GDCM, niftilib, NrrdIO,
# MetaIO, ...) under Modules/ThirdParty, so the build needs no network: BUILD_TESTING=OFF keeps
# ExternalData (test-data downloads) out, and no Remote module or DCMTK is enabled.
# Re-runnable: both build trees are incremental CMake/Ninja trees (gitignored by upstream's `build*/`).
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${SRC:=/mayhem}"
export CC CXX
cd "$SRC"

# SanCov on the project (not only the harness) so the fuzzer sees ITK's own edges.
COV_FLAGS="-fsanitize=fuzzer-no-link"
# Two UBSan checks are relaxed, each only on the one vendored C library that trips it on valid input
# (CMake project-include hooks append the flag to that project's CMAKE_C_FLAGS; see mayhem/cmake/):
#  - function, HDF5 only (hdf5-ubsan.cmake): HDF5 calls its callbacks through mismatched
#    function-pointer types (H5Iint.c, H5Eint.c);
#  - pointer-overflow, NrrdIO only (nrrdio-ubsan.cmake): formatNRRD.c:635 advances a NULL data
#    pointer on every header-only read of a valid NRRD file.
# ASan and every other UBSan check stay on and halting, and both checks stay on in the rest of ITK.
FLAGS="$SANITIZER_FLAGS $COV_FLAGS $DEBUG_FLAGS"

MODULES=(ITKIONIFTI ITKIOGDCM ITKIOTIFF ITKIOHDF5 ITKIOBruker ITKIOPhilipsREC ITKIOGIPL ITKIOGE
         ITKIOSiemens ITKIOIPL ITKIOVTK ITKIOMeta ITKIONRRD ITKIOXML ITKIOMRC ITKIOBMP ITKIOStimulate
         ITKIOBioRad ITKIOLSM ITKIOMeshVTK ITKIOMeshOBJ ITKIOMeshOFF ITKIOMeshBYU ITKIOMeshFreeSurfer
         ITKIOTransformInsightLegacy ITKMesh
         ITKIOTransformMatlab ITKIOTransformHDF5 ITKIOSpatialObjects ITKIOMeshGifti ITKIOCSV)
MOD_ARGS=()
for m in "${MODULES[@]}"; do MOD_ARGS+=("-DModule_${m}=ON"); done

# 1) ITK itself, sanitized + instrumented. Release with -O1 (no -DNDEBUG: keep asserts).
cmake -S "$SRC" -B "$SRC/build-mayhem-itk" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="$FLAGS" -DCMAKE_CXX_FLAGS="$FLAGS" \
  -DCMAKE_C_FLAGS_RELEASE="-O1" -DCMAKE_CXX_FLAGS_RELEASE="-O1" \
  -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF \
  -DITK_BUILD_DEFAULT_MODULES=OFF -DITK_WRAP_PYTHON=OFF -DITK_USE_GPU=OFF \
  -DITK_FORBID_DOWNLOADS=ON \
  -DCMAKE_PROJECT_HDF5_INCLUDE="$SRC/mayhem/cmake/hdf5-ubsan.cmake" \
  -DCMAKE_PROJECT_NrrdIO_INCLUDE="$SRC/mayhem/cmake/nrrdio-ubsan.cmake" \
  "${MOD_ARGS[@]}"
cmake --build "$SRC/build-mayhem-itk" -j"$MAYHEM_JOBS"

# 2) Harnesses (+ standalone reproducers) and the selftest, against that tree.
cmake -S "$SRC/mayhem" -B "$SRC/build-mayhem-fuzz" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS_RELEASE="-O1" -DCMAKE_CXX_FLAGS_RELEASE="-O1" \
  -DCMAKE_C_FLAGS="$FLAGS" -DCMAKE_CXX_FLAGS="$FLAGS" \
  -DITK_DIR="$SRC/build-mayhem-itk" \
  -DFZ_ENGINE="$LIB_FUZZING_ENGINE" -DFZ_STANDALONE_MAIN="$STANDALONE_FUZZ_MAIN" \
  -DFZ_LSAN_OFF="$SRC/mayhem/lsan_off.cc"   # linked into every fuzz + -standalone binary
cmake --build "$SRC/build-mayhem-fuzz" -j"$MAYHEM_JOBS"
