/*
    SPDX-FileCopyrightText: 2015-2019 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

class QString;
#include <QList>

extern const QString soapEnvNsUri;
extern const QString ewsMsgNsUri;
extern const QString ewsTypeNsUri;

typedef enum {
    EwsFolderTypeMail = 0,
    EwsFolderTypeCalendar,
    EwsFolderTypeContacts,
    EwsFolderTypeSearch,
    EwsFolderTypeTasks,
    EwsFolderTypeUnknown,
} EwsFolderType;

typedef enum {
    EwsResponseSuccess = 0,
    EwsResponseWarning,
    EwsResponseError,
    EwsResponseParseError, // Internal - never returned by an Exchange server
    EwsResponseUnknown // Internal - never returned by an Exchange server
} EwsResponseClass;

typedef enum {
    EwsDIdCalendar = 0,
    EwsDIdContacts,
    EwsDIdDeletedItems,
    EwsDIdDrafts,
    EwsDIdInbox,
    EwsDIdJournal,
    EwsDIdNotes,
    EwsDIdOutbox,
    EwsDIdSentItems,
    EwsDIdTasks,
    EwsDIdMsgFolderRoot,
    EwsDIdRoot,
    EwsDIdJunkEmail,
    EwsDIdSearchFolders,
    EwsDIdVoiceMail,
    EwsDIdRecoverableItemsRoot,
    EwsDIdRecoverableItemsDeletions,
    EwsDIdRecoverableItemsVersions,
    EwsDIdRecoverableItemsPurges,
    EwsDIdArchiveRoot,
    EwsDIdArchiveMsgFolderRoot,
    EwsDIdArchiveDeletedItems,
    EwsDIdArchiveRecoverableItemsRoot,
    EwsDIdArchiveRecoverableItemsDeletions,
    EwsDIdArchiveRecoverableItemsVersions,
    EwsDIdArchiveRecoverableItemsPurges
} EwsDistinguishedId;

typedef enum {
    EwsShapeIdOnly = 0,
    EwsShapeDefault,
    EwsShapeAllProperties
} EwsBaseShape;

typedef enum {
    EwsPropSetMeeting = 0,
    EwsPropSetAppointment,
    EwsPropSetCommon,
    EwsPropSetPublicStrings,
    EwsPropSetAddress,
    EwsPropSetInternetHeaders,
    EwsPropSetCalendarAssistant,
    EwsPropSetUnifiedMessaging
} EwsDistinguishedPropSetId;

typedef enum {
    EwsPropTypeApplicationTime = 0,
    EwsPropTypeApplicationTimeArray,
    EwsPropTypeBinary,
    EwsPropTypeBinaryArray,
    EwsPropTypeBoolean,
    EwsPropTypeCLSID,
    EwsPropTypeCLSIDArray,
    EwsPropTypeCurrency,
    EwsPropTypeCurrencyArray,
    EwsPropTypeDouble,
    EwsPropTypeDoubleArray,
    EwsPropTypeError,
    EwsPropTypeFloat,
    EwsPropTypeFloatArray,
    EwsPropTypeInteger,
    EwsPropTypeTntegerArray,
    EwsPropTypeLong,
    EwsPropTypeLongArray,
    EwsPropTypeNull,
    EwsPropTypeObject,
    EwsPropTypeObjectArray,
    EwsPropTypeShort,
    EwsPropTypeShortArray,
    EwsPropTypeSystemTime,
    EwsPropTypeSystemTimeArray,
    EwsPropTypeString,
    EwsPropTypeStringArray
} EwsPropertyType;

typedef enum {
    EwsTraversalShallow = 0,
    EwsTraversalDeep,
    EwsTraversalSoftDeleted,
    EwsTraversalAssociated,
} EwsTraversalType;

typedef enum {
    EwsItemTypeItem = 0,
    EwsItemTypeMessage,
    EwsItemTypeCalendarItem,
    EwsItemTypeContact,
    EwsItemTypeDistributionList,
    EwsItemTypeMeetingMessage,
    EwsItemTypeMeetingRequest,
    EwsItemTypeMeetingResponse,
    EwsItemTypeMeetingCancellation,
    EwsItemTypeTask,
    EwsItemTypeAbchPerson,
    EwsItemTypePostItem,
    EwsItemTypeUnknown
} EwsItemType;

typedef enum {
    EwsItemSensitivityNormal,
    EwsItemSensitivityPersonal,
    EwsItemSensitivityPrivate,
    EwsItemSensitivityConfidential
} EwsItemSensitivity;

/**
 *  @brief List of fields in EWS Item and its descendants
 *
 *  The list is based on the XSD schema and contains duplicates, which were commented out.
 */
typedef enum {
    EwsItemFieldInvalid = -1,

    // Folder
    EwsFolderFieldFolderId,
    EwsFolderFieldParentFolderId,
    EwsFolderFieldFolderClass,
    EwsFolderFieldDisplayName,
    EwsFolderFieldTotalCount,
    EwsFolderFieldChildFolderCount,
    EwsFolderFieldManagedFolderInformation,
    EwsFolderFieldEffectiveRights,
    // Calendar folder
    EwsFolderFieldPermissionSet,
    // Contacts folder
    // EwsFolderFieldPermissionSet,          DUPLICATE
    // Mail folder
    EwsFolderFieldUnreadCount,
    // EwsFolderFieldPermissionSet,          DUPLICATE
    // Search folder
    // EwsFolderFieldUnreadCount,            DUPLICATE
    EwsFolderFieldSearchParameters,
    // Tasks folder
    // EwsFolderFieldUnreadCount,            DUPLICATE

    // Item
    EwsItemFieldMimeContent,
    EwsItemFieldItemId,
    EwsItemFieldParentFolderId,
    EwsItemFieldItemClass,
    EwsItemFieldSubject,
    EwsItemFieldSensitivity,
    EwsItemFieldBody,
    EwsItemFieldAttachments,
    EwsItemFieldDateTimeReceived,
    EwsItemFieldSize,
    EwsItemFieldCategories,
    EwsItemFieldImportance,
    EwsItemFieldInReplyTo,
    EwsItemFieldIsSubmitted,
    EwsItemFieldIsDraft,
    EwsItemFieldIsFromMe,
    EwsItemFieldIsResend,
    EwsItemFieldIsUnmodified,
    EwsItemFieldInternetMessageHeaders,
    EwsItemFieldDateTimeSent,
    EwsItemFieldDateTimeCreated,
    EwsItemFieldResponseObjects,
    EwsItemFieldReminderDueBy,
    EwsItemFieldReminderIsSet,
    EwsItemFieldReminderMinutesBeforeStart,
    EwsItemFieldDisplayCc,
    EwsItemFieldDisplayTo,
    EwsItemFieldHasAttachments,
    EwsItemFieldCulture,
    EwsItemFieldEffectiveRights,
    EwsItemFieldLastModifiedName,
    EwsItemFieldLastModifiedTime,
    EwsItemFieldIsAssociated,
    EwsItemFieldWebClientReadFormQueryString,
    EwsItemFieldWebClientEditFormQueryString,
    EwsItemFieldConversationId,
    EwsItemFieldUniqueBody,
    EwsItemFieldFlag,
    EwsItemFieldStoreEntryId,
    EwsItemFieldInstanceKey,
    EwsItemFieldNormalizedBody,
    EwsItemFieldEntityExtractionResult,
    EwsItemFieldPolicyTag,
    EwsItemFieldArchiveTag,
    EwsItemFieldRetentionDate,
    EwsItemFieldPreview,
    EwsItemFieldRightsManagementLicenseData,
    EwsItemFieldPredictedActionReasons,
    EwsItemFieldIsClutter,
    EwsItemFieldBlockStatus,
    EwsItemFieldHasBlockedImages,
    EwsItemFieldTextBody,
    EwsItemFieldIconIndex,
    EwsItemFieldSearchKey,
    EwsItemFieldSortKey,
    EwsItemFieldHashtags,
    EwsItemFieldMentions,
    EwsItemFieldMentionedMe,
    EwsItemFieldMentionsPreview,
    EwsItemFieldMentionsEx,
    EwsItemFieldAppliedHashtags,
    EwsItemFieldAppliedHashtagsPreview,
    EwsItemFieldLikes,
    EwsItemFieldLikesPreview,
    EwsItemFieldPendingSocialActivityTagIds,
    EwsItemFieldAtAllMention,
    EwsItemFieldCanDelete,
    EwsItemFieldInferenceClassification,
    // Message
    EwsItemFieldSender,
    EwsItemFieldToRecipients,
    EwsItemFieldCcRecipients,
    EwsItemFieldBccRecipients,
    EwsItemFieldIsReadReceiptRequested,
    EwsItemFieldIsDeliveryReceiptRequested,
    EwsItemFieldConversationIndex,
    EwsItemFieldConversationTopic,
    EwsItemFieldFrom,
    EwsItemFieldInternetMessageId,
    EwsItemFieldIsRead,
    EwsItemFieldIsResponseRequested,
    EwsItemFieldReferences,
    EwsItemFieldReplyTo,
    EwsItemFieldReceivedBy,
    EwsItemFieldReceivedRepresenting,
    // Task
    EwsItemFieldActualWork,
    EwsItemFieldAssignedTime,
    EwsItemFieldBillingInformation,
    EwsItemFieldChangeCount,
    EwsItemFieldCompanies,
    EwsItemFieldCompleteDate,
    EwsItemFieldContacts,
    EwsItemFieldDelegationState,
    EwsItemFieldDelegator,
    EwsItemFieldDueDate,
    EwsItemFieldIsAssignmentEditable,
    EwsItemFieldIsComplete,
    EwsItemFieldIsRecurring,
    EwsItemFieldIsTeamTask,
    EwsItemFieldMileage,
    EwsItemFieldOwner,
    EwsItemFieldPercentComplete,
    EwsItemFieldRecurrence,
    EwsItemFieldStartDate,
    EwsItemFieldStatus,
    EwsItemFieldStatusDescription,
    EwsItemFieldTotalWork,
    // Calendar
    EwsItemFieldUID,
    EwsItemFieldRecurrenceId,
    EwsItemFieldDateTimeStamp,
    EwsItemFieldStart,
    EwsItemFieldEnd,
    EwsItemFieldOriginalStart,
    EwsItemFieldIsAllDayEvent,
    EwsItemFieldLegacyFreeBusyStatus,
    EwsItemFieldLocation,
    EwsItemFieldWhen,
    EwsItemFieldIsMeeting,
    EwsItemFieldIsCancelled,
    // EwsItemFieldIsRecurring,              DUPLICATE
    EwsItemFieldMeetingRequestWasSent,
    // EwsItemFieldIsResponseRequested,      DUPLICATE
    EwsItemFieldCalendarItemType,
    EwsItemFieldMyResponseType,
    EwsItemFieldOrganizer,
    EwsItemFieldRequiredAttendees,
    EwsItemFieldOptionalAttendees,
    EwsItemFieldResources,
    EwsItemFieldConflictingMeetingCount,
    EwsItemFieldAdjacentMeetingCount,
    EwsItemFieldConflictingMeetings,
    EwsItemFieldAdjacentMeetings,
    EwsItemFieldDuration,
    EwsItemFieldTimeZone,
    EwsItemFieldStartTimeZone,
    EwsItemFieldEndTimeZone,
    EwsItemFieldAppointmentReplyTime,
    EwsItemFieldAppointmentSequenceNumber,
    EwsItemFieldAppointmentState,
    // EwsItemFieldRecurrence,               DUPLICATE
    EwsItemFieldFirstOccurrence,
    EwsItemFieldLastOccurrence,
    EwsItemFieldModifiedOccurrences,
    EwsItemFieldDeletedOccurrences,
    EwsItemFieldMeetingTimeZone,
    EwsItemFieldConferenceType,
    EwsItemFieldAllowNewTimeProposal,
    EwsItemFieldIsOnlineMeeting,
    EwsItemFieldMeetingWorkspaceUrl,
    EwsItemFieldNetShowUrl,
    EwsItemFieldEnhancedLocation,
    EwsItemFieldStartWallClock,
    EwsItemFieldEndWallClock,
    EwsItemFieldStartTimeZoneId,
    EwsItemFieldEndTimeZoneId,
    EwsItemFieldIntendedFreeBusyStatus,
    EwsItemFieldJoinOnlineMeetingUrl,
    EwsItemFieldOnlineMeetingSettings,
    EwsItemFieldIsOrganizer,
    EwsItemFieldCalendarActivityData,
    EwsItemFieldDoNotForwardMeeting,
    // MeetingMessage
    EwsItemFieldAssociatedCalendarItemId,
    EwsItemFieldIsDelegated,
    EwsItemFieldIsOutOfDate,
    EwsItemFieldHasBeenProcessed,
    EwsItemFieldResponseType,
    // EwsItemFieldUID,                      DUPLICATE
    // EwsItemFieldRecurrenceId,             DUPLICATE
    // EwsItemFieldDateTimeStamp,            DUPLICATE
    // MeetingRequestMessage
    EwsItemFieldMeetingRequestType,
    // EwsItemFieldIntendedFreeBusyStatus,   DUPLICATE
    // EwsItemFieldStart,                    DUPLICATE
    // EwsItemFieldEnd,                      DUPLICATE
    // EwsItemFieldOriginalStart,            DUPLICATE
    // EwsItemFieldIsAllDayEvent,            DUPLICATE
    // EwsItemFieldLegacyFreeBusyStatus,     DUPLICATE
    // EwsItemFieldLocation,                 DUPLICATE
    // EwsItemFieldWhen,                     DUPLICATE
    // EwsItemFieldIsMeeting,                DUPLICATE
    // EwsItemFieldIsCancelled,              DUPLICATE
    // EwsItemFieldIsRecurring,              DUPLICATE
    // EwsItemFieldMeetingRequestWasSent,    DUPLICATE
    // EwsItemFieldCalendarItemType,         DUPLICATE
    // EwsItemFieldMyResponseType,           DUPLICATE
    // EwsItemFieldOrganizer,                DUPLICATE
    // EwsItemFieldRequiredAttendees,        DUPLICATE
    // EwsItemFieldOptionalAttendees,        DUPLICATE
    // EwsItemFieldResources,                DUPLICATE
    // EwsItemFieldConflictingMeetingCount,  DUPLICATE
    // EwsItemFieldAdjacentMeetingCount,     DUPLICATE
    // EwsItemFieldConflictingMeetings,      DUPLICATE
    // EwsItemFieldAdjacentMeetings,         DUPLICATE
    // EwsItemFieldDuration,                 DUPLICATE
    // EwsItemFieldTimeZone,                 DUPLICATE
    // EwsItemFieldAppointmentReplyTime,     DUPLICATE
    // EwsItemFieldAppointmentSequenceNumber,DUPLICATE
    // EwsItemFieldAppointmentState,         DUPLICATE
    // EwsItemFieldRecurrence,               DUPLICATE
    // EwsItemFieldFirstOccurrence,          DUPLICATE
    // EwsItemFieldLastOccurrence,           DUPLICATE
    // EwsItemFieldModifiedOccurrences,      DUPLICATE
    // EwsItemFieldDeletedOccurrences,       DUPLICATE
    // EwsItemFieldMeetingTimeZone,          DUPLICATE
    // EwsItemFieldConferenceType,           DUPLICATE
    // EwsItemFieldAllowNewTimeProposal,     DUPLICATE
    // EwsItemFieldIsOnlineMeeting,          DUPLICATE
    // EwsItemFieldMeetingWorkspaceUrl,      DUPLICATE
    // EwsItemFieldNetShowUrl,               DUPLICATE
    // Contact
    EwsItemFieldFileAs,
    EwsItemFieldFileAsMapping,
    EwsItemFieldDisplayName,
    EwsItemFieldGivenName,
    EwsItemFieldInitials,
    EwsItemFieldMiddleName,
    EwsItemFieldNickname,
    EwsItemFieldCompleteName,
    EwsItemFieldCompanyName,
    EwsItemFieldEmailAddresses,
    EwsItemFieldPhysicalAddresses,
    EwsItemFieldPhoneNumbers,
    EwsItemFieldAssistantName,
    EwsItemFieldBirthday,
    EwsItemFieldBusinessHomePage,
    EwsItemFieldChildren,
    // EwsItemFieldCompanies,                DUPLICATE
    EwsItemFieldContactSource,
    EwsItemFieldDepartment,
    EwsItemFieldGeneration,
    EwsItemFieldImAddresses,
    EwsItemFieldJobTitle,
    EwsItemFieldManager,
    // EwsItemFieldMileage,                  DUPLICATE
    EwsItemFieldOfficeLocation,
    EwsItemFieldPostalAddressIndex,
    EwsItemFieldProfession,
    EwsItemFieldSpouseName,
    EwsItemFieldSurname,
    EwsItemFieldWeddingAnniversary,
    // DistributionList
    // EwsItemFieldDisplayName,              DUPLICATE
    // EwsItemFieldFileAs,                   DUPLICATE
    // EwsItemFieldContactSource,            DUPLICATE
    // Additional fields not in EWS specification
    EwsItemFieldBodyIsHtml,
    EwsItemFieldExtendedProperties,
    EwsItemFieldExchangePersonIdGuid,
} EwsItemFields;

typedef enum {
    EwsItemImportanceLow,
    EwsItemImportanceNormal,
    EwsItemImportanceHigh
} EwsItemImportance;

typedef enum {
    EwsBasePointBeginning,
    EwsBasePointEnd
} EwsIndexedViewBasePoint;

typedef enum {
    EwsCalendarItemSingle = 0,
    EwsCalendarItemOccurrence,
    EwsCalendarItemException,
    EwsCalendarItemRecurringMaster
} EwsCalendarItemType;

typedef enum {
    EwsEventResponseUnknown = 0,
    EwsEventResponseOrganizer,
    EwsEventResponseTentative,
    EwsEventResponseAccept,
    EwsEventResponseDecline,
    EwsEventResponseNotReceived
} EwsEventResponseType;

typedef enum {
    EwsLfbStatusFree = 0,
    EwsLfbStatusTentative,
    EwsLfbStatusBusy,
    EwsLfbOutOfOffice,
    EwsLfbNoData
} EwsLegacyFreeBusyStatus;

typedef enum {
    EwsDispSaveOnly = 0,
    EwsDispSendOnly,
    EwsDispSendAndSaveCopy,
} EwsMessageDisposition;

typedef enum {
    EwsResolNeverOverwrite = 0,
    EwsResolAutoResolve,
    EwsResolAlwaysOverwrite,
} EwsConflictResolution;

typedef enum {
    EwsMeetingDispSendToNone = 0,
    EwsMeetingDispSendOnlyToAll,
    EwsMeetingDispSendOnlyToChanged,
    EwsMeetingDispSendToAllAndSaveCopy,
    EwsMeetingDispSendToChangedAndSaveCopy,
    EwsMeetingDispUnspecified
} EwsMeetingDisposition;

typedef enum {
    EwsCopiedEvent = 0,
    EwsCreatedEvent,
    EwsDeletedEvent,
    EwsModifiedEvent,
    EwsMovedEvent,
    EwsNewMailEvent,
    EwsFreeBusyChangedEvent,
    EwsStatusEvent,
    EwsUnknownEvent
} EwsEventType;

typedef enum {
    EwsResponseCodeNoError = 0,
    EwsResponseCodeErrorServerBusy,
    EwsResponseCodeUnauthorized,
    EwsResponseCodeUnknown,
} EwsResponseCode;

template<typename T, std::size_t SIZE>
T decodeEnumString(const QString &str, const std::array<QLatin1StringView, SIZE> &table, bool *ok)
{
    unsigned i = 0;
    for (const auto &value : table) {
        if (str == value) {
            *ok = true;
            return static_cast<T>(i);
        }
        i++;
    }
    *ok = false;
    return T();
}

inline bool isEwsMessageItemType(EwsItemType type)
{
    return (type == EwsItemTypeItem) || (type == EwsItemTypePostItem);
}

extern const QList<QString> ewsItemTypeNames;

EwsResponseCode decodeEwsResponseCode(const QString &code);
bool isEwsResponseCodeTemporaryError(EwsResponseCode code);
