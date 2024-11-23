import math
import os
import serial



# Function to calculate CRC32 using MPEG-2 polynomial
def calculate_crc32_mpeg2(data):
    if len(data) != 500:
        raise ValueError("Input data must be exactly 500 bytes.")
    
    crc = 0xFFFFFFFF

    for i in range(0, len(data), 4):
        chunk = data[i:i + 4]
        
        while len(chunk) < 4:
            chunk += b'\x00'

        word = (chunk[0] << 24) | (chunk[1] << 16) | (chunk[2] << 8) | chunk[3]
        crc ^= word

        for _ in range(32):
            if crc & 0x80000000:
                crc = (crc << 1) ^ 0x04C11DB7
            else:
                crc <<= 1
            crc &= 0xFFFFFFFF # Ensure 32-bit limit

    return crc.to_bytes(4, byteorder='big')





CHUNK_SIZE = 500

serial_instance = serial.Serial(port="/dev/ttyACM0", baudrate=115200)

print("Server is running...")


while True:
    value = serial_instance.readline().decode('utf-8').strip()
    print(value)
    
    cmd = value.split()
    
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

                    if file_size:
                        crc_bit = calculate_crc32_mpeg2(chunk)
                        packet = chunk+crc_bit
                        packets.append(packet)
                    
                    serial_instance.write(packets[current_chunk_number - 1])
                    ack = serial_instance.readline().decode('utf-8').strip()
                    
                    if ack != "NACK":
                        percent = int((current_chunk_number / total_chunk_number) * 100)
                        print(f"{percent}% package sent succesfully")
                        current_chunk_number += 1
                        reapeted = 0
                        
                        if file_size >= CHUNK_SIZE:
                            file_size -= CHUNK_SIZE
                        else:
                            file_size = 0
                            
                    elif ack == "NACK":
                        if(reapeted > 3):
                            print("Error while sending the package")
                            break
                        reapeted += 1
                        print(f"Error while sending chunk number {current_chunk_number}")
                        print("Resending the chunk...")
                
            
            if file_size == 0:
                print("File sent successfully")
                    
