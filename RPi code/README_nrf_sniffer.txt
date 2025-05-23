to install:
I forgot to reset the package so first you should remove build components.
Go to package location and run in the terminal:
	rm -rf build dist *.egg-info

Place the folder in the same directory as snd.py and run:
	pip install --break-system-packages -e [path to sniffer folder]


to sniff an advertising dongle run in a terminal at the directory:
	nrf sniff --n [name]
sniffed packets are saved with name "cap_[device name]_[id].pcap"