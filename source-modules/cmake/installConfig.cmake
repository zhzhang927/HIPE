include(GetPrerequisites)
#installConfig
if("${CMAKE_INSTALL_CONFIG_NAME}" STREQUAL "Debug") 
	set(BUILD_CONFIG "Debug")
else()
	set(BUILD_CONFIG "Release")
endif()

include("${PROJECT_SOURCE_DIR}/../../cmake/getDependencies.cmake")


message(STATUS "Find shared library dependencies...")
hipe_find_dependencies(list_deps ${target_name} ${BUILD_CONFIG})
message(STATUS "All libraries deps found are : ${list_deps}")

if (WIN32)
  SET(LIB_DIR "bin")
else()
  SET(LIB_DIR "lib")
endif()

foreach(filename ${list_deps})
	message(STATUS "Copy file ${filename}  ---> ${CMAKE_INSTALL_PREFIX}/${LIB_DIR}/${BUILD_CONFIG}")
	FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/${LIB_DIR}/${BUILD_CONFIG}" TYPE FILE FILES "${filename}" )
endforeach()


