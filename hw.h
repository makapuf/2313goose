#pragma once


#define SAMPLE_RATE 8000
#define BUTTON_PIN PD2
#define LED_PIN PD1
#define SPEAKER_PIN PB2
#define PAGE_LENGTH 264
#define BUTTON_PRESSED ((PIND & _BV(BUTTON_PIN)) == 0)
#define BUT_TIMEOUT 0xff

void setup();
void setup_io(); // io only, no dataflash
void play(uint8_t sample_id);
void play_nowait(uint8_t sample_id);
void stop();
uint8_t wait_but(uint8_t max_tenth);
void wait_snd() ;
void set_led(uint8_t on);
