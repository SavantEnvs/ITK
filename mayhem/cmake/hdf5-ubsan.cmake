# Included right after ITK's vendored HDF5 calls project(HDF5 C) (CMAKE_PROJECT_HDF5_INCLUDE in
# mayhem/build.sh), so the flag lands on HDF5's own C compile lines only, not on the rest of ITK
# or the harnesses. -fsanitize=function checks at the call site, and HDF5's call sites call
# callbacks through mismatched function-pointer types (H5Iint.c discard_cb/free_func,
# H5Eint.c auto_op.func2 cast from H5E__print2).
if(CMAKE_C_FLAGS MATCHES "-fsanitize=[^ ]*undefined")
  string(APPEND CMAKE_C_FLAGS " -fno-sanitize=function")
endif()
