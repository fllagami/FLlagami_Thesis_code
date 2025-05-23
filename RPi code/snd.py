import serial
import time
import serial.tools.list_ports
import os
import threading
from queue import Queue
import termios
import fcntl
import select
import sys
from datetime import datetime
# from nrf.SnifferAPI.SnifferThread import SnifferThread
from nrf.SnifferAPI.cli2 import main_toCall

nameDevice = "Calmr"
# Get the list of files in the current directory
files = os.listdir()
# Iterate over the files
for file in files:
    # Check if the file ends with .pcap
    if file.endswith('.pcap'):
        try:
            # Delete the file
            os.remove(file)
            print(f"Deleted: {file}")
        except Exception as e:
            print(f"Error deleting {file}: {e}")
            
# Replace '/dev/ttyACM0' with the correct port for your dongle
ports = serial.tools.list_ports.comports()
numSniffers = 0
thread_names = []
for port in ports:
    print(port)
    if 'nRF52 USB' in port.description:
        print(f"Connected to Port: {port.device}, Description: {port.description}, Manufacturer: {port.manufacturer}")
        SERIAL_PORT = port.device#'/dev/ttyACM0'
    elif 'Sniffer' in port.description:
        numSniffers+=1
        thread_names.append("")

threads = []
do_sniff = True
numFreeThreads = 0
waitingEndThread = ""
th_started=[]
for i in range(numSniffers):
    snifferThread = threading.Thread(target=main_toCall, args=(nameDevice,), daemon=True)
    # snifferThread = SnifferThread(nameDevice)
    # snifferThread.daemon = True
    threads.append(snifferThread)
    th_started.append(False)
    
# time.sleep(10)
BAUD_RATE = 115200
CHUNK_SIZE = 58
SIZE_T0_SEND = 0#4000#61455#5000#72300 = 300 packets #61432 = 254 packets to send 
TOTAL_TO_SEND = 0#10000#12000
ACK = "ACK"
# Shared queue for received messages
message_queue = Queue()

# Condition variable for synchronization
condition = threading.Condition()
sent_data_size = 0
already_sent = 0
GOT_ACK = False
console_buff = ""

CHOSEN_FILE = "testbig.txt"#"testfile.txt"  "tstshort.txt"


def trigger_sound():
    # laptop_ip = "192.168.0.213"  # Replace with your actual laptop's IP
    # port = 12345  # Must match the port in the listener script

    # try:
    #     client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #     client.connect((laptop_ip, port))
    #     client.sendall(b"PLAY_SOUND")  # Send a trigger message
    #     client.close()
    #     print("Sound trigger sent to laptop!")
    # except Exception as e:
    #     print("Error sending trigger:", e)
    print("PLAY_SOUND")
isleft = False
def receive_messages():
    global already_sent, sent_data_size, GOT_ACK, SIZE_T0_SEND, TOTAL_TO_SEND, isleft
    # while True:
    try:
        # message = ser.readline().decode().strip()
        message = ser.readline().decode('utf-8', errors='replace').strip()
        if message:
            try:
                
                # os.system("ssh fjord@192.168.0.213 'powershell -c \"[System.Media.SystemSounds]::Exclamation.Play()\"'")
                if "XXXXX" in message:
                    print("XXXXX")
                    isleft = True
                elif "beep" in message:
                    if isleft:
                        print("beep LEFT")
                    else:
                        print("beep RIGHT")
                    isleft = not isleft
                else:
                    print(f"{message}")
            except BlockingIOError as e:
                print(f"Non-blocking error occurred: {e}", file=sys.stderr)
            clean_message = message.strip().lstrip('.').strip()
            # if message.startswith("Connected to dev "):
            #     if do_sniff:
            #         for i, thread in enumerate(threads):
            #             if not thread.is_alive():
            #                 done = True
            #         #         try:
            #         #             print(f"Starting sniffer thread {i}")
            #         #             thread.start()  # Start the thread
            #         #         except RuntimeError as e:
            #         #             print(f"Error starting thread {i}: {e}")
            #         #             done = False
            #                 if done:
            #                     break
                            
            #         #     # elif i == numSniffers - 1:
            #         #         # thread_names[i] = message[10:15]
            #         # # numFreeThreads-=1
            # el
            if clean_message == "Start thread":
                if do_sniff:
                    # print(f"Starting {numSniffers} threads")
                    # time.sleep(2)
                    for thread in threads:
                        if not th_started[0]:
                            print("Start")
                            thread.start()
                            th_started[0] = True
                            break
                    #     print("Sleep")
                    
                    # time.sleep(4)
                    # for thread in threads:
                    #     print("Stop")
                    #     thread.stop()
                    #     thread.join()
                    # threads[0].start()
                    # thread_names[0] = "free"
                    # numFreeThreads+=1
            # elif clean_message.startswith("Disconnected thread ") or clean_message.startswith("DISCONNECTED from central Ca"):
                # if numFreeThreads == 0 or numFreeThreads == 2:
                    
                    # print("stopped thread 1")
                    # if th_started[0]:
                    #     threads[0].stop()
                    #     th_started[0] = False
                        # endName = message[20:25]
                        # i = 0
                        # if numFreeThreads == 1:
                        #     while i < numSniffers:
                        #         if thread_names[i] == endName:
                        #             threads[i].stop()
                        #             threads[i].join()
                        #             thread_names[i] = "free"

            elif clean_message.startswith("Give me data 0 "):
                pckg_size = number = int(clean_message[15:])
                SIZE_T0_SEND = (pckg_size - 3) * 256
                TOTAL_TO_SEND = SIZE_T0_SEND * 2
                print(f"Set GOT_ACK to true SENT={already_sent}, SIZE={SIZE_T0_SEND}, TOTAL={TOTAL_TO_SEND}")
                GOT_ACK = True

                # TEST
                if(GOT_ACK):
                    print("GOT_ACK is True")
                    file_path = CHOSEN_FILE
                    if os.path.isfile(file_path):
                        print("GOT_ACK is False on function")
                        # GOT_ACK = False
                        # send_file(file_path)
                        if  TOTAL_TO_SEND > SIZE_T0_SEND + already_sent:
                            total_size_now = SIZE_T0_SEND
                            # if not GOT_ACK:    
                            print(f"SIZE {total_size_now} 1 , {already_sent}\n") 
                            ser.write(f"SIZE {total_size_now} 1\n".encode()) 
                        else:
                            total_size_now = TOTAL_TO_SEND - already_sent
                            # if not GOT_ACK:
                            print(f"SIZE {total_size_now} 0, {already_sent}\n") 
                            ser.write(f"SIZE {total_size_now} 0\n".encode())

                        with open(file_path, 'rb') as file:
                            file.seek(already_sent)
                            GOT_ACK = False
                            while True:
                                if total_size_now < sent_data_size + CHUNK_SIZE:
                                    chunk = file.read(total_size_now-sent_data_size)
                                    sent_data_size += total_size_now-sent_data_size
                                else:
                                    chunk = file.read(CHUNK_SIZE)
                                    sent_data_size += CHUNK_SIZE
                                
                                if not chunk:
                                    already_sent = already_sent + sent_data_size
                                    print(f"Broke if1 with already_sent = {already_sent}, sent_data_size = {sent_data_size}")
                                    break
                                ser.write(chunk)
                                if sent_data_size >= total_size_now:
                                    already_sent += sent_data_size
                                    print(f"Broke if2 with already_sent = {already_sent}, sent_data_size = {sent_data_size}")
                                    if TOTAL_TO_SEND == already_sent:
                                        already_sent = 0
                                    break

                        sent_data_size = 0

                        GOT_ACK = False
                    else:
                        print(f"File {file_path} not found.")
                        GOT_ACK = False
                    i = 0
                    print(f"Vuna i = {i}")
                    # break 

                
    except UnicodeDecodeError as e:
        print(f"UnicodeDecodeError: {e}. Skipping the problematic message.")


def make_console_non_blocking():
    fd = sys.stdin.fileno()
    # Save old terminal settings
    old_settings = termios.tcgetattr(fd)
    # Modify the terminal settings
    new_settings = termios.tcgetattr(fd)
    # new_settings[3] = new_settings[3] & ~termios.ICANON & ~termios.ECHO  # Disable canonical mode and echo
    # termios.tcsetattr(fd, termios.TCSADRAIN, new_settings)
    # Make stdin non-blocking
    fcntl.fcntl(fd, fcntl.F_SETFL, fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK)
    return old_settings

# Restore the console settings after the program ends
def restore_console_settings(old_settings):
    fd = sys.stdin.fileno()
    termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)


def handle_input():
    global console_buff, do_sniff
    try:
        if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
            # user_input = sys.stdin.read(1)
            user_input = sys.stdin.readline()
            if user_input:
                console_buff += user_input#.strip()
                print(f"Got input {console_buff}")
                if "\n" in console_buff:
                    command = console_buff.split("\n")[0]
                    console_buff = "" 
                    print(f"Got {command}")
                    print("Enter command:")
                    if command.startswith("-gf"):
                        ser.write("GET FILE \n".encode()) 
                    elif command.startswith("-gaf"):
                        ser.write("GET ALL FILE \n".encode()) 
                    elif command.startswith("-on"):
                        do_sniff = True
                    elif command.startswith("-off"):
                        do_sniff = False
                    elif command.startswith("-gr"):
                        ser.write("GET RECS \n".encode()) 
                    elif command.startswith("-mtx"):
                        try:
                            parts = command.split()
                            txp = int(parts[1])
                            txc = int(parts[2])
                            ser.write(f"SET TX PERI CENT {txp} {txc} \n".encode())   
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-dh "):
                        try:
                            handle = int(command[4:])
                            ser.write(f"DISCONNECT {handle} \n".encode())  
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-df"):
                        ser.write("DEL FILE \n".encode()) 
                    elif command.startswith("-trssi"):
                        try:
                            ser.write(f"TEST RSSI \n".encode()) 
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-irssi"):
                        try:
                            ser.write(f"INIT SEND TEST RSSI \n".encode()) 
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-srssi"):
                        try:
                            ser.write(f"SKIP SEND TEST RSSI \n".encode()) 
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-atx"):
                        try:
                            number = int(command[5:])
                            ser.write(f"ADVTX {number} \n".encode())  
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-sk "):
                        try:
                            number = int(command[4:])
                            ser.write(f"SEND KOT {number} \n".encode()) 
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-sp "):
                        try:
                            number = int(command[4:])
                            ser.write(f"SET PERIOD {number} \n".encode()) 
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-ctx"):
                        try:
                            parts = command.split()
                            handle = int(parts[1])
                            power = int(parts[2])
                            ser.write(f"CONTX {handle} {power} \n".encode())   
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-c"):
                        try:
                            ser.write("ADV START FOR PHONE".encode())
                            for i, thread in enumerate(threads):
                                if not thread.is_alive():
                                    done = True
                                    try:
                                        print(f"Starting sniffer thread {i}")
                                        thread.start()  # Start the thread
                                    except RuntimeError as e:
                                        print(f"Error starting thread {i}: {e}")
                                        done = False
                                    if done:
                                        break     
                        except ValueError:
                            print("Invalid number format.")
                    elif command.startswith("-as"):
                        ser.write("ADVS \n".encode()) 
                    elif command.startswith("-ss"):
                        ser.write("SCAN STOP \n".encode()) 
                    elif command.startswith("-s"):
                        ser.write("SCAN \n".encode()) 
                    elif command.startswith("-a"):
                        ser.write("ADV ".encode())
                    elif command.startswith("-gdf"):
                        try:
                            parts = command.split()
                            chunky = int(parts[1])
                        except ValueError:
                            print("Invalid chunk format.")
                        if(chunky<0):
                            print(f"Invalid chunk format: {chunky}")
                        
                        ser.write(f"GET DESIRED FILE {chunky}\n".encode())   
                    elif command.startswith("-res"):
                        ser.write("SEND RESTART \n".encode())
                    elif command.startswith("-rf"):
                        ser.write("REQUEST DATA \n".encode())
                    elif command.startswith("-rs"):
                        try:
                            parts = command.split()
                            handle = int(parts[1])
                            ser.write(f"RSSI {handle}\n".encode())   
                        except ValueError:
                            print("Invalid number format.")
                        
                    elif command.startswith("-r"):
                        ser.write("SENDREQ \n".encode())
                    elif command.startswith("-h"):
                        ser.write("HANDLES \n".encode())
                    else:
                        print("Invalid command format.")
    except IOError:
        pass  # Ignore if there's no input


def main():
    global ser
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(1)  # Give some time for the connection to establish
        print(f"Ser is open {ser.is_open}")
        old_settings = make_console_non_blocking()
        print("Enter command:")
        # ser.write("HANDLES \n".encode())
        # ser.write(f"Star 3 \n".encode())
        # ser.write(f"ADD NM  \n".encode()) 
        # ser.write(f"Star 3 \n".encode())
        while True:
            receive_messages()  # Check for serial data
            handle_input() 

    except serial.SerialException as e:
        print(f"Could not open serial port {SERIAL_PORT}: {e}")
    except KeyboardInterrupt:
        print("Exiting script.")
    finally:
        restore_console_settings(old_settings)
        if ser.is_open:
            print("close usb")
            ser.close()
            # for thread in threads:
            #     thread.exit()

if __name__ == "__main__":
    main()