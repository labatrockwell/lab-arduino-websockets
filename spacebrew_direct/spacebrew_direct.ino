#include "Arduino.h"
#include <Ethernet.h>
#include <SPI.h>
#include <WebSocketClient.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "lab-macbookpro-02.rockwellgroup.com";//"echo.websocket.org";//
WebSocketClient client;
int next = 0;
int timeoutDelay = 1000;
static char buffer[150];
static uint8_t toSend[400];//max message of 1000 chars (minus headers)

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);
  if (!client.connect(server, "/", 9000)){
    Serial.println("failed!");
  } else {
    Serial.println("connected!");
  }
  client.setDataArrivedDelegate(dataArrived);
  String output = "test";
  //const uint8_t toSend[] = {B10000001, B10000100, B00000000, B00000000, B00000000, B00000000, output.charAt(0) ^ B00000000, output.charAt(1) ^ B00000000, output.charAt(2) ^ B00000000, output.charAt(3) ^ B00000000};
  //String out = String(toSend);
  //client._client.write(toSend, 10);
  send("{\"config\":{\"name\":\"Arduino\",\"description\":\"Analog inputs to spacebrew\",\"publish\":{\"messages\":[{\"name\":\"A0\",\"type\":\"range\"},{\"name\":\"A1\",\"type\":\"range\"},{\"name\":\"A2\",\"type\":\"range\"},{\"name\":\"A3\",\"type\":\"range\"},{\"name\":\"A4\",\"type\":\"range\"},{\"name\":\"A5\",\"type\":\"range\"}]},\"subscribe\":{\"messages\":[{\"name\":\"LED\",\"type\":\"boolean\"}]}}}");
  //char toSend[] = {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f};
  //client._client.print(toSend);
  /*
  //http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-17#section-5.2
  client._client.print((byte)B10000001);//FIN, RSV1, RSV2, RSV3, OPCODE(4)
  client._client.print((byte)(B10000000 | output.length()));//MASK, LEN(7)
  client._client.print((byte)B00000000);//MASK_KEY 0
  client._client.print((byte)B00000000);//MASK_KEY 1
  client._client.print((byte)B00000000);//MASK_KEY 2
  client._client.print((byte)B00000000);//MASK_KEY 3
  for(int i = 0; i < output.length(); i++){
    client._client.print((byte)(output.charAt(i) ^ B00000000));
  }*/
  //but it is only showing 3 bytes on server side?
  //and the web data from chrome is showing 596 bytes
  
  //client.send("test");//{\"config\":{\"name\":\"Arduino\",\"description\":\"Analog inputs to spacebrew\",\"publish\":{\"messages\":[{\"name\":\"A0\",\"type\":\"range\"},{\"name\":\"A1\",\"type\":\"range\"},{\"name\":\"A2\",\"type\":\"range\"},{\"name\":\"A3\",\"type\":\"range\"},{\"name\":\"A4\",\"type\":\"range\"},{\"name\":\"A5\",\"type\":\"range\"}]},\"subscribe\":{\"messages\":[{\"name\":\"LED\",\"type\":\"boolean\"}]}}}");
}

void send(String s){
  //TODO: perhaps only send packets with a max length of 125, 
  //      so we don't have to deal with buffering larger packets
  //      and larger data can be split across multiple packets.
  //TODO: implement both hixie 76 and hybi 17? use flag to switch between them?
  //TODO: look into possibilities for not needing to build the whole packet in it's own
  //      buffer before sending. some way of streaming data through the Ethernet lib?
  //TODO: correctly implement security key/check and proper mask generation every frame
  //TODO: roll into library
  int totalSize = 6 + (s.length() < 126 ? 0 : s.length() < 2^16 ? 2 : 8) + s.length();
  if (totalSize > 400){//limited by static toSend array size
    Serial.println("string too long: ");
    Serial.println(s);
    Serial.println(totalSize);
    return;
  }
  //uint8_t toSend[totalSize];
  toSend[0] = B10000001;//FIN,RSV1,RSV2,RSV3,OPCODE(4)
  toSend[1] = B10000000 + (s.length() < 126 ? s.length() : (s.length() < 2^16 ? 126 : 127));//MASK, LEN(7)
  int i = 2;
  if (s.length() < 126){
    //don't do anything
  } else if (s.length() < 2^16){
    toSend[2] = (uint8_t)((s.length() >> 8) & 0xff);
    toSend[3] = (uint8_t)(s.length() & 0xff);
    i = 4;
  } else {
    //we should never get here, since this would mean the String is using 65k of memory
    Serial.println("shouldn't be here");
  }
  for(int n = 0; n < 4; n++, i++){
    toSend[i] = B00000000;//MASK_KEY(8)
  }
  for(int n = 0; n < s.length(); n++, i++){
    toSend[i] = s.charAt(n) ^ B00000000;
  }
  client._client.write(toSend, totalSize);
}

void loop() {
  client.monitor();
  if (next < millis()){
    // output the value of each analog input pin
    for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
      //clear out the previous contents from the buffer
      for(int i = 0; i < 150; i++){
        buffer[i] = 255;
      }
      //char buffer[150];
      strcpy(buffer, "{\"message\":{\"clientName\":\"Arduino\",\"name\":\"A");
      char c[] = {char(analogChannel+'0'), 0};
      strcat(buffer, c);
      strcat(buffer, "\",\"type\":\"range\",\"value\":\"");
      char out[4];
      uint8_t i = 0;
      int in = analogRead(analogChannel);
      while(i < 4 && in > 0){
        out[i] = in%10;
        i++;
        in /= 10;
      }
      while(i > 0){
        i--;
        char n[] = {char(out[i] + '0'), 0};
        strcat(buffer, c);
      }
      strcat(buffer, "\"}}");
      send(String(buffer));
//      client.send(buffer);
//      client.send("{\"message\":{\"clientName\":\"Arduino\",\"name\":\"A"+analogChannel+"\",\"type\":\"range\",\"value\":\""+analogRead(analogChannel)+"\"}}");
    }
    next = millis() + timeoutDelay;
    Serial.println(millis());
  }
}

void dataArrived(WebSocketClient client, String data) {
  Serial.println("Data Arrived: " + data);
}
