if(WIN32)
  set(PLATFORM "Windows")
else()
  set(PLATFORM "Linux")
endif()

MESSAGE(${PLATFORM} " DETECTED")