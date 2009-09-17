FIND_PATH(STICKYNOTES_INCLUDE_DIR StickyNotes
    ${CMAKE_INSTALL_PREFIX}/include
    /usr/local/include
    /usr/include)

FIND_LIBRARY(STICKYNOTES_LIBRARY NAMES stickynotes PATH
    ${CMAKE_INSTALL_PREFIX}/lib
    /usr/local/lib
    /usr/lib)

FIND_LIBRARY(STICKYNOTES_STANDALONE_LIBRARY NAMES stickynotes-standalone PATH
    ${CMAKE_INSTALL_PREFIX}/lib
    /usr/local/lib
    /usr/lib)

IF (STICKYNOTES_INCLUDE_DIR AND STICKYNOTES_LIBRARY)
   SET(STICKYNOTES_FOUND TRUE)
ENDIF (STICKYNOTES_INCLUDE_DIR AND STICKYNOTES_LIBRARY)

IF (STICKYNOTES_FOUND)
   IF (NOT StickyNotes_FIND_QUIETLY)
      MESSAGE(STATUS "Found StickyNotes: ${STICKYNOTES_LIBRARY}")
   ENDIF (NOT StickyNotes_FIND_QUIETLY)
ELSE (STICKYNOTES_FOUND)
   IF (StickyNotes_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find StickyNotes")
   ENDIF (StickyNotes_FIND_REQUIRED)
ENDIF (STICKYNOTES_FOUND)

