/* Basic example of delay with NOP loop*/

void setup() {
}

void loop() {
  // set pin HIGH
        R_PFS->PORT[0].PIN[12].PmnPFS = 0x05; // ON
        // delay very crudely in a NOP loop, 1s delay at 8MHz reset clock
        for(volatile int i=0; i < 8*1000*1000/2; i++) {} //Delay 0.5 Sec
        // set LOW
        R_PFS->PORT[0].PIN[12].PmnPFS = 0x04; // OFF
        // delay very crudely in a NOP loop, 1s delay at 8MHz reset clock
        for(volatile int i=0; i < 8*1000*1000/2; i++) {} //Delay 0.5 Sec

}
