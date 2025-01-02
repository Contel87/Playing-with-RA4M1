/* Example of using a message from serial monitor (BE SURE to use Carriage Return char at end of message)
 * To switch on and off led "L" of Arduino UNO Minima R4
 * "LED_ON"  Switch ON led "L"
 * "LED_OFF" Switch OFF led "L"
*/

#define NULL_CHAR           '\0'
#define RESET_VALUE         (0x00)
#define APP_READ(read_data) serialRead(read_data)
#define PRINTF_BUFFER_SIZE  (32u)  // Size of buffer for Serial input to bulk-send chars via Serial

static char rByte[PRINTF_BUFFER_SIZE] = { NULL_CHAR };  // Variabile nella quale salvare l'ingresso da seriale

void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(115200);
  delay(2000);
}

void loop() {

  APP_READ(rByte);  
  if (rByte[0] != NULL_CHAR) {  // Controlla se e' arrivato qualcosa nella variabile rByte
 if(!strcmp(rByte, "LED_ON")) digitalWrite(13, HIGH);
 if(!strcmp(rByte, "LED_OFF")) digitalWrite(13, LOW);
  /*
 for (int i = 0; i < strlen(rByte); i++){ // Considera solo la lunghezza reale della variabile rByte
  Serial.print("rByte[");Serial.print(i);Serial.print("]:");Serial.print(rByte[i]);Serial.print(" ");
  }
  Serial.println();
 }
 */
 rByte[0]=NULL_CHAR;
  }
}

char serialRead(char *buffer) {
  char buf[PRINTF_BUFFER_SIZE] = { NULL_CHAR };
  static unsigned char index = 0;
  while (Serial.available() > 0) {
    char inch = Serial.read();
    if (inch == '\r') {
      for (int i = 0; i < strlen(buffer); i++) {
        buf[i] = buffer[i];
      }
      buffer[index] = NULL_CHAR;  // Aggiunge il terminatore di stringa
      index = RESET_VALUE;        // Reimposta l'indice per il prossimo comando
      return *buf;
    } else {
      if (index < PRINTF_BUFFER_SIZE - 1) {
        buffer[index++] = inch;     // Mette in coda il carattere ricevuto
        buffer[index] = NULL_CHAR;  // Aggiunge il terminatore di stringa
      }
    }
  }
}
