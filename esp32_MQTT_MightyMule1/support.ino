// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
  #if DEBUG == 1
  //listNetworks();
  #endif
  // Stop if already connected.
  if (mqtt.connected()) {
    MQTTPings = 0;
    return;
  }

  debugPrint("Connecting to MQTT... ");

  uint8_t retries = 60; //60 * 10 = 600 seconds (10 minutes)
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       debugPrintln(mqtt.connectErrorString(ret));
       debugPrintln("Retrying MQTT connection in 10 seconds...");
       mqtt.disconnect();
       delay(10000);  // wait 10 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         debugPrintln("Died waiting for MQTT");
       }
  }
  debugPrintln("MQTT Connected!");
  if (millis()> 20000) {
    debugPrint(millis()); debugPrintln("       ...  Why did we disconnect? (updating MQTTConnections)");
  if (! MQTTConnections.publish(String(++MQTTretries).c_str())) {
    debugPrintln(F("Failed"));
  } else {
    debugPrintln(F("OK!"));
  }
    
  }
}//MQTT_connect


#if DEBUG ==1  
void listNetworks() {
  // scan for nearby networks:
  debugPrintln("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1)
  {
    debugPrintln("Couldn't get a WiFi connection");
    while (true);
  }

  // print the list of networks seen:
  debugPrint("number of available networks: ");
  debugPrintln(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    debugPrint(thisNet + 1);
    debugPrint(") ");
    debugPrint("Signal: ");
    debugPrint(WiFi.RSSI(thisNet));
    debugPrint(" dBm");
    debugPrint("\tChannel: ");
    debugPrint(WiFi.channel(thisNet));
    byte bssid[6];
    debugPrint("\t\tSSID: ");
    debugPrintln(WiFi.SSID(thisNet));
    Serial.flush();
  }//for thisNet
}//listNetworks

#endif
