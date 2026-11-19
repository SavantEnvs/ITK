// Functional oracle for mayhem/test.sh (and seed generator for the fuzz targets).
//
//   itk_io_selftest <ITK source dir>   run every check; prints "PASS <name> <detail>" / "FAIL <name> <detail>"
//   itk_io_selftest --seeds <dir> <fmt> write one small valid file of that format into <dir>
//
// Checks: (1) write a known 3-D image in each format with its ImageIO, read it back, compare every
// pixel, spacing and size; (2) read the sample images shipped in Examples/Data and print their size
// and pixel sum, which test.sh compares to known values; (3) round-trip a transform and a mesh.
#include <cstdio>
#include <string>
#include <unistd.h>

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkMesh.h"
#include "itkMeshFileReader.h"
#include "itkMeshFileWriter.h"
#include "itkRegularSphereMeshSource.h"
#include "itkTransformFactoryBase.h"
#include "itkTransformFileReader.h"
#include "itkTransformFileWriter.h"
#include "itkTxtTransformIOFactory.h"
#include "itkAffineTransform.h"

#include "itkBMPImageIO.h"
#include "itkGDCMImageIO.h"
#include "itkGiplImageIO.h"
#include "itkHDF5ImageIO.h"
#include "itkMRCImageIO.h"
#include "itkMetaImageIO.h"
#include "itkNiftiImageIO.h"
#include "itkNrrdImageIO.h"
#include "itkTIFFImageIO.h"
#include "itkVTKImageIO.h"
#include "itkStimulateImageIO.h"
#include "itkOBJMeshIO.h"
#include "itkOFFMeshIO.h"
#include "itkVTKPolyDataMeshIO.h"
#include "itkBYUMeshIO.h"

using Image3 = itk::Image<unsigned char, 3>;
using Image2 = itk::Image<unsigned char, 2>;
static int g_fail = 0;

static void
Report(bool ok, const std::string & name, const std::string & detail)
{
  printf("%s %s %s\n", ok ? "PASS" : "FAIL", name.c_str(), detail.c_str());
  if (!ok)
    ++g_fail;
}

template <typename TImage>
static typename TImage::Pointer
MakeImage(unsigned int sx, unsigned int sy, unsigned int sz)
{
  auto img = TImage::New();
  typename TImage::SizeType size;
  size.Fill(1);
  size[0] = sx;
  size[1] = sy;
  if (TImage::ImageDimension > 2)
    size[2] = sz;
  img->SetRegions(size);
  typename TImage::SpacingType sp;
  for (unsigned int i = 0; i < TImage::ImageDimension; ++i)
    sp[i] = 0.5 + i;
  img->SetSpacing(sp);
  img->Allocate();
  itk::ImageRegionIterator<TImage> it(img, img->GetLargestPossibleRegion());
  unsigned int v = 0;
  for (it.GoToBegin(); !it.IsAtEnd(); ++it, ++v)
    it.Set(static_cast<unsigned char>((v * 7 + 3) & 0xff));
  return img;
}

template <typename TImage>
static void
RoundTrip(const std::string & dir, const std::string & name, itk::ImageIOBase * io, bool checkSpacing)
{
  const std::string fn = dir + "/rt." + name;
  try
  {
    auto img = MakeImage<TImage>(9, 7, 5);
    auto w = itk::ImageFileWriter<TImage>::New();
    w->SetFileName(fn);
    w->SetImageIO(io);
    w->SetInput(img);
    w->Update();
    auto r = itk::ImageFileReader<TImage>::New();
    r->SetFileName(fn);
    r->SetImageIO(io);
    r->Update();
    auto out = r->GetOutput();
    bool ok = out->GetLargestPossibleRegion().GetSize() == img->GetLargestPossibleRegion().GetSize();
    if (ok && checkSpacing)
      for (unsigned int i = 0; i < TImage::ImageDimension; ++i)
        ok = ok && std::abs(out->GetSpacing()[i] - img->GetSpacing()[i]) < 1e-4;
    unsigned long diff = 0, sum = 0;
    if (ok)
    {
      itk::ImageRegionConstIterator<TImage> a(img, img->GetLargestPossibleRegion());
      itk::ImageRegionConstIterator<TImage> b(out, out->GetLargestPossibleRegion());
      for (; !a.IsAtEnd(); ++a, ++b)
      {
        diff += a.Get() != b.Get();
        sum += b.Get();
      }
    }
    Report(ok && diff == 0, "roundtrip_" + name, "sum=" + std::to_string(sum) + " diff=" + std::to_string(diff));
  }
  catch (const itk::ExceptionObject & e)
  {
    Report(false, "roundtrip_" + name, e.GetDescription());
  }
}

static void
ReadSample(const std::string & src, const std::string & rel)
{
  using Img = itk::Image<float, 3>;
  try
  {
    auto r = itk::ImageFileReader<Img>::New();
    r->SetFileName(src + "/" + rel);
    if (rel.find(".nii") != std::string::npos)
      r->SetImageIO(itk::NiftiImageIO::New());
    else if (rel.find(".nhdr") != std::string::npos)
      r->SetImageIO(itk::NrrdImageIO::New());
    else
      r->SetImageIO(itk::MetaImageIO::New());
    r->Update();
    auto out = r->GetOutput();
    auto sz = out->GetLargestPossibleRegion().GetSize();
    double sum = 0;
    itk::ImageRegionConstIterator<Img> it(out, out->GetLargestPossibleRegion());
    for (; !it.IsAtEnd(); ++it)
      sum += it.Get();
    char buf[256];
    snprintf(buf, sizeof buf, "size=%lux%lux%lu sum=%.1f", sz[0], sz[1], sz[2], sum);
    Report(true, "sample_" + rel, buf);
  }
  catch (const itk::ExceptionObject & e)
  {
    Report(false, "sample_" + rel, e.GetDescription());
  }
}

static void
TransformRoundTrip(const std::string & dir)
{
  try
  {
    using T = itk::AffineTransform<double, 3>;
    auto t = T::New();
    T::ParametersType p(t->GetNumberOfParameters());
    for (unsigned int i = 0; i < p.size(); ++i)
      p[i] = 0.25 * (i + 1);
    t->SetParameters(p);
    const std::string fn = dir + "/rt.tfm";
    auto w = itk::TransformFileWriterTemplate<double>::New();
    w->SetFileName(fn);
    w->SetInput(t);
    w->Update();
    auto r = itk::TransformFileReaderTemplate<double>::New();
    r->SetFileName(fn);
    r->Update();
    auto l = r->GetTransformList();
    bool ok = l->size() == 1;
    double s = 0;
    if (ok)
    {
      auto q = l->front()->GetParameters();
      ok = q.size() == p.size() && std::string(l->front()->GetNameOfClass()) == "AffineTransform";
      for (unsigned int i = 0; ok && i < q.size(); ++i)
      {
        ok = std::abs(q[i] - p[i]) < 1e-12;
        s += q[i];
      }
    }
    Report(ok, "roundtrip_tfm", "psum=" + std::to_string(s));
  }
  catch (const itk::ExceptionObject & e)
  {
    Report(false, "roundtrip_tfm", e.GetDescription());
  }
}

static void
MeshRoundTrip(const std::string & dir, const std::string & ext, itk::MeshIOBase * io)
{
  using M = itk::Mesh<float, 3>;
  try
  {
    auto src = itk::RegularSphereMeshSource<M>::New();
    src->SetResolution(1);
    src->Update();
    const std::string fn = dir + "/rt." + ext;
    auto w = itk::MeshFileWriter<M>::New();
    w->SetFileName(fn);
    w->SetMeshIO(io);
    w->SetInput(src->GetOutput());
    w->Update();
    auto r = itk::MeshFileReader<M>::New();
    r->SetFileName(fn);
    r->SetMeshIO(io);
    r->Update();
    auto m = r->GetOutput();
    bool ok = m->GetNumberOfPoints() == src->GetOutput()->GetNumberOfPoints() &&
              m->GetNumberOfCells() == src->GetOutput()->GetNumberOfCells();
    double s = 0;
    for (unsigned long i = 0; ok && i < m->GetNumberOfPoints(); ++i)
    {
      auto a = m->GetPoint(i), b = src->GetOutput()->GetPoint(i);
      for (int k = 0; k < 3; ++k)
      {
        ok = ok && std::abs(a[k] - b[k]) < 1e-4;
        s += std::abs(a[k]);
      }
    }
    char buf[128];
    snprintf(buf, sizeof buf, "points=%lu cells=%lu abssum=%.3f", (unsigned long)m->GetNumberOfPoints(),
             (unsigned long)m->GetNumberOfCells(), s);
    Report(ok, "roundtrip_mesh_" + ext, buf);
  }
  catch (const itk::ExceptionObject & e)
  {
    Report(false, "roundtrip_mesh_" + ext, e.GetDescription());
  }
}

int
main(int argc, char ** argv)
{
  itk::Object::GlobalWarningDisplayOff();
  itk::TransformFactoryBase::RegisterDefaultTransforms();
  itk::TxtTransformIOFactory::RegisterOneFactory();
  if (argc == 4 && std::string(argv[1]) == "--seeds")
  {
    // --seeds <dir> <fmt>: one format per run, so a fault in one writer cannot stop the others.
    const std::string d = argv[2], f = argv[3];
    if (f == "nii") RoundTrip<Image3>(d, f, itk::NiftiImageIO::New(), true);
    else if (f == "gipl") RoundTrip<Image3>(d, f, itk::GiplImageIO::New(), true);
    else if (f == "tif") RoundTrip<Image3>(d, f, itk::TIFFImageIO::New(), false);
    else if (f == "vtk") RoundTrip<Image3>(d, f, itk::VTKImageIO::New(), true);
    else if (f == "mha") RoundTrip<Image3>(d, f, itk::MetaImageIO::New(), true);
    else if (f == "nrrd") RoundTrip<Image3>(d, f, itk::NrrdImageIO::New(), true);
    else if (f == "mrc") RoundTrip<Image3>(d, f, itk::MRCImageIO::New(), false);
    else if (f == "h5") RoundTrip<Image3>(d, f, itk::HDF5ImageIO::New(), true);
    else if (f == "dcm") RoundTrip<Image3>(d, f, itk::GDCMImageIO::New(), false);
    else if (f == "bmp") RoundTrip<Image2>(d, f, itk::BMPImageIO::New(), false);
    else if (f == "spr") RoundTrip<Image3>(d, f, itk::StimulateImageIO::New(), false);
    else if (f == "tfm") TransformRoundTrip(d);
    else if (f == "mvtk") MeshRoundTrip(d, "vtk", itk::VTKPolyDataMeshIO::New());
    else if (f == "obj") MeshRoundTrip(d, f, itk::OBJMeshIO::New());
    else if (f == "off") MeshRoundTrip(d, f, itk::OFFMeshIO::New());
    else if (f == "byu") MeshRoundTrip(d, f, itk::BYUMeshIO::New());
    return g_fail != 0;
  }
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <ITK source dir> | --seeds <dir>\n", argv[0]);
    return 2;
  }
  const char * t = getenv("TMPDIR");
  std::string tmpl = std::string(t && *t ? t : "/tmp") + "/itkselftest.XXXXXX";
  std::vector<char> b(tmpl.begin(), tmpl.end());
  b.push_back('\0');
  if (!mkdtemp(b.data()))
    return 2;
  const std::string d = b.data();
  RoundTrip<Image3>(d, "nii", itk::NiftiImageIO::New(), true);
  RoundTrip<Image3>(d, "gipl", itk::GiplImageIO::New(), true);
  RoundTrip<Image3>(d, "tif", itk::TIFFImageIO::New(), false);
  RoundTrip<Image3>(d, "vtk", itk::VTKImageIO::New(), true);
  RoundTrip<Image3>(d, "mha", itk::MetaImageIO::New(), true);
  RoundTrip<Image3>(d, "nrrd", itk::NrrdImageIO::New(), true);
  RoundTrip<Image3>(d, "mrc", itk::MRCImageIO::New(), false);
  RoundTrip<Image3>(d, "h5", itk::HDF5ImageIO::New(), true);
  RoundTrip<Image3>(d, "dcm", itk::GDCMImageIO::New(), false);
  TransformRoundTrip(d);
  MeshRoundTrip(d, "vtk", itk::VTKPolyDataMeshIO::New());
  MeshRoundTrip(d, "obj", itk::OBJMeshIO::New());
  MeshRoundTrip(d, "off", itk::OFFMeshIO::New());
  MeshRoundTrip(d, "byu", itk::BYUMeshIO::New());
  const std::string src = argv[1];
  ReadSample(src, "Examples/Data/KmeansTest_T1UCharRaw.nii.gz");
  ReadSample(src, "Examples/Data/BrainProtonDensity3Slices.mha");
  ReadSample(src, "Examples/Data/BrainProtonDensitySliceBorder20.nhdr");
  ReadSample(src, "Examples/Data/RatLungSlice1.mha");
  std::string cmd = "rm -rf '" + d + "'";
  if (system(cmd.c_str()) != 0)
    ++g_fail;
  return g_fail != 0;
}
