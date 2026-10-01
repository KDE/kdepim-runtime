if(KUnifiedPush_FOUND AND DAV_ENABLE_PUSH_NOTIFICATIONS)
    add_subdirectory(pushbridge)
endif()
add_subdirectory(resource)
