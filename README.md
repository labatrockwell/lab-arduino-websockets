lab-arduino-websockets
======================

some tests to make an arduino websocket program that connects to spacebrew (using node ws module as the server)

These tests are based on https://github.com/krohling/ArduinoWebsocketClient
We are looking into extending this library to support hixie 76 and hypi 17 protocols. At least one of those is necessary to connect to the spacebrew websocket server which is using nodejs' ws module.
