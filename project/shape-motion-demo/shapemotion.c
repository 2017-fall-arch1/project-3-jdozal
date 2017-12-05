/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6
//#define RED_LED BIT0

char stateSound = 0;
AbRect rect515 = {abRectGetBounds, abRectCheck, {4,17}}; /**< 5x15 rectangle */
AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 30};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 5, screenHeight/2 - 10}
};

Layer layer4 = {
  (AbShape *)&rightArrow,
  {(screenWidth/2)+10, (screenHeight/2)}, /* center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  0
};
  

Layer layer3 = {		/**< Layer with p1 */
  (AbShape *)&rect515,
  {12, 38}, /* top left */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  0,
};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, (screenHeight/2)+5},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_PINK,
  &layer3
};

Layer layer1 = {		/**< Layer with p2 */
  (AbShape *)&rect515,
  {115,130}, /* bottom right */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLUE,
  &fieldLayer,
};

Layer layer0 = {		/**< Layer with ball */
  (AbShape *)&circle6,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &layer1,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = { &layer3, {8,8}, 0 }; /**< not all layers move */
MovLayer ml1 = { &layer1, {8,8}, 0 }; 
MovLayer ml0 = { &layer0, {2,2}, 0 }; 


/* Scores */
char scr1[3];
char scr2[3];


void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, MovLayer *p1, MovLayer *p2, Region *fence)
{
  Vec2 newPos, newP1, newP2;
  u_char axis;
  Region shapeBoundary, p1bound, p2bound;
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    vec2Add(&newP1, &p1->layer->posNext, &p1->velocity);
    vec2Add(&newP2, &p2->layer->posNext, &p2->velocity);
   

    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    abShapeGetBounds(p1->layer->abShape, &newP1, &p1bound);
    abShapeGetBounds(p2->layer->abShape, &newP2, &p2bound); 
    for (axis = 0; axis < 2; axis ++) {
      //int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];

      // if ball hits paddle p1
      if((shapeBoundary.topLeft.axes[0]+7 < p1bound.botRight.axes[0])&&(shapeBoundary.topLeft.axes[1]<p1bound.botRight.axes[1])&&(shapeBoundary.botRight.axes[1]>p1bound.topLeft.axes[1])){
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	stateSound = 1;
	//newPos.axes[axis] += 1;
      }

      //if ball hits paddle p2
       if((shapeBoundary.botRight.axes[0] > p2bound.topLeft.axes[0]-4)&&(shapeBoundary.topLeft.axes[1]<p2bound.botRight.axes[1])&&(shapeBoundary.botRight.axes[1]>p2bound.topLeft.axes[1])){
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	stateSound = 1;
	//newPos.axes[axis] += 1;
      }
     
      // if ball hits one of walls 
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {       
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	// if ball hits left wall
	if(shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]){
	  //buzzer_set_period(1000);
	  stateSound = 2;
	  scr2[1]+=1;
	  scr2[2]=0;
	  if(scr2[1] == ':'){
	    scr2[1]='0';
	    scr2[0]+=1;
	    scr2[2]=0;
	  }
	  if(scr2[0] == ':'){
	    // exit
	  }
	}
	// if ball hits right wall
	if(shapeBoundary.botRight.axes[0] > fence->botRight.axes[0]){
	  stateSound = 2;
	  scr1[1]+=1;
	  scr1[2]=0;
	  if(scr1[1] == ':'){
	    scr1[1]='0';
	    scr1[0]+=1;
	    scr1[2]=0;
	  }
	}
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  
  //buzzer_init();
}

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlUpDown(MovLayer *ml, Region *fence, char dir)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    newPos.axes[0] = ml->layer->pos.axes[0];
    newPos.axes[1] = dir ? ml->layer->pos.axes[1]+ml->velocity.axes[1] : ml->layer->pos.axes[1]-ml->velocity.axes[1];

    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) { 
	return;
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}


u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  buzzer_init();
  
  layerInit(&layer0);
  layerDraw(&layer0);


  layerGetBounds(&fieldLayer, &fieldFence);

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

   u_int switches = p2sw_read();
   scr1[0]='0';
   scr1[1]='0';
   scr1[2]=0;
   scr2[0]='0';
   scr2[1]='0';
   scr2[2]=0;
   
  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED; /**< Green led on when CPU on */
    redrawScreen = 1;
    movLayerDraw(&ml0, &layer0);
    movLayerDraw(&ml3, &layer0);
    movLayerDraw(&ml1, &layer0);
    drawString5x7(10,0,"P1: ",COLOR_GREEN,bgColor);
    drawString5x7(80,0,"P2: ",COLOR_BLUE,bgColor);
    drawString5x7(35,0,scr1,COLOR_WHITE,bgColor);
    drawString5x7(105,0,scr2,COLOR_WHITE,bgColor);
 
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  int switches = p2sw_read();
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 15) {
     mlAdvance(&ml0, &ml3, &ml1, &fieldFence);
    if (switches == 14){
      mlUpDown(&ml3, &fieldFence,0);
      //redrawScreen = 1;
    }
    if (switches == 13){
      mlUpDown(&ml3, &fieldFence,1);
      //redrawScreen = 1;
    }
    if (switches == 11){
      mlUpDown(&ml1, &fieldFence,0);
      //redrawScreen = 1;
    }
    if (switches == 7){
      mlUpDown(&ml1, &fieldFence,1);
      //redrawScreen = 1;
    }
    if(stateSound == 1 ){
      buzzer_set_period(2000);
      stateSound = 0;
    }
    else if(stateSound == 2){
      buzzer_set_period(4000);
      stateSound = 0;
    }
    else if(stateSound == 0) {
      buzzer_set_period(0);
    }
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
