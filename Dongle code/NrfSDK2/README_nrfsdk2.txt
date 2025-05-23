Donwload the NrfSDK2 folder in the device's main storage driver, e.g. C:/.
Do not use white spaces when naming folders, may cause path problems.

The written thesis code is found in:
	C:\NrfSDK2\examples\dongle_usb_crc_acm\pca10059\mbr\ses\usbd_cdc_acm_pca10059.emProject

Specific files can be accessed with normal IDE-s but to be able to make changes or more easily inspect the code you should install Segger v5.42.
	https://www.segger.com/downloads/embedded-studio/

If NrfSDK2 folder is stored not in C:/ or is contained in a folder, you need to change the paths of required files:
	1) Open the project in Segger.
	2) In Project Items (file panel in left side), right click on Project 'usb_cdc_acm_pca10059'
	3) Click options.
	4) Top right corner of opened window, select Common. On the left side go to Code->Preprocessor.
	5) Open user include directories.
	6) Copy all. Paste in a text file. Replace all "C:/" with the path of the NrfSDK2 folder.
	7) The same in Build->Project Macros. (Do this only if you get an error before trying with only 	   previous.)

To build the project:
	1) Select Release in the box under Project explorer on top left corner, above Project items.
	2) Click F7 or Build-> Build_usb_cdc_acm_pca10059

All necessary files are in the folder Application in the Project Items.
To change device name, choose which sensor device you want to emulate:
	1) Go to sdk_config.h
	2) Lines 56-88, Uncomment the line block with the desired DEVICE_NAME, comment out previous choice.

To use default communications:
	1) Go to device_conn.c
	2) Line 7, set bool doStar=true
	3) In sdk_config.h, Line 51 set #define doStarCount to the desired number of devices the sink node will connect to. For all devices use 7.

To use proposed methods:
	1) Go to device_conn.c, Line 7, set bool doStar=false

To be able to program the dongles:
	1) install NRF connect for desktop
		https://docs.nordicsemi.com/bundle/nrf-connect-desktop/page/download_cfd.html
	2) Open it and install the Programmer app

To program dongles:
	1) plug in dongle and press a sideways button, should be on the opposite side of the usb part, facing 	   in the different direction. Dongle led should be blinking red afterwards.
	2) Choose the dongle on the select device
	3) Add files:
		(Necessary low-level machine code)
		C:\NrfSDK2\components\softdevice\s140\hex\s140_nrf52_7.2.0_softdevice.hex
		(Dongle application)
		C:\NrfSDK2\examples\dongle_usb_crc_acm\pca10059\mbr\ses\Output\Release\Exe\usbd_cdc_acm_pca10059.hex
	4) Press write. Unplug after it is done.

To use dongle:
	1) Start a SSH connection to the Raspberry Pi
	2) Plug in Raspberry Pi.
	2) Open the terminal and run python3 snd.py.