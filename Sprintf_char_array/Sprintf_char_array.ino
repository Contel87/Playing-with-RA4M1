/*
 * Esempio su come accodare variabili in un array di char e poi stamparle su seriale...
*/

#define USB_EP_PACKET_SIZE                        (512U)
#define KIT_INFO_PRIMARY_TEXT "Test uso sprintf con array di char... "

const char *p_mcu_temp = "\r\n MCU Die temperature (F/C):  ";
char kitinfo[USB_EP_PACKET_SIZE] = {'\0'};

void setup() {
  Serial.begin(115200);
  delay(2000);
  process_kit_info();
}

void loop() {
  // put your main code here, to run repeatedly:

}

/*****************************************************************************************************************
 *  @brief      Process kit information
 *  @param[in]  None
 *  @retval     None
 ****************************************************************************************************************/
static void process_kit_info(void)
{
    uint16_t buffer_index_count = 0x0000;
    uint16_t adc_data                    = 0;
    float mcu_temp_f                     = 13.33333;
    float mcu_temp_c                     = 8.66666;

    /* clear kit info buffer before updating data */
    memset(kitinfo, '\0', 511);

    /* update  predefined text in the buffer */
    memcpy(kitinfo, (char *)KIT_INFO_PRIMARY_TEXT, strlen((char *)KIT_INFO_PRIMARY_TEXT) );

    /* calculate current data filled length */
    buffer_index_count = ((uint16_t)(strlen(kitinfo)));


    /* appends the data from current buffer_index_count */
    sprintf((char *)&kitinfo[buffer_index_count],
            "%s\t%.02f/%.02f",p_mcu_temp,mcu_temp_f,mcu_temp_c);
            Serial.print(kitinfo);

    buffer_index_count  = 0U;

}

