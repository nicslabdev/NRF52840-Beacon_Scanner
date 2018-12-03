# nRF52840 - Beacon scanner #

This is a Bluetooth 5 advertisements' scanner developed for [nRF52840 Dongle](https://www.nordicsemi.com/eng/Products/nRF52840-Dongle) and [nRF52840 Development Kit](https://www.nordicsemi.com/eng/Products/nRF52840-DK).

This application captures advertisements with extended advertising characteristic activated and BT5 PHYs: 2Mbps, 1Mbps and PHY Coded.

The application sends the captured advertisements through serial port. It simply reads each beacon advertisements and prints it by serial port. Currently this only reads 1MBPS PHY advertisements, but you can easily change the used PHY from code.~~ (this will be updated soon)

This repository is the base of [MOTAM-Scanner](https://github.com/nicslabdev/MOTAM-Scanner).

## Requeriments

This project uses:
-   nRF5 SDK version 15.2.0
-   S140 SoftDevice v6.1.0 API
-   nRF52840 Dongle (PCA10059) or nRF52840 PDK (PCA10056)

## Get started

You can find a hex folder with the **precompiled applications** for PCA10056 and PCA10059.

In order to program the application on the nRF52840, you can use [nRF Connect application for Desktop](https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF-Connect-for-Desktop) from Nordic Semiconductor or nrfjprog (just in case you are using nRF52840 Development Kit).

> Note: This application uses S140 SoftDevice, so don't forget program
> SoftDevice hex file (you can find it in hex folder).

In order to stablish serial connection with your PC, you can use PUTTY if you are a Windows user or GNU Screen if you are a Linux user.

**PUTTY connection parameters:**

    Baud rate: 115.200
    8 data bits
    1 stop bit
    No parity
    HW flow control: None

**GNU screen**

    sudo screen /dev/ttyACM0 115200
    
Where */dev/ttyACM0* is the nRF52840 device. You can know what is the path of your nRF52840 connecting it and executing *dmesg* on UNIX terminal.

In case you are working on nRF52840 Development Kit:

- Use nRF USB connector (J3 connector) for **beacon scanner function**.
- Use MCU USB connector (J2 connector) in order to see logger messages.

## Compiling the applications

If you want to compile the project, you can use GCC and Eclipse. Put the downloaded folder into 
	
	\nRF5_SDK_15.2.0\examples\ble_central folder.
	
More info about using GCC and Eclipse [here](https://devzone.nordicsemi.com/tutorials/b/getting-started/posts/development-with-gcc-and-eclipse).