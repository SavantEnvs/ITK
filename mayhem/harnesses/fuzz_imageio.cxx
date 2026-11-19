// One ImageIO class, chosen at compile time (build.sh: -DFZ_HEADER=... -DFZ_CLASS=... -DFZ_NAME=...),
// fed a file whose name carries the extension the class reads. See fuzz_common.h.
#include "fuzz_common.h"
#include FZ_HEADER

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  fz::Init();
  fz::ScratchDir dir;
  const std::string fn = dir.Write(FZ_NAME, data, size);
  auto io = itk::FZ_CLASS::New();
  fz::ReadImage(io, fn);
  return 0;
}
