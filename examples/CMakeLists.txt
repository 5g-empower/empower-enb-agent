add_executable(simpleError simpleError.cpp)
target_link_libraries (simpleError LINK_PUBLIC ${EMPOWER_ENB_AGENT2_LIBS})

add_executable(agentserv agentserv.cpp)
target_link_libraries (agentserv LINK_PUBLIC ${EMPOWER_ENB_AGENT2_LIBS})


add_executable(agentclient agentclient.cpp)
target_link_libraries (agentclient LINK_PUBLIC ${EMPOWER_ENB_AGENT2_LIBS})

