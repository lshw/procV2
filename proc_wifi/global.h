#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include "config.h"
#include "ht16c21.h"
extern char ram_buf[10];
void send_ram();
float get_batt();
float v;
void timer300ms(){
digitalWrite(PC_RESET,!digitalRead(PC_RESET));
}

void settimer300ms(){
}
void test(){
  pinMode(_24V_OUT,OUTPUT);
  pinMode(PC_RESET,OUTPUT);
  pinMode(PC_POWER,OUTPUT);
  digitalWrite(_24V_OUT,LOW);
  digitalWrite(PC_RESET,HIGH);
  digitalWrite(PC_POWER,LOW);
  settimer300ms();
  delay(10000); 
analogWriteFreq(400);
  analogWrite(PWM,512);
  for(uint8_t i=20;i>0;i--) {
    delay(500);
    digitalWrite(_24V_OUT,!digitalRead(_24V_OUT));
    digitalWrite(PC_RESET,!digitalRead(PC_RESET));
    digitalWrite(PC_POWER,!digitalRead(PC_POWER));
    if(i%2) analogWrite(PWM,512+i*10);
    else analogWrite(PWM,512-i*10);
  }
  digitalWrite(_24V_OUT,LOW);
  digitalWrite(PC_RESET,HIGH);
  digitalWrite(PC_POWER,HIGH);
}
void poweroff(uint32_t sec) {
  delay(sec*1000);
  ESP.restart();
}
float get_batt() {//电压
  uint32_t dat = analogRead(A0);
  dat = analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0);

  v = (float) dat / 8 * (1000.0 + 100.0) / 100.0 / 1023 ;
  return v;
}

#endif
