# Privacy policy: Desk Assistant

Desk Assistant is a personal, open-source desk display project. This page explains what happens to Google Calendar data when the project's bridge service is linked to a Google account.

## What the app accesses

With the account owner's permission it requests one read-only scope, `https://www.googleapis.com/auth/calendar.readonly`. It reads the titles and times of the events on the account's primary calendar for the current day.

## How the data is used

- The events are fetched by the bridge, a small program the account owner runs on their own home server.
- A reduced copy (event title and start and end time) is sent over the owner's home network to the owner's own display device and shown on its screen.
- The data is used only for that display. It is not used for advertising, analytics, profiling or machine learning.

## What is stored

- The bridge keeps an OAuth refresh token on the owner's own server so it can keep reading the calendar. It is never published or committed to the source code.
- Today's event list is held in memory for a few minutes as a cache. Nothing else from the calendar is written to disk.

## Sharing

No calendar data is sent to the developer or to any third party. There is no server run by the developer that receives it.

## Revoking access

The account owner can remove the app's access at any time at https://myaccount.google.com/permissions and can delete the stored token from their own server.

## Contact

Open an issue at https://github.com/Purushotam4-25/desk-assistant/issues.
