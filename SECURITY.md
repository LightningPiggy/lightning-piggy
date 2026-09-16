# Security Policy

## Reporting a vulnerability

Please report security issues privately by email to **oink@lightningpiggy.com**.
Do not open a public GitHub issue for anything you believe is a security
vulnerability.

Include what you can of the following:

- a description of the issue and its impact
- the firmware version (shown on the display at boot and in the web UI) and
  the board it was observed on
- steps to reproduce, or a proof of concept
- whether the issue is already public

You will get an acknowledgement within 7 days. We will keep you informed of
progress and agree a disclosure date with you once a fix is available. Credit
is given in the release notes unless you prefer otherwise.

## Scope

This repository contains the Lightning Piggy Classic firmware for the TTGO
LilyGo ePaper boards (ESP32, Arduino). It is a display-only piggy bank: it
shows a balance, recent payments and a receive code from an externally
managed wallet (LNbits or Nostr Wallet Connect). It holds no spending keys and
cannot move funds.

In scope:

- handling of the credentials stored on the device (LNbits keys, NWC
  connection strings, WiFi passwords) in flash and in the web configuration UI
- the configuration web UI itself (authentication, exposure on the network,
  input handling)
- network handling (LNbits, Nostr relays) including TLS use and parsing of
  remote data
- anything that could make the display show incorrect balances or payments
- the web installer at https://lightningpiggy.github.io (firmware images and
  manifests)

Out of scope, but please report them to the right place:

- the MicroPythonOS-based Lightning Piggy app:
  https://github.com/LightningPiggy/LightningPiggyApp
- MicroPythonOS itself: https://github.com/MicroPythonOS/MicroPythonOS
- LNbits, Nostr relays or wallet services the firmware talks to

## Supported versions

Security fixes are made on the latest release published on GitHub Releases and
the web installer. Older releases are not maintained; please update to the
latest version.

## Practical notes for users

- An NWC connection string or LNbits key lets its holder see your balance and
  transactions; treat them like passwords, and revoke them from your wallet
  service if a device is lost.
- Prefer read-only credentials (an LNbits invoice/read key, a read-only NWC
  connection) for a piggy that is on display.
- The configuration web UI is only meant to be reachable on your own network;
  do not expose the device to the internet.
