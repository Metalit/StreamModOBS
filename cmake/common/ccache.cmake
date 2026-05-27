# OBS CMake ccache module

include_guard(GLOBAL)

if(NOT DEFINED CCACHE_PROGRAM)
  message(DEBUG "Trying to find ccache on build host")
  find_program(CCACHE_PROGRAM "ccache")
  mark_as_advanced(CCACHE_PROGRAM)
endif()

if(CCACHE_PROGRAM)
  message(DEBUG "Trying to find ccache on build host - done")
  message(DEBUG "Ccache found as ${CCACHE_PROGRAM}")
  option(ENABLE_CCACHE "Enable compiler acceleration with ccache" OFF)

  if(ENABLE_CCACHE)
    if(OS_WINDOWS)
      file(COPY_FILE ${CCACHE_PROGRAM} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)
      set(
        CMAKE_VS_GLOBALS
        "CLToolExe=cl.exe"
        "CLToolPath=${CMAKE_BINARY_DIR}"
        "UseMultiToolTask=true"
        "DebugInformationFormat=OldStyle"
      )
    else()
      if(OS_MACOS)
        set(CLANG_ENABLE_EXPLICIT_MODULES_WITH_COMPILER_LAUNCHER YES)
      endif()
      set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
      set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
      set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
      set(CMAKE_OBJCXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
      set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    endif()
  endif()
else()
  message(DEBUG "Trying to find ccache on build host - skipped")
endif()
