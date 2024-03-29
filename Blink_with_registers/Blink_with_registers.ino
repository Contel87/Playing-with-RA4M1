/*--- Manipolazione diretta del registro P111 a cui e' collegato il led di ARDUINO UNO R4, con stampa su Serial monitor dello stato ...
Tutti i registri sono mappati nel CORE di arduino, su Windows si puo' avere l'elenco completo dei registri (UNO MINIMA e core 1.1.0) all'indirizzo
c:\users\UTENTE PC\appdata\local\arduino15\packages\arduino\hardware\renesas_uno\1.1.0\variants\minima\includes\ra\fsp\src\bsp\cmsis\device\renesas\include\R7FA4M1AB.h
ATTENZIONE la cartella appdata e' nascosta.
*/

#define PORTBASE 0x40040000 /* Port Base */
#define P111 ((volatile unsigned char  *)(PORTBASE + 0x0843 + (11 * 4)))


bool ledStatus = false;

//uint32_t Regval = *P111;

void setup() {
  Serial.begin(9600);
  delay(2000);
}

void loop() {
   if ( ledStatus ) {
      R_PFS->PORT[1].PIN[11].PmnPFS = 0x04; //Switch OFF LED_BUILTIN

      /*--- Check register status ---*/
      Serial.println("LED_BUILTIN OFF - Port P111");
      Serial.print("[Register address] ");Serial.print("0x");Serial.println(PORTBASE+0x0843+(11*4), HEX);
      Serial.print("Directly read the register status: ");Serial.println(*P111, BIN);
      Serial.print("[Register status] ");
      Serial.print("PSEL:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PSEL);Serial.print(" ");
      Serial.print("PMR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PMR);Serial.print(" ");
      Serial.print("ASEL:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.ASEL);Serial.print(" ");
      Serial.print("ISEL:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.ISEL);Serial.print(" ");
      Serial.print("EOFR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.EOFR);Serial.print(" ");
      Serial.print("DSCR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.DSCR);Serial.print(" ");
      Serial.print("NCODR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.NCODR);Serial.print(" ");
      Serial.print("PIM:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PIM);Serial.print(" ");
      Serial.print("PCR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PCR);Serial.print(" ");
      Serial.print("PDR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PDR);Serial.print(" ");
      Serial.print("PIDR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PIDR);Serial.print(" ");
      Serial.print("PODR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PODR);Serial.println(" ");

          } else {

      R_PFS->PORT[1].PIN[11].PmnPFS = 0x05; //Switch ON LED_BUILDIN
      /*--- Check register status ---*/
      Serial.println("LED_BUILTIN ON - Port P111");
      Serial.print("[Register address] ");Serial.print("0x");Serial.println(PORTBASE+0x0843+(11*4), HEX);
      Serial.print("Directly read the register status: ");Serial.println(*P111, BIN);
      Serial.print("[Register status] ");
      Serial.print("PSEL:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PSEL);Serial.print(" ");
      Serial.print("PMR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PMR);Serial.print(" ");
      Serial.print("ASEL:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.ASEL);Serial.print(" ");
      Serial.print("ISEL:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.ISEL);Serial.print(" ");
      Serial.print("EOFR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.EOFR);Serial.print(" ");
      Serial.print("DSCR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.DSCR);Serial.print(" ");
      Serial.print("NCODR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.NCODR);Serial.print(" ");
      Serial.print("PIM:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PIM);Serial.print(" ");
      Serial.print("PCR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PCR);Serial.print(" ");
      Serial.print("PDR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PDR);Serial.print(" ");
      Serial.print("PIDR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PIDR);Serial.print(" ");
      Serial.print("PODR:"); Serial.print(R_PFS->PORT[1].PIN[11].PmnPFS_b.PODR);Serial.println(" ");
          }
      
   ledStatus = !ledStatus;
          
   delay (5000);
   
}