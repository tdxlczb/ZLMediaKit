set(TARGET_NAME zlmplayer)

file(GLOB_RECURSE ${TARGET_NAME}_SOURCES FOLLOW_SYMLINKS LIST_DIRECTORIES false CONFIGURE_DEPENDS
     *.h
     *.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${${TARGET_NAME}_SOURCES})

add_library(${TARGET_NAME} SHARED)
target_sources(${TARGET_NAME} PRIVATE ${${TARGET_NAME}_SOURCES})

target_compile_options(${TARGET_NAME}
  PRIVATE ${COMPILE_OPTIONS_DEFAULT})

target_compile_definitions(${TARGET_NAME}
  PRIVATE ${MK_COMPILE_DEFINITIONS})

set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${TARGET_NAME}$<$<CONFIG:Debug>:d>)

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(${TARGET_NAME} -Wl,--start-group ${MK_LINK_LIBRARIES} -Wl,--end-group)
else()
  target_link_libraries(${TARGET_NAME} ${MK_LINK_LIBRARIES})
endif()

add_definitions("-D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING")

#relase 类型时额外输出debug调试信息
string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)
if(UNIX AND ENABLE_OBJCOPY)
  if("${CMAKE_BUILD_TYPE_LOWER}" STREQUAL "release")
    find_program(OBJCOPY_FOUND objcopy)
    if (OBJCOPY_FOUND)
      add_custom_command(TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND objcopy --only-keep-debug ${EXECUTABLE_OUTPUT_PATH}/libzlmplayer.so ${EXECUTABLE_OUTPUT_PATH}/libzlmplayer.so.debug
        COMMAND objcopy --strip-all ${EXECUTABLE_OUTPUT_PATH}/libzlmplayer.so
        COMMAND objcopy --add-gnu-debuglink=${EXECUTABLE_OUTPUT_PATH}/libzlmplayer.so.debug ${EXECUTABLE_OUTPUT_PATH}/libzlmplayer.so
      )
      install(FILES ${EXECUTABLE_OUTPUT_PATH}/libzlmplayer.so.debug DESTINATION ${INSTALL_PATH_RUNTIME})
    else()
      message(STATUS "not found objcopy, generate libzlmplayer.so.debug skip")
    endif()
  endif()
endif()