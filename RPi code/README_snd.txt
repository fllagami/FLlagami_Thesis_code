This program is the CLI to the sensor dongles.
Place this file in the same directory as the nrf sniffer code (file nrf).
In the beginning of file set deviceName to the name of the emulated sensor used on the raspberry pi.

All previously registered .pcap sniffer files are deleted when this program is started.

commands:
-a
	start advertising. First activation of this automatically starts up a sniffer dongle if one is available. File is saved with name "cap[device name]_[id].pcap"
	
-s
	start scanning for sensor dongles.
-h
	show connected sensor names and their handle number.
-d [handleNum]
	disconnect from handle number handleNum