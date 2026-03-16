# 🏃‍♂️ Wireless Gait & Performance Tracker

An embedded system designed to capture biomechanical gait data from a user's ankle, measure heart rate at the wrist, and fuse the data for comprehensive performance tracking and offline analysis.

## 📝 Overview
This project uses an STM32-based architecture to monitor physical activity. The ankle node reads 6-DoF kinematic data and converts it into step data. This is transmitted over a 2.4GHz RF link to a wrist-mounted receiver. The wrist node concurrently measures heart rate, fuses it with the incoming step data, and writes the combined dataset to an SD card for visualization via a Python script.

## 🗺️ System Architecture
![System Schematic](documents/schematics/schematic(abstract).drawio.png)

## 🛠️ Hardware Architecture

### 1. Ankle Node (Step Tracker & Transmitter)
* **Microcontroller:** STM32 Nucleo-F303RE
* **Sensor:** MPU6050 (6-Axis IMU)
* **Wireless:** nRF24L01+ Transceiver

### 2. Wrist Node (Heart Rate, Receiver & Logger)
* **Microcontroller:** STM32 
* **Sensor:** Heart Rate Monitor
* **Wireless:** nRF24L01+ Transceiver
* **Storage:** MicroSD Card Module

## 🧠 Algorithms & Data Flow

### Ankle Node: IMU to Step Conversion
The ankle device processes raw MPU6050 data (acceleration and rotation) to identify discrete steps before transmission, reducing the required wireless bandwidth.
![IMU Algorithm](documents/schematics/IMU_Algorithm(Abstarct).drawio.png)

### Wrist Node: Sensor Fusion
The wrist device runs an algorithm to measure the user's heart rate and fuses this data in real-time with the step data received from the ankle device.
![Heart Rate Algorithm](documents/schematics/HeartRate_Algorithm(Abstract).drawio.png)

## 💻 Software Stack
* **Firmware:** C (STM32 HAL Library configured via CubeMX)
* **Data Visualization:** Python (`matplotlib`, `pandas`)

## ⚙️ How to Use
1. Flash the respective firmware to the Ankle and Wrist nodes.
2. Insert a FAT32-formatted SD card into the Wrist Node.
3. Power both devices. The system will automatically begin measuring, fusing, and logging the data.
4. Remove the SD card, transfer `data.csv` to your PC, and run the Python visualization script.

---
**License:** MIT
