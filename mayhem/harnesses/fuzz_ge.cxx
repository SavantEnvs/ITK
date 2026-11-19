// GE Genesis (GE4/GE5), GE Advantage Windows and Siemens Vision header readers (the IPL family):
// the same file is offered to each in turn, as ImageIOFactory would.
#include "fuzz_common.h"
#include "itkGE4ImageIO.h"
#include "itkGE5ImageIO.h"
#include "itkGEAdwImageIO.h"
#include "itkSiemensVisionImageIO.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  fz::Init();
  fz::ScratchDir dir;
  const std::string fn = dir.Write("input.ima", data, size);
  fz::ReadImage(itk::GE4ImageIO::New(), fn);
  fz::ReadImage(itk::GE5ImageIO::New(), fn);
  fz::ReadImage(itk::GEAdwImageIO::New(), fn);
  fz::ReadImage(itk::SiemensVisionImageIO::New(), fn);
  return 0;
}
