#include <msp430.h>
#include "libTimer.h"
#include "buzzer.h"
int state = 0;
int counter = 0;
void buzzer_init()
{
    /* 
       Direct timer A output "TA0.1" to P2.6.  
        According to table 21 from data sheet:
          P2SEL2.6, P2SEL2.7, anmd P2SEL.7 must be zero
          P2SEL.6 must be 1
        Also: P2.6 direction must be output
    */
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */

    buzzer_advance_frequency();	/* start buzzing!!! */
  
}

void buzzer_advance_frequency() {
  short song1[10]= {0,950,0,950,0,950,710,710,0,560};
  short song2[10]= {1130,0,710,0,950,0,630,0,710,0};
  short song3[10]= {200,0,600,0,8000,0,500,0,4000,0};
   // Counter to keep songs going 
  if(counter == 11)
      counter = 0;
  else
      counter++;

  // Button 1, panic button
  if(state == 1) {
    buzzer_set_period(1000);
  }
  // Button 2, plays La Cucaracha
  if(state == 2){
    buzzer_set_period(song1[counter]);
  }
  // Button 3, plays song 2
  else if(state == 3){
    buzzer_set_period(song2[counter]);
  }
  //Button 4, plays song 3
  else if(state == 4){
    buzzer_set_period(song3[counter]);
  }
}

void buzzer_set_period(short cycles)
{
  CCR0 = cycles; 
  CCR1 = cycles >> 1;		/* one half cycle */
}
