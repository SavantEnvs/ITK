// ITK's text transform format (.tfm) through itk::TransformFileReader: TxtTransformIO parsing plus
// TransformFactory creation and SetParameters/SetFixedParameters on the created transforms.
#include "fuzz_common.h"
#include "itkTransformFactoryBase.h"
#include "itkTransformFileReader.h"
#include "itkTxtTransformIOFactory.h"

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
  static bool registered = false;
  if (!registered)
  {
    registered = true;
    itk::TransformFactoryBase::RegisterDefaultTransforms();
    itk::TxtTransformIOFactory::RegisterOneFactory();
  }
  fz::Init();
  fz::ScratchDir dir;
  const std::string fn = dir.Write("input.tfm", data, size);
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
