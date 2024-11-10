#include <EEPROM.h>
char inizio[] = "Start of EEPROM";
char fine[] = "END of EEPROM";

void setup() {
  Serial.begin(9600);
  delay(2000);
  for (int i = 0; i < sizeof(inizio); i++) EEPROM.write(i, inizio[i]);                                                                     // Write at START of data flash memory (EEPROM)
  for (int j = EEPROM.length() - sizeof(fine); j < EEPROM.length(); j++) EEPROM.write(j + 1, fine[j - (EEPROM.length() - sizeof(fine))]);  // Write a message at END of data flash memory (EEPROM)
  Serial.print("EEprom size is: ");Serial.print(EEPROM.length());Serial.println(" byte");
 /*
  eeprom_read_block(0);
  eeprom_read_block(1);
  eeprom_read_block(2);
  eeprom_read_block(3);
  eeprom_read_block(4);
  eeprom_read_block(5);
  eeprom_read_block(6);
  eeprom_read_block(7);
  */
  eeprom_dump();
}

void loop() {
  // put your main code here, to run repeatedly:
}


void eeprom_read_block(uint8_t eeaddr) {
  Serial.print("Block ");Serial.print((eeaddr), HEX);Serial.print(": ");
  if (eeaddr < 0 || eeaddr > 7) {
    Serial.println("Error - Number of block must be from 0 to 7");
  } else {
    byte buffer[1024];  // 1Kb of single block
    uint16_t baddr;
    if (eeaddr == 0) {
      baddr = 0;
    } else {
      baddr = eeaddr * 1024;
    }
    for (uint16_t i=0, j=baddr; i< 1024, j<(baddr + 1024); i++, j++) {
      buffer[i] = EEPROM.read(j);
      char outbuf[2];
      sprintf(outbuf, "%02X ", buffer[i]);
      Serial.print(outbuf);
    }
    Serial.print(" |");
    for (uint16_t i = 0, j = baddr; i < 1024, j < (baddr + 1024); i++, j++) {
      buffer[i] = EEPROM.read(j);
      if (isprint(char(buffer[i]))) {
        Serial.print(char(buffer[i]));
      } else {
        Serial.print('.');
      }
    }
    Serial.println("|");
  }
}

void eeprom_dump() {
  delay(100);
   /*
   Serial.print("Block # | ");
   for( uint16_t b = 0; b<1024 ; b++){
      char outbuf[2];
      sprintf(outbuf, "%02X ", b);
      Serial.print(outbuf);
   }
   Serial.print(" |    ACII        |");
   Serial.println();
   */

   for (uint16_t i = 0; i < EEPROM.length(); i += 1024) {
       byte buffer[EEPROM.length()]; // Hold a page of EEPROM
       char outbuf[6];  //Room for three hex digits and ':' and ' ' and '\0'
       sprintf(outbuf, "%08X: ", i);
       Serial.print(outbuf);

   for( uint16_t a = i; a<(i+1024) ; a++){
     buffer[a] = EEPROM.read(a);
      char outbuf[2];
      sprintf(outbuf, "%02X ", buffer[a]);
      Serial.print(outbuf);
      
    }
       
       Serial.print(" |");
       for (uint16_t j = i; j < (i+1024); j++) {
           if (isprint(char(buffer[j]))) {
               Serial.print(char(buffer[j]));
           }
           else {
               Serial.print('.');
           }
       }
       Serial.println("|");
           
       }
       
      
   }