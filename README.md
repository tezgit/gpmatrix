## RP2040 LED Matrix Controller 

A firmware project for RP2040-ZERO to control an 8x8 WS2812 LED matrix with serial command interface. Designed for (vehicle) status indication with multiple visualization modes.

Features
4 Operating Modes: **Off**, **Illumination**, **State Display**, **Beacon**

Serial Control: **Simple 5-byte command protocol over USB**

Visual Feedback: **Real-time LED matrix responses**


Diagnostic Tools: **Built-in RGB test pattern**

<hr>

Hardware Requirements: <br>
- **RP2040-ZERO** (or any RP2040 board) <br>
- **8x8 WS2812 LED matrix** <br>
- **POWER SOURCE +5V** <br>
- **CAPACITOR 1000uF 16V** <br>
- **USB-C CABLE** for power/programming/communication


<img src="img/hw-scheme.png"> <br>
<hr>

### ARDUINO BOARD/PORT CONFIGURATION

<img src="img/install-board.png"> <br>
<img src="img/select-board.png"> <br>
<hr>

### Command Protocol
The device accepts 5-byte packets in the format:


[mode],[brightness],[byte3],[byte4],[byte5]

Send commands via serial communication at 115200 baud (comma-separated values, terminated with newline).

<br>
<img src="img/protocol-scheme.png"> <br>


<hr>
**Mode 0: OFF**
Turns all LEDs off.<br><br>
example: 0,0,0,0,0
<hr>
**Mode 1: ILLUMINATION**
Full matrix illumination with any RGB color.

brightness: 0-100% (global brightness)<br>
byte3: Red value (0-255)<br>
byte4: Green value (0-255)<br>
byte5: Blue value (0-255)<br>

Examples:

1,50,255,0,0     - Red at 50% brightness<br>
1,80,0,255,0     - Green at 80% brightness<br>
1,100,0,0,255    - Blue at full brightness<br>
1,70,255,255,0   - Yellow at 70% brightness<br>
<hr>

**Mode 2: STATE** <br>
Three-state display using pre-defined icons and progress bar.

brightness: 0-100% (global brightness)

byte3: State (0=Good, 1=Okay, 2=Issue)

byte4: Category (1=Battery, 2=Navigation)

byte5: Intensity percentage (1-100) for progress bar

State 0 - Good:<br>
Displays green square icon (icon_02_ok)<br>
example:  2,50,0,1,1

State 2 - Issue:<br>
Displays red X icon (icon_00_error)<br>
example: 2,50,2,1,1

State 1 - Okay:<br>
Split-screen with progress bar:
<br>
Rows 0-2: Solid color (Yellow for Battery, Cyan for Navigation)
<br>
Rows 3-4: Black separator
<br>
Rows 5-7: Purple progress bar filling left-to-right based on intensity%
<br><br>
Examples:<br>
2,50,1,1,25    - Battery, 25% progress (2 columns purple)
<br>
2,50,1,2,50    - Navigation, 50% progress (4 columns purple)
<br>
2,50,1,1,75    - Battery, 75% progress (6 columns purple)
<br>
2,50,1,2,100   - Navigation, 100% progress (all 8 columns purple)

<hr>

**Mode 3: BEACON** <br>
Strobing red/blue with adjustable timing.

brightness: 0-100% (global brightness)

byte3: Base time in milliseconds (0-255)

byte4: Multiplier (1-255)

Strobe interval = baseTime × multiplier ms

<br>
Examples:<br>
3,80,100,2,0   - Strobe every 200ms
<br>
3,50,250,4,0    - Strobe every 1 second
<br>
3,90,10,10,0   - Fast strobe every 100ms
<br>

<hr>

**SPECIAL COMMANDS** <br>

**HELP**     - Display command reference
<br>
**RGBTEST**  - Run RGB color test sequence
<br>

<hr>

Icon Definitions
The project includes three built-in icons:

icon_00_error: Red X mark (State 2 - Issue)

icon_01_warning: Yellow warning triangle (reserved)

icon_02_ok: Green square (State 0 - Good)

Icons are stored as 8x8x3 RGB arrays in icons.h and displayed with serpentine layout compensation.

Serpentine Layout Handling
The code automatically compensates for WS2812 matrix serpentine wiring:

Even rows (from bottom): Display left-to-right

Odd rows (from bottom): Display right-to-left

This ensures that visual patterns appear correctly on the physical matrix.


Upon startup, the device flashes red/green/blue (power-on test). This provides visual confirmation that the matrix is working.

Enters standby mode and waits for first serial command.

<hr>

**Serial Communication** <br>
Baud Rate: 115200
<br>
Format: ASCII comma-separated values
<br>
Termination: Newline character ('\n')
<br>
Response: OK/ERR messages with debug output
<br>
<br>
**Example serial session:**

RP2040 Matrix Display Ready
System in STANDBY mode
Type HELP for commands
2,50,1,1,30
DEBUG: Received raw string: '2,50,1,1,30'
DEBUG: Values valid, executing...
OK:STATE_OKAY_C1_P30

<hr>

**Key Functions** <br>
modeOff(): Turn all LEDs off
<br>
modeIllumination(): Full matrix RGB illumination
<br>
modeState(): Icon and progress bar display
<br>
modeBeacon(): Adjustable strobe effect
<br>
displayIcon(): Render 8x8 icon with serpentine compensation
<br>
parseCommand(): Handle serial input
<br>
runRGBTest(): Diagnostic color test

<hr>
**Future Customization** <br>

*Adding New Icons*<br>
Define new icon in icons.h as const uint8_t <br>
icon_name[8][8][3]<br>
Add to icon_table[] array<br>
Update NUM_ICONS define <br>

*Modifying Colors* <br>
Adjust RGB values in icon definitions or mode functions: 
<br>
Top colors (yellow/cyan) in modeState()
<br>
Progress bar color (purple) in modeState()
<br>
Beacon colors (red/blue) in modeBeacon()
<br>

<hr>

**Troubleshooting** <br>
*No LEDs lighting up:*
<br>
Check data pin connection (GPIO 29)
<br>
Verify power supply (5V)
<br>
Run RGBTEST command
<br><br>
*Commands not working:*
<br>
Ensure baud rate is 115200
<br>
Use comma-separated format
<br>
Terminate with newline
<br><br>
<hr>

**VISUAL EXAMPLES** <br>

<img src="img/all-good.png"> <br><br>
<img src="img/major-issue.png"> <br><br>
<img src="img/battery25.png"> <br><br>
<img src="img/navigation50.png"> <br><br>
matrix test: RGBTEST
<img src="img/matrix-test.gif">
<br>
<br>
beacon 1 second: 3,50,250,4,0 
<img src="img/matrix-strobe.gif">
<br>
<br>