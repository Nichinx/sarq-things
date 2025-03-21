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
from scipy.optimize import curve_fit

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

# def piecewise_fit(measured_vin, actual_vin, split_voltage=8.0):
#     """Perform a hybrid piecewise fit: quadratic for low range, cubic for high range."""
#     measured_vin = np.array(measured_vin)
#     actual_vin = np.array(actual_vin)
#     split_index = np.argmax(measured_vin > split_voltage)

#     if split_index == 0 or split_index == len(measured_vin) - 1:
#         print("⚠️ Not enough data for a hybrid model. Using full cubic fit instead.")
#         cubic_coeffs = np.polyfit(measured_vin, actual_vin, 3)
#         return cubic_coeffs, None

#     low_coeffs = np.polyfit(measured_vin[:split_index], actual_vin[:split_index], 2)
#     high_coeffs = np.polyfit(measured_vin[split_index:], actual_vin[split_index:], 3)

#     print(f"✅ Quadratic Coefficients (Low Range < {split_voltage}V): {low_coeffs}")
#     print(f"✅ Cubic Coefficients (High Range >= {split_voltage}V): {high_coeffs}")

#     return low_coeffs, high_coeffs


# def piecewise_advanced_fit(measured_vin, actual_vin):
#     """Perform a segmented polynomial fit: Quadratic for low & mid range, Cubic for high range."""
#     measured_vin = np.array(measured_vin)
#     actual_vin = np.array(actual_vin)

#     # Define split points dynamically (e.g., based on actual input range)
#     v_low = np.percentile(measured_vin, 30)  # First third (low range)
#     v_mid = np.percentile(measured_vin, 70)  # Second third (mid range)

#     idx_low = np.argmax(measured_vin > v_low)
#     idx_mid = np.argmax(measured_vin > v_mid)

#     if idx_low == 0 or idx_mid == 0 or idx_mid == len(measured_vin) - 1:
#         print("⚠️ Not enough data for segmented model. Using full cubic fit.")
#         cubic_coeffs = np.polyfit(measured_vin, actual_vin, 3)
#         return cubic_coeffs, None, None

#     # Fit quadratic for low range
#     low_coeffs = np.polyfit(measured_vin[:idx_low], actual_vin[:idx_low], 2)
#     # Fit quadratic for mid range
#     mid_coeffs = np.polyfit(measured_vin[idx_low:idx_mid], actual_vin[idx_low:idx_mid], 2)
#     # Fit cubic for high range
#     high_coeffs = np.polyfit(measured_vin[idx_mid:], actual_vin[idx_mid:], 3)

#     print(f"✅ Quadratic Fit (Low < {v_low:.2f}V): {low_coeffs}")
#     print(f"✅ Quadratic Fit (Mid {v_low:.2f}V - {v_mid:.2f}V): {mid_coeffs}")
#     print(f"✅ Cubic Fit (High ≥ {v_mid:.2f}V): {high_coeffs}")

#     return low_coeffs, mid_coeffs, high_coeffs, v_low, v_mid


# def generate_arduino_formula(low_coeffs, high_coeffs, split_voltage=8.0):
#     """Generate the Arduino function for calibrateVIN()."""
#     if high_coeffs is None:
#         return f"""
#         float calibrateVIN(float vin) {{
#             return ({low_coeffs[0]:.6f} * vin * vin) + ({low_coeffs[1]:.6f} * vin) + {low_coeffs[2]:.6f}; 
#         }}
#         """
#     else:
#         return f"""
#         float calibrateVIN(float vin) {{
#             if (vin < {split_voltage:.1f}) {{
#                 return ({low_coeffs[0]:.6f} * vin * vin) + ({low_coeffs[1]:.6f} * vin) + {low_coeffs[2]:.6f};
#             }} else {{
#                 return ({high_coeffs[0]:.6f} * vin * vin * vin) + ({high_coeffs[1]:.6f} * vin * vin) + ({high_coeffs[2]:.6f} * vin) + {high_coeffs[3]:.6f};
#             }}
#         }}
#         """

# def generate_arduino_formula_advanced(low_coeffs, mid_coeffs, high_coeffs, v_low, v_mid):
#     """Generate a more refined Arduino function using a segmented model."""
#     return f"""
#     float calibrateVIN(float vin) {{
#         if (vin < {v_low:.2f}) {{
#             return ({low_coeffs[0]:.6f} * vin * vin) + ({low_coeffs[1]:.6f} * vin) + {low_coeffs[2]:.6f};
#         }} else if (vin < {v_mid:.2f}) {{
#             return ({mid_coeffs[0]:.6f} * vin * vin) + ({mid_coeffs[1]:.6f} * vin) + {mid_coeffs[2]:.6f};
#         }} else {{
#             return ({high_coeffs[0]:.6f} * vin * vin * vin) + ({high_coeffs[1]:.6f} * vin * vin) + ({high_coeffs[2]:.6f} * vin) + {high_coeffs[3]:.6f};
#         }}
#     }}
#     """
    
# def cubic_model(vin, a, b, c, d):
#     """Cubic polynomial model."""
#     return a * vin**3 + b * vin**2 + c * vin + d

# def quadratic_model(vin, a, b, c):
#     """Quadratic polynomial model."""
#     return a * vin**2 + b * vin + c

# def calculate_fit(measured_vin, actual_vin):
#     """Determine the best-fit polynomial dynamically."""
#     measured_vin = np.array(measured_vin)
#     actual_vin = np.array(actual_vin)

#     # Fit cubic model
#     cubic_params, _ = curve_fit(cubic_model, measured_vin, actual_vin)
#     cubic_fit = cubic_model(measured_vin, *cubic_params)

#     # Compute R² for cubic fit
#     residuals = actual_vin - cubic_fit
#     ss_res = np.sum(residuals**2)
#     ss_tot = np.sum((actual_vin - np.mean(actual_vin))**2)
#     r2_cubic = 1 - (ss_res / ss_tot)

#     print(f"🔹 Cubic Fit R² = {r2_cubic:.6f}")
    
#     # If cubic fit is already good, return it
#     if r2_cubic > 0.99:
#         return {
#             "low_coeffs": None,
#             "mid_coeffs": None,
#             "high_coeffs": cubic_params,
#             "v_low": None,
#             "v_mid": None
#         }

#     # If cubic fit isn't perfect, attempt three-segment piecewise fitting
#     v_low = np.percentile(measured_vin, 33)
#     v_mid = np.percentile(measured_vin, 66)

#     idx_low = np.argmax(measured_vin > v_low)
#     idx_mid = np.argmax(measured_vin > v_mid)

#     if idx_low == 0 or idx_mid == 0 or idx_mid == len(measured_vin) - 1:
#         print("⚠️ Not enough data for full piecewise fitting. Using cubic only.")
#         return {
#             "low_coeffs": None,
#             "mid_coeffs": None,
#             "high_coeffs": cubic_params,
#             "v_low": None,
#             "v_mid": None
#         }

#     # Split into three segments
#     low_vin, mid_vin, high_vin = measured_vin[:idx_low], measured_vin[idx_low:idx_mid], measured_vin[idx_mid:]
#     low_actual, mid_actual, high_actual = actual_vin[:idx_low], actual_vin[idx_low:idx_mid], actual_vin[idx_mid:]

#     # Fit quadratics to low and mid segments, cubic to high
#     low_params, _ = curve_fit(quadratic_model, low_vin, low_actual)
#     mid_params, _ = curve_fit(quadratic_model, mid_vin, mid_actual)
#     high_params, _ = curve_fit(cubic_model, high_vin, high_actual)

#     print(f"✅ Quadratic Fit (Low < {v_low:.2f}V): {low_params}")
#     print(f"✅ Quadratic Fit (Mid {v_low:.2f}V - {v_mid:.2f}V): {mid_params}")
#     print(f"✅ Cubic Fit (High >= {v_mid:.2f}V): {high_params}")

#     return {
#         "low_coeffs": low_params,
#         "mid_coeffs": mid_params,
#         "high_coeffs": high_params,
#         "v_low": v_low,
#         "v_mid": v_mid
#     }
    
# def generate_arduino_function(cubic_params, low_params, high_params, split_point):
#     """Generate Arduino C++ function dynamically."""
#     if cubic_params:
#         return f"""
# float calibrateVIN(float vin) {{
#    return ({cubic_params[0]:.6f} * vin * vin * vin) + ({cubic_params[1]:.6f} * vin * vin) + ({cubic_params[2]:.6f} * vin) + {cubic_params[3]:.6f};
# }}
#         """
#     else:
#         return f"""
# float calibrateVIN(float vin) {{
#    if (vin < {split_point:.2f}) {{
#        return ({low_params[0]:.6f} * vin * vin) + ({low_params[1]:.6f} * vin) + {low_params[2]:.6f};
#    }} else {{
#        return ({high_params[0]:.6f} * vin * vin * vin) + ({high_params[1]:.6f} * vin * vin) + ({high_params[2]:.6f} * vin) + {high_params[3]:.6f};
#    }}
# }}
#         """


#####################
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
#####################

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

# def update_ino_file(script_dir, new_function):
#     """Find and update the calibrateVIN() function inside the correct .ino file."""
#     ino_filepath = get_valid_ino_filepath(script_dir)  # Get the correct .ino file

#     try:
#         with open(ino_filepath, 'r', encoding='utf-8') as file:
#             ino_code = file.read()

#         # Match the calibrateVIN function using regex
#         pattern = re.compile(r"float\s+calibrateVIN\s*\(.*?\)\s*\{.*?\}", re.DOTALL)
#         if pattern.search(ino_code):
#             updated_code = pattern.sub(new_function.strip(), ino_code)
#         else:
#             print("⚠️ No existing calibrateVIN() function found. Appending the new function.")
#             updated_code = ino_code + "\n\n" + new_function.strip()

#         with open(ino_filepath, 'w', encoding='utf-8') as file:
#             file.write(updated_code)

#         print(f"✅ Successfully updated {ino_filepath} with new calibration function.")

#     except FileNotFoundError:
#         print(f"❌ Error: File '{ino_filepath}' not found. Please check the directory.")
#     except Exception as e:
#         print(f"❌ Error updating .ino file: {e}")


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


# def main():
#     board_no = get_board_number()
#     script_dir = os.path.dirname(os.path.abspath(__file__))
#     test_dir = os.path.join(script_dir, "tests")
#     os.makedirs(test_dir, exist_ok=True)

#     # Separate filenames for evaluation and calibration
#     eval_filename = os.path.join(test_dir, f"sarq_eval-{board_no}.csv")
#     calib_filename = os.path.join(test_dir, f"sarq_calib-{board_no}.csv")

#     port = list_serial_ports()
#     if port is None:
#         return
    
#     ser = setup_serial(port)
#     if ser is None:
#         return

#     # Ask user whether they want Calibration or Evaluation
#     mode = input("Select mode - Calibration (c) or Evaluation (e): ").strip().lower()
#     while mode not in ['c', 'e']:
#         mode = input("Invalid choice. Enter 'c' for Calibration or 'e' for Evaluation: ").strip().lower()
    
#     calibrate = mode == 'c'

#     # Printout for mode selection
#     if calibrate:
#         print("🔧 Starting Calibration Mode...")
#         csv_file = calib_filename
#         headers = ["Measured VIN", "Actual VIN"]
#     else:
#         print("📊 Starting Evaluation Mode...")
#         csv_file = eval_filename
#         headers = ["Raw ADC", "ESP ADC Cal Raw Voltage", "Calculated VIN", "Calibrated VIN", "Actual Input", "Difference"]

#     # Open the correct CSV file and write headers
#     with open(csv_file, mode='w', newline='') as file:
#         writer = csv.writer(file)
#         writer.writerow(headers)

#     measured_vin = []
#     actual_vin = []

#     while True:
#         try:
#             if calibrate:
#                 user_input = input("Enter 'a' to input Actual Voltage, or 'q' to quit: ")
#                 if user_input.lower() == 'q':
#                     break
#                 elif user_input.lower() == 'a':
#                     actual_input = float(input("Actual Input Voltage: "))
#                     ser.write(b'a')

#                     print("Waiting for serial data...")
#                     while True:
#                         line = ser.readline().decode('utf-8').strip()
#                         if line:
#                             print(f"Received: {line}")
#                             measured_value = parse_serial_data(line)
#                             if measured_value is not None:
#                                 # Log to calibration file immediately
#                                 with open(calib_filename, mode='a', newline='') as file:
#                                     writer = csv.writer(file)
#                                     writer.writerow([measured_value, actual_input])

#                                 print(f"Logged: Measured VIN={measured_value}, Actual VIN={actual_input}")

#                                 measured_vin.append(measured_value)
#                                 actual_vin.append(actual_input)
#                                 break
#                         else:
#                             print("No data received. Check if Arduino is sending data.")

#             else:  # Evaluation Mode
#                 print("Waiting for serial data...")
#                 line = ser.readline().decode('utf-8').strip()
#                 if line:
#                     print(f"Received: {line}")
#                     match = re.search(r"Raw ADC: (\d+) \| ESP ADC Cal Raw to Voltage: ([\d.]+) \| Calculated VIN: ([\d.]+) \| Calibrated VIN: ([\d.]+)", line)
#                     if match:
#                         raw_adc = int(match.group(1))
#                         raw_voltage = float(match.group(2))
#                         calculated_vin = float(match.group(3))
#                         calibrated_vin = float(match.group(4))

#                         actual_input = input("Enter actual input voltage (or 'q' to quit): ").strip()
#                         if actual_input.lower() == 'q':
#                             break
#                         actual_input = float(actual_input)
#                         difference = actual_input - calibrated_vin

#                         # Log to evaluation file immediately
#                         with open(eval_filename, mode='a', newline='') as file:
#                             writer = csv.writer(file)
#                             writer.writerow([raw_adc, raw_voltage, calculated_vin, calibrated_vin, actual_input, difference])

#                         print(f"Logged: Raw ADC={raw_adc}, Calibrated VIN={calibrated_vin}, Actual={actual_input}, Diff={difference}")

#         except ValueError:
#             print("Invalid input, please enter a valid number.")
#         except KeyboardInterrupt:
#             print("Exiting...")
#             break

#     ser.close()

#     # Proceed to calibration if needed
#     if calibrate and len(measured_vin) >= 4:
#         print("⚙️ Starting calibration process...")
#         coeffs = cubic_fit(measured_vin, actual_vin)

#         # Append coefficients to the calibration CSV file
#         with open(calib_filename, mode='a', newline='') as file:
#             writer = csv.writer(file)
#             writer.writerow([])
#             writer.writerow(["Cubic Fit Coefficients:"] + list(coeffs))

#         # Generate and update Arduino function
#         arduino_function = generate_arduino_formula(coeffs)
#         update_ino_file("adc_for_calib.ino", arduino_function)

#         print("✅ Calibration completed! The Arduino code has been updated.")

# def main():
#     board_no = get_board_number()
#     script_dir = os.path.dirname(os.path.abspath(__file__))
#     test_dir = os.path.join(script_dir, "tests")
#     os.makedirs(test_dir, exist_ok=True)

#     # Separate filenames for evaluation and calibration
#     eval_filename = os.path.join(test_dir, f"sarq_eval-{board_no}.csv")
#     calib_filename = os.path.join(test_dir, f"sarq_calib-{board_no}.csv")

#     port = list_serial_ports()
#     if port is None:
#         return
    
#     ser = setup_serial(port)
#     if ser is None:
#         return

#     while True:
#         # Ask user whether they want Calibration or Evaluation
#         mode = input("\nSelect mode - Calibration (c) or Evaluation (e): ").strip().lower()
#         while mode not in ['c', 'e']:
#             mode = input("Invalid choice. Enter 'c' for Calibration or 'e' for Evaluation: ").strip().lower()
        
#         calibrate = mode == 'c'

#         if calibrate:
#             print("🔧 Starting Calibration Mode...")
#             csv_file = calib_filename
#             headers = ["Measured VIN", "Actual VIN"]
#         else:
#             print("📊 Starting Evaluation Mode...")
#             csv_file = eval_filename
#             headers = ["Raw ADC", "ESP ADC Cal Raw Voltage", "Calculated VIN", "Calibrated VIN", "Actual Input", "Difference"]

#         # Open the correct CSV file and write headers
#         with open(csv_file, mode='w', newline='') as file:
#             writer = csv.writer(file)
#             writer.writerow(headers)

#         measured_vin = []
#         actual_vin = []

#         while True:
#             try:
#                 if calibrate:
#                     user_input = input("Enter 'a' to input Actual Voltage, or 'q' to finish calibration: ")
#                     if user_input.lower() == 'q':
#                         break
#                     elif user_input.lower() == 'a':
#                         actual_input = float(input("Actual Input Voltage: "))
#                         ser.write(b'a')

#                         print("Waiting for serial data...")
#                         while True:
#                             line = ser.readline().decode('utf-8').strip()
#                             if line:
#                                 print(f"Received: {line}")
#                                 match = re.search(r"Raw ADC: (\d+) \| ESP ADC Cal Raw to Voltage: ([\d.]+) \| Calculated VIN: ([\d.]+) \| Calibrated VIN: ([\d.]+)", line)
#                                 if match:
#                                     calculated_vin = float(match.group(3))  # Log only Calculated VIN
                                    
#                                     # Log to calibration file immediately
#                                     with open(calib_filename, mode='a', newline='') as file:
#                                         writer = csv.writer(file)
#                                         writer.writerow([calculated_vin, actual_input])

#                                     print(f"Logged: Measured VIN={calculated_vin}, Actual VIN={actual_input}")

#                                     measured_vin.append(calculated_vin)
#                                     actual_vin.append(actual_input)
#                                     break
#                             else:
#                                 print("No data received. Check if Arduino is sending data.")

#                 else:  # Evaluation Mode
#                     print("Waiting for serial data...")
#                     line = ser.readline().decode('utf-8').strip()
#                     if line:
#                         print(f"Received: {line}")
#                         match = re.search(r"Raw ADC: (\d+) \| ESP ADC Cal Raw to Voltage: ([\d.]+) \| Calculated VIN: ([\d.]+) \| Calibrated VIN: ([\d.]+)", line)
#                         if match:
#                             raw_adc = int(match.group(1))
#                             raw_voltage = float(match.group(2))
#                             calculated_vin = float(match.group(3))
#                             calibrated_vin = float(match.group(4))

#                             actual_input = input("Enter actual input voltage (or 'q' to quit): ").strip()
#                             if actual_input.lower() == 'q':
#                                 break
#                             actual_input = float(actual_input)
#                             difference = actual_input - calibrated_vin

#                             # Log to evaluation file immediately
#                             with open(eval_filename, mode='a', newline='') as file:
#                                 writer = csv.writer(file)
#                                 writer.writerow([raw_adc, raw_voltage, calculated_vin, calibrated_vin, actual_input, difference])

#                             print(f"Logged: Raw ADC={raw_adc}, Calibrated VIN={calibrated_vin}, Actual={actual_input}, Diff={difference}")

#             except ValueError:
#                 print("Invalid input, please enter a valid number.")
#             except KeyboardInterrupt:
#                 print("Exiting...")
#                 break

#         ser.close()

#         # Proceed to calibration if needed
#         if calibrate and len(measured_vin) >= 4:
#             print("⚙️ Starting calibration process...")
#             coeffs = cubic_fit(measured_vin, actual_vin)

#             # Append coefficients to the calibration CSV file
#             with open(calib_filename, mode='a', newline='') as file:
#                 writer = csv.writer(file)
#                 writer.writerow([])
#                 writer.writerow(["Cubic Fit Coefficients:"] + list(coeffs))

#             # Generate and update Arduino function
#             arduino_function = generate_arduino_formula(coeffs)
#             update_ino_file("adc_for_calib.ino", arduino_function)

#             print("✅ Calibration completed! The Arduino code has been updated.")

#         # After Calibration, Ask for Evaluation
#         if calibrate:
#             choice = input("\nDo you want to proceed with Evaluation Mode? (y/n): ").strip().lower()
#             if choice == 'y':
#                 mode = 'e'  # Set to Evaluation Mode
#                 continue  # Restart loop for Evaluation Mode
#             else:
#                 print("Exiting program.")
#                 break  # Exit loop if user chooses not to evaluate
#         else:
#             break  # Exit after evaluation

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
            # print("📊 Starting Evaluation Mode...")
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
            
            # low_coeffs, high_coeffs = piecewise_fit(measured_vin, actual_vin)
            # low_coeffs, mid_coeffs, high_coeffs, v_low, v_mid = piecewise_advanced_fit(measured_vin, actual_vin)
            # fit_result = calculate_fit(measured_vin, actual_vin)
            # low_coeffs = fit_result["low_coeffs"]
            # mid_coeffs = fit_result["mid_coeffs"]
            # high_coeffs = fit_result["high_coeffs"]
            # v_low = fit_result["v_low"]
            # v_mid = fit_result["v_mid"]

            # Append coefficients to the calibration CSV file
            with open(calib_filename, mode='a', newline='') as file:
                writer = csv.writer(file)
                writer.writerow([])
                writer.writerow(["Cubic Fit Coefficients:"] + list(coeffs))
                
                # writer.writerow(["Quadratic Fit Coefficients (Low Range):"] + list(low_coeffs))
                # if high_coeffs is not None and high_coeffs.size > 0:
                #     writer.writerow(["Cubic Fit Coefficients (High Range):"] + list(high_coeffs))

                # if fit_result["low_coeffs"] is not None:
                #     writer.writerow(["Quadratic Fit (Low):"] + list(fit_result["low_coeffs"]))
                
                # if fit_result["mid_coeffs"] is not None:
                #     writer.writerow(["Quadratic Fit (Mid):"] + list(fit_result["mid_coeffs"]))
                
                # if fit_result["high_coeffs"] is not None:
                #     writer.writerow(["Cubic Fit (High):"] + list(fit_result["high_coeffs"]))

            # Generate and update Arduino function
            arduino_function = generate_arduino_formula(coeffs)
            # arduino_function = generate_arduino_formula(low_coeffs, high_coeffs)
            # arduino_function = generate_arduino_formula_advanced(low_coeffs, mid_coeffs, high_coeffs, v_low, v_mid)
            # arduino_function = generate_arduino_function(fit_result)
            
            update_ino_file("adc_for_calib.ino", arduino_function)

            print("✅ Calibration completed! The Arduino code has been updated.")

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
