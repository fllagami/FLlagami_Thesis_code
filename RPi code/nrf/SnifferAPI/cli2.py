# Copyright (c) Nordic Semiconductor ASA
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form, except as embedded into a Nordic
#    Semiconductor ASA integrated circuit in a product or a software update for
#    such product, must reproduce the above copyright notice, this list of
#    conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of Nordic Semiconductor ASA nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
# 4. This software, with or without modification, must only be used with a
#    Nordic Semiconductor ASA integrated circuit.
#
# 5. Any software provided in binary form under this license must not be reverse
#    engineered, decompiled, modified and/or disassembled.
#
# THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
import argparse
import time
from SnifferAPI import Sniffer, UART, Devices

# name = "DEMO"
portName = ""
def setup(capture_file, name):
    global portName
    ports, devices = UART.find_sniffer()
    
    if len(ports) > 0:
        file = "cap" + name + "_" + devices[0][-1] + ".pcap"
        portName = "" + devices[0][-1]
        print("port name ="  + portName +"!")
        sniffer = Sniffer.Sniffer(portnum=ports[0], baudrate=1000000, capture_file_path=file)#capture_file_path=capture_file)
        sniffer.setSupportedProtocolVersion(2)

    else:
        print("No sniffers found!")
        return

    sniffer.start()
    return sniffer

def setup2(name):
    global portName
    ports, devices = UART.find_sniffer()
    sniffPorts = []
    sniffers = []
    if len(ports) > 0:
        for i in range(len(ports)):
            file = "cap" + name + "_" + devices[i][-1] + ".pcap"
            # threadIdx.append(i)
            sniffer = Sniffer.Sniffer(portnum=ports[i], baudrate=1000000, capture_file_path=file)#capture_file_path=capture_file)
            sniffers.append(sniffer)
            sniffPorts.append(""+devices[i][-1] )
            sniffer.setSupportedProtocolVersion(2)
            sniffer.start()
    else:
        print("No sniffers found!")
    return sniffers, sniffPorts

def scan(sniffer, timeout, address=None, name=None):
    sniffer.scan()
    devices = None
    for _ in range(timeout):
        time.sleep(1)
        devices = sniffer.getDevices()
        if address is not None:
            device = _find_device_by_address(devices, address)
            if device is not None:
                return device

        elif name is not None:
            device = devices.find(name)
            if device is not None:
                return device
    print(f"Device {name} not found.")

    return devices


def _find_device_by_address(devices, address):
    for device in devices.asList():
        if _address_to_string(device) == address.replace(':', ''):
            return device


def follow(sniffer, device, name, irk=None):
    sniffer.follow(device)
    if irk is not None:
        sniffer.sendIRK(irk)
    loop(sniffer, name)


def loop(sniffer, name):
    # Enter main loop
    connection = False
    master = "None"
    loops = 0
    notcon = 10
    packet_count = 0
    print(portName + " --- kot ")
    print(f"{portName} --- kot ")
    while True:
        time.sleep(0.2)
        # Get (pop) unprocessed BLE packets.
        packets = sniffer.getPackets()
        packet_count = packet_count + len(packets)
        loops += 1

        # print diagnostics every so often
        if loops % notcon == 0:
            # print(portName + " --- kot ")
            # print(f"{portName} --- kot ")
            if not connection and connection == sniffer.inConnection:
                print(f"[{portName}] NOT Connected {sniffer.inConnection} with {master}, packetCount {packet_count}")
            elif (connection != sniffer.inConnection):
                connection = sniffer.inConnection
                print()
                if connection:
                    print(f"[{portName}] sniffer MSTRaddr is -{sniffer.masterAddr}-")
                    # if sniffer.masterAddr.equals("E1:76:52:04:1D:36"):
                    if sniffer.masterAddr.startswith("EC") and sniffer.masterAddr.endswith("DB"):# == "E1:76:52:04:1D:36":
                        new_master = "Calmr" #ec:c8:fb:5f:b4:db
                    # elif sniffer.masterAddr.equals("F4:7D:CD:C4:18:25"):
                    elif sniffer.masterAddr.startswith("E9") and sniffer.masterAddr.endswith("F0"):# == "F4:7D:CD:C4:18:25":
                        new_master = "PHONE"#e9:61:ae:74:5f:f0
                    else:
                        new_master = "OTHER"
                    if not new_master == master:
                        print(f"[{portName}] ##[Master changed from {master} to {new_master}]##")

                    master = new_master
                    print(f"[{portName}] ##[CONNECTED to Master {master}]## inConnection {sniffer.inConnection}, packetCount {packet_count}")
                else:
                    notcon = 30
                    if master.startswith("N"):# == None:
                        print(f"[{portName}] ##[NOT connected anymore]## inConnection {sniffer.inConnection}, packetCount {packet_count}")
                    else:
                        print(f"[{portName}] ##[NOT connected anymore to Master {master}]## inConnection {sniffer.inConnection}, packetCount {packet_count}")
                print()
            elif loops % 30 == 0:
                print()
                if sniffer.inConnection:
                    print(f"[{portName}] inConnection {sniffer.inConnection} with {master}, packetCount {packet_count}")
                # else:
                #     print(f"[{portName}] NOT Connected {sniffer.inConnection} with {master}, packetCount {packet_count}")
            # print("currentConnectRequest", sniffer.currentConnectRequest)
            # print("packetsInLastConnection", sniffer.packetsInLastConnection)
            # print("packetCount", packet_count)
                print()


def main_toCall(deviceName):
    # global name 
    name = deviceName
    sniffer = setup("", name)

    if not sniffer:
        return

    device = scan(sniffer, 240, None, name)
    follow(sniffer, device, name)


def _address_to_string(dev):
    return ''.join('%02x' % b for b in dev.address[:6])


def _hex_to_byte_list(value):
    if value.startswith('0x'):
        value = value[2:]

    if len(value) % 2 != 0:
        value = '0' + value

    a = list(value)
    return [int(x + y, 16) for x, y in zip(a[::2], a[1::2])]


if __name__ == "__main__":
    main()
