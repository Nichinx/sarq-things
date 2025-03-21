# -*- coding: utf-8 -*-
"""
Created on Fri Mar 21 11:20:34 2025

@author: nichm
"""

import serial
import serial.tools.list_ports
import csv
import time
import re
import os  # Import os to handle file paths

def get_board_number():
    while True:
        board_no = input("Board no. (in XXXX): ")
        if board_no.isdigit() and len(board_no) == 4:
            return board_no
        print("Invalid input. Enter a 4-digit board number.")

def list_serial_ports():
    ports = serial.tools.list_ports.comports()
    filtered_ports = []
    
    if not ports:
        print("No serial ports detected.")
        return None
    
    print("Available COM ports:")
    for i, port in enumerate(ports):
        description = port.description
        if "Bluetooth" not in description:
            filtered_ports.append(port.device)
            print(f"[{len(filtered_ports)}] {port.device} - {description}")
    
    if not filtered_ports:
        print("No valid serial ports detected.")
        return None
    
    while True:
        choice = input("Select COM port number: ")
        if choice.isdigit() and 1 <= int(choice) <= len(filtered_ports):
            return filtered_ports[int(choice) - 1]
        print("Invalid selection. Try again.")

def setup_serial(port, baudrate=115200):
    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)  # Wait for connection to establish
        print(f"Connected to {port}")
        return ser
    except serial.SerialException as e:
        print(f"Error: {e}")
        return None

def parse_serial_data(line):
    match = re.search(r"Raw ADC: (\d+) \| ESP ADC Cal Raw to Voltage: ([\d.]+) \| Calculated VIN: ([\d.]+) \| Calibrated VIN: ([\d.]+)", line)
    if match:
        raw_adc = int(match.group(1))
        adc_cal_voltage = float(match.group(2))
        calculated_vin = float(match.group(3))
        calibrated_vin = float(match.group(4))
        return raw_adc, adc_cal_voltage, calculated_vin, calibrated_vin
    return None

def main():
    board_no = get_board_number()
    
    # Define the directory for saving files
    script_dir = os.path.dirname(os.path.abspath(__file__))  
    tests_folder = os.path.join(script_dir, "tests")  # Create 'tests' folder in script directory
    os.makedirs(tests_folder, exist_ok=True)  # Ensure the folder exists
    
    filename = os.path.join(tests_folder, f"sarq-{board_no}.csv")  # CSV inside 'tests' folder
    
    port = list_serial_ports()
    if port is None:
        return
    
    ser = setup_serial(port)
    if ser is None:
        return
    
    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["Raw ADC", "ESP ADC Cal Raw Voltage", "Calculated VIN", "Calibrated VIN", "Actual Input", "Difference"])
        
        while True:
            try:
                # Wait for user input before sending the trigger
                user_input = input("Enter 'a' to input Actual Voltage (or 'q' to quit): ")
                if user_input.lower() == 'q':
                    print("Exiting...")
                    break  # Exit loop
                
                if user_input.lower() == 'a':
                    actual_input = float(input("Actual Input Voltage: "))
                    ser.write(b'a')  # Send 'a' to Arduino to request data
                    
                    print("Waiting for serial data...")
                    line = ser.readline().decode('utf-8').strip()
                    if line:
                        print(f"Received: {line}")
                        parsed_data = parse_serial_data(line)
                        if parsed_data:
                            raw_adc, adc_cal_voltage, calculated_vin, calibrated_vin = parsed_data
                            
                            difference = round(calibrated_vin - actual_input, 4) if actual_input is not None else 'N/A'
                            writer.writerow([raw_adc, adc_cal_voltage, calculated_vin, calibrated_vin, actual_input, difference])
                            print(f"Logged: {raw_adc}, {adc_cal_voltage}, {calculated_vin}, {calibrated_vin}, {actual_input}, {difference}")
                    else:
                        print("No data received. Check if the Arduino is sending data.")
            
            except ValueError:
                print("Invalid voltage input. Please enter a valid number.")
            except KeyboardInterrupt:
                print("Exiting...")
                break
    
    ser.close()

if __name__ == "__main__":
    main()
