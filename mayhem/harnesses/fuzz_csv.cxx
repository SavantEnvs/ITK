// CSV files through itk::CSVArray2DFileReader<double>: the first input byte sets the reader's
// header options (bit 0: column headers, bit 1: row headers), the rest is the file. Parse() then
// tokenizes every line and converts each field to double.
#include "fuzz_common.h"
#include "itkCSVArray2DFileReader.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  if (size < 1)
    return -1;
  fz::Init();
  fz::ScratchDir dir;
  const uint8_t opts = data[0];
  const std::string fn = dir.Write("input.csv", data + 1, size - 1);
  auto reader = itk::CSVArray2DFileReader<double>::New();
  reader->SetFileName(fn);
  reader->SetFieldDelimiterCharacter(',');
  reader->SetHasColumnHeaders((opts & 1) != 0);
  reader->SetHasRowHeaders((opts & 2) != 0);
  try
  {
    reader->Parse();
    auto out = reader->GetOutput();
    (void)out;
  }
  catch (const itk::ExceptionObject &)
  {}
  return 0;
}
