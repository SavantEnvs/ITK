// ITK's XML DOM reader (ITKIOXML over the vendored expat): parses the file into an itk::DOMNode tree.
#include "fuzz_common.h"
#include "itkDOMNodeXMLReader.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  fz::Init();
  fz::ScratchDir dir;
  const std::string fn = dir.Write("input.xml", data, size);
  auto reader = itk::DOMNodeXMLReader::New();
  reader->SetFileName(fn);
  try
  {
    reader->Update();
  }
  catch (const itk::ExceptionObject &)
  {}
  return 0;
}
