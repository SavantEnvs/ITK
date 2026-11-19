// Shared helpers for the ITK file-reader fuzz harnesses (mayhem/harnesses/*.cxx).
//
// Every ITK reader takes a FILE NAME, and many gate on the extension, so each input is written to a
// fresh scratch directory under $TMPDIR (fallback /tmp) with the name the reader expects, read through
// the reader's public API, then removed. Nothing survives across inputs: the directory is per input,
// and the only process-global setting (warning display off) is set once and never changed by input.
//
// Errors the readers report are itk::ExceptionObject and are caught: that is ITK's documented way to
// reject a bad file. Anything else (std::invalid_argument from a raw std::sto*, std::bad_alloc, ...)
// escapes and is a finding, as is any sanitizer report.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>

#include "itkImageIOBase.h"
#include "itkMeshIOBase.h"
#include "itkObject.h"
#include "itkMultiThreaderBase.h"

namespace fz
{
// Reads above this size are skipped: the header claims a buffer we would have to allocate, and a
// multi-GB allocation is an OOM report, not a parser bug. The header parse itself always runs.
static const size_t kMaxBytes = 64u << 20;

inline void
Init()
{
  static bool done = false;
  if (done)
    return;
  done = true;
  itk::Object::GlobalWarningDisplayOff();
  itk::MultiThreaderBase::SetGlobalDefaultNumberOfThreads(1);
}

struct ScratchDir
{
  std::string path;
  std::vector<std::string> files;
  ScratchDir()
  {
    const char * t = getenv("TMPDIR");
    std::string tmpl = std::string(t && *t ? t : "/tmp") + "/itkfuzz.XXXXXX";
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    if (mkdtemp(buf.data()) == nullptr)
      abort();
    path = buf.data();
  }
  // Writes `n` bytes (or n zero bytes when data == nullptr) to <dir>/<name>; returns the full path.
  std::string
  Write(const std::string & name, const uint8_t * data, size_t n)
  {
    std::string p = path + "/" + name;
    FILE * f = fopen(p.c_str(), "wb");
    if (!f)
      abort();
    if (data)
    {
      if (n && fwrite(data, 1, n, f) != n)
        abort();
    }
    else
    {
      std::vector<uint8_t> z(n, 0);
      if (n && fwrite(z.data(), 1, n, f) != n)
        abort();
    }
    fclose(f);
    files.push_back(p);
    return p;
  }
  ~ScratchDir()
  {
    for (auto & f : files)
      unlink(f.c_str());
    rmdir(path.c_str());
  }
};

// CanReadFile -> ReadImageInformation -> Read of the whole image (what itk::ImageFileReader does).
inline void
ReadImage(itk::ImageIOBase * io, const std::string & fn, bool checkCanRead = true)
{
  try
  {
    if (checkCanRead && !io->CanReadFile(fn.c_str()))
      return;
    io->SetFileName(fn);
    io->ReadImageInformation();
    const unsigned int nd = io->GetNumberOfDimensions();
    if (nd == 0 || nd > 16)
      return;
    itk::ImageIORegion region(nd);
    size_t pixels = 1;
    for (unsigned int i = 0; i < nd; ++i)
    {
      const size_t d = io->GetDimensions(i);
      if (d == 0 || d > kMaxBytes)
        return;
      pixels *= d;
      if (pixels > kMaxBytes)
        return;
      region.SetIndex(i, 0);
      region.SetSize(i, d);
    }
    io->SetIORegion(region);
    const size_t bytes = static_cast<size_t>(io->GetImageSizeInBytes());
    if (bytes == 0 || bytes > kMaxBytes)
      return;
    std::vector<char> buffer(bytes);
    io->Read(buffer.data());
  }
  catch (const itk::ExceptionObject &)
  {}
}

// CanReadFile -> ReadMeshInformation -> ReadPoints/Cells/PointData/CellData (what itk::MeshFileReader does).
inline void
ReadMesh(itk::MeshIOBase * io, const std::string & fn)
{
  try
  {
    if (!io->CanReadFile(fn.c_str()))
      return;
    io->SetFileName(fn);
    io->ReadMeshInformation();
    auto sized = [](size_t a, size_t b, size_t c, size_t & out) -> bool {
      if (a == 0 || b == 0 || c == 0)
        return false;
      if (a > kMaxBytes || b > kMaxBytes || c > kMaxBytes)
        return false;
      out = a * b;
      if (out > kMaxBytes)
        return false;
      out *= c;
      return out <= kMaxBytes;
    };
    size_t n = 0;
    if (io->GetUpdatePoints() &&
        sized(io->GetNumberOfPoints(), io->GetPointDimension(), io->GetComponentSize(io->GetPointComponentType()), n))
    {
      std::vector<char> b(n);
      io->ReadPoints(b.data());
    }
    if (io->GetUpdateCells() &&
        sized(io->GetCellBufferSize(), 1, io->GetComponentSize(io->GetCellComponentType()), n))
    {
      std::vector<char> b(n);
      io->ReadCells(b.data());
    }
    if (io->GetUpdatePointData() && sized(io->GetNumberOfPointPixels(),
                                          io->GetNumberOfPointPixelComponents(),
                                          io->GetComponentSize(io->GetPointPixelComponentType()),
                                          n))
    {
      std::vector<char> b(n);
      io->ReadPointData(b.data());
    }
    if (io->GetUpdateCellData() && sized(io->GetNumberOfCellPixels(),
                                         io->GetNumberOfCellPixelComponents(),
                                         io->GetComponentSize(io->GetCellPixelComponentType()),
                                         n))
    {
      std::vector<char> b(n);
      io->ReadCellData(b.data());
    }
  }
  catch (const itk::ExceptionObject &)
  {}
}
} // namespace fz
