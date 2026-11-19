// Bruker ParaVision 2dseq: the reader is given <dir>/2dseq and parses the JCAMP-DX header <dir>/visu_pars
// (the fuzzed bytes). The 2dseq pixel file itself is a fixed zero-filled blob.
#include "fuzz_common.h"
#include "itkBruker2dseqImageIO.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  fz::Init();
  fz::ScratchDir dir;
  dir.Write("visu_pars", data, size);
  const std::string fn = dir.Write("2dseq", nullptr, 65536);
  auto io = itk::Bruker2dseqImageIO::New();
  fz::ReadImage(io, fn);
  return 0;
}
