
if (BUILD_TESTING)
    add_subdirectory( autotests )
endif()

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_maildir_resource\")

set(maildir_LIB_SRCS keycache.cpp maildir.cpp)
ecm_qt_declare_logging_category(maildir_LIB_SRCS HEADER libmaildir_debug.h IDENTIFIER LIBMAILDIR_LOG CATEGORY_NAME org.kde.pim.libmaildir
    DESCRIPTION "libmaildir (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_libmaildir
    EXPORT KDEPIMRUNTIME
    )



add_library(maildir  ${maildir_LIB_SRCS})
generate_export_header(maildir BASE_NAME maildir)

target_link_libraries(maildir
    PUBLIC
    KF5::AkonadiMime
    PRIVATE
    KF5::I18n
    Qt::Network
    )

set_target_properties(maildir PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )

install(TARGETS maildir ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
