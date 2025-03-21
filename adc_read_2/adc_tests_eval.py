# -*- coding: utf-8 -*-
"""
Created on Fri Mar 21 11:20:34 2025

@author: nichm
"""

import serial
import csv
import time

def get_board_number():
    while True:
        board_no = input("Board no. (in XXXX): ")
        if board_no.isdigit() and len(board_no) == 4:
            return board_no
        print("Invalid input. Enter a 4-digit board number.")

def setup_serial(port='/dev/ttyUSB0', baudrate=115200):
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        time.sleep(2)  # Wait for connection to establish
        return ser
    except serial.SerialException as e:
        print(f"Error: {e}")
        return None

def main():
    board_no = get_board_number()
    filename = f"sarq-{board_no}.csv"
    
    ser = setup_serial()
    if ser is None:
        return
    
    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["Raw ADC", "Calculated VIN", "Calibrated VIN", "Actual Input", "Difference"])
        
        actual_input = None
        while True:
            try:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    parts = line.split(',')  # Expected format: rawADC, VIN, calibratedVIN
                    if len(parts) != 3:
                        continue
                    
                    raw_adc, calculated_vin, calibrated_vin = map(float, parts)
                    
                    # Prompt for actual input voltage if user enters 'a'
                    if actual_input is None:
                        user_input = input("Enter 'a' to input Actual Voltage, or press Enter to continue: ")
                        if user_input.lower() == 'a':
                            actual_input = float(input("Actual Input Voltage: "))
                    
                    difference = round(calibrated_vin - actual_input, 4) if actual_input is not None else 'N/A'
                    writer.writerow([raw_adc, calculated_vin, calibrated_vin, actual_input, difference])
                    print(f"Logged: {raw_adc}, {calculated_vin}, {calibrated_vin}, {actual_input}, {difference}")
                    
            except ValueError:
                print("Error in parsing serial data.")
            except KeyboardInterrupt:
                print("Exiting...")
                break
    
    ser.close()

if __name__ == "__main__":
    main()
