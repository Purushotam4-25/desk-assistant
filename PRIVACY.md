# Privacy policy: Desk Assistant

Last updated: 2026-11-09

Desk Assistant is a personal desk display project; its source code is published at https://github.com/Purushotam4-25/desk-assistant. This page explains what happens to Google Calendar data when the project's bridge service is linked to a Google account. The service is meant to be run by its owner for their own use, on their own hardware. It is not a hosted service and has no user accounts.

## What the app accesses

With the account owner's permission it requests one read-only scope, `https://www.googleapis.com/auth/calendar.readonly`. For the primary calendar and the current day only, it reads each event's title, start and end time, whether it is cancelled or all-day, and whether the account owner has declined it (so declined events can be hidden). It cannot create, change or delete anything.

## How the data is used

- The events are fetched by the bridge, a small program the account owner runs on their own home server.
- A reduced copy (event title and start and end time) is sent to the owner's own display device over the owner's home network and shown on its screen. This local traffic is plain HTTP, not encrypted, and is not reachable from the internet unless the owner chooses to expose it.
- The data is used only to show the day's schedule on that display. It is not used for advertising, analytics, profiling, or training machine learning models, and it is not sold.

## What is stored

- The bridge keeps the OAuth client ID and secret and a refresh token in one file on the owner's own server, so it can keep reading the calendar without asking again. This file is never part of the published source code.
- Today's event list is held in the bridge's memory for a few minutes as a cache, and in the display device's memory while it is shown. It is not saved to disk or to any other service, and it is discarded when the program restarts or the day changes.

## Sharing

No calendar data is sent to the developer or to any third party. There is no server run by the developer that receives it. The only calendar traffic that leaves the owner's home network is the owner's server talking to Google's own API; the rest stays between the owner's server and display device.

## Google API Services User Data Policy

Desk Assistant's use and transfer to any other app of information received from Google APIs will adhere to the [Google API Services User Data Policy](https://developers.google.com/terms/api-services-user-data-policy), including the Limited Use requirements.

## Revoking access and deleting data

The account owner can remove the app's access at any time at https://myaccount.google.com/permissions. Deleting the stored token file from their own server removes the saved credentials, and restarting the bridge clears the cached events.

## Contact

Open an issue at https://github.com/Purushotam4-25/desk-assistant/issues.
