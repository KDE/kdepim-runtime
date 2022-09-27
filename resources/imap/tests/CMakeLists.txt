kde_enable_exceptions()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

set(testsubscriptiondialog_SRCS
  testsubscriptiondialog.cpp
  ../imapaccount.cpp
  ../subscriptiondialog.cpp
  ../imapresource_debug.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)
add_executable(testsubscriptiondialog ${testsubscriptiondialog_SRCS})

target_link_libraries(testsubscriptiondialog KF5::KIOCore KF5::IMAP KF5::Mime KF5::ItemModels KF5::ConfigGui Qt::Widgets KF5::I18n KF5::KIOWidgets KF5::AkonadiCore KF5::MailTransport)

