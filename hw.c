#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "hw.h"
// PWM Audio from http://playground.arduino.cc/Code/PCMAudio


#define LOW_CS PORTB &= ~_BV(PB4)
#define HIGH_CS PORTB |= _BV(PB4)


// Data  from  dataflash, 8khz pcm 8bits 4mbits = 512ko/8=64s
// nb si encoded as 4bit dpcm audio (uniform ? a voir) @ 8khz 
//   flash out : 32kbps ; 4mbits/32kbps = 2min ! 


volatile uint16_t sample;
volatile uint16_t sounddata_length; // max 64k/8000 = 8s !

// data SPI via http://playground.arduino.cc/Code/USI-SPI + dataflash info (simple modes : streaming read)

static uint8_t spi_transfer(uint8_t data) 
{
  USIDR = data;
  USISR = _BV(USIOIF); // clear flag
 
  while ( (USISR & _BV(USIOIF)) == 0 ) {
   USICR = (1<<USIWM0)|(1<<USICS1)|(1<<USICLK)|(1<<USITC);
  }
  return USIDR;
}

static void setup_usi_spi()
{
    // DON'T use the MOSI/MISO pins. They're for ISP programming only!
    DDRB |= _BV(PB4); // as output (latch)
    DDRB |= _BV(PB6); // as output (DO) - data out
    DDRB |= _BV(PB7); // as output (USISCK) - clock
    DDRB &= ~_BV(PB5); // as input (DI) - data in
    PORTB |= _BV(PB5); // pullup on (DI)
}


static void dataflash_readbuf1(uint16_t offset, uint8_t nb, char *buf)
{
    // read page X to buffer 1
    HIGH_CS;
    LOW_CS;
    spi_transfer(0xD1); // READ TO BUF 1, 15b dont care,  9b addr, 8b dont care
    spi_transfer(0); // 8b dont care
    spi_transfer((offset>>8)&1); // 7b dont care+1b pageid
    spi_transfer(offset & 0xff); // low 8b pg id

    for (int i=0;i<nb;i++)
        *buf++ = spi_transfer(0x55); // read bytes
    HIGH_CS;
}

static void dataflash_flash2buf1(uint16_t page_id)
{
    // read page X to buffer 1
    HIGH_CS;
    LOW_CS;
    spi_transfer(0x53); // READ TO BUF 1, 55 to BUF2,4b reserved, 11=4+7b addr, 9=1+8b dont care
    spi_transfer((page_id>>7)&0xf); // 4b x+hi 4b 
    spi_transfer((page_id&0x7f)<<1); // low 7b pg id+1b x
    spi_transfer(0); // 8b dont care
    HIGH_CS; // start_transfer
}

static void dataflash_arrayread_setup(uint16_t page_id)
/* alwayts starts at the beginning of a page */
{
    HIGH_CS;
    LOW_CS;
    spi_transfer(0xE8); // 24 bits address + 32 bits dont care 
    spi_transfer((page_id>>7)&0xf); // page_id is 11 bits
    spi_transfer((page_id&0x7f)<<1); // page_id is 11 bits
    spi_transfer(0); // low bits = 0 since offset is zero

    spi_transfer(0); // dont care
    spi_transfer(0); // dont care
    spi_transfer(0); // dont care
    spi_transfer(0); // dont care
}

static uint8_t dataflash_arrayread_read()
/* alwayts start at th beginning of a page (normal) */
{
    return spi_transfer(55); // dont care oput 
}


static void dataflash_arrayread_end()
/* alwayts start at th beginning of a page (normal) */
{
    HIGH_CS;
}

// ------------------------------------------------
void setup_io()
{
    DDRB |= _BV(SPEAKER_PIN);
    
    DDRD |= _BV(LED_PIN);
    PORTD |= _BV(LED_PIN); // set it on

    DDRD &= ~(_BV(BUTTON_PIN)); // input
    PORTD |= _BV(BUTTON_PIN); // pullup 
}


void setup()
{
	setup_usi_spi();
    // load FAT to dataflash buffer 
    dataflash_flash2buf1(0);
    setup_io();
    sounddata_length=0;
}

void stop(void); 

// This is called at 8000 Hz to load the next sample.
ISR(TIMER1_COMPA_vect) {
    if (!sounddata_length) {
        // ramp down
        // if (sample)
        // {
        //     sample--;
        //     OCR0A = sample;
        // }
        // else
            stop(); 
    }
    else {
        sounddata_length--;
        // DPCM ?
        OCR0A = sample;
        // load next sample
        sample = dataflash_arrayread_read(); // read next sample
    }
}

void play_nowait(uint8_t sample_id)
{

    uint8_t datafat[6]; // start_page, end_pos,  next_page_start ; lus d'un coup
    uint16_t p1, p2, ofs; //. direct ?

    // load page_start,next_page_start, position_end from page 0 buffer
    /* rappel de flasher.py page zero = 
        SIGNATURE = YEAH
        FAT = liste de (
            HiPagestart,LoPagestart,
            HiLastBytes,LoLastBytes
        ) avec dernier record = xx,xx,00,00 où xx est la premiere page libre.

    */

    dataflash_readbuf1(4+4*(uint16_t)sample_id,6,datafat); // sample id is at position 4

    p1 =(datafat[0]<<8)+datafat[1];
    ofs=(datafat[2]<<8)+datafat[3];
    p2 =(datafat[4]<<8)+datafat[5];

    sounddata_length = (p2-p1-2)*PAGE_LENGTH+ofs;

    // issue read continuous value from page X
    //dataflash_arrayread_setup(datafat[0]+1); // 1 page for filename, then file data
    dataflash_arrayread_setup(p1+1); // 1 page for filename, then file data


    // Set up Timer 0 to do pulse width modulation 
    // on the speaker pin 0c0A, ie PB2, ie pin14

    // Use internal clock @ 8MHz - so don't change cksel. Also must remove 8MHz fuse.

    // Set fast PWM mode on TIMER0
    TCCR0A |= _BV(WGM00) | _BV(WGM01);
    TCCR0A &= ~_BV(WGM02);

    /* Clear 0C0A on compare. */
    TCCR0A |= _BV(COM0A1);
    TCCR0A &= ~_BV(COM0A0);

    /* Start timer, no prescaling. */
    TCCR0B = _BV(CS00);

    // Set initial pulse width to the first sample (first value)
    OCR0A = 0; 
    sample = 0;

    // Set up Timer 1 to send a sample every interrupt.
    cli();

    
    // Set CTC mode (Clear Timer on Compare Match) (p.133)
    // Have to set OCR1A *after*, otherwise it gets reset to 0!
    TCCR1B = (TCCR1B & ~_BV(WGM13)) | _BV(WGM12);
    TCCR1A = TCCR1A & ~(_BV(WGM11) | _BV(WGM10));

    // No prescaler (p.134)
    TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);

    // Set the compare register (OCR1A).
    // OCR1A is a 16-bit register, so we have to do this with
    // interrupts disabled to be safe.
    OCR1A = F_CPU / SAMPLE_RATE;    
    // 16e6 / 8000 = 2000, 1e6/8000= 125, 8e6/8000 = 1000, also the cycles we have per sample.

    // Enable interrupt when TCNT1 == OCR1A on output compare
    TIMSK |= _BV(OCIE1A);

    sei();
}


void stop()
{
    // End continuous read Dataflash

    dataflash_arrayread_end();

    // Disable playback per-sample interrupt.
    TIMSK &= ~_BV(OCIE1A);

    // Disable the per-sample timer completely.
    TCCR1B &= ~_BV(CS10);

    // Disable the PWM timer.
    TCCR0B &= ~_BV(CS10);

    // set to low
    PORTB &= ~_BV(SPEAKER_PIN);
}


uint8_t wait_but(uint8_t max_tenth)
/* attend <max_tenth> 1/10s seconds at most. 
 * returns the number of 1/10s pressed, at least 1 
 * or 0 if not pressed before timeout 
 * *** DEBOUNCE !
 */
{   
    _delay_ms(100);
    for (uint8_t tenth=1;tenth<max_tenth;tenth++)
    {
        // flash led quickly 
        if ((tenth&3) ==0) {
            set_led(1);
            _delay_ms(20);
            set_led(0);    
        }
        
        for (uint8_t i=0;i<10;i++)
        {
            if (BUTTON_PRESSED) return tenth;
             // adds randomness ton rand
            rand();
            _delay_ms(10);
        }
    }
    return BUT_TIMEOUT;
}

void wait_snd() 
// attend le stop
{
    while(sounddata_length>0 ); 
}

void play(uint8_t sample_id)
{
    play_nowait(sample_id);
    wait_snd();
    _delay_ms(100); // XXX
}

void set_led(uint8_t on)
{
    if (on)
        PORTD |= (1 << LED_PIN);
    else
        PORTD &= ~(1 << LED_PIN);
}