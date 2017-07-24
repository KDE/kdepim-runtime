include(ECMMarkAsTest)

find_package(Qt5Test CONFIG REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)


qt5_add_dbus_interface(extra_SRC
    ${CMAKE_CURRENT_BINARY_DIR}/../org.kde.Akonadi.Maildir.Settings.xml maildirsettings Akonadi_Maildir_Resource::MaildirSettings maildirsettingsinterface MaildirSettingsInterface
)

add_akonadi_isolated_test_advanced(synctest.cpp "${extra_SRC}" "")

