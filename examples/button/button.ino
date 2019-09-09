
#include <Six302.h>

/* For this demo, make sure to have the `#define S302_SERIAL`
   macro enabled in the library. 

   This demo presents a button that when pressed increments
   the number displayed */

// microseconds
#define STEP_TIME 10000
#define REPORT_TIME 50000

CommManager cm(STEP_TIME, REPORT_TIME);

bool input;
int32_t output;
float slido;

void setup() {
   
   /* Add modules */
   output=1;
   slido = 0.2;
   //cm.addButton(&input, "Increment");
   //cm.addSlider(&slido, "Slider");
   cm.addSlider(&slido, "Left",   -1,   +1, 0.1, false); // true indicates toggle
   cm.addNumber(&output, "Sum");

   /* Ready to communicate over Serial */
   cm.connect(&Serial, 115200);
}

void loop() {
   if( input )
      output++;
   cm.step();
}
