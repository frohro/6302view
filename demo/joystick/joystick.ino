
#include "Six302.h"
#include <math.h>

#define TAU (2*PI)

/* For this demo, make sure to have the `#define S302_SERIAL`
   macro enabled in the library. 

   This demo presents a joystick whose vector is used to
   calculate distance and angle. */

// microseconds
#define STEP_TIME 100000
#define REPORT_TIME 500000

CommManager cm(STEP_TIME, REPORT_TIME);

float x;
float y;

float distance;
float angle;

void setup() {
   
   /* Add modules */
   cm.addJoystick(&x, &y, "Vector", -1, +1, -1, +1, 0.1);
   cm.addPlot(&distance, "Magnitude", -0.1, 2);
   cm.addNumber(&angle, "Angle");

   /* Ready to communicate over Serial */
   cm.connect(&Serial, 115200);
}

void loop() {

   distance = sqrt( x*x + y*y );
   angle = atan2(y, x) * 360/TAU; // degrees instead of radians

   cm.step();
}