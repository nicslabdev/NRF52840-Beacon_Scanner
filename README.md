# nRF52840 - Beacon scanner #

This is a Bluetooth 5 advertisements' scanner developed over [nRF52840 Preview Development Kit](https://www.nordicsemi.com/eng/Products/nRF52840-DK).

This application captures advertisements with extended advertising characteristic activated and with BT5 PHYs: 2Mbps, 1Mbps and PHY Coded. By default, this application will show PHY Coded advertisements.

The application sends the captured advertisements through serial port.

This repository is part of [MOTAM project](https://www.nics.uma.es/projects/motam), but you can use beacon_scanner for general purposes.

## Requeriments

Currently, this project uses:
-   nRF5 SDK version 15.1.0
-   S140 SoftDevice v6.1.0 API
-   nRF52840 PDK (PCA10056)

## Get started

To compile it, put the folders inside nRF52 into the \nRF5_SDK_15.1.0\examples\ble_central folder.

In order to stablish serial connection with your PC, you can use PUTTY if you are a Windows user or GNU Screen if you are a Linux user.

**PUTTY connection parameters:**

    Baud rate: 115.200
    8 data bits
    1 stop bit
    No parity
    HW flow control: None

**GNU screen**

    sudo screen /dev/ttyACM0 115200
    
Where */dev/ttyACM0* is the nRF52840 PDK device. You can know what is the path of your nRF52840 connecting it and executing *dmesg* in the Linux terminal.

## Applications' description

- **beacon_scanner:** This application is for general purposes. It simply reads each beacon advertisements and prints it by serial port. Currently this only reads PHY Coded advertisements (long range advertisements), but you can easily change the used PHY from code.

- **MOTAM_scanner:** Reads advertisements and filters it in order to show by serial port only the MOTAM beacons. In this case, the nRF52840 will be connected by USB port to the [MOTAM gateway](https://github.com/nicslabdev/MOTAM-Gateway). MOTAM beacons report on the state of the environment, so the gateway will collect this information.
