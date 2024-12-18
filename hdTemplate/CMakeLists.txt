cmake_minimum_required(VERSION 3.31)
project(hdTemplate)

set(CMAKE_CXX_STANDARD 17)

get_filename_component(SYCL_COMPILER_NAME ${CMAKE_CXX_COMPILER} NAME_WE)
if (WIN32 AND (SYCL_COMPILER_NAME STREQUAL "icx" OR SYCL_COMPILER_NAME STREQUAL "icpx"))
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /Qoption,link,/DEPENDENTLOADFLAG:0x2000")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /Qoption,link,/DEPENDENTLOADFLAG:0x2000")
elseif (WIN32) #MSVC
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEPENDENTLOADFLAG:0x2000")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEPENDENTLOADFLAG:0x2000")
endif()

# Add a relative target path to the HDTEMPLATE_RPATH.
function(_add_target_to_rpath CURRENT_RPATH target)
  get_target_property(LIBRARY_FILE ${target} LOCATION)

  get_filename_component(LIBRARY_PATH ${LIBRARY_FILE} DIRECTORY)

  file(RELATIVE_PATH
      LIBRARY_PATH_REL
      "${CMAKE_INSTALL_PREFIX}/plugin/usd"
      ${LIBRARY_PATH}   
  )
  set (HDTEMPLATE_RPATH "${CURRENT_RPATH}:$ORIGIN/${LIBRARY_PATH_REL}" PARENT_SCOPE)
endfunction()

# Here is where we add all our used files (cpp, h, json, etc.)
add_library(hdTemplate SHARED
    renderParam.h
    config.cpp
    instancer.cpp
    mesh.cpp
    templateScene.cpp
    renderer.cpp
    renderBuffer.cpp
    renderPass.cpp
    renderDelegate.cpp
    rendererPlugin.cpp
    plugInfo.json
)

set_property(TARGET hdTemplate PROPERTY CXX_STANDARD 17)

target_include_directories(hdTemplate PUBLIC ${PXR_INCLUDE_DIRS})
target_link_libraries(hdTemplate PUBLIC ${PXR_LIBRARIES})

target_compile_definitions(hdTemplate
  PRIVATE
  MFB_PACKAGE_NAME=hdTemplate
  MFB_ALT_PACKAGE_NAME=hdTemplate
  NOMINMAX
  $<$<CXX_COMPILER_ID:MSVC>:/MP /wd4244 /wd4305 /wd4996>
  )


# The plugInfo.json is placed into a resources folder and links the .so from outside the delegate folder.
#
# /plugin/usd
#   hdTemplate.so
#   hdTemplate/    
#     resources/
#         plugInfo.json
#


install(TARGETS hdTemplate DESTINATION plugin/usd)
# plugInfo.json
set(PLUG_INFO_LIBRARY_PATH "../hdTemplate${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(PLUG_INFO_RESOURCE_PATH "resources")
set(PLUG_INFO_ROOT "..")
configure_file(plugInfo.json plugInfo.json @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json" DESTINATION plugin/usd/hdTemplate/resources)

set (HDTEMPLATE_RPATH "$ORIGIN")
set (HDTEMPLATE_RPATH "${HDBREEZE_RPATH}:/../../lib")
set (HDTEMPLATE_RPATH "${HDBREEZE_RPATH}:${pxr_DIR}/lib")

set_target_properties(hdTemplate PROPERTIES
  PREFIX ""
  INSTALL_RPATH ${HDTEMPLATE_RPATH}
)