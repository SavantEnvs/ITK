# Included right after ITK's vendored NrrdIO calls project(NrrdIO C) (CMAKE_PROJECT_NrrdIO_INCLUDE in
# mayhem/build.sh), so the flag lands on NrrdIO's own C compile lines only. NrrdIO's NRRD read routine
# advances its data pointer even when ITK reads only the header (data == NULL):
# formatNRRD.c:635 `data += valsPerPiece * nrrdElementSize(nrrd)` -> "applying non-zero offset 315 to
# null pointer" on every valid .nrrd/.nhdr (itk_io_selftest's sample_*.nhdr and roundtrip_nrrd cases).
if(CMAKE_C_FLAGS MATCHES "-fsanitize=[^ ]*undefined")
  string(APPEND CMAKE_C_FLAGS " -fno-sanitize=pointer-overflow")
endif()
