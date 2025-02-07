if (WIN32)
  # We should try detaching ServerState from the inodes
  # so that we don't need to make the Windows code depend
  # on it.
  add_library(
    eden_inodes STATIC
      ServerState.cpp
  )
  target_link_libraries(
    eden_inodes
    PUBLIC
      eden_journal
      Folly::folly
  )
else()
  file(GLOB INODES_SRCS "*.cpp")
  add_library(
    eden_inodes STATIC
      ${INODES_SRCS}
  )
  target_link_libraries(
    eden_inodes
    PUBLIC
      eden_overlay_thrift
      eden_overlay
      eden_fuse
      eden_journal
      eden_store
      eden_config
      eden_utils
      eden_service
      eden_takeover_thrift
      eden_tracing
  )

  add_subdirectory(overlay)
endif()
