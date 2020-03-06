set(newmailnotifieragent_common_SRCS)
if (ECM_VERSION VERSION_LESS "5.68.0")
    ecm_qt_declare_logging_category(newmailnotifieragent_common_SRCS HEADER newmailnotifier_debug.h IDENTIFIER NEWMAILNOTIFIER_LOG CATEGORY_NAME org.kde.pim.newmailnotifier)
else()
    ecm_qt_declare_logging_category(newmailnotifieragent_common_SRCS HEADER newmailnotifier_debug.h IDENTIFIER NEWMAILNOTIFIER_LOG CATEGORY_NAME org.kde.pim.newmailnotifier
        DESCRIPTION "mailnotifier agent (kdepim-runtime)"
        OLD_CATEGORY_NAMES log_newmailnotifier
        EXPORT KDEPIMRUNTIME
    )
endif()



set(newmailnotifierselectcollectionwidget_SRCS newmailnotifierselectcollectionwidget_test.cpp ../newmailnotifierselectcollectionwidget.cpp ${newmailnotifieragent_common_SRCS})
add_executable(newmailnotifierselectcollectionwidgettest ${newmailnotifierselectcollectionwidget_SRCS})
target_link_libraries(newmailnotifierselectcollectionwidgettest
    Qt5::Widgets
    KF5::I18n
    KF5::AkonadiCore
    KF5::Mime
    KF5::Completion
    KF5::AkonadiMime)
