# -*- coding: utf-8 -*-
"""
Created on Fri Mar 21 15:05:06 2025

@author: nichm
"""

import serial
import serial.tools.list_ports
import csv
import time
import re
import os
import numpy as np

def get_board_number():
    """Prompt user for board number."""
    while True:
        board_no = input("Board no. (in XXXX): ")
        if board_no.isdigit() and len(board_no) == 4:
            return board_no
        print("Invalid input. Enter a 4-digit board number.")

def list_serial_ports():
    """List available serial ports and let the user select one."""
    ports = serial.tools.list_ports.comports()
    filtered_ports = [p.device for p in ports if "Bluetooth" not in p.description]

    if not filtered_ports:
        print("No valid serial ports detected.")
        return None

    print("Available COM ports:")
    for i, port in enumerate(filtered_ports):
        print(f"[{i + 1}] {port}")

    while True:
        choice = input("Select COM port number: ")
        if choice.isdigit() and 1 <= int(choice) <= len(filtered_ports):
            return filtered_ports[int(choice) - 1]
        print("Invalid selection. Try again.")

def setup_serial(port, baudrate=115200):
    """Initialize serial connection."""
    try:
        ser = serial.Serial(port, baudrate, timeout=2, dsrdtr=False, rtscts=False)
        time.sleep(2)
        print(f"Connected to {port}")
        return ser
    except serial.SerialException as e:
        print(f"Error: {e}")
        return None

def parse_serial_data(line):
    """Extract measured VIN from the serial output."""
    match = re.search(r"Raw ADC: (\d+) \| ESP ADC Cal Raw to Voltage: ([\d.]+) \| Calculated VIN: ([\d.]+) \| Calibrated VIN: ([\d.]+)", line)
    if match:
        return float(match.group(3))  # Extract 'Calculated VIN' as the measured VIN
    return None

def cubic_fit(x_vals, y_vals):
    """Perform a cubic polynomial fit."""
    coeffs = np.polyfit(x_vals, y_vals, 3)
    print(f"Cubic Fit Coefficients: {coeffs}")
    return coeffs

def generate_arduino_formula(coeffs):
    """Generate the Arduino function for calibrateVIN()."""
    return f"""
    float calibrateVIN(float vin) {{
        return ({coeffs[0]:.6f} * vin * vin * vin) + ({coeffs[1]:.6f} * vin * vin) + ({coeffs[2]:.6f} * vin) + {coeffs[3]:.6f}; 
    }}
    """

def update_ino_file(ino_filepath, new_function):
    """Replace the existing calibrateVIN() function in the .ino file."""
    try:
        with open(ino_filepath, 'r') as file:
            ino_code = file.read()

        # Find and replace the existing calibrateVIN() function
        pattern = re.compile(r"float\s+calibrateVIN\s*\(.*?\)\s*\{.*?\}", re.DOTALL)
        updated_code = pattern.sub(new_function.strip(), ino_code)

        with open(ino_filepath, 'w') as file:
            file.write(updated_code)

        print(f"Updated {ino_filepath} with new calibration function.")
    except Exception as e:
        print(f"Error updating .ino file: {e}")

def main():
    board_no = get_board_number()
    script_dir = os.path.dirname(os.path.abspath(__file__))
    test_dir = os.path.join(script_dir, "tests")
    os.makedirs(test_dir, exist_ok=True)
    filename = os.path.join(test_dir, f"sarq-{board_no}.csv")

    port = list_serial_ports()
    if port is None:
        return
    
    ser = setup_serial(port)
    if ser is None:
        return

    # Ask if the user wants to calibrate
    calibrate = input("Do you want to calibrate the board? (y/n): ").strip().lower() == 'y'

    measured_vin = []
    actual_vin = []

    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["Measured VIN", "Actual VIN"])

        while True:
            try:
                user_input = input("Enter 'a' to input Actual Voltage, or 'q' to quit: ")
                if user_input.lower() == 'q':
                    break
                elif user_input.lower() == 'a':
                    actual_input = float(input("Actual Input Voltage: "))
                    ser.write(b'a')  # Request data from Arduino
                    
                    print("Waiting for serial data...")
                    while True:
                        line = ser.readline().decode('utf-8').strip()
                        if line:
                            print(f"Received: {line}")
                            measured_value = parse_serial_data(line)
                            if measured_value is not None:
                                # Always log measured and actual VIN
                                writer.writerow([measured_value, actual_input])
                                print(f"Logged: Measured VIN={measured_value}, Actual VIN={actual_input}")
                                
                                # If calibrating, store values for fitting
                                if calibrate:
                                    measured_vin.append(measured_value)
                                    actual_vin.append(actual_input)
                                
                                break
                        else:
                            print("No data received. Check if Arduino is sending data.")
            
            except ValueError:
                print("Invalid input, please enter a valid number.")
            except KeyboardInterrupt:
                print("Exiting...")
                break

    ser.close()

    # Proceed to calibration if needed
    if calibrate and len(measured_vin) >= 4:
        print("Starting calibration...")
        coeffs = cubic_fit(measured_vin, actual_vin)

        # Generate Arduino function
        arduino_function = generate_arduino_formula(coeffs)

        # Path to the existing .ino file
        ino_filepath = "adc_read_2.ino"  # Update with actual path

        # Update the .ino file
        update_ino_file(ino_filepath, arduino_function)

        print("Calibration completed! The Arduino code has been updated.")

if __name__ == "__main__":
    main()
