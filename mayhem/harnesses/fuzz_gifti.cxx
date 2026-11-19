// GIFTI XML surface meshes (.gii) through itk::GiftiMeshIO and the vendored gifticlib (expat XML,
// base64/gzip data arrays): CanReadFile -> ReadMeshInformation -> ReadPoints/Cells/PointData/CellData.
#include "fuzz_common.h"
#include "itkGiftiMeshIO.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  fz::Init();
  fz::ScratchDir dir;
  fz::ReadMesh(itk::GiftiMeshIO::New(), dir.Write("input.gii", data, size));
  return 0;
}
