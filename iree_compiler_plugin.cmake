set(IREE_DUMMY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")

message(STATUS "Loaded Dummy compiler plugin: ${CMAKE_CURRENT_LIST_FILE}")

add_subdirectory(
  "${CMAKE_CURRENT_LIST_DIR}/compiler/plugins/target/Dummy"
  target/Dummy)
