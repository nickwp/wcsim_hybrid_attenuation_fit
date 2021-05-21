SET(OPTICALFIT_LIB_LIST "libOpticalFit.so")
SET(OPTICALFIT_LIBFLAG_LIST "-lOpticalFit")

string(REPLACE ";" " -l" ROOT_LIB_FLAGS "${ROOT_LIBS}")
string(REPLACE ";" " " ROOT_CXX_FLAGS_SEP "${ROOT_CXX_FLAGS}")

SET(ROOT_FLAGS "${ROOT_CXX_FLAGS_SEP} -L${ROOT_LIBDIR} -l${ROOT_LIB_FLAGS}")

SET(OPTICALFIT_FEATURES)
if (USE_WCSIM)
    string(REPLACE ";" " -L" WCSIM_LINK_FLAGS "${WCSIM_LIBDIR}")
    string(REPLACE ";" " -l" WCSIM_LIB_FLAGS "${WCSIM_LIBS}")
    string(REPLACE ";" " " WCSIM_INCLUDE_FLAGS "${WCSIM_CXX_FLAGS}")

    SET(WCSIM_FLAGS "${WCSIM_INCLUDE_FLAGS} -L${WCSIM_LINK_FLAGS} -l${WCSIM_LIB_FLAGS}")

    LIST(APPEND OPTICALFIT_FEATURES "WCSIM")
ENDIF()

configure_file(cmake/opticalfit-config.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/opticalfit-config" @ONLY)
install(PROGRAMS
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/opticalfit-config" DESTINATION
  bin)