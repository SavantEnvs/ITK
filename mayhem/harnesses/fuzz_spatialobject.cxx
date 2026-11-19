// MetaIO scene / spatial-object files (.tre, .meta, ...) through itk::SpatialObjectReader:
// MetaScene parsing of every MetaIO object type (tube, blob, ellipse, landmark, contour, mesh,
// surface, line, group, image, ...) and MetaSceneConverter's conversion to ITK spatial objects.
#include "fuzz_common.h"
#include "itkSpatialObjectReader.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  fz::Init();
  fz::ScratchDir dir;
  const std::string fn = dir.Write("input.meta", data, size);
  auto reader = itk::SpatialObjectReader<3>::New();
  reader->SetFileName(fn);
  try
  {
    reader->Update();
  }
  catch (const itk::ExceptionObject &)
  {}
  return 0;
}
