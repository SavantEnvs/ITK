// Small legacy image formats; the first input byte picks the reader (and so the extension it requires):
// Stimulate .spr (text header; its .sdt data file is a fixed zero blob), Bio-Rad .pic, Zeiss LSM .lsm.
#include "fuzz_common.h"
#include "itkBioRadImageIO.h"
#include "itkLSMImageIO.h"
#include "itkStimulateImageIO.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  if (size < 1)
    return -1;
  fz::Init();
  fz::ScratchDir dir;
  const uint8_t sel = data[0] % 3;
  ++data;
  --size;
  if (sel == 0)
  {
    const std::string fn = dir.Write("input.spr", data, size);
    dir.Write("input.sdt", nullptr, 65536);
    fz::ReadImage(itk::StimulateImageIO::New(), fn);
  }
  else if (sel == 1)
    fz::ReadImage(itk::BioRadImageIO::New(), dir.Write("input.pic", data, size));
  else
    fz::ReadImage(itk::LSMImageIO::New(), dir.Write("input.lsm", data, size));
  return 0;
}
