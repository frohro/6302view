# 6302view

## Table of contents

I've structured this page roughly in order of increasing detail.

[**Example**](#example)<br>
[**Set-up**](#set-up)<br>
&emsp;&emsp;[Using 6302view in a sketch](#using-6302view-in-a-sketch)<br>
&emsp;&emsp;[GUI](#gui)<br>
&emsp;&emsp;[Serial](#serial)<br>
&emsp;&emsp;[WebSockets](#websockets)<br>
[**Primary commands**](#primary-commands)<br>
&emsp;&emsp;[Adding modules](#adding-modules)<br>
&emsp;&emsp;&emsp;&emsp;[Controls](#controls)<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[Toggles](#toggles)<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[Buttons](#buttons)<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[Sliders](#sliders)<br>
&emsp;&emsp;&emsp;&emsp;[Reporters](#reporters)<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[Plots](#plots)<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[Numerical reporters](#numerical-reporters)<br>
&emsp;&emsp;[`cm.step`: Loop control](#cmstep)<br>
&emsp;&emsp;[`cm.pinToCore`: Dual core on the ESP32](#dual-core)<br>
[**How the information is communicated**](#how-the-information-is-communicated)<br>
&emsp;&emsp;[GUI → Microcontroller](#gui--microcontroller)<br>
&emsp;&emsp;[Microcontroller → GUI](#microcontroller--gui)<br>
&emsp;&emsp;&emsp;&emsp;[How build instructions are sent](#how-build-instructions-are-sent)<br>
&emsp;&emsp;&emsp;&emsp;[How the data are reported](#how-the-data-are-reported)<br>
&emsp;&emsp;&emsp;&emsp;[How debug messages are sent](#how-debug-messages-are-sent)<br>
[**Microcontroller differences**](#microcontroller-differences)<br>
&emsp;&emsp;[Quick table](#quick-table)<br>
&emsp;&emsp;[Arduino Uno](#arduino-uno)<br>
&emsp;&emsp;[Teensy](#teensy)<br>
&emsp;&emsp;[ESP8266](#esp8266)<br>
&emsp;&emsp;[ESP32](#esp32)<br>

## Example

```cpp
#include <Six302.h>

CommManager cm(5000, 50000);

// controls
bool tgl;
float input;

// reporters
float output;

void setup() {
   // initialize values
   tgl = true;

   // Add modules
   cm.addToggle(&tgl, "Add ten");
   cm.addSlider(&input, "Input", -5, 5, 0.01);

   cm.addPlot(&output, "Output", 0, 35);

   // Connect via serial
   cm.connect(&Serial, 115200);
}

void loop() {

   output = input * input;
   if( tgl )
      output += 10;

   cm.step();
}
```

This `.ino` creates two controls (a toggle and a slider) and one reporter (a plot). The input is squared into the output, so, sliding from -5 to 0 to +5 moves the plot from 25 down to 0 up to 25, plus ten if the toggle is switched on.

![(image of example)](https://i.imgur.com/6IyXB53.png)

Above is what the example renders to in the GUI, with the slider modified and the toggle switched off less than a second beforehand.

## Set-up

[Download this repo](https://github.com/almonds0166/6302view/archive/master.zip) and extract the contents to an unzipped folder. Copy the inner `6302view/` folder that contains the C/C++ library to your [Arduino libraries folder](https://support.arduino.cc/hc/en-us/articles/4415103213714-Find-sketches-libraries-board-cores-and-other-files-on-your-computer).

Default libraries folder locations:
* Windows: `C:\Users{username}\Documents\Arduino\libraries` \*
* macOS: `/Users/{username}/Documents/Arduino/libraries`
* Linux: `/home/{username}/Arduino/libraries`

\* = If you use OneDrive on your Windows computer, then it will probably be located at `C:\Users\{username}\OneDrive\Documents\Arduino\libraries` instead.

### Using 6302view in a sketch

Start off including the library and creating an instance of the `CommManager` class.

```cpp
#include <Six302.h>

CommManager cm(1000, 5000);
```

The numbers control the rate at which the system operates. `1000` is the time in microseconds between loops (step period), and `5000` is the time in microseconds between each *report* (report period). I find cm(5000, 50000) is fine enough, though adjust it to your needs.

### GUI

The GUI is located at `gui/gui.html`. The page initially looks like this:

![(image of blank gui)](https://i.imgur.com/TJKfr3J.png)

There are, so far, two modes of communication between the GUI and the microcontroller: data can be communicated over **Serial** or by **WebSockets**.

### Serial

This is default communication mode. To connect, enter a Serial pointer and BAUD rate:

```cpp
cm.connect(&Serial, 115200);
```

When you've uploaded your code, run `gui/local_server.py`. It broadcasts your microcontroller's Serial to your localhost (`127.0.0.1`) via WebSockets, making the GUI work. The Python script uses the `websockets` and `pyserial` modules.

![(console image))](https://i.imgur.com/iTfgD7Q.png)

The script supports command-line arguments, which are best explained by running with the `-h` or `--help` flags.

### WebSockets

This is not the default. Choose `#define S302_WEBSOCKETS` at the top of `Six302.h` for this mode.

**Note**: This method can only work on the ESP8266 or ESP32. Make sure you have the `WebSockets` library installed (`Manage libraries...` > Search for and install [`WebSockets`](https://github.com/Links2004/arduinoWebSockets) by Markus Sattler).

To connect, enter your SSID and p/w:

```cpp
cm.connect("Mom use this one", "password");
```

The Serial monitor will display the local IP address of your microcontroller that is used in the GUI. Something like:

```plaintext
Connecting to Mom use this one WiFi .. connected!
--> 10.0.0.18:80 <--
```

In this example, in the GUI, you would use `10.0.0.18` for the Local IP and `80` for the Port.

## Primary commands

In addition to the constructor and `cm.connect`, the following sections describe some other important commands to know.

### Adding modules

To add controls and reporters, use the `CommManager` routines described below.

In general, the order of arguments is:

* A pointer to the variable
* A title or name for the module
* ... followed by other, potentially optional args.

Check the following sections or the header file for what arguments these take specifically.

#### Controls

There are currently three fully-functioning control modules.

##### Toggles

Add a toggle module with `addToggle`.

It takes only a `bool` pointer and a title.

```cpp
// Example toggle
cm.addToggle(&toggle, "Toggle");
```

<p align="center">
  <img alt="(toggle)" src="https://i.imgur.com/q1dpjXH.png">
</p>

##### Buttons

Add a button module with `addButton`.

It takes only a `bool` pointer and a title.

```cpp
// Example button
cm.addButton(&button, "Button");
```

<p align="center">
  <img alt="(button)" src="https://i.imgur.com/DFuQadR.png">
</p>

##### Sliders

Add a slider module with `addSlider`.

It takes a pointer to a `float`, a title, followed by three `float`s representing the lower end of the range, the higher end of the range, and the resolution.

```cpp
// Example slider
cm.addSlider(&input, "Slider", -20, 20, 0.01);
```

<p align="center">
  <img alt="(slider)" src="https://i.imgur.com/KPHizUg.png">
</p>

An optional sixth parameter controls whether you'd like the slider to toggle between two numbers like in the image below. (Default is `false`.)

```cpp
// Example alternating slider
cm.addSlider(&input, "Slider", -20, 20, 0.01, true);
```

<p align="center">
  <img alt="(slider with toggle)" src="https://i.imgur.com/kweSfs4.gif">
</p>

#### Reporters

There are currently two fully-functioning reporting modules.

##### Plots

Add a plot module with `addPlot`.

It takes a `float` pointer, a title, and follows with two `float`s representing the lower end of y-range and the upper end of the y-range.

```cpp
#include <Six302.h>
#include <math.h>

CommManager cm(5000, 50000);

float t, output;

void setup() {
   // Example plot
   cm.addPlot(&output, "Plot", -1.1, 1.1);
   // Connect via serial
   cm.connect(&Serial, 115200);
}

void loop() {
   output = sin(t = t + 0.02);
   cm.step();
}
```

<p align="center">
  <img alt="(plot)" src="https://i.imgur.com/ZPP8szI.png">
</p>

There are two optional parameters for plots.

The first changes the number of ticks displayed (default `10`).

The second, `burst`, changes how many data points to send up per report (default `1`). This is useful if you would like to record at a high frequency, while at the same time, to send up a report less occasionally. The recorded data points are as evenly spaced out as the step period permits. This parameter must not be greater than the `MAX_BURST` defined for the device.

<!-- (There is a third parameter under development(maybe)) -->

##### Numerical reporters

Add a plain number module with `addNumber`.

It first takes a pointer to either a real number (`float`) or a 32-bit integer (`int32_t`, AKA `long int`), and ends with a title.

```cpp
// Example number reporters
cm.addNumber(&i, "Count");
cm.addNumber(&t, "Temperature");
```

<p align="center">
  <img alt="(number)" src="https://i.imgur.com/TyGHUyS.png"> 
</p>

There is one optional parameter that controls how many data points are recorded per report period, identical to the corresponding optional parameter described above for [plots](#plots) (default `1`).

<a id="cmstep"></a>

### `cm.step`: Loop control

`cm.step` updates the inputs, reports the outputs, and conveniently blocks according to your given loop rate (e.g. the 5000 microseconds from [above](#example)).

```cpp
void loop() {
   /* do stuff */
   cm.step();
}
```

<a id="dual-core"></a>

### `cm.pinToCore`: Dual core on the ESP32

ESP32 and ESP32-S3 modules come with two cores. With 6302view, you can get the second core to handle the `cm.step` calls for you by calling `cm.pinToCore` in `setup` (*after* `cm.connect`!). This is useful if you need your microcontroller-GUI system running at as fast a rate as possible, while your main code runs on the first core.

```cpp
void setup() {
   /* do stuff */
   cm.pinToCore();
}

void loop() {
   /* do stuff */
}
```

Please note, if `cm.pinToCore` is used in this way, `cm.step` should not also be used in `loop` on the first core.

## How the information is communicated

### GUI → Microcontroller

* The `cm.step` routine listens for messages from the GUI of the form `id:value\n` where `\n` is a newline character. For example, if the ID index of a `float` control were `0`, and the GUI wants to set it to `6.28`, then it would send `0:6.28\n`. If the control were for a `bool`, then the message would be `0:true\n` or `0:false\n`.
<!-- A joystick controls two `float`s and is controlled with two `id:value\n` messages. -->
* The GUI asks the microcontroller for the buildstring by just sending `\n`.

### Microcontroller → GUI

There are three types of signals sent from the microcontroller:

* What modules to **B**uild
* The data **R**eport
* **D**ebugger messages

**Note:** All messages sent from the microcontroller to the GUI are enclosed in `\f` to start and `\n` to close.

#### How build instructions are sent

The build instructions' syntax is `\fB` followed by the list of modules, then the values of the controls at the time of requesting the build string, and finally closing with `\n`.

Each module starts with a letter to signify the type, follows with the name, and then with the remaining arguments as they are defined in the routine.
* `T` for Toggle
* `B` for Button
* `S` for Slider
* `P` for Plot
* `N` for Numerical reporter
<!-- * `J` for Joystick -->

Each module, as well as the arguments of each module, are separated by `\r`.

Following the modules is a list of the current values of the controls, denoted by the `#` symbol. Each value is followed by `\r`. This list ends with the finalizing `\n`.

For example, the build string for [the code above](#example) (the one that adds a toggle, slider, and plot), at initialization, is:

```plaintext
\fBT\rAdd ten\rS\rInput\r-5.000000\r5.000000\r0.010000\rFalse\rP\rOutput\r0.000000\r35.000000\r10\r1\r1\r#\rtrue\r0.000000\r\n
```

If the user changes the value of `input` to `2.96` and they switch the toggle off, and the GUI requests the build string again, then the message sent will change to:

```plaintext
\fBT\rAdd ten\rS\rInput\r-5.000000\r5.000000\r0.010000\rFalse\rP\rOutput\r0.000000\r35.000000\r10\r1\r1\r#\rfalse\r2.960000\r\n
```

#### How the data are reported

Report messages take the form of `\fR` followed by packs of 4 bytes, where each pack represent a `float` or 32-bit `int` value, closing with `\n`. The bytes are sent in the order they were added in setup, which is precisely the order as they appear in the build string.

Therefore, from the GUI perspective, messages coming in starting with `\fR` will have at least `4 * _total_reporters` bytes follow\*, then the closing `\n`.

\* more than this calculation, if reporting modules send multiple data points per report via their respective optional parameters. See [#Reporters](#reporters).

#### How debug messages are sent

When using a serial communication setup, the intended way to write debug messages is with `cm.debug`. Debug messages start with `\fD`, then with four bytes representing the lowest headroom encountered over the last report period as a `float`, follows with the user's actual message, and terminates by `\n`. Multiple lines in one debug message are separated by `\r`. The debug string is sent once per report period.

(Currently only `char` arrays and `String`s are supported.)

(Picture to be added once implemented.)

(This feature currently operates in the browser's console log rather than something more explicit on the webpage itself.)

## Microcontroller differences

(In rough order of least capability to most capability.)

### Quick table

| Microcontroller | `MAX_CONTROLS` | `MAX_REPORTERS` | `MAX_BURST` | `MAX_DEBUG_LEN` | `MAX_TITLE_LEN` |
| ---------------:|:--------------:|:---------------:|:-----------:|:---------------:|:---------------:|
| Arduino Uno     | 5              | 5               | 5           | 500             | 20              |
| Teensy          | 20             | 10              | 10          | 1000            | 30              |
| ESP8266         | 20             | 10              | 10          | 1000            | 30              |
| ESP32           | 20             | 10              | 100         | 1000            | 30              |

Attempting to add more controls or reporters when the respective maximum is met will not add more.

`MAX_BURST` sets the maximum number of data recordings to send, per reporter, per report period, to the GUI server. See [#Plots](#plots) for more details. For example, an Arduino Uno with an `int` reporter will record up to `5` values before it is time to report to the GUI. For this reason, it's a good rule of thumb to keep your device's report period close to `MAX_BURST` times the step period. These details are especially important when recording CSVs, where you'd probably need stable, even readings. <!--`MAX_BURST` is an 8-bit unsigned intger.-->

`MAX_DEBUG_LEN` sets the maximum amount of characters you are able to send per report period using the `debug` routine. If your debug messages are being cut off, either shorten your messages, send less of them per report period, or increase this constant.

`MAX_TITLE_LEN` sets the maximum length of titles. Long titles are truncated in the [build string](#how-build-instructions-are-sent).

Please note that you can modify the library file (`Six302.h`) to experiment/suit your needs.

### Arduino Uno

The Arduino Uno has remarkable limitations in comparison to the other microcontrollers supported in this project. Because of this, constraints are in place with respect to memory.

The Uno cannot natively format floats into strings or char arrays using `sprintf`. Instead, it uses `dtostrf`, which takes an argument specifying the number of digits after the decimal point to be printed. One may change the `MAX_PREC` definition in the `.h` to adjust this argument.

The Uno cannot communicate over WebSockets.

### Teensy

For the Teensy, ESP8266, and ESP32, curly braces are expected to denote ranges when adding elements. The Teensy can only communicate over Serial, like the Uno.

### ESP8266

The ESP8266 is practically the same as the Teensy, except it supports communication over WebSockets.

### ESP32

Because the ESP32 has a second core, it is desirable to run the `CommManager` over there, rather than on the primary core, to open up headroom. And since the ESP32 has a decent amount of dynamic memory, `MAX_BURST` is defined at a respectable 100. In my testing, without other code running on the second core, an ESP32-S3 can run 20 kHz just fine with 30 μs of headroom with a configuration of `CommManager(50, 5000)`.

The ESP32 also supports communication over WebSockets, like the ESP8266.



