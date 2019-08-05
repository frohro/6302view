# 6302view

Diagrams to be added. This page to be prettified.

## Table of contents

* [Set-up](#set-up)
  * [Serial](#serial)
  * [WebSockets](#websockets)
  * [Adding modules](#adding-modules)
    * [Controls](#controls)
    * [Reporters](#reporters)
  * [`cm.step`](#cmstep)
  * [For example](#for-example)
* [Microcontroller differences](#microcontroller-differences)
  * [Quick table](#quick-table)
  * [Arduino Uno](#arduino-uno)
  * [Teensy](#teensy)
  * [ESP8266](#esp8266)
  * [ESP32](#esp32)
* [How the information is communicated (the details)](#how-the-information-is-communicated-the-details)
  * [GUI → Microcontroller](#gui--microcontroller)
  * [Microcontroller → GUI](#microcontroller--gui)
    * [How build instructions are sent](#how-build-instructions-are-sent)
    * [How the data are reported](#how-the-data-are-reported)
    * [How debug messages are sent](#how-debug-messages-are-sent)

## Set-up

Start off including the library and creating an instance of the `CommManager` class.

```cpp
#include <Six302.h>

CommManager cm(1000, 5000);
```

The numbers control the rate at which the system operates. `1000` is the time in microseconds between loops (step period), and `5000` is the time in microseconds between each *report* (report period).

There are, so far, two modes of communication between the GUI and the microcontroller. Data can be communicated over **Serial**, or by **WebSockets**.

### Serial

This is default. To connect, enter a Serial pointer and BAUD rate:

```cpp
cm.connect(&Serial, 115200);
```

### WebSockets

This is not the default. Choose `#define S302_WEBSOCKETS` at the top of `Six302.h` for this mode. This method can only work on the ESP8266 or ESP32. To connect, enter your SSID and p/w:

```cpp
cm.connect("Mom use this one", "password");
```

The Serial monitor will display the local IP address of your microcontroller that is used in the GUI. Something like:

```plaintext
Connecting to Mom use this one WiFi .. connected!
--> 10.0.0.18 <--
```

### Adding modules

To add controls and reporters, use the following `CommManager` routines.

<!--Pictures to be added.-->

#### Controls

* Add a toggle module with `addToggle`
* Add a button module with `addButton`
* Add a slider module with `addSlider`
* Add a joystick module with `addJoystick`

#### Reporters

* Add a plot module with `addPlot`
* Add a plain number module with `addNumber`

In general, the arguments these take are:

* `pointer`
* `title`
* followed by other, potentially optional args.

I will add comprehensive examples later in development. For the time being, check the header file for what arguments these take specifically.

### `cm.step`

`cm.step` updates the inputs, reports the outputs, and conveniently blocks according to your given loop rate (e.g. the 1000 microseconds below).

```cpp
void loop() {
   /* do stuff */
   cm.step();
}
```

## For example

```cpp
#include <Six302.h>

CommManager cm(1000, 5000);

float input, output;

void setup() {
   // Add modules
   cm.addSlider(&input, "Input", {-5, 5}, 0.01);
   cm.addPlot(&output, "Output", {-1, 30});
   // Connect via serial
   cm.connect(&Serial, 115200);
}

void loop() {
   output = input * input;
   cm.step();
}
```

This creates one control (a slider) and one reporter (a plot). The input is squared into the output, so, sliding from -5 to 0 to +5 moves the plot from 25 down to 0 up to 25.

(Pictures to be added.)

## Microcontroller differences

(In rough order of least capability to most capability.)

### Quick table

| Microcontroller | `MAX_CONTROLS` | `MAX_REPORTERS` | `MAX_TALLY` | `MAX_DEBUG_LEN` |
| ---------------:|:--------------:|:---------------:|:-----------:|:---------------:|
| Arduino Uno     | 5              | 5               | 5           | 500             |
| Teensy          | 20             | 10              | 10          | 1000            |
| ESP8266         | 20             | 10              | 10          | 1000            |
| ESP32           | 20             | 10              | 10          | 1000            |

Attempting to add more controls or reporters when the respective maximum is met will not add more.

`MAX_TALLY` sets the maximum number of data recordings, per reporter, per report period. See the (currently non-existent) section for more details.

`MAX_DEBUG_LEN` sets the maximum amount of characters you are able to send per report period using the `debug` routine. If your debug messages are being cut off, either shorten your messages, send less of them per report period, or increase this constant.

### Arduino Uno

The Arduino Uno has remarkable limitations in comparison to the other microcontrollers supported in this project. Because of this, constraints are in place with respect to memory and syntax.

For the routines that add elements that require you to specify a range (namely, `addSlider`, `addJoystick`, and `addPlot`), curly braces (`initializer_list`s) should not be used to specify the ranges; in its place, give two arguments -- for the lower and the higher end of the range. For example:

```cpp
// supported only on the Teensy, ESP8266, and ESP32:
cm.addSlider(&input, "Input", {-5, 5}, 0.1);
cm.addPlot(&output, "Output", {-1, 30});

// supported only on the Uno:
cm.addSlider(&input, "Input", -5, 5, 0.1);
cm.addPlot(&output, "Output", -1, 30);
```

The curly braces are there as sugar to make your code a bit more human-readable. The intent is also to make it less difficult to remember the list of arguments of these routines. For instance, it's probably easier to recognize that `cm.addSlider(&x, "x", {0, 10}, 0.1);` creates a slider between 0 and 10 at a step size of 0.1 than it is to get the same idea from `cm.addSlider(&x, "x", 0, 10, 0.1);`, and it's probably easier to remember "link, name, range" than "link, name, low, high" for the `addPlot` routine after a long weekend.

The Uno cannot natively format floats into strings or char arrays using `sprintf`. Instead, it uses `dtostrf`, which takes an argument specifying the number of digits after the decimal point to be printed. One may change the `MAX_PREC` definition in the `.h` to adjust this argument.

The Uno cannot communicate over WebSockets.

### Teensy

For the Teensy, ESP8266, and ESP32, curly braces are expected to denote ranges when adding elements. The Teensy can only communicate over Serial, like the Uno.

### ESP8266

The ESP8266 is practically the same as the Teensy, except it supports communication over WebSockets.

### ESP32

Because the ESP32 has a second core, it is desirable to run the `CommManager` over there, rather than on the primary core, to open up headroom. `cm.step` in your `loop` routine will still work; however, it's only sugar offering timing control. `cm.headroom()` returns the headroom for your `loop` routine because it is likely to be more useful than the headroom for the task running on the second core, which is generally constant around six microseconds off of the step period.

The ESP32 also supports communication over WebSockets.

## How the information is communicated (the details)

### GUI → Microcontroller

* The `cm.step` routine listens for messages from the GUI of the form `id:value\n` where `\n` is a newline character. For example, if the ID index of a `float` control were `0`, and the GUI wants to set it to `6.28`, then it would send `0:6.28\n`. If the control were for a `bool`, then the message would be `0:true\n` or `0:false\n`. A joystick controls two `float`s and is controlled with two `id:value\n` messages.
* The GUI asks the microcontroller for the buildstring by just sending `\n`.

### Microcontroller → GUI

There are three types of signals sent from the microcontroller:

* What modules to **B**uild
* The data **R**eport
* **D**ebugger messages

**Note:** All messages sent from the microcontroller to the GUI are enclosed in `\f` to start and `\n` to close.

#### How build instructions are sent

The build instructions' syntax is `\fB` followed by the list of modules and closing with `\n`. Each module starts with a letter to signify the type, follows with the name, and then with the remaining arguments as they are defined in the routine.
* `T` for Toggle
* `B` for Button
* `S` for Slider
* `J` for Joystick
* `P` for Plot
* `N` for Numerical reporter

Each module, as well as the arguments of each module, are separated by `\r`.

For example, the build string for the quick example above (the one that adds a slider and a plot) is:

```plaintext
\fBS\rInput\r-5.000000\r5.000000\r0.100000\rFalse\rP\rOutput\r-1.000000\r30.000000\r10\r1\r\n
```

(Picture to be added.)

<!--(Sadly, the carriage return is not rendered as a new line in the serial monitor (Arduino IDE 1.8.9 on Windows 10).)-->

#### How the data are reported

Report messages take the form of `\fR` followed by packs of 4 bytes, where each pack represent a `float` or 32-bit `int` value, closing with `\n`. The bytes are sent in the order they were added in setup, which is precisely the order as they appear in the build string.

Therefore, from the GUI perspective, messages coming in starting with `\fR` will have `4 * _total_reporters` bytes follow, then the closing `\n`.

#### How debug messages are sent

When using a serial communication setup, the intended way to write debug messages is with `cm.debug`. Debug messages start with `\fD`, then with four bytes representing the lowest headroom over the last report period as a `float`, follows with the user's actual message, and terminates by `\n`. Multiple lines in one debug message are separated by `\r`. The debug string is sent once per report period.

(Currently only `char` arrays are supported.)

(Picture to be added.)

