volatile uint16_t reader_size = 0;
volatile unsigned char reader_data[20];// size of buffer to store data read until we send it to the serial monitor
volatile uint8_t reader_pin_last=0;
volatile char reader_byte=0;
volatile uint8_t reader_byte_counter=0;

volatile uint8_t reader_started = 0;
volatile unsigned char packet_data[15];
volatile uint8_t packet_waiting;
volatile uint16_t packet_size;
byte VPW_len;
boolean ECU_Live = 0; //ECU is sending messages

#define T_64US 0x7F
#define T_128US 0xFF
#define T_200US 0x18F
#define T_200US_2 (T_200US - 256)

const boolean Supress_Non_BIN = false; //True means will only print .bin once read has started (also supresses CRC)
const boolean Print_CRC = true; //True means will print check sum
const int reader_Begin_Byte = Supress_Non_BIN ? 10:0; //if Supress_Non_BIN = true then set to 10
const int reader_End_Cut = (Supress_Non_BIN||!Print_CRC) ? 3:0; //if Supress_Non_BIN = true then set to 3 to leave off CRC

String VPW_Message ="";
byte VPW_data[15];

#define input_pin 12 ///must change which pin cange interupt is being use if
byte bit_RX;
byte port_RX;
volatile uint8_t *out_RX;
volatile uint8_t *in_RX;
volatile uint8_t *direction_RX;
long timer;

int w;
#define output_pin 5
byte bit_TX;
byte port_TX;
volatile uint8_t *out_TX;
volatile uint8_t *direction_TX;

#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>