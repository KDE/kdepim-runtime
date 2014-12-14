
add_subdirectory( autotests )

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_maildir_resource\")

set(maildir_LIB_SRCS keycache.cpp maildir.cpp libmaildir_debug.cpp)

add_library(maildir ${LIBRARY_TYPE} ${maildir_LIB_SRCS})
generate_export_header(maildir BASE_NAME maildir)

target_link_libraries(maildir KF5::Codecs KF5::AkonadiMime KF5::AkonadiCore)

set_target_properties(maildir PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )

install(TARGETS maildir ${INSTALL_TARGETS_DEFAULT_ARGS})
