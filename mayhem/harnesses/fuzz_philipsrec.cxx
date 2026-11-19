// Philips PAR/REC: the fuzzed bytes are the text header x.PAR; x.REC is a fixed zero-filled blob.
#include "fuzz_common.h"
#include "itkPhilipsRECImageIO.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  fz::Init();
  fz::ScratchDir dir;
  const std::string fn = dir.Write("x.PAR", data, size);
  dir.Write("x.REC", nullptr, 65536);
  auto io = itk::PhilipsRECImageIO::New();
  fz::ReadImage(io, fn);
  return 0;
}
