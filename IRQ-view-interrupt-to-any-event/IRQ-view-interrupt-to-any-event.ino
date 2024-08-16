/*  Arduino UNO R4 code for general method to attach an interrupt to ANY event on R4 MINIMA

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/


void setup() {
  Serial.begin(9600);
  delay(2000);
  listEventLinks();

}

void loop() {
  // put your main code here, to run repeatedly:

}

const char string_00[] PROGMEM = "No_Event";
const char string_01[] PROGMEM = "PORT_IRQ0";
const char string_02[] PROGMEM = "PORT_IRQ1";
const char string_03[] PROGMEM = "PORT_IRQ2";
const char string_04[] PROGMEM = "PORT_IRQ3";
const char string_05[] PROGMEM = "PORT_IRQ4";
const char string_06[] PROGMEM = "PORT_IRQ5";
const char string_07[] PROGMEM = "PORT_IRQ6";
const char string_08[] PROGMEM = "PORT_IRQ7";
const char string_09[] PROGMEM = "PORT_IRQ8";
const char string_0A[] PROGMEM = "PORT_IRQ9";
const char string_0B[] PROGMEM = "PORT_IRQ10";
const char string_0C[] PROGMEM = "PORT_IRQ11";
const char string_0D[] PROGMEM = "PORT_IRQ12";
const char string_0E[] PROGMEM = "PORT_UNUSED";
const char string_0F[] PROGMEM = "PORT_IRQ14";
const char string_10[] PROGMEM = "PORT_IRQ15";
const char string_11[] PROGMEM = "DMAC0_INT";
const char string_12[] PROGMEM = "DMAC1_INT";
const char string_13[] PROGMEM = "DMAC2_INT";
const char string_14[] PROGMEM = "DMAC3_INT";
const char string_15[] PROGMEM = "DTC_COMPLETE";
const char string_16[] PROGMEM = "UNUSED";
const char string_17[] PROGMEM = "ICU_SNZCANCEL";
const char string_18[] PROGMEM = "FCU_FRDYI";
const char string_19[] PROGMEM = "LVD_LVD1";
const char string_1A[] PROGMEM = "LVD_LVD2";
const char string_1B[] PROGMEM = "VBATT_LVD";
const char string_1C[] PROGMEM = "MOSC_STOP";
const char string_1D[] PROGMEM = "SYSTEM_SNZREQ";
const char string_1E[] PROGMEM = "AGT0_AGTI";
const char string_1F[] PROGMEM = "AGT0_AGTCMAI";
const char string_20[] PROGMEM = "AGT0_AGTCMBI";
const char string_21[] PROGMEM = "AGT1_AGTI";
const char string_22[] PROGMEM = "AGT1_AGTCMAI";
const char string_23[] PROGMEM = "AGT1_AGTCMBI";
const char string_24[] PROGMEM = "IWDT_NMIUNDF";
const char string_25[] PROGMEM = "WDT_NMIUNDF";
const char string_26[] PROGMEM = "RTC_ALM";
const char string_27[] PROGMEM = "RTC_PRD";
const char string_28[] PROGMEM = "RTC_CUP";
const char string_29[] PROGMEM = "ADC140_ADI";
const char string_2A[] PROGMEM = "ADC140_GBADI";
const char string_2B[] PROGMEM = "ADC140_CMPAI";
const char string_2C[] PROGMEM = "ADC140_CMPBI";
const char string_2D[] PROGMEM = "ADC140_WCMPM";
const char string_2E[] PROGMEM = "ADC140_WCMPUM";
const char string_2F[] PROGMEM = "ACMP_LP0";
const char string_30[] PROGMEM = "ACMP_LP1";
const char string_31[] PROGMEM = "USBFS_D0FIFO";
const char string_32[] PROGMEM = "USBFS_D1FIFO";
const char string_33[] PROGMEM = "USBFS_USBI";
const char string_34[] PROGMEM = "USBFS_USBR";
const char string_35[] PROGMEM = "IIC0_RXI";
const char string_36[] PROGMEM = "IIC0_TXI";
const char string_37[] PROGMEM = "IIC0_TEI";
const char string_38[] PROGMEM = "IIC0_EEI";
const char string_39[] PROGMEM = "IIC0_WUI";
const char string_3A[] PROGMEM = "IIC1_RXI";
const char string_3B[] PROGMEM = "IIC1_TXI";
const char string_3C[] PROGMEM = "IIC1_TEI";
const char string_3D[] PROGMEM = "IIC1_EEI";
const char string_3E[] PROGMEM = "SSIE0_SSITXI";
const char string_3F[] PROGMEM = "SSIE0_SSIRXI";
const char string_40[] PROGMEM = "UNUSED";
const char string_41[] PROGMEM = "SSIE0_SSIF";
const char string_42[] PROGMEM = "CTSU_CTSUWR";
const char string_43[] PROGMEM = "CTSU_CTSURD";
const char string_44[] PROGMEM = "CTSU_CTSUFN";
const char string_45[] PROGMEM = "KEY_INTKR";
const char string_46[] PROGMEM = "DOC_DOPCI";
const char string_47[] PROGMEM = "CAC_FERRI";
const char string_48[] PROGMEM = "CAC_MENDI";
const char string_49[] PROGMEM = "CAC_OVFI";
const char string_4A[] PROGMEM = "CAN0_ERS";
const char string_4B[] PROGMEM = "CAN0_RXF";
const char string_4C[] PROGMEM = "CAN0_TXF";
const char string_4D[] PROGMEM = "CAN0_RXM";
const char string_4E[] PROGMEM = "CAN0_TXM";
const char string_4F[] PROGMEM = "IOPORT_GROUP1";
const char string_50[] PROGMEM = "IOPORT_GROUP2";
const char string_51[] PROGMEM = "IOPORT_GROUP3";
const char string_52[] PROGMEM = "IOPORT_GROUP4";
const char string_53[] PROGMEM = "ELC_SWEVT0";
const char string_54[] PROGMEM = "ELC_SWEVT1";
const char string_55[] PROGMEM = "POEG_GROUP0";
const char string_56[] PROGMEM = "POEG_GROUP1";
const char string_57[] PROGMEM = "GPT0_CCMPA";
const char string_58[] PROGMEM = "GPT0_CCMPB";
const char string_59[] PROGMEM = "GPT0_CMPC";
const char string_5A[] PROGMEM = "GPT0_CMPD";
const char string_5B[] PROGMEM = "GPT0_CMPE";
const char string_5C[] PROGMEM = "GPT0_CMPF";
const char string_5D[] PROGMEM = "GPT0_OVF";
const char string_5E[] PROGMEM = "GPT0_UDF";
const char string_5F[] PROGMEM = "GPT1_CCMPA";
const char string_60[] PROGMEM = "GPT1_CCMPB";
const char string_61[] PROGMEM = "GPT1_CMPC";
const char string_62[] PROGMEM = "GPT1_CMPD";
const char string_63[] PROGMEM = "GPT1_CMPE";
const char string_64[] PROGMEM = "GPT1_CMPF";
const char string_65[] PROGMEM = "GPT1_OVF";
const char string_66[] PROGMEM = "GPT1_UDF";
const char string_67[] PROGMEM = "GPT2_CCMPA";
const char string_68[] PROGMEM = "GPT2_CCMPB";
const char string_69[] PROGMEM = "GPT2_CMPC";
const char string_6A[] PROGMEM = "GPT2_CMPD";
const char string_6B[] PROGMEM = "GPT2_CMPE";
const char string_6C[] PROGMEM = "GPT2_CMPF";
const char string_6D[] PROGMEM = "GPT2_OVF";
const char string_6E[] PROGMEM = "GPT2_UDF";
const char string_6F[] PROGMEM = "GPT3_CCMPA";
const char string_70[] PROGMEM = "GPT3_CCMPB";
const char string_71[] PROGMEM = "GPT3_CMPC";
const char string_72[] PROGMEM = "GPT3_CMPD";
const char string_73[] PROGMEM = "GPT3_CMPE";
const char string_74[] PROGMEM = "GPT3_CMPF";
const char string_75[] PROGMEM = "GPT3_OVF";
const char string_76[] PROGMEM = "GPT3_UDF";
const char string_77[] PROGMEM = "GPT4_CCMPA";
const char string_78[] PROGMEM = "GPT4_CCMPB";
const char string_79[] PROGMEM = "GPT4_CMPC";
const char string_7A[] PROGMEM = "GPT4_CMPD";
const char string_7B[] PROGMEM = "GPT4_CMPE";
const char string_7C[] PROGMEM = "GPT4_CMPF";
const char string_7D[] PROGMEM = "GPT4_OVF";
const char string_7E[] PROGMEM = "GPT4_UDF";
const char string_7F[] PROGMEM = "GPT5_CCMPA";
const char string_80[] PROGMEM = "GPT5_CCMPB";
const char string_81[] PROGMEM = "GPT5_CMPC";
const char string_82[] PROGMEM = "GPT5_CMPD";
const char string_83[] PROGMEM = "GPT5_CMPE";
const char string_84[] PROGMEM = "GPT5_CMPF";
const char string_85[] PROGMEM = "GPT5_OVF";
const char string_86[] PROGMEM = "GPT5_UDF";
const char string_87[] PROGMEM = "GPT6_CCMPA";
const char string_88[] PROGMEM = "GPT6_CCMPB";
const char string_89[] PROGMEM = "GPT6_CMPC";
const char string_8A[] PROGMEM = "GPT6_CMPD";
const char string_8B[] PROGMEM = "GPT6_CMPE";
const char string_8C[] PROGMEM = "GPT6_CMPF";
const char string_8D[] PROGMEM = "GPT6_OVF";
const char string_8E[] PROGMEM = "GPT6_UDF";
const char string_8F[] PROGMEM = "GPT7_CCMPA";
const char string_90[] PROGMEM = "GPT7_CCMPB";
const char string_91[] PROGMEM = "GPT7_CMPC";
const char string_92[] PROGMEM = "GPT7_CMPD";
const char string_93[] PROGMEM = "GPT7_CMPE";
const char string_94[] PROGMEM = "GPT7_CMPF";
const char string_95[] PROGMEM = "GPT7_OVF";
const char string_96[] PROGMEM = "GPT7_UDF";
const char string_97[] PROGMEM = "GPT_UVWEDGE";
const char string_98[] PROGMEM = "SCI0_RXI";
const char string_99[] PROGMEM = "SCI0_TXI";
const char string_9A[] PROGMEM = "SCI0_TEI";
const char string_9B[] PROGMEM = "SCI0_ERI";
const char string_9C[] PROGMEM = "SCI0_AM";
const char string_9D[] PROGMEM = "SCI0_RXI_OR_ERI";
const char string_9E[] PROGMEM = "SCI1_RXI";
const char string_9F[] PROGMEM = "SCI1_TXI";
const char string_A0[] PROGMEM = "SCI1_TEI";
const char string_A1[] PROGMEM = "SCI1_ERI";
const char string_A2[] PROGMEM = "SCI1_AM";
const char string_A3[] PROGMEM = "SCI2_RXI";
const char string_A4[] PROGMEM = "SCI2_TXI";
const char string_A5[] PROGMEM = "SCI2_TEI";
const char string_A6[] PROGMEM = "SCI2_ERI";
const char string_A7[] PROGMEM = "SCI2_AM";
const char string_A8[] PROGMEM = "SCI9_RXI";
const char string_A9[] PROGMEM = "SCI9_TXI";
const char string_AA[] PROGMEM = "SCI9_TEI";
const char string_AB[] PROGMEM = "SCI9_ERI";
const char string_AC[] PROGMEM = "SCI9_AM";
const char string_AD[] PROGMEM = "SPI0_SPRI";
const char string_AE[] PROGMEM = "SPI0_SPTI";
const char string_AF[] PROGMEM = "SPI0_SPII";
const char string_B0[] PROGMEM = "SPI0_SPEI";
const char string_B1[] PROGMEM = "SPI0_SPTEND";
const char string_B2[] PROGMEM = "SPI1_SPRI";
const char string_B3[] PROGMEM = "SPI1_SPTI";
const char string_B4[] PROGMEM = "SPI1_SPII";
const char string_B5[] PROGMEM = "SPI1_SPEI";
const char string_B6[] PROGMEM = "SPI1_SPTEND";
const char string_B7[] PROGMEM = "UNUSED";

const char *const string_table[] PROGMEM = {
string_00, string_01, string_02, string_03, string_04, string_05, string_06, string_07,
string_08, string_09, string_0A, string_0B, string_0C, string_0D, string_0E, string_0F,
string_10, string_11, string_12, string_13, string_14, string_15, string_16, string_17,
string_18, string_19, string_1A, string_1B, string_1C, string_1D, string_1E, string_1F,
string_20, string_21, string_22, string_23, string_24, string_25, string_26, string_27,
string_28, string_29, string_2A, string_2B, string_2C, string_2D, string_2E, string_2F,
string_30, string_31, string_32, string_33, string_34, string_35, string_36, string_37,
string_38, string_39, string_3A, string_3B, string_3C, string_3D, string_3E, string_3F,
string_40, string_41, string_42, string_43, string_44, string_45, string_46, string_47,
string_48, string_49, string_4A, string_4B, string_4C, string_4D, string_4E, string_4F,
string_50, string_51, string_52, string_53, string_54, string_55, string_56, string_57,
string_58, string_59, string_5A, string_5B, string_5C, string_5D, string_5E, string_5F,
string_60, string_61, string_62, string_63, string_64, string_65, string_66, string_67,
string_68, string_69, string_6A, string_6B, string_6C, string_6D, string_6E, string_6F,
string_70, string_71, string_72, string_73, string_74, string_75, string_76, string_77,
string_78, string_79, string_7A, string_7B, string_7C, string_7D, string_7E, string_7F,
string_80, string_81, string_82, string_83, string_84, string_85, string_86, string_87,
string_88, string_89, string_8A, string_8B, string_8C, string_8D, string_8E, string_8F,
string_90, string_91, string_92, string_93, string_94, string_95, string_96, string_97,
string_98, string_99, string_9A, string_9B, string_9C, string_9D, string_9E, string_9F,
string_A0, string_A1, string_A2, string_A3, string_A4, string_A5, string_A6, string_A7,
string_A8, string_A9, string_AA, string_AB, string_AC, string_AD, string_AE, string_AF,
string_B0, string_B1, string_B2, string_B3, string_B4, string_B5, string_B6, string_B7
};

char message_buffer[30];  //


void listEventLinks(){
  for (uint8_t i=0; i<32; i++){
    volatile uint32_t val = R_ICU->IELSR[i];
    if(val == 0) break;
    Serial.print("IELSR");
    Serial.print(i);
    Serial.print(" : ");
    strcpy_P(message_buffer, (char *)pgm_read_word(&(string_table[val])));  // 
    Serial.print(val, HEX);
    Serial.print(" - ");  
    Serial.println(message_buffer);
  }
}