<!--
SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
SPDX-License-Identifier: LGPL-2.0-or-later
-->

# Microsoft 365 (Graph) resource

Connects KMail / KOrganizer / KAddressBook to **Microsoft 365 / Exchange Online**
through the **Microsoft Graph API** — mail, calendar and contacts, read and write.

It is the successor to the EWS resource: Microsoft is switching Exchange Web Services
off for Exchange Online (blocked from October 2026). The code structure deliberately
mirrors `resources/ews`.

## Feature overview

- Mail: folder tree, envelopes at sync time, bodies on demand (`/$value`, batched via
  `/$batch`), flags/move/delete/folder management, delta sync per folder.
- Sending: separate transport agent (`akonadi_graphmta_resource`) forwards MIME over
  D-Bus to the master resource; the sent copy is filed server-side without duplicates.
- Special folders (Inbox/Sent/Drafts/Trash/Junk/Outbox) tagged via
  `SpecialCollectionAttribute` — unified-mailbox agent compatible.
- Calendar events (KCalendarCore), contacts (KContacts, including photos and
  categories) and Microsoft To Do tasks, read and write, delta sync; read-only
  calendars (Birthdays, Holidays, …) are exposed read-only.
- OAuth2 authorization code + PKCE (public client), refresh token in QtKeychain,
  silent refresh, proactive renewal; per-user Azure app registration
  (see [docs/azure-setup.md](docs/azure-setup.md)).
- REST layer with 429/503 `Retry-After` back-off and transparent `@odata` paging.

## Known limitations

Deliberate gaps, roughly in the order users are likely to notice them:

- **Global Address List (GAL)** — only the personal contacts folder syncs; looking up
  other people in the organisation (`/users`, `/me/people`) is not implemented (it
  would need an additional OAuth scope and a read-only collection).
- **Shared and delegated mailboxes** — the resource only accesses the signed-in
  user's own mailbox (`/me/…`).
- **Task checklist items** — subtasks inside a Microsoft To Do task (`checklistItems`)
  are not mapped; iCalendar has no direct equivalent. They are preserved on the server
  when a task is edited from KDE.
- **"Nth weekday" recurrences** — rules like *every first Monday*
  (`relativeMonthly`/`relativeYearly`) are read as an approximation (monthly/yearly by
  date) and never written; deliberately left out rather than written wrongly. Note
  also that Microsoft To Do normalises counted repetitions (*10 times*) to
  never-ending server-side; calendar events keep their count.
- **Journal entries** (VJOURNAL) — no Graph equivalent, not synced.
- **Contact birthdays are read-only** — writing them would make Exchange create
  birthday calendar events on its own.
- **Messages copied into the account arrive flagged as draft** — Graph's MIME
  ingestion always creates drafts; the flag currently stays on the copy.
- **No account wizard integration yet** — accounts are added via KMail's
  *Add Custom Account…* plus the configure dialog.

## Users: setting up an account

Each user (or organisation) registers their own — free — Azure application and enters
its client id and tenant in the account dialog. Step-by-step guide:
[docs/azure-setup.md](docs/azure-setup.md).

## Code layout

```text
graphresource.{h,cpp}        Akonadi::ResourceBase — retrieve*/change-replay dispatch
graphmtaresource.{h,cpp}     transport agent, forwards MIME to the master over D-Bus
graphconfig.cpp              account configuration plugin (runs in the client process)
graphsyncstateattribute.*    per-collection @odata.deltaLink storage
graphclient/
  graphclient.{h,cpp}        QNetworkAccessManager + bearer token holder
  graphrequest.{h,cpp}       one REST call: auth + back-off + paging
  auth/graphoauth.{h,cpp}    OAuth2 (auth-code + PKCE), keychain persistence, refresh
mail/graphmailhandler        message  <-> MIME / flags
calendar/grapheventhandler   event    <-> KCalendarCore::Event
contact/graphcontacthandler  contact  <-> KContacts::Addressee
todo/graphtodohandler        task     <-> KCalendarCore::Todo
jobs/                        one KJob per high-level operation
```

`retrieveItems()` and the change-replay handlers dispatch on the collection/item content
MIME type, so a new content type is: one handler, one fetch job, one dispatch branch.

Standalone development (out-of-tree build against installed KDE PIM packages, plus live
test tools that run against a real tenant) happens at
<https://github.com/mzilinski/akonadi-microsoft365>.
