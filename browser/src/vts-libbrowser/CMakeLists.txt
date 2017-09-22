
set(EXTRA_SRC_LIST)
set(EXTRA_LIB_DEPS)
set(EXTRA_MODULE_DEPS)

if(IOS)

  # ios
  find_package(ZLIB REQUIRED)
  include_directories(${ZLIB_INCLUDE_DIRS})
  list(APPEND EXTRA_LIB_DEPS ${ZLIB_LIBRARIES} iconv objc "-framework CoreGraphics")
  list(APPEND EXTRA_SRC_LIST fetcher/ios.mm image/ios.mm)

else()

  # desktop
  find_package(JPEG REQUIRED)
  find_package(PNG REQUIRED)
  find_package(CURL REQUIRED)
  add_subdirectory(fetcher/http)
  include_directories(fetcher)
  list(APPEND EXTRA_MODULE_DEPS http)
  list(APPEND EXTRA_SRC_LIST fetcher/curl.cpp image/desktop.cpp)

endif()

define_module(LIBRARY vts-browser DEPENDS vts-libs-nucleus jsoncpp
  ${EXTRA_MODULE_DEPS})

set(PUB_HDR_LIST
  include/vts-browser/buffer.hpp
  include/vts-browser/foundation.hpp
  include/vts-browser/options.hpp
  include/vts-browser/resources.hpp
  include/vts-browser/fetcher.hpp
  include/vts-browser/map.hpp
  include/vts-browser/draws.hpp
  include/vts-browser/statistics.hpp
  include/vts-browser/view.hpp
  include/vts-browser/math.hpp
  include/vts-browser/exceptions.hpp
  include/vts-browser/credits.hpp
  include/vts-browser/search.hpp
  include/vts-browser/log.hpp
  include/vts-browser/celestial.hpp
  include/vts-browser/boostProgramOptions.hpp
)

set(HDR_LIST
  coordsManip.hpp
  image.hpp
  map.hpp
  resources.hpp
  credits.hpp
  obj.hpp
  cache.hpp
  piha.hpp
)

set(SRC_LIST
  buffer.cpp
  color.cpp
  credits.cpp
  coordsManip.cpp
  exceptions.cpp
  fetcher.cpp
  mapApi.cpp
  mapUtilities.cpp
  math.cpp
  navigation.cpp
  navigationAltitude.cpp
  obj.cpp
  options.cpp
  renderer.cpp
  auth.cpp
  mapConfig.cpp
  mesh.cpp
  texture.cpp
  resourceOther.cpp
  resourceManager.cpp
  statistics.cpp
  search.cpp
  cache.cpp
  piha.cpp
  log.cpp
  boostProgramOptions.cpp
  sri.cpp
  image.cpp
  ${EXTRA_SRC_LIST}
)

add_library(vts-browser ${LIB_TYPE} ${SRC_LIST} ${HDR_LIST} ${PUB_HDR_LIST})
target_compile_definitions(vts-browser ${LIB_VIS} VTS_BROWSER_BUILD_${LIB_TYPE})
target_link_libraries(vts-browser ${LIB_VIS} ${MODULE_LIBRARIES} ${EXTRA_LIB_DEPS})
buildsys_library(vts-browser)
buildsys_soversion(vts-browser ${vts-browser_SO_VERSION})
buildsys_target_compile_definitions(vts-browser ${MODULE_DEFINITIONS})
buildsys_position_independent(${MODULE_LIBRARIES})


#######################################################################
# Install

if(NOT IOS)

set(INCLUDE_INSTALL_DIR include)
set(LIB_INSTALL_DIR lib)
set(BIN_INSTALL_DIR bin)
set(SHARE_INSTALL_DIR share)
set(VTSBROWSER_LIBRARIES vts-browser)

# headers
install(FILES ${PUB_HDR_LIST}
  DESTINATION ${INCLUDE_INSTALL_DIR}/vts-browser
  COMPONENT browser-dev
  )

# library and binaries
install(TARGETS vts-browser
  EXPORT vts-browser
  RUNTIME DESTINATION ${BIN_INSTALL}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  COMPONENT browser
  )

install(EXPORT vts-browser
  DESTINATION ${SHARE_INSTALL_DIR}/VtsBrowser
  COMPONENT browser-dev
  )

# VtsBrowserConfig.cmake and VtsBrowserVersion.cmake
include(CMakePackageConfigHelpers)

configure_package_config_file(
  VtsBrowserConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/VtsBrowserConfig.cmake
  INSTALL_DESTINATION ${SHARE_INSTALL_DIR}/VtsBrowser
  PATH_VARS INCLUDE_INSTALL_DIR
  )

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/VtsBrowserConfigVersion.cmake
  VERSION ${vts-browser_SO_VERSION}
  COMPATIBILITY SameMajorVersion
  )

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/VtsBrowserConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/VtsBrowserConfigVersion.cmake
  DESTINATION ${SHARE_INSTALL_DIR}/VtsBrowser
  COMPONENT browser-dev
  )

endif()
