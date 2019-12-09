set(TARGETNAME EmpowerAgentProto)
set(DIRNAME empoweragentproto)

add_library(${TARGETNAME}
  utils.cpp
  buffers.cpp
  protocol.cpp
  io.cpp
  tlvencoding.cpp
  tlvs.cpp)


target_include_directories (${TARGETNAME}
  PUBLIC
  $<BUILD_INTERFACE:${EMPOWER_ENB_AGENT2_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include/${DIRNAME}>)

file(GLOB HEADERS
  LIST_DIRECTORIES false
  ../../include/${DIRNAME}/*.hh)

set_target_properties(${TARGETNAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS ${TARGETNAME}
  EXPORT ${TARGETNAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  PUBLIC_HEADER DESTINATION include/${DIRNAME})

install(FILES ${TARGETNAME}Config.cmake DESTINATION lib)
install(EXPORT ${TARGETNAME}
  DESTINATION lib)



