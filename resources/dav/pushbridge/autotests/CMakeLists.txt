# SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>
# SPDX-License-Identifier: BSD-2-Clause

include(ECMAddTests)

ecm_add_test(pushnotificationmessageparsertest.cpp ../pushnotificationmessageparser.cpp TEST_NAME pushnotificationmessageparsertest LINK_LIBRARIES Qt::Test Qt::Core)
target_compile_definitions(pushnotificationmessageparsertest PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
