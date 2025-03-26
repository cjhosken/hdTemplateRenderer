cmake_minimum_required(VERSION 3.26)
project(hdTemplatePlugin)

set(CMAKE_CXX_STANDARD 17)

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
