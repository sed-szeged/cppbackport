include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../clang-modernize/
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${ClangReplaceLocation}
  )

add_subdirectory(tool)
