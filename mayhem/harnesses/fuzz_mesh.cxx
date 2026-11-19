// Text/binary mesh readers; the first input byte picks the reader and its extension:
// Wavefront .obj, Geomview .off, Movie.BYU .byu, FreeSurfer ASCII .fsa and binary .fsb.
#include "fuzz_common.h"
#include "itkBYUMeshIO.h"
#include "itkFreeSurferAsciiMeshIO.h"
#include "itkFreeSurferBinaryMeshIO.h"
#include "itkOBJMeshIO.h"
#include "itkOFFMeshIO.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  if (size < 1)
    return -1;
  fz::Init();
  fz::ScratchDir dir;
  const uint8_t sel = data[0] % 5;
  ++data;
  --size;
  switch (sel)
  {
    case 0:
      fz::ReadMesh(itk::OBJMeshIO::New(), dir.Write("input.obj", data, size));
      break;
    case 1:
      fz::ReadMesh(itk::OFFMeshIO::New(), dir.Write("input.off", data, size));
      break;
    case 2:
      fz::ReadMesh(itk::BYUMeshIO::New(), dir.Write("input.byu", data, size));
      break;
    case 3:
      fz::ReadMesh(itk::FreeSurferAsciiMeshIO::New(), dir.Write("input.fsa", data, size));
      break;
    default:
      fz::ReadMesh(itk::FreeSurferBinaryMeshIO::New(), dir.Write("input.fsb", data, size));
      break;
  }
  return 0;
}
