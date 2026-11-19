// Legacy VTK polydata mesh reader (.vtk): header, points, cells, point and cell data.
#include "fuzz_common.h"
#include "itkVTKPolyDataMeshIO.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  fz::Init();
  fz::ScratchDir dir;
  fz::ReadMesh(itk::VTKPolyDataMeshIO::New(), dir.Write("input.vtk", data, size));
  return 0;
}
