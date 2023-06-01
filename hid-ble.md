```
1. logging is thread safe, and can be called in interrupt
2. delete bound after changing services or chars
3. bt name: bt_le_adv_start->bt_le_adv_start_legacy->le_adv_update->bt_get_name
4. LOGICAL_MINIMUM to LOGICAL_MAXIMUM (linear interpolate)--> PHYSICAL_MINIMUM to PHYSICAL_MAXIMUM
5. hid report must be inside one collection(application)
```


|          Service           | Requirement |
| :------------------------: | :---------: |
|        HID Service         |      M      |
|      Battery Service       |      M      |
| Device Information Service |      M      |
|  Scan Parameters Service   |      O      |

```
all services should be Primary Service

The HID Device shall use LE Security Mode 1 and either Security Level 2 or 3

adv requirements:
    1.service UUIDs AD Type
    2.local name AD Type
    3.appearance AD Type
```

## BAS
```
Battery Level Characteristics is mandatory;
Battery Level descripto is mandatory if a device has more than one instance of Battery Service;
all other chars are optional.

Read Characteristic Descriptors are mandatory.
```

## DIS 
PnP ID Characteristics is mandatory by HID spec
```
Mandatory Characteristics
The Device Information Service shall include the PnP ID characteristic for reading the
PnP ID fields for the HID Device.
```

### PnP ID 0x2a50
```
1.Vendor ID Source 
    0x01: bluetooth sig assigned
    0x02: usb assigned

2. VID,PID
    from usb, 2 bytes

3. Product Version
    0xJJMN for version JJ.M.N (JJ – major version number, M – minor version number, N – sub-minor version number)
```

### Model Nuber String and Manufacturer Name String
included in zephyr

## HID

|        GATT Sub-Procedure        | Requirement |
| :------------------------------: | :---------: |
|  Read Long Characteristic Value  |      M      |
|      Write Without Response      |      M      |
|    Write Characteristic Value    |      M      |
|          Notifications           |      M      |
| Read Characteristic Descriptors  |      M      |
| Write Characteristic Descriptors |      M      |

### chars
|     Characteristic Name     | Requirement |       Mandatory Properties        | Optional Properties | Security Permissions |
| :-------------------------: | :---------: | :-------------------------------: | :-----------------: | :------------------: |
|        Protocol Mode        |     C.4     |    Read /WriteWithoutResponse     |                     |         None         |
|           Report            |      O      |                                   |                     |                      |
|  Report: Input-Report Type  |     C.1     |            Read/Notify            |        Write        |         None         |
| Report: Output-Report Type  |     C.1     | Read/Write/Write Without Response |                     |         None         |
| Report: Feature Report Type |     C.1     |            Read/Write             |                     |         None         |
|         Report Map          |      M      |               Read                |                     |         None         |
| Boot Keyboard Input Report  |     C.2     |            Read/Notify            |        Write        |         None         |
| Boot Keyboard Output Report |     C.2     | Read/Write/Write Without Response |                     |         None         |
|   Boot Mouse Input Report   |     C.3     |            Read/Write             |                     |         None         |
|       HID Information       |      M      |               Read                |                     |         None         |
|      HID Control Point      |      M      |       WriteWithoutResponse        |                     |         None         |

```
C.1: Mandatory to support at least one Report Type if the Report characteristic is supported
C.2: Mandatory for HID Devices operating as keyboards, else excluded.
C.3: Mandatory for HID Devices operating as mice, else excluded.
C.4: Mandatory for HID Devices supporting Boot Protocol Mode, otherwise optional. 

little endian
```

```
Protocol Mode: boot or report
HID Information： 2 octets bcdHID, 1 octet country code, 1 octet flag: bit0-remotewakeup, bit1-adv when bound but not connected
HID Control Point: suspend(0) and exit-suspend(1)
```

Report:
|  Report Type   | Requirement | Read  | Write | Write Without Response | Notify |
| :------------: | :---------: | :---: | :---: | :--------------------: | :----: |
|  Input Report  |     C.1     |   M   |   O   |           X            |   M    |
| Output Report  |     C.1     |   M   |   M   |           M            |   X    |
| Feature Report |     C.1     |   M   |   M   |           X            |   X    |
```
1.Client Characteristic Configuration Descriptor
    chars with input report

2.Report Reference Characteristic Descriptor
    Report ID(1 octet) and Report Type(bit1-input; bit2-output; bit3-feature)
    in each report characteristic for Report Protocol Mode.
```


## Windows XUSB map to HID
LT and RT couldn't trigger simultaneously.
|           Control            | HID Usage Name | Usage Page |  Usage ID  |
| :--------------------------: | :------------: | :--------: | :--------: |
|          Left Stick          |      X, Y      |    0x01    | 0x30, 0x31 |
|         Right Stick          |     Rx, Ry     |    0x01    | 0x33, 0x34 |
| Left Trigger + Right Trigger |       Z*       |    0x01    |    0x32    |
| D-Pad Up, Down, Left, Right  |   Hat Switch   |    0x01    |    0x39    |
|              A               |    Button 1    |    0x09    |    0x01    |
|              B               |    Button 2    |    0x09    |    0x02    |
|              X               |    Button 3    |    0x09    |    0x03    |
|              Y               |    Button 4    |    0x09    |    0x04    |
|       LB (left bumper)       |    Button 5    |    0x09    |    0x05    |
|      RB (right bumper)       |    Button 6    |    0x09    |    0x06    |
|             BACK             |    Button 7    |    0x09    |    0x07    |
|            START             |    Button 8    |    0x09    |    0x08    |
|   LSB (left stick button)    |    Button 9    |    0x09    |    0x09    |
|   RSB (right stick button)   |   Button 10    |    0x09    |    0x0A    |


## MSP430 example
```
UsagePage(USB_HID_GENERIC_DESKTOP),
Usage(USB_HID_JOYSTICK),
Collection(USB_HID_APPLICATION),
//
// The axis for the controller.
//
UsagePage(USB_HID_GENERIC_DESKTOP),
Usage (USB_HID_POINTER),
Collection (USB_HID_PHYSICAL),

    //
    // The X, Y and Z values which are specified as 8-bit absolute
    // position values.
    //
    Usage (USB_HID_X),
    Usage (USB_HID_Y),
    Usage (USB_HID_Z),
    Usage (USB_HID_RX),
    Usage (USB_HID_RY),
    Usage (USB_HID_RZ),
    Usage (USB_HID_SLIDER),
    Usage (USB_HID_DIAL),
    //
    // 8 16-bit absolute values.
    //
    ReportSize(16),
    ReportCount(8),
    Input(USB_HID_INPUT_DATA | USB_HID_INPUT_VARIABLE |
          USB_HID_INPUT_ABS),

    //
    // Max 32 buttons.
    //
    UsagePage(USB_HID_BUTTONS),
    UsageMinimum(1),
    UsageMaximum(NUM_BUTTONS),
    LogicalMinimum(0),
    LogicalMaximum(1),
    PhysicalMinimum(0),
    PhysicalMaximum(1),

    //
    // 8 - 1 bit values for the buttons.
    //
    ReportSize(1),
    ReportCount(32),
    Input(USB_HID_INPUT_DATA | USB_HID_INPUT_VARIABLE |
          USB_HID_INPUT_ABS),

     //
     // Max 16 indicator bits
     //
     UsagePage(USB_HID_BUTTONS),
     UsageMinimum(1),
     UsageMaximum(NUM_INDICATORS),
     LogicalMinimum(0),
     LogicalMaximum(1),
     PhysicalMinimum(0),
     PhysicalMaximum(1),

     //
     // 8 - 1 bit values for the leds.
     //
     ReportSize(1),
     ReportCount(16),
     Output(USB_HID_INPUT_DATA | USB_HID_INPUT_VARIABLE |
           USB_HID_INPUT_ABS),

EndCollection,
EndCollection
```


little endian
|    left X 2Bytes     |    left Y 2Bytes     |  right X    2Bytes   |   right Y  2Bytes    | LT 2Bytes | RT 2Bytes |        D-pad 1Byte        |
| :------------------: | :------------------: | :------------------: | :------------------: | :-------: | :-------: | :-----------------------: |
| 0~0xffff left->right | 0~0xffff top->bottom | 0~0xffff left->right | 0~0xffff top->bottom |  0~0x3ff  |  0~0x3ff  | 1~8 top clockwise; 0-idle |

3 bytes buttons
```
B[0]
LB--40h
RB--80h
X---08h
Y---10h
A---01h
B---02h

B[1]
XBOX--10h
Lop---04h
Rop---08h
L3----20h
R3----40h

B[2]
reserved
```

## todos
1.bonds & direct adv -- ok
2.logging
3.settings subsystem
4.usb subsystem
5.boards
6.boards low energy
7.mcuboot
