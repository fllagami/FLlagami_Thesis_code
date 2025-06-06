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


def setup(capture_file):
    ports = UART.find_sniffer()

    if len(ports) > 0:
        sniffer = Sniffer.Sniffer(portnum=ports[0], baudrate=1000000, capture_file_path=capture_file)
        sniffer.setSupportedProtocolVersion(2)

    else:
        print("No sniffers found!")
        return

    sniffer.start()
    return sniffer


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

    return devices


def _find_device_by_address(devices, address):
    for device in devices.asList():
        if _address_to_string(device) == address.replace(':', ''):
            return device


def follow(sniffer, device, irk=None):
    sniffer.follow(device)
    if irk is not None:
        sniffer.sendIRK(irk)
    loop(sniffer)


def loop(sniffer):
    # Enter main loop
    loops = 0
    packet_count = 0
    while True:
        time.sleep(0.1)
        # Get (pop) unprocessed BLE packets.
        packets = sniffer.getPackets()
        packet_count = packet_count + len(packets)
        loops += 1

        # print diagnostics every so often
        if loops % 20 == 0:
            print("inConnection", sniffer.inConnection)
            print("currentConnectRequest", sniffer.currentConnectRequest)
            print("packetsInLastConnection", sniffer.packetsInLastConnection)
            print("packetCount", packet_count)
            print()


def main():
    parser = argparse.ArgumentParser(description='Sniff Bluetooth LE traffic over the air.')
    subparsers = parser.add_subparsers(dest='command', required=True)
    sniff_parser = subparsers.add_parser('sniff', help='Start sniffing a Bluetooth LE device. Sniffing requires either '
                                                       '--address or --name to be given. Saves the captured packets to '
                                                       'capture file')
    subparsers.add_parser('scan', help='Scans for devices and outputs list of address/name pairs of advertising '
                                       'Bluetooth LE devices. Also saves the advertising packets to the capture file.')
    group = sniff_parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--address', '-a',
                        type=str,
                        required=False,
                        dest='address',
                        help='Start sniffing the Bluetooth LE device by address.')
    group.add_argument('--irk',
                        type=str,
                        required=False,
                        dest='irk',
                        help='Start sniffing the Bluetooth LE device by IRK. Expects big-endian, hexadecimal format.')
    group.add_argument('--name', '-n',
                        type=str,
                        required=False,
                        dest='name',
                        help='Start sniffing the Bluetooth LE device by name.')
    parser.add_argument('--capture-file', '-c',
                        type=str,
                        required=False,
                        dest='capture_file',
                        help='Name of the file where the sniffer writes the captured Bluetooth LE packets. The file '
                             'can be opened with Wireshark.',
                        default='captureC2.pcap')
    args = parser.parse_args()

    sniffer = setup(args.capture_file)

    if not sniffer:
        return

    if args.command == 'scan':
        devices = scan(sniffer, 5)
        for dev in devices.asList():
            address = _address_to_string(dev)
            name = dev.name.replace('"', '')
            print(f'{address} {name}')
    elif args.command == 'sniff':
        if args.irk is None:
            device = scan(sniffer, 5, args.address, args.name)
            follow(sniffer, device)
        else:
            sniffer.scan()
            device = Devices.Device(address=[], name='""', RSSI=0)
            follow(sniffer, device, _hex_to_byte_list(args.irk))
    else:
        raise AssertionError(f'Unknown command: {args.command}')


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
