#include <dataflash.h>


#define LED_HB 9 // 13 is already taken by a pin !
#define LED_W 6 // DESACTIVE

#define PAGE_LEN 264 

// STK definitions
#define STK_OK '$'
#define STK_FAILED 0x11
#define STK_UNKNOWN '?'
#define STK_INSYNC '+'
#define STK_NOSYNC '!'
#define STK_NOSYNC2 '%'
#define STK_NOSYNC3 '*'
#define STK_NOSYNC4 '#'
#define STK_NOSYNC5 '%'
#define CRC_EOP ' ' 

Dataflash dflash; 

void setup()
{
  Serial.begin(9600);
    
  dflash.init(); //initialize the memory (pins are defined in dataflash.cpp)
  analogReference(DEFAULT); // restore analog reference (to have 5V on it!)
  
  // status LEDS  
  pinMode(LED_HB, OUTPUT);
  pinMode(LED_W, OUTPUT);
  pulse(LED_HB, 2);
  pulse(LED_W, 2);

}

// this provides a heartbeat on pin 9 (not 13 because already taken) , so you can tell the software is running.
uint8_t hbval=128;
int8_t hbdelta=8;
void heartbeat() {
  if (hbval > 192) hbdelta = -hbdelta;
  if (hbval < 32) hbdelta = -hbdelta;
  hbval += hbdelta;
  analogWrite(LED_HB, hbval);
  analogWrite(LED_W, 255-hbval);
  delay(40);
}

#define PTIME 200
void pulse(int pin, int times) {
  do {
    digitalWrite(pin, HIGH);
    delay(PTIME);
    digitalWrite(pin, LOW);
    delay(PTIME);
  } while (--times);

}


void loop(void) {
  // light the heartbeat LED
  heartbeat();
  if (Serial.available()) {
    flasher();
  }
}


uint8_t getch() {
  while(!(Serial.available()>0));
  return (uint8_t) Serial.read();
}


void read_page() {
  // R_hilo_ -> OK+page
  uint8_t c;
  
  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC); return;}
  unsigned int page_id = ((unsigned int)getch()<<8) + getch();

  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC2); return;}
  dflash.Page_To_Buffer(page_id,2); 


  // envoie OK
  Serial.print((char)STK_OK);
  
  // envoie buffer
  for (unsigned int i=0;i<PAGE_LEN;i++) {
    Serial.print((char) dflash.Buffer_Read_Byte(2,i));
  }  
}


void write_page() {
  // W_HiLo_(PAGE_LEN bytes)_ -> OK/KO
  
  uint8_t c;

  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC); return ;}

  unsigned int page_id = (getch()<<8) + getch();
  
  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC2); return ;}
  
  for (unsigned int i=0;i<PAGE_LEN;i++) {
    c = getch(); 
    dflash.Buffer_Write_Byte(1,i,c);  //write to buffer 1, 1 byte at a time
  }
  

  // ecrit effectivement la page
  dflash.Buffer_To_Page(1, page_id); //write the buffer to the memory on page: here
  //pulse(LED_W,1); // ralentit trop

  Serial.print((char)STK_OK);
}


int flasher() { 
  uint8_t ch = getch();
  switch (ch) {
  case 'H': // Hello
    if (getch()!=CRC_EOP) {
      Serial.print((char)STK_NOSYNC5);
    } else {
      char *welcome = "Salut, envoyez la puree!\n";
      for (char *c = welcome;*c != '\0';c++) {
        Serial.write(*c);
      }
    }
    break;
  case 'R': // Read Page X

    read_page(); 
    break;

  case 'W': //STK_PROG_PAGE
    write_page();
    break;
    
  // expecting a command, not CRC_EOP
  // this is how we can get back in sync
  case CRC_EOP:
    Serial.write((char) STK_NOSYNC3);
    break;

    // anything else we will return STK_UNKNOWN
  default:
    if (CRC_EOP == getch()) 
      Serial.write((char)STK_UNKNOWN);
    else
      Serial.write((char)STK_NOSYNC4);
  }
}


