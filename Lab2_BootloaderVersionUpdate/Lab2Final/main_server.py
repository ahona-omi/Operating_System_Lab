import math
import os       # interact with the operating system
import serial   # communication with serial ports

# Function to calculate CRC32
def calculate_crc32_mpeg2(data):
    if len(data) != 500:
        raise ValueError("Input data must be exactly 500 bytes.")
    
    crc = 0xFFFFFFFF    # Initializes the CRC value to all 1s (32-bit)

    # Processes the input data 4 bytes (32 bits) at a time
    for i in range(0, len(data), 4):
        chunk = data[i:i + 4]
        
        # If the chunk is less than 4 bytes (e.g., near the end of data), it pads with null bytes (\x00)
        # this ensures data alignment and consistency during CRC calculation
        while len(chunk) < 4:
            chunk += b'\x00'

        #Combines 4 bytes into a 32-bit word and XORs it with the current CRC value
        word = (chunk[0] << 24) | (chunk[1] << 16) | (chunk[2] << 8) | chunk[3]
        crc ^= word

        # Simulates the polynomial division for 32 bits
        for _ in range(32):
            if crc & 0x80000000:
                crc = (crc << 1) ^ 0x04C11DB7       # If the MSB is 1, XOR with the MPEG-2 polynomial 0x04C11DB7
            else:
                crc <<= 1
            crc &= 0xFFFFFFFF # Left-shift & Ensure 32-bit limit

    return crc.to_bytes(4, byteorder='big')     # Converts the final CRC value to a 4-byte big-endian representation

CHUNK_SIZE = 500

serial_instance = serial.Serial(port="/dev/ttyACM0", baudrate=115200)

print("Server is running...")

while True:
    # Continuously reads incoming data from the serial port, decodes it, and prints it
    value = serial_instance.readline().decode('utf-8').strip()
    print(value)
    
    cmd = value.split()     #Splits the incoming command into components for easier parsing
    # parsing typically involves breaking down a string into smaller parts (called "tokens") to make the data easier to process
    # The first word (cmd[0]) tells the program what operation to perform, such as CHECK_VERSION or GET_UPDATE
    # The remaining parts of the command (e.g., cmd[1]) provide additional information, like the version number or file name.
    
    if cmd[0] == "CHECK_VERSION":
        current_version = cmd[1]
        with open('version.txt', 'r') as file:
            latest_version = file.read()
        if current_version == latest_version:
            response = "NO UPDATES AVAILABLE\n".encode('utf-8')
        else:
            response = f"UPDATE AVAILABLE {latest_version}\n".encode('utf-8')
        serial_instance.write(response)
        
    elif cmd[0] == "GET_UPDATE":
        with open('file_path.txt', 'r') as file:
            FILE_PATH = file.read().strip()
        file_size = os.path.getsize(FILE_PATH)
        print(f"File size: {file_size} Bytes")
        packet = str(file_size).encode('utf-8') + b'$'
        
        serial_instance.write(packet)
        ack = serial_instance.readline().decode('utf-8').strip()
        print(ack)
        
        if ack == "ACK":
            print("File size sent successfully")
            
            with open(FILE_PATH, 'rb') as file:
                current_chunk_number = 1
                total_chunk_number = int(math.ceil(file_size / CHUNK_SIZE))
                packets = []
                reapeted = 0
                
                while current_chunk_number <= total_chunk_number:
                    if file_size:
                        chunk = file.read(CHUNK_SIZE)

                    if file_size < CHUNK_SIZE:
                        chunk += b'\x00' * (CHUNK_SIZE - file_size)

                    # Computes the CRC for the chunk and appends it to the chunk
                    if file_size:
                        crc_bit = calculate_crc32_mpeg2(chunk)

                        # Print the CRC value
                        print(f"CRC for chunk {current_chunk_number}: {crc_bit.hex()}")
                        
                        packet = chunk + crc_bit
                        packets.append(packet)
                    
                    serial_instance.write(packets[current_chunk_number - 1])
                    ack = serial_instance.readline().decode('utf-8').strip()
                    
                    if ack != "NACK":
                        percent = int((current_chunk_number / total_chunk_number) * 100)
                        print(f"{percent}% package sent successfully")
                        current_chunk_number += 1
                        reapeted = 0
                        
                        if file_size >= CHUNK_SIZE:
                            file_size -= CHUNK_SIZE
                        else:
                            file_size = 0
                            
                    elif ack == "NACK":
                        if reapeted > 3:
                            print("Error while sending the package")
                            break
                        reapeted += 1
                        print(f"Error while sending chunk number {current_chunk_number}")
                        print("Resending the chunk...")
                
            if file_size == 0:
                print("File sent successfully")
                
                # Safely update the version
                try:
                    with open('version.txt', 'w') as file:
                        file.write(latest_version)
                    print(f"Version updated to {latest_version} in version.txt")
                except Exception as e:
                    print(f"Error writing to version.txt: {e}")