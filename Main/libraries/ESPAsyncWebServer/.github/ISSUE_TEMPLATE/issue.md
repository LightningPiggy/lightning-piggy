---
name: Bug Report
about: Only for bugs founds in the library! Otherwise, please go to the discussions section!
title: "[BUG] "
labels: triage
assignees: ""
---

PLEASE MAKE SURE TO READ THE RECOMMENDATIONS BEFORE OPENING A BUG REPORT:

https://github.com/ESP32Async/ESPAsyncWebServer?tab=readme-ov-file#important-recommendations-for-build-options

PLEASE READ THE FOLLOWING BEFORE OPENING A BUG REPORT:

If you want to discuss a feature, improvement, or as a question, please create a discussion here: https://github.com/ESP32Async/ESPAsyncWebServer/discussions/categories/general

If you have a crash, regression or found an issue, please give the following information:

- Which board: 
- Which platform (ESP8266, ESP32): 
- AsyncTCP version (if applicable): 
- AsyncTCP repository location you use (if applicable and if you use one coming from elsewhere): 
- ESPAsyncTCP version (if applicable):
- ESPAsyncTCP repository location you use (if applicable and if you use one coming from elsewhere): 

Please post a complete decoded stack trace below in case of a crash (if applicable):

```plaintext

```

Please post some code, ino file, minimal reproductible use case  (if applicable):

```cpp

```

IMPORTANT MESSAGE FOR RP2040 USERS and AsyncTCPSock USERS:

RP2040 and AsyncTCPSock are not supported by ESP32 organization. We only support these libraries:

- https://github.com/ESP32Async/ESPAsyncWebServer
- https://github.com/ESP32Async/AsyncTCP
- https://github.com/ESP32Async/ESPAsyncTCP

If you are using RP2040 or AsyncTCPSock, please go to the respective repositories to open an issue:

- https://github.com/yubox-node-org/AsyncTCPSock
- https://github.com/khoih-prog/AsyncTCP_RP2040W

If you are 100% sure that your issue is not related to these libraries but ESPAsyncWebServer itself, please provide the necessary information and we will try to help you.
