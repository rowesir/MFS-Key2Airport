# Key2Airport

🌐 [简体中文](docs/README_cn.md) · [English](README.md)

K2A is a key-binding utility for Microsoft Flight Simulator.
It listens for keyboard, mouse, and game-controller input, then sends aircraft
button events according to a JSON configuration.

## 1. Connecting and basic use

1. Start Microsoft Flight Simulator.
2. Start K2A.
3. Choose how to load the configuration:
   - Enable automatic configuration to let the program select the JSON file for the current aircraft.
   - Disable automatic configuration and select a JSON file from the drop-down list.
4. Click `Connect` before entering Career Mode, Free Flight, or another actual flight.
5. Once the status becomes `Aircraft Loaded`, the `Enum` and `Test` buttons are available.

### Configuration and reload

Configuration files are stored in the `config` folder next to the program. Click
`Folder` to open this folder.

When automatic configuration is enabled and no JSON file exists for the current
aircraft, the program creates a basic JSON template automatically. Edit that
file to add your Hash values and key bindings.

After editing a configuration file, click the refresh button (the circular-arrow
icon) while in flight. The program will reload the configuration. In automatic
mode it reloads the file for the current aircraft; in manual mode it rescans the
folder and reloads the selected file.

Status meanings:

| Status | Meaning |
| --- | --- |
| `Standby...` | Not connected |
| `Waiting MFS...` | Waiting for the simulator or reconnecting |
| `Connected` | Connected to the simulator, but not in an actual flight |
| `Aircraft Loaded` | An actual flight has started |

Input listening starts when the program starts. Press a keyboard or peripheral
button and the main window will show its input name, for example:

```text
V
NUM 7
L CTRL+B
Mouse Wheel Up
```

The key name in the JSON configuration must exactly match the name shown by the
program.

### Avoid duplicate in-game bindings

Keys used by this tool should first be unassigned from the corresponding
controls in the simulator. Otherwise the simulator's original binding and
K2A may respond to the same key at the same time.

For a complete example, see the `SF50` template included in the release
package. Hash values may differ between users, so verify them in your own
environment. The template demonstrates the following bindings:

| Configuration | Physical key | Action |
| --- | --- | --- |
| Page switch | `NUM -` | Switch between configuration pages |
| Page 1 | `L CTRL+V` | Press the VNAV button |
| Page 1 | `[` | Rotate the barometric-pressure knob left |
| Page 1 | `]` | Rotate the barometric-pressure knob right |
| Page 1 | `L SHIFT+[` | Press the barometric-pressure knob |
| Page 2 | `B` | Turn the landing lights on or off |

After the flight ends, click `Disconn` to disconnect. If you disconnect or close
the window while still in flight, the program will ask for confirmation.

## 2. RA and LR

| Option | Purpose |
| --- | --- |
| `RA` (Radio Altitude) | Reads and displays radio altitude; voice callouts are also available when an audio device is present |
| `LR` (Landing Rate) | Records and displays the landing rate at touchdown |

These options can be set before connecting, or given default values in the JSON
configuration:

```json
"RA_CHECKBOX": true,
"LR_CHECKBOX": true
```

## 3. Enum, Listen, and Test

`Enum` and `Test` become available after the program enters an actual flight and
the status shows `Aircraft Loaded`.

Recommended workflow:

1. Use `Enum` to find event names and Hash values for the current aircraft.
2. Use `Listen` while operating a button in the simulator to confirm its Hash, parameter type, and value.
3. Use `Test` to read or send a value and verify that the event works.
4. Put the confirmed Hash and value into the JSON configuration.

### Enum

Click `Enum` to enumerate all InputEvents available for the current aircraft.

The full enumeration table shows each event name and its Hash. The `Filtra`
field filters by event name in real time and is case-insensitive.

### Listen

Opening the Enum window also starts event listening. Operate a button, switch,
or other control in the simulator and inspect the corresponding entry in the
Listen table:

- Hash
- Parameter type
- Parameter value
- Parameter size
- Latest change time

Use Enum to find the event name, then use Listen to observe the Hash and
parameters produced by the actual simulator operation.

### Test

Use `Test` to check a Hash:

1. Enter the decimal Hash.
2. Click the read button to view its current value.
3. Enter a value and click the send button to test whether the event accepts it.

Test and JSON rules currently focus on InputEvents whose parameter type is
`DOUBLE`.

### Test environment

- Microsoft Flight Simulator version: `1.8.16.0`
- Microsoft Flight Simulator SDK version: `1.7.3`

## 4. JSON configuration

Configuration files are stored in the `config` folder next to the program. They
must be standard JSON; `//` and `/* */` comments are not supported.

A minimal complete configuration looks like this:

```json
{
  "AIRCRAFT": "SF50",
  "RA_CHECKBOX": true,
  "LR_CHECKBOX": true,
  "PAGESWITCH": "NUM -",
  "VARIABLES": [
    { "vnav": "1234567890" }
  ],
  "PAGE_1": {
    "V": [
      { "THEN": "vnav = 1" }
    ]
  },
  "PAGE_2": {}
}
```

### Top-level fields

| Field | Purpose |
| --- | --- |
| `AIRCRAFT` | Aircraft associated with this configuration |
| `RA_CHECKBOX` | Whether RA is enabled by default |
| `LR_CHECKBOX` | Whether LR is enabled by default |
| `PAGESWITCH` | Key used to switch pages; leave it empty to disable page switching |
| `VARIABLES` | Maps readable variable names to InputEvent Hash values |
| `PAGE_1` | Key bindings for page 1 |
| `PAGE_2` | Key bindings for page 2 |

### VARIABLES

Store each Hash as a string and give it a readable variable name. Hash values
can be obtained from Enum, Listen, or Test.

```json
"VARIABLES": [
  { "vnav": "1234567890" },
  { "hdg": "9876543210" }
]
```

Variable names may contain letters, digits, and underscores, but must not start
with a digit. You can then use `vnav` and `hdg` in rules instead of repeating
the Hash values.

### Keys and rules

The fields inside `PAGE_1` and `PAGE_2` are input names, and their values are
arrays of rules. The input name must exactly match the name shown in the main
window:

```json
"V": [
  {
    "THEN": "vnav = 1"
  }
]
```

When `V` is pressed, the program sends the value `1` to the Hash assigned to
`vnav`.

Each rule can contain these fields:

| Field | Purpose |
| --- | --- |
| `IF` | Optional condition; without `IF`, `THEN` is executed directly |
| `THEN` | Actions to run when the condition is true |
| `ELSE` | Optional actions to run when the condition is false |

A key may contain multiple rules. They run from top to bottom in the order in
which they appear in the JSON file.

### Conditions

Conditions can use variables, numbers, comparison operators, and logical
operators:

```text
==   >=   >   <=   <
&&   ||
```

Example:

```json
"IF": "vnav == 1 && hdg == 0"
```

Parentheses are supported. Evaluation order is: parentheses, comparisons,
`&&`, then `||`.

`!=` and arithmetic expressions such as `+`, `-`, `*`, and `/` are not
supported.

### Actions

An action uses the form `variable = number`:

```text
vnav = 1
```

Separate multiple actions with semicolons. They run from left to right:

```json
"THEN": "vnav = 0; hdg = 1"
```

The final semicolon is optional. The right-hand side must be a number; another
variable or an arithmetic expression cannot be used there.

### Common examples

Send a fixed value:

```json
"V": [
  { "THEN": "vnav = 1" }
]
```

Toggle a switch according to its current value:

```json
"NUM 7": [
  {
    "IF": "gearLight == 1",
    "THEN": "gearLight = 0",
    "ELSE": "gearLight = 1"
  }
]
```

Set the page-switch key:

```json
"PAGESWITCH": "NUM -"
```

Pressing `NUM -` switches between `PAGE_1` and `PAGE_2`.
Using different configuration pages allows the same physical key to be bound
to different aircraft buttons or knobs.

## Notes

- Connect before entering an actual flight. If you connect after the flight has started, the program may not receive that flight-start notification.
- Configuration reads and writes currently focus on `DOUBLE` InputEvents.
- For many aircraft knobs, you can try sending `-1` for left and `1` for right after finding the corresponding Hash. Behavior may vary between aircraft and knobs.
- Some lever switches, such as lights, use `0` for off and `1` for on. Use an `IF` condition to toggle them when binding them to one key:

  ```json
  "IF": "light == 1",
  "THEN": "light = 0",
  "ELSE": "light = 1"
  ```

- Most momentary push-button switches can be toggled by sending `1`, but users should verify the behavior for each aircraft.
- The software has currently only been tested with the Cirrus G2 (SF50) in Microsoft Flight Simulator 2024. Other aircraft and third-party aircraft add-ons have not been tested.
- Hash values, input names, and variable names must be correct for a rule to work.
