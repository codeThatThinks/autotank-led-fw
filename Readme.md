# autotank-led-fw

A CANbus-enabled LED and GPIO controller -- firmware


## CANbus Commands

The default Dev ID is `0xA3`

<table>
	<tr>
		<th>Command</th>
		<th colspan="2">Extended ID</th>
		<th></th>
		<th colspan="4">Send Payload</th>
		<th></th>
		<th colspan="2">Recv Payload</th>
	</tr>
	<tr>
		<th></th>
		<th>28:8</th>
		<th>7:0</th>
		<th></th>
		<th>Byte 0</th>
		<th>Byte 1</th>
		<th>Byte 2</th>
		<th>Byte 3</th>
		<th></th>
		<th>Byte 0</th>
		<th>Byte 1</th>
	</tr>
	<tr>
		<td>Read Pins</td>
		<td>0</td>
		<td>Dev ID</td>
		<td></td>
		<td>Dev ID</td>
		<td></td>
		<td></td>
		<td></td>
		<td></td>
		<td>In States</td>
		<td>Out States</td>
	</tr>
	<tr>
		<td>Write Pins</td>
		<td>1</td>
		<td>Dev ID</td>
		<td></td>
		<td>States</td>
		<td></td>
		<td></td>
		<td></td>
		<td></td>
		<td></td>
		<td></td>
	</tr>
	<tr>
		<td>Write Pin</td>
		<td>2</td>
		<td>Dev ID</td>
		<td></td>
		<td>Pin</td>
		<td>State</td>
		<td></td>
		<td></td>
		<td></td>
		<td></td>
		<td></td>
	</tr>
	<tr>
		<td>Truth Table</td>
		<td>3</td>
		<td>Dev ID</td>
		<td></td>
		<td>Pin</td>
		<td>Enable</td>
		<td colspan="2">Values</td>
		<td></td>
		<td></td>
		<td></td>
	</tr>
	<tr>
		<td>Pin Interrupt</td>
		<td>4</td>
		<td>Dev ID</td>
		<td></td>
		<td>Pin</td>
		<td>Mode</td>
		<td>Dev ID</td>
		<td></td>
		<td></td>
		<td>Pin</td>
		<td>State</td>
	</tr>
	<tr>
		<td>RGB Strip 1</td>
		<td>5</td>
		<td>Dev ID</td>
		<td></td>
		<td>Mode</td>
		<td>Red</td>
		<td>Green</td>
		<td>Blue</td>
		<td></td>
		<td></td>
		<td></td>
	</tr>
	<tr>
		<td>RGB Strip 2</td>
		<td>6</td>
		<td>Dev ID</td>
		<td></td>
		<td>Mode</td>
		<td>Red</td>
		<td>Green</td>
		<td>Blue</td>
		<td></td>
		<td></td>
		<td></td>
	</tr>
</table>


## GPIO

Pin states use the following payload byte format (MSB first):

<table>
	<tr>
		<th colspan="8">Byte n</th>
	</tr>
	<tr>
		<td colspan="4">7:4 <i>Reserved</i></td>
		<td>Pin 4</td>
		<td>Pin 3</td>
		<td>Pin 2</td>
		<td>Pin 1</td>
	</tr>
</table>


## Output Truth Table

As an alternative to writing output pins states, a logic truth table can be enabled to automatically set output pins based on input pin states. Writes are ignored when a truth table is enabled for a pin.

Each output pin has the following truth table:

<table>
	<tr>
		<th colspan="2" rowspan="2">Inputs</th>
		<th colspan="4">IN2 IN1</th>
	</tr>
	<tr>
		<th>00</th>
		<th>01</th>
		<th>10</th>
		<th>11</th>
	</tr>
	<tr>
		<th rowspan="4">IN4<br>IN3</th>
		<th>00</th>
		<td>X<sub>1</sub></td>
		<td>X<sub>2</sub></td>
		<td>X<sub>3</sub></td>
		<td>X<sub>4</sub></td>
	</tr>
	<tr>
		<th>01</th>
		<td>X<sub>5</sub></td>
		<td>X<sub>6</sub></td>
		<td>X<sub>7</sub></td>
		<td>X<sub>8</sub></td>
	</tr>
	<tr>
		<th>10</th>
		<td>X<sub>9</sub></td>
		<td>X<sub>10</sub></td>
		<td>X<sub>11</sub></td>
		<td>X<sub>12</sub></td>
	</tr>
	<tr>
		<th>11</th>
		<td>X<sub>13</sub></td>
		<td>X<sub>14</sub></td>
		<td>X<sub>15</sub></td>
		<td>X<sub>16</sub></td>
	</tr>
</table>

Which is represented as the following payload byte format (MSB first):

<table>
	<tr>
		<th colspan="8">Byte 2</th>
		<th colspan="8">Byte 3</th>
	</tr>
	<tr>
		<td>X<sub>16</sub></td>
		<td>X<sub>15</sub></td>
		<td>X<sub>14</sub></td>
		<td>X<sub>13</sub></td>
		<td>X<sub>12</sub></td>
		<td>X<sub>11</sub></td>
		<td>X<sub>10</sub></td>
		<td>X<sub>9</sub></td>
		<td>X<sub>8</sub></td>
		<td>X<sub>7</sub></td>
		<td>X<sub>6</sub></td>
		<td>X<sub>5</sub></td>
		<td>X<sub>4</sub></td>
		<td>X<sub>3</sub></td>
		<td>X<sub>2</sub></td>
		<td>X<sub>1</sub></td>
	</tr>
</table>


## Pin Interrupts

CANbus messages can be sent in response to pin change events by enabling a pin interrupt. One interrupt can be configured for each pin. Messages will be sent to the device ID provided with the configuration command.

The following interrupt modes are available:

<table>
	<tr>
		<th>Mode</th>
		<th>Value</th>
	</tr>
	<tr>
		<td>Disabled</td>
		<td>0</td>
	</tr>
	<tr>
		<td>Rising Edge</td>
		<td>1</td>
	</tr>
	<tr>
		<td>Falling Edge</td>
		<td>2</td>
	</tr>
	<tr>
		<td>Any Change</td>
		<td>3</td>
	</tr>
</table>


## RGB Strip

Two addressable RGB strips can be controlled via CANbus messages. Only WS2812-compatible LEDs which use the GRB color format can be controlled.

The following pattern modes are available:

<table>
	<tr>
		<th>Mode</th>
		<th>Value</th>
	</tr>
	<tr>
		<td>Disabled</td>
		<td>0</td>
	</tr>
	<tr>
		<td>Solid Color</td>
		<td>1</td>
	</tr>
	<tr>
		<td>Rainbow</td>
		<td>2</td>
	</tr>
</table>


## Development

This project uses PlatformIO.


### Compiling

For actual hardware:

```
pio run
```

For ST Nucleo-144 dev board:

```
pio run -e nucleo

```

### Flashing

Connect an ST-LINK to the programming header, apply 5V power, and run:

For actual hardware:

```
pio run --target upload
```

For ST Nucleo-144 dev board:

```
pio run -e nucleo --target upload

```
