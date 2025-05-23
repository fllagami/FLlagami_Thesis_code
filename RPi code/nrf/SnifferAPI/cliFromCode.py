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

class SnifferThread(threading.Thread):
    def __init__(self, name="Cent1"):
        super().__init__()
        self.name = name
        self.sniffer = None
        self.stop_event = threading.Event()

    def stop(self):
        self.stop_event.set()
# name = "Cent1"
    def setup(capture_file):
        ports, devices = UART.find_sniffer()
        
        if len(ports) > 0:
            file = "cap" + self.name + "_" + + devices[0][-1] + ".pcap"
            self.sniffer = Sniffer.Sniffer(portnum=ports[0], baudrate=1000000, capture_file_path=file)#capture_file_path=capture_file)
            self.sniffer.setSupportedProtocolVersion(2)

        else:
            print("No sniffers found!")
            return

        self.sniffer.start()
        return self.sniffer


    def follow(self, device):
        self.sniffer.follow(device)
        loop(self.sniffer)


    def loop(self):
        # Enter main loop
        loops = 0
        packet_count = 0
        while not self.stop_event.is_set():
            time.sleep(0.1)
            # Get (pop) unprocessed BLE packets.
            packets = self.sniffer.getPackets()
            packet_count = packet_count + len(packets)
            loops += 1

            # print diagnostics every so often
            if loops % 100 == 0:
                # print("inConnection", sniffer.inConnection)
                # print("currentConnectRequest", sniffer.currentConnectRequest)
                # print("packetsInLastConnection", sniffer.packetsInLastConnection)
                print(f"packetCount for {self.name}", packet_count)
                print()
        # self.sniffer.stop()


    def run(self):
        sniffer = setup("")
        if not sniffer:
            return
        device = scan(sniffer, 5, None, name)
        follow(sniffer, device)

    def _address_to_string(self,dev):
        return ''.join('%02x' % b for b in dev.address[:6])


    def _hex_to_byte_list(self,value):
        if value.startswith('0x'):
            value = value[2:]

        if len(value) % 2 != 0:
            value = '0' + value

        a = list(value)
        return [int(x + y, 16) for x, y in zip(a[::2], a[1::2])]
