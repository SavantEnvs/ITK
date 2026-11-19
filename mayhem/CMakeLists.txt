# Out-of-tree project for the fuzz harnesses and the functional selftest, linked against the ITK
# build tree that mayhem/build.sh configures (-DITK_DIR=...).
cmake_minimum_required(VERSION 3.16)
project(itk_mayhem C CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

# Each ASan+libFuzzer link of the static ITK libraries takes GBs of RAM; cap concurrent links so a
# memory-limited build container (e.g. 12 GB) is not OOM-killed by 16 parallel linkers.
set(FZ_LINK_JOBS 2 CACHE STRING "concurrent harness links")
set_property(GLOBAL APPEND PROPERTY JOB_POOLS fz_link=${FZ_LINK_JOBS})
set(CMAKE_JOB_POOL_LINK fz_link)

set(FZ_ENGINE "-fsanitize=fuzzer" CACHE STRING "libFuzzer link flags")
set(FZ_STANDALONE_MAIN "" CACHE FILEPATH "LLVM run-once driver (C)")
separate_arguments(FZ_ENGINE_LIST UNIX_COMMAND "${FZ_ENGINE}")

set(FZ_LSAN_OFF "${CMAKE_CURRENT_SOURCE_DIR}/lsan_off.cc" CACHE FILEPATH "LeakSanitizer-off hook")
add_library(fz_lsan_off OBJECT ${FZ_LSAN_OFF})
add_library(fz_standalone_main OBJECT ${FZ_STANDALONE_MAIN})
set_source_files_properties(${FZ_STANDALONE_MAIN} PROPERTIES LANGUAGE C)

function(fz name src)
  add_executable(${name} harnesses/${src} $<TARGET_OBJECTS:fz_lsan_off>)
  target_compile_definitions(${name} PRIVATE ${ARGN})
  target_include_directories(${name} PRIVATE harnesses)
  target_link_libraries(${name} ${ITK_LIBRARIES})
  target_link_options(${name} PRIVATE ${FZ_ENGINE_LIST})
  add_executable(${name}-standalone harnesses/${src} $<TARGET_OBJECTS:fz_lsan_off>
                 $<TARGET_OBJECTS:fz_standalone_main>)
  target_compile_definitions(${name}-standalone PRIVATE ${ARGN})
  target_include_directories(${name}-standalone PRIVATE harnesses)
  target_link_libraries(${name}-standalone ${ITK_LIBRARIES})
endfunction()

function(fz_image name header class fname)
  fz(${name} fuzz_imageio.cxx "FZ_HEADER=\"${header}\"" "FZ_CLASS=${class}" "FZ_NAME=\"${fname}\"")
endfunction()

fz_image(fuzz_nifti itkNiftiImageIO.h NiftiImageIO input.nii)
fz_image(fuzz_gdcm itkGDCMImageIO.h GDCMImageIO input.dcm)
fz_image(fuzz_tiff itkTIFFImageIO.h TIFFImageIO input.tif)
fz_image(fuzz_hdf5 itkHDF5ImageIO.h HDF5ImageIO input.h5)
fz_image(fuzz_gipl itkGiplImageIO.h GiplImageIO input.gipl)
fz_image(fuzz_vtk itkVTKImageIO.h VTKImageIO input.vtk)
fz_image(fuzz_meta itkMetaImageIO.h MetaImageIO input.mha)
fz_image(fuzz_nrrd itkNrrdImageIO.h NrrdImageIO input.nrrd)
fz_image(fuzz_mrc itkMRCImageIO.h MRCImageIO input.mrc)
fz_image(fuzz_bmp itkBMPImageIO.h BMPImageIO input.bmp)
fz(fuzz_bruker fuzz_bruker.cxx)
fz(fuzz_philipsrec fuzz_philipsrec.cxx)
fz(fuzz_ge fuzz_ge.cxx)
fz(fuzz_legacyimage fuzz_legacyimage.cxx)
fz(fuzz_vtkmesh fuzz_vtkmesh.cxx)
fz(fuzz_mesh fuzz_mesh.cxx)
fz(fuzz_transform fuzz_transform.cxx)
fz(fuzz_xml fuzz_xml.cxx)
fz(fuzz_transform_matlab fuzz_transform_io.cxx "FZ_HEADER=\"itkMatlabTransformIOFactory.h\""
   "FZ_FACTORY=MatlabTransformIOFactory" "FZ_NAME=\"input.mat\"")
fz(fuzz_transform_hdf5 fuzz_transform_io.cxx "FZ_HEADER=\"itkHDF5TransformIOFactory.h\""
   "FZ_FACTORY=HDF5TransformIOFactory" "FZ_NAME=\"input.h5\"")
fz(fuzz_spatialobject fuzz_spatialobject.cxx)
fz(fuzz_gifti fuzz_gifti.cxx)
fz(fuzz_csv fuzz_csv.cxx)

add_executable(itk_io_selftest harnesses/itk_io_selftest.cxx)
target_link_libraries(itk_io_selftest ${ITK_LIBRARIES})
