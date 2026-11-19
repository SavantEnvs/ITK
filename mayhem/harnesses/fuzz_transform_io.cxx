// Binary transform formats through itk::TransformFileReader, one target per format (CMake picks it
// with FZ_HEADER / FZ_FACTORY / FZ_NAME): MATLAB v4 .mat (MatlabTransformIO, vnl_matlab_read) and
// HDF5 .h5 (HDF5TransformIO, the vendored HDF5 library). As in fuzz_transform, the reader also runs
// TransformFactory creation and SetParameters/SetFixedParameters on the transforms it creates.
#include "fuzz_common.h"
#include "itkTransformFactoryBase.h"
#include "itkTransformFileReader.h"
#include FZ_HEADER

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  // One-time, input-independent setup: the transform and IO factory registries.
  static bool registered = false;
  if (!registered)
  {
    registered = true;
    itk::TransformFactoryBase::RegisterDefaultTransforms();
    itk::FZ_FACTORY::RegisterOneFactory();
  }
  fz::Init();
  fz::ScratchDir dir;
  const std::string fn = dir.Write(FZ_NAME, data, size);
  auto reader = itk::TransformFileReaderTemplate<double>::New();
  reader->SetFileName(fn);
  try
  {
    reader->Update();
  }
  catch (const itk::ExceptionObject &)
  {}
  return 0;
}
