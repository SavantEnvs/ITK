#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# Runs the ITK I/O selftest (built by build.sh from mayhem/harnesses/itk_io_selftest.cxx against the
# same ITK tree as the fuzzers): per-format write->read round trips with pixel/spacing comparison, a
# transform and four mesh round trips, and reads of ITK's shipped sample images. Every expected line,
# including the computed pixel sums, must appear exactly; a missing line counts as a failure.
BIN="$SRC/build-mayhem-fuzz/itk_io_selftest"
if [ ! -x "$BIN" ]; then
  echo "missing $BIN — run mayhem/build.sh first" >&2
  emit_ctrf "itk-io-selftest" 0 1 0
  exit 1
fi
OUT="$("$BIN" "$SRC" 2>/dev/null)"
rc=$?
echo "$OUT"
EXPECTED=(
  "PASS roundtrip_nii sum=39162 diff=0"
  "PASS roundtrip_gipl sum=39162 diff=0"
  "PASS roundtrip_tif sum=39162 diff=0"
  "PASS roundtrip_vtk sum=39162 diff=0"
  "PASS roundtrip_mha sum=39162 diff=0"
  "PASS roundtrip_nrrd sum=39162 diff=0"
  "PASS roundtrip_mrc sum=39162 diff=0"
  "PASS roundtrip_h5 sum=39162 diff=0"
  "PASS roundtrip_dcm sum=39162 diff=0"
  "PASS roundtrip_tfm psum=19.500000"
  "PASS roundtrip_mesh_vtk points=18 cells=32 abssum=22.971"
  "PASS roundtrip_mesh_obj points=18 cells=32 abssum=22.971"
  "PASS roundtrip_mesh_off points=18 cells=32 abssum=22.971"
  "PASS roundtrip_mesh_byu points=18 cells=32 abssum=22.971"
  "PASS sample_Examples/Data/KmeansTest_T1UCharRaw.nii.gz size=128x128x62 sum=19533798.0"
  "PASS sample_Examples/Data/BrainProtonDensity3Slices.mha size=181x217x3 sum=14725708.0"
  "PASS sample_Examples/Data/BrainProtonDensitySliceBorder20.nhdr size=221x257x1 sum=4861905.0"
  "PASS sample_Examples/Data/RatLungSlice1.mha size=128x128x1 sum=857357.0"
)
passed=0; failed=0
for e in "${EXPECTED[@]}"; do
  if grep -qxF -- "$e" <<<"$OUT"; then passed=$((passed+1)); else failed=$((failed+1)); echo "MISMATCH: expected '$e'" >&2; fi
done
if [ "$rc" -ne 0 ] && [ "$failed" -eq 0 ]; then failed=1; echo "selftest exited $rc" >&2; fi
emit_ctrf "itk-io-selftest" "$passed" "$failed" 0
