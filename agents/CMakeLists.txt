add_subdirectory(maildispatcher)
add_subdirectory(newmailnotifier)
# For the moment disable it as we don't have KUiServerJobTracker support on windows it seems
if(NOT WIN32)
    add_subdirectory(migration)
endif()

