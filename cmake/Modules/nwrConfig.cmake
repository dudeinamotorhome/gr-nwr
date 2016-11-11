INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_NWR nwr)

FIND_PATH(
    NWR_INCLUDE_DIRS
    NAMES nwr/api.h
    HINTS $ENV{NWR_DIR}/include
        ${PC_NWR_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    NWR_LIBRARIES
    NAMES gnuradio-nwr
    HINTS $ENV{NWR_DIR}/lib
        ${PC_NWR_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NWR DEFAULT_MSG NWR_LIBRARIES NWR_INCLUDE_DIRS)
MARK_AS_ADVANCED(NWR_LIBRARIES NWR_INCLUDE_DIRS)

