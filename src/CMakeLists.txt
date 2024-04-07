# Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

if(WIN32)
  set(sources_windows
    ${CMAKE_SOURCE_DIR}/src/common/Debugging/WheatyExceptionReport.cpp
    ${CMAKE_SOURCE_DIR}/src/common/Debugging/WheatyExceptionReport.h
  )
endif(WIN32)
  
add_subdirectory(genrev)
add_subdirectory(common)

if(SERVERS)
  add_subdirectory(server)
endif(SERVERS)

if(TOOLS)
  add_subdirectory(tools)
endif(TOOLS)

# Voice Chat server and handlers
if(VOICECHAT)
  include(FetchContent)

  FetchContent_Declare(
    voicechat-server
    GIT_REPOSITORY "https://github.com/celguar/voicechat-server.git"
  GIT_TAG "main"
  )

  if(NOT voicechat-server_POPULATED)
  message(STATUS "Fetching VoiceChat Server...")

    FetchContent_Populate(voicechat-server)

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/voicechat-server)
      file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/voicechat-server)
  endif()

  file(COPY ${voicechat-server_SOURCE_DIR}/. DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/voicechat-server)
    message(STATUS "VoiceChat Server fetched and populated in ${CMAKE_CURRENT_SOURCE_DIR}/voicechat-server")
  endif()

  add_subdirectory(voicechat-server)
else()
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/voicechat-server)
    file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/voicechat-server)
  endif()
endif()

