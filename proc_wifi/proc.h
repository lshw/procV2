#ifndef __PROC_H__
#define __PROC_H__

#define MAX_SRV_CLIENTS 2
WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

#define S_TCP  1
#define S_SERIAL 0

void proc_setup() {
  pinMode(_24V_OUT, OUTPUT);
  digitalWrite(_24V_OUT, HIGH); //默认24V开启输出
  pinMode(PC_RESET, OUTPUT);
  digitalWrite(PC_RESET, LOW);
  pinMode(PC_POWER, OUTPUT);
  digitalWrite(PC_POWER, LOW);
  uint32_t com_speed = eeprom_read_u32(SPEED0);
  if (com_speed == 0) com_speed = 115200;
  uint8_t com_set = get_comset();
  Serial.begin(com_speed, com_set);
  hello(&Serial);
  digitalWrite(_24V_OUT, eeprom_read(VOUT_SET));
  Serial.println(F("#+++++[enter] into main menu"));
  server.begin();
  server.setNoDelay(true);
  remote_cycle = eeprom_read(REMOTE_CYCLE);
}

void proc_loop() {
  //check if there are any new clients
  if (server.hasClient()) {
    //find free/disconnected spot
    int i;
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
      if (!serverClients[i]) { // equivalent to !serverClients[i].connected()
        serverClients[i] = server.available();
        break;
      }
    //no free/disconnected spot so reject
    if (i == MAX_SRV_CLIENTS) {
      server.available().println("busy");
      // hints: server.available() is a WiFiClient with short-term scope
      // when out of scope, a WiFiClient will
      // - flush() - all data will be sent
      // - stop() - automatically too
    }
  }
  //check TCP clients for data
#if 1
  // Incredibly, this code is faster than the bufferred one below - #4620 is needed
  // loopback/3000000baud average 348KB/s
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    while (serverClients[i].available() && Serial.availableForWrite() > 0) {
      // working char by char is not very efficient
      Serial.write(serverClients[i].read());
    }
#else
  // loopback/3000000baud average: 312KB/s
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    while (serverClients[i].available() && Serial.availableForWrite() > 0) {
      size_t maxToSerial = std::min(serverClients[i].available(), Serial.availableForWrite());
      maxToSerial = std::min(maxToSerial, (size_t)STACK_PROTECTOR);
      uint8_t buf[maxToSerial];
      size_t tcp_got = serverClients[i].read(buf, maxToSerial);
      size_t serial_sent = Serial.write(buf, tcp_got);
      if (serial_sent != maxToSerial) {
        logger->printf("len mismatch: available:%zd tcp-read:%zd serial-write:%zd\n", maxToSerial, tcp_got, serial_sent);
      }
    }
#endif

  // determine maximum output size "fair TCP use"
  // client.availableForWrite() returns 0 when !client.connected()
  size_t maxToTcp = 0;
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    if (serverClients[i]) {
      size_t afw = serverClients[i].availableForWrite();
      if (afw) {
        if (!maxToTcp) {
          maxToTcp = afw;
        } else {
          maxToTcp = std::min(maxToTcp, afw);
        }
      }
    }

  //check UART for data
  size_t len = std::min((size_t)Serial.available(), maxToTcp);
  len = std::min(len, (size_t)STACK_PROTECTOR);
  if (len) {
    uint8_t sbuf[len];
    size_t serial_got = Serial.readBytes(sbuf, len);
    // push UART data to all connected telnet clients
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
      // if client.availableForWrite() was 0 (congested)
      // and increased since then,
      // ensure write space is sufficient:
      if (serverClients[i].availableForWrite() >= serial_got) {
        size_t tcp_sent = serverClients[i].write(sbuf, serial_got);
      }
  }
}

void run_script( Stream * s, uint8_t script_n) {
  uint8_t i, ch;
  uint16_t eeprom_addr, n;
  eeprom_addr = SCRIPT_ADDR + SCRIPT_SIZE * script_n;
  s->print(F("run script")); s->println(script_n);
  ch = eeprom_read(eeprom_addr);
  if (ch == 0 || ch >= 0x80) return;
  while (ch != 0 && ch < 0x80) {
    s->write(ch);
    switch (ch) {
      case 'p':
      case 'P':
        if (next_is_number(eeprom_addr)) {
          eeprom_addr++;
          pc_power_on = get_uint16(eeprom_addr); //addr要被更新
          s->print(pc_power_on);
        } else {
          if (ch == 'P')
            digitalWrite(PC_POWER, HIGH);
          else
            digitalWrite(PC_POWER, LOW);
        }
        break;
      case 'r':
      case 'R':
        if (next_is_number(eeprom_addr)) {
          eeprom_addr++;
          pc_reset_on = get_uint16(eeprom_addr); //i要被更新
          s->print(pc_reset_on);
        } else {
          if (ch == 'R')
            digitalWrite(PC_RESET, HIGH);
          else
            digitalWrite(PC_RESET, LOW);
        }
        break;
      case 'v':
        digitalWrite(_24V_OUT, LOW);
      case 'V':
        if (next_is_number(eeprom_addr)) {
          eeprom_addr++;
          n = get_uint16(eeprom_addr);
          analogWrite(_24V_OUT, n);
          s->print(n);
        } else {
          if (ch == 'V')
            digitalWrite(_24V_OUT, HIGH);
          else
            digitalWrite(_24V_OUT, LOW);
        }
        break;
      case 't':
      case 'T':
        if (next_is_number(eeprom_addr)) {
          eeprom_addr++;
          n = get_uint16(eeprom_addr);
          s->print(n);
          delay(n);
        }
        break;
      case 'm':
      case 'M':
        if (next_is_number(eeprom_addr)) {
          eeprom_addr++;
          pwm = get_uint16(eeprom_addr);
          s->print(pwm);
          analogWrite(PWM, pwm);
        }
        break;
    }
    eeprom_addr++;
    ch = eeprom_read(eeprom_addr);
  }
}

#endif //__PROC_H__
