macro (add directory)
	file(GLOB_RECURSE tempH "../${directory}/*.h")
	file(GLOB_RECURSE tempCPP "../${directory}/*.c*")
	file(GLOB_RECURSE tempINC "../${directory}/*.inc")
	list(APPEND BackportSources ${tempH})
	list(APPEND BackportSources ${tempCPP})
	list(APPEND BackportSources ${tempINC})
	SOURCE_GROUP("Header Files\\${directory}" FILES ${tempH})
	SOURCE_GROUP("Source Files\\${directory}" FILES ${tempCPP})
	SOURCE_GROUP("Source Files\\${directory}" FILES ${tempINC})
endmacro (add)


set(LLVM_LINK_COMPONENTS support)

set (BackportSources
  backport.cpp
  )

# Backport manager
add("BackportManager")
# Database 
add("Database")
# Transforms
add("Transforms")
# TransformBase
add("TransformBase")
# Util
add("Util")
# Serialize
add("Serialize")

add_subdirectory(../Process ${CMAKE_CURRENT_BINARY_DIR}/Process-build)

add_clang_executable(backport
  ${BackportSources}
  )

add_dependencies(backport
  clang-headers clang-apply-replacements
  )

target_link_libraries(backport
  clangAST
  clangASTMatchers
  clangBasic
  clangFormat
  clangFrontend
  clangLex
  clangTooling
  clangToolingCore
  modernizeCore
  Process
  )

install(TARGETS backport
  RUNTIME DESTINATION bin)
