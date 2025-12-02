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
