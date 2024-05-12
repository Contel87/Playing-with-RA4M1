/*Il modulo CAC su RA4M1 serve a confrontare 2 segnali di clock sia interni che esterni sui pin P204 o P400;
Il segnale di riferimento (targhet) deve essere più grande del segnale da confrontare, questo si puo' ottenere configurando i divisori di frequenza nei registri CACR1 e CACR2;
Ad ogni cambio di stato del segnale da controllare (tick) vengono contati quanti altri tick servono fino a quando cambia di stato anche il segnale di riferimento;
La differenta di tick tra i due segnali viene scritta nel registro CACNTBR e viene confrontata con la soglia minima e la soglia massima dei registri CALLVR e CAULVR;
Se il valore del registro CACNTBR e' compreso tra i valori massimo e minimo, il confronto termina e va a 1 il bit MENDF;
Se il valore del registro CACNTBR e' fuori dalle soglie minima e massima, va a 1 il bit FERRF che indica l'errore.
*/

void setup() {
 Serial.begin(9600);
 delay(2000);
 configCAC();
}

void loop() {
startCAC();
Serial.print("MENDIE: "); Serial.println(R_CAC->CASTR_b.MENDF );  // Confronto al cambio di stato dei 2 segnali avvenuto
Serial.print("FERRIE: "); Serial.println(R_CAC->CASTR_b.FERRF );  // Il valore misurato e' fuori dalle soglie MIN e MAX
Serial.print("OVERFLOW: "); Serial.println(R_CAC->CAICR_b.OVFIE); // Overflow
viewCACNTBR(); // Mostra il valore dei 2 segnali messi a confronto ... il valore teorico dovrebbe essere 1465 (48MHz / 32.768Khz)
//stopCAC();
delay(500);
}


void configCAC(){
  R_MSTP->MSTPCRC &= ~0x1UL; // Permette di scrivere sui registri CALLVR e CAULVR
  R_CAC->CALLVR = 1455;      // Soglia MIN
  R_CAC->CAULVR = 1475;      // Soglia MAX
  R_CAC->CACR1 = 0x34;       // Imposta l'oscillatore interno HOCO (48Mhz) come frequenza Targhet
  R_CAC->CACR2 = 0x09;       // Imposta l'oscillatore interno LOCO (32.768Khz) con ratio 1/32 come segnale da confrontare (48,000,000 / 32,768 == 1464.84375)
  R_CAC->CAICR = 0x70;       // Disabilita l'uso degli interrupt globali
}

void startCAC(){
  R_CAC->CACR0_b.CFME = 1; //Enable CAC counter
}

void stopCAC(){
  R_CAC->CACR0_b.CFME = 0; //Disable CAC counter
}

void viewCACNTBR(){
  char cBuff[10];
  sprintf(cBuff, "CACNTBR = %04d", R_CAC->CACNTBR);
  Serial.println(cBuff);
}
