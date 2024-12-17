cmake_minimum_required(VERSION 3.31)
project(hdTemplatePlugin)

set(CMAKE_CXX_STANDARD 17)

find_package(X11 REQUIRED)
find_package(pxr CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(hdTemplate)


# In cases where you have multiple hdDelegates, we use a plugInfo.json with "Includes": {"*/resources"} to get the plugInfo for each of the delegates.
#  root/
#   plugInfo.json -> to */resources
#   hdDelegateA.so
#   hdDelegateB.so
#   hdDelegateA/
#       resources/
#           plugInfo.json
#   hdDelegateB/
#       resources/
#           plugInfo.json
#

install(FILES plugInfo.json DESTINATION plugin/usd)
