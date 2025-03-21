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
import subprocess

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
    """Extract values from the serial output."""
    match = re.search(r"Raw ADC: (\d+) \| ESP ADC Cal Raw to Voltage: ([\d.]+) \| Calculated VIN: ([\d.]+) \| Calibrated VIN: ([\d.]+)", line)
    if match:
        return (
            int(match.group(1)),    # Raw ADC
            float(match.group(2)),  # ESP ADC Cal Raw Voltage
            float(match.group(3)),  # Calculated VIN
            float(match.group(4))   # Calibrated VIN
        )
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
def evaluate_fit(coeffs, measured, actual):
    """Evaluate the cubic fit by comparing predicted values with actual."""
    predicted = np.polyval(coeffs, measured)
    errors = np.abs(predicted - actual)
    avg_error = np.mean(errors)
    max_error = np.max(errors)
    print(f"📊 Average Calibration Error: {avg_error:.4f}V")
    print(f"❌ Max Calibration Error: {max_error:.4f}V")

def get_valid_ino_filepath(script_dir):
    """Prompt the user to enter a valid .ino filename if not found."""
    while True:
        # List available .ino files in the script directory
        ino_files = [f for f in os.listdir(script_dir) if f.endswith(".ino")]
        
        if ino_files:
            print("\nAvailable .ino files in this folder:")
            for i, file in enumerate(ino_files):
                print(f"[{i + 1}] {file}")
        else:
            print("\nNo .ino files found in the script directory.")

        ino_filename = input("\nEnter the correct .ino file name (e.g., adc_read.ino): ").strip()
        ino_filepath = os.path.join(script_dir, ino_filename)

        if os.path.exists(ino_filepath):
            return ino_filepath
        print(f"File '{ino_filename}' not found. Please enter a valid filename.")

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

        print(f"✅ Updated {ino_filepath} with new calibration function.")
    except FileNotFoundError:
        print(f"Error: File '{ino_filepath}' not found.")
        ino_filepath = get_valid_ino_filepath(os.path.dirname(ino_filepath))
        update_ino_file(ino_filepath, new_function)  # Retry update with new file path
    except Exception as e:
        print(f"Error updating .ino file: {e}")

def run_evaluation(ser, filename):
    """Continue logging measured VIN and actual VIN to CSV after calibration."""
    print("\n📊 Starting Evaluation Mode...\n")
    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["Raw ADC", "ESP ADC Cal Raw Voltage", "Calculated VIN", "Calibrated VIN", "Actual Input", "Difference"])

        no_data_count = 0  # Counter to track how long we've been waiting

        while True:
            try:
                # Prompt the user to trigger a new reading
                user_input = input("Enter 'a' to input Actual Voltage, or 'q' to quit: ").strip().lower()
                if user_input == 'q':
                    "Exiting..."
                    break
                elif user_input == 'a':
                    ser.write(b'a')  # Send 'a' to Arduino to request new data

                    print("Waiting for serial data...")
                    while True:
                        line = ser.readline().decode('utf-8').strip()

                        if not line:
                            no_data_count += 1
                            if no_data_count > 10:  # Stop waiting after 10 attempts
                                print("❌ No data received from the Arduino. Please check the connection and restart the device.")
                                return
                            continue

                        print(f"Received: {line}")
                        match = re.search(r"Raw ADC: (\d+) \| ESP ADC Cal Raw to Voltage: ([\d.]+) \| Calculated VIN: ([\d.]+) \| Calibrated VIN: ([\d.]+)", line)
                        if match:
                            raw_adc = int(match.group(1))
                            raw_voltage = float(match.group(2))
                            calculated_vin = float(match.group(3))
                            calibrated_vin = float(match.group(4))

                            # Prompt for actual input voltage
                            actual_input = input("Actual Input Voltage: ").strip()
                            actual_input = float(actual_input)
                            difference = round(actual_input - calibrated_vin,4)

                            # Log to evaluation file immediately
                            writer.writerow([raw_adc, raw_voltage, calculated_vin, calibrated_vin, actual_input, difference])
                            print(f"Logged: Raw ADC={raw_adc}, Calibrated VIN={calibrated_vin}, Actual={actual_input}, Diff={difference}")

                            # Reset no-data counter after receiving valid data
                            no_data_count = 0  
                            break  # Exit the inner while loop to wait for next 'a' input

            except ValueError:
                print("Invalid input, please enter a valid number.")
            except KeyboardInterrupt:
                print("Exiting...")
                break
            
def compile_and_upload(ino_filepath, board_fqbn, port):
    """Compiles and uploads the .ino file to the Arduino board."""
    try:
        # Compile the sketch
        compile_cmd = f'arduino-cli compile --fqbn {board_fqbn} "{ino_filepath}"'
        print(f"🔹 Compiling: {compile_cmd}")
        compile_result = subprocess.run(compile_cmd, shell=True, capture_output=True, text=True)

        if compile_result.returncode != 0:
            print(f"❌ Compilation failed:\n{compile_result.stderr}")
            return

        # Upload the compiled sketch
        upload_cmd = f'arduino-cli upload --port {port} --fqbn {board_fqbn} "{ino_filepath}"'
        print(f"🔹 Uploading: {upload_cmd}")
        upload_result = subprocess.run(upload_cmd, shell=True, capture_output=True, text=True)

        if upload_result.returncode == 0:
            print("✅ Upload successful!")
        else:
            print(f"❌ Upload failed:\n{upload_result.stderr}")

    except Exception as e:
        print(f"⚠️ Error during compile/upload: {e}")

def main():
    board_no = get_board_number()
    script_dir = os.path.dirname(os.path.abspath(__file__))
    test_dir = os.path.join(script_dir, "tests")
    os.makedirs(test_dir, exist_ok=True)

    # Separate filenames for evaluation and calibration
    eval_filename = os.path.join(test_dir, f"sarq_eval-{board_no}.csv")
    calib_filename = os.path.join(test_dir, f"sarq_calib-{board_no}.csv")

    port = list_serial_ports()
    if port is None:
        return
    
    ser = setup_serial(port)
    if ser is None:
        return

    while True:
        # Ask user whether they want Calibration or Evaluation
        mode = input("\nSelect mode - Calibration (c) or Evaluation (e): ").strip().lower()
        while mode not in ['c', 'e']:
            mode = input("Invalid choice. Enter 'c' for Calibration or 'e' for Evaluation: ").strip().lower()
        
        calibrate = mode == 'c'

        if calibrate:
            print("\n🔧 Starting Calibration Mode...\n")
            csv_file = calib_filename
            headers = ["Measured VIN", "Actual VIN"]
        else:
            csv_file = eval_filename
            headers = ["Raw ADC", "ESP ADC Cal Raw Voltage", "Calculated VIN", "Calibrated VIN", "Actual Input", "Difference"]

        # Open the correct CSV file and write headers
        with open(csv_file, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(headers)

        measured_vin = []
        actual_vin = []

        while True:
            try:
                if calibrate:
                    user_input = input("Enter 'a' to input Actual Voltage, or 'q' to finish calibration: ")
                    if user_input.lower() == 'q':
                        break
                    elif user_input.lower() == 'a':
                        actual_input = float(input("Actual Input Voltage: "))
                        ser.write(b'a')

                        print("Waiting for serial data...")
                        while True:
                            line = ser.readline().decode('utf-8').strip()
                            if line:
                                print(f"Received: {line}")
                                match = re.search(r"Raw ADC: (\d+) \| ESP ADC Cal Raw to Voltage: ([\d.]+) \| Calculated VIN: ([\d.]+) \| Calibrated VIN: ([\d.]+)", line)
                                if match:
                                    calculated_vin = float(match.group(3))  # Log only Calculated VIN
                                    
                                    # Log to calibration file immediately
                                    with open(calib_filename, mode='a', newline='') as file:
                                        writer = csv.writer(file)
                                        writer.writerow([calculated_vin, actual_input])

                                    print(f"Logged: Measured VIN={calculated_vin}, Actual VIN={actual_input}")

                                    measured_vin.append(calculated_vin)
                                    actual_vin.append(actual_input)
                                    break
                            else:
                                print("No data received. Check if Arduino is sending data.")

                else:  # Evaluation Mode
                    run_evaluation(ser, eval_filename)  # ✅ CALL run_evaluation()
                    break  # Exit after evaluation

            except ValueError:
                print("Invalid input, please enter a valid number.")
            except KeyboardInterrupt:
                print("Exiting...")
                break

        # Close serial before reopening (to avoid conflicts)
        ser.close()

        # Proceed to calibration if needed
        if calibrate and len(measured_vin) >= 4:
            print("⚙️ Starting calibration process...")
            coeffs = cubic_fit(measured_vin, actual_vin)
            evaluate_fit(coeffs, measured_vin, actual_vin)

            # Append coefficients to the calibration CSV file
            with open(calib_filename, mode='a', newline='') as file:
                writer = csv.writer(file)
                writer.writerow([])
                writer.writerow(["Cubic Fit Coefficients:"] + list(coeffs))
                
            # Generate and update Arduino function
            arduino_function = generate_arduino_formula(coeffs)            
            update_ino_file("adc_for_calib.ino", arduino_function)

            print("✅ Calibration completed! The Arduino code has been updated.")

            # 🟢 Compile & Upload the new Arduino code
            board_fqbn = "esp32:esp32:esp32"  # Change this based on your board
            compile_and_upload("adc_for_calib.ino", board_fqbn, port)

        # After Calibration, Ask for Evaluation
        if calibrate:
            choice = input("\nDo you want to proceed with Evaluation Mode? (y/n): ").strip().lower()
            if choice == 'y':
                print("♻️ Reopening serial connection for Evaluation Mode...")
                ser = setup_serial(port)  # Reopen Serial Port
                if ser is None:
                    print("❌ Failed to reopen serial port. Exiting.")
                    break

                # ✅ Run evaluation immediately after reopening the serial port
                run_evaluation(ser, eval_filename)
                break  # Exit the loop after evaluation mode

            else:
                print("Exiting program.")
                break  # Exit loop if user chooses not to evaluate
        else:
            break  # Exit after evaluation


if __name__ == "__main__":
    main()
