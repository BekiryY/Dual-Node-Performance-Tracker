# Complete Setup Guide for STM32CubeIDE

## 🎯 Method 1: Import .ioc File (FASTEST - 5 minutes)

### Step 1: Create New Project
1. Open **STM32CubeIDE**
2. **File** → **New** → **STM32 Project**
3. Select **STM32F446RETx** from the MCU list
4. Click **Next**
5. Project Name: `STM32F446RE_DataLogger`
6. Click **Finish**

### Step 2: Import Configuration
1. Close the `.ioc` file that opens automatically
2. Copy the **STM32F446RE.ioc** file I provided above
3. Save it as `STM32F446RE_DataLogger.ioc` in your project root folder
4. Double-click the `.ioc` file in CubeIDE
5. Click **Yes** to generate code
6. **Project** → **Generate Code**

### Step 3: Add Your Custom Files

#### A. Copy Core Files
Navigate to your project folder, then:

**In `Core/Inc/` folder, add:**
- `max30102.h`
- `nrf24.h`

**In `Core/Src/` folder, add:**
- `max30102.c`
- `nrf24.c`

**Replace these files:**
- Replace `Core/Src/main.c` with your complete main.c
- Replace `Core/Inc/main.h` with your complete main.h
- Replace `Core/Src/stm32f4xx_hal_msp.c` with the MSP file from artifacts

#### B. Replace FATFS Files

**Replace `FATFS/App/fatfs.c`** with the one from artifacts
**Replace `FATFS/App/fatfs.h`** with the one from artifacts

**Replace `FATFS/Target/user_diskio.c`** with the one from artifacts
**Replace `FATFS/Target/user_diskio.h`** with the one from artifacts

**In `Middlewares/Third_Party/FatFs/src/` folder:**
- Replace `ffconf.h` with the one from artifacts
- Replace `diskio.c` and `diskio.h` with the ones from artifacts
- Add `ff_gen_drv.c` and `ff_gen_drv.h` from artifacts
- **Keep the existing `ff.c` and `ff.h`** (already provided by CubeMX!)

### Step 4: Project Configuration

Right-click project → **Properties** → **C/C++ Build** → **Settings** → **Tool Settings**

#### MCU GCC Compiler → Miscellaneous
Add to "Other flags":
```
-u _printf_float
```

#### MCU GCC Linker → Libraries
Add to Libraries (-l):
```
m
```

Click **Apply and Close**

### Step 5: Build and Upload
1. **Project** → **Build All** (Ctrl+B)
2. Connect STM32 Nucleo board via USB
3. **Run** → **Debug** (F11) or **Run** (Ctrl+F11)
4. Open **Serial Monitor** (115200 baud)

---

## 🎯 Method 2: Manual Configuration (If .ioc doesn't work)

### Step 1: Create New STM32 Project
1. **File** → **New** → **STM32 Project**
2. Select **STM32F446RETx**
3. Name: `STM32F446RE_DataLogger`
4. Click **Finish**

### Step 2: Configure Peripherals in .ioc

#### Pinout & Configuration Tab:

**System Core → RCC:**
- High Speed Clock (HSE): Disable
- Low Speed Clock (LSE): Disable

**System Core → SYS:**
- Debug: Serial Wire

**Connectivity → I2C1:**
- Mode: I2C
- I2C Speed: 400000 Hz (Fast Mode)
- Pins: PB8 (SCL), PB9 (SDA)

**Connectivity → SPI1:**
- Mode: Full-Duplex Master
- Prescaler: 16
- Pins: PA5 (SCK), PA6 (MISO), PA7 (MOSI)

**Connectivity → SPI3:**
- Mode: Full-Duplex Master
- Prescaler: 8
- Pins: PC10 (SCK), PC11 (MISO), PC12 (MOSI)

**Connectivity → USART2:**
- Mode: Asynchronous
- Baud Rate: 115200
- Pins: PA2 (TX), PA3 (RX)

**System Core → GPIO:**
- PA4: GPIO_Output (Label: NRF24_CSN)
- PA5: Already configured (LED - built-in)
- PA15: GPIO_Output (Label: SD_CS)
- PC7: GPIO_Output (Label: NRF24_CE)

**Middleware → FATFS:**
- Mode: User-defined
- Check "Set Defines"

#### Clock Configuration Tab:
- Input frequency: 16 MHz (HSI)
- PLLM: 8
- PLLN: 180
- PLLP: 2
- System Clock Mux: PLLCLK
- HCLK: 180 MHz
- APB1 Prescaler: /4 (45 MHz)
- APB2 Prescaler: /2 (90 MHz)

#### Project Manager Tab:
- Project Name: STM32F446RE_DataLogger
- Project Location: Choose your workspace
- Toolchain: STM32CubeIDE
- Click **GENERATE CODE**

### Step 3: Add Files (Same as Method 1, Step 3)

### Step 4: Build Configuration (Same as Method 1, Step 4)

---

## 📋 Complete File Checklist

After setup, verify you have these files:

### Core Files
```
Core/
├── Inc/
│   ├── main.h                      ✅ Your version
│   ├── max30102.h                  ✅ Your file
│   ├── nrf24.h                     ✅ Your file
│   ├── stm32f4xx_hal_conf.h        ✅ Auto-generated
│   └── stm32f4xx_it.h              ✅ Auto-generated
└── Src/
    ├── main.c                      ✅ Your version
    ├── max30102.c                  ✅ Your file
    ├── nrf24.c                     ✅ Your file
    ├── stm32f4xx_hal_msp.c         ✅ Your version
    ├── stm32f4xx_it.c              ✅ Auto-generated
    ├── system_stm32f4xx.c          ✅ Auto-generated
    └── syscalls.c                  ✅ Auto-generated
```

### FATFS Files
```
FATFS/
├── App/
│   ├── fatfs.c                     ✅ Your version
│   └── fatfs.h                     ✅ Your version
└── Target/
    ├── user_diskio.c               ✅ Your version
    └── user_diskio.h               ✅ Your version
```

### Middleware Files
```
Middlewares/Third_Party/FatFs/src/
├── ff.c                            ✅ From CubeMX (keep it!)
├── ff.h                            ✅ From CubeMX (keep it!)
├── ffconf.h                        ✅ Your version
├── diskio.c                        ✅ Your version
├── diskio.h                        ✅ Your version
├── ff_gen_drv.c                    ✅ Your file (add it)
└── ff_gen_drv.h                    ✅ Your file (add it)
```

---

## 🔧 Common Issues & Solutions

### Issue 1: "undefined reference to `_write`"
**Solution:** Already included in your `main.c` - make sure you didn't delete it

### Issue 2: "undefined reference to sqrt/fabs"
**Solution:** Add `-lm` to linker libraries (Step 4 above)

### Issue 3: Printf doesn't show floats
**Solution:** Add `-u _printf_float` to compiler flags (Step 4 above)

### Issue 4: FATFS errors during compilation
**Solution:** Make sure `ffconf.h` is using your version from artifacts, not the default

### Issue 5: I2C/SPI not working
**Solution:** Verify pins are correctly configured in `.ioc` file and regenerate code

---

## 🚀 Quick Test Procedure

### 1. First Compile (No Hardware)
```
Project → Clean
Project → Build All
```
Should compile with **0 errors**

### 2. Connect Hardware
- Connect MAX30102 to I2C1 (PB8, PB9)
- Connect nRF24L01 to SPI1 (PA4, PA5, PA6, PA7, PC7)
- Connect SD Card to SPI3 (PA15, PC10, PC11, PC12)
- Connect USB cable

### 3. Upload and Monitor
```
Run → Debug (F11)
Window → Show View → Console
```

Expected output:
```
========================================
  STM32F446RE Data Logger
  MAX30102 + nRF24L01 + SD Card
========================================

Initializing MAX30102...
MAX30102 initialized successfully!
Initializing nRF24L01...
nRF24L01 initialized! Payload size: 28 bytes
Mounting SD Card...
SD Card mounted successfully!
CSV file ready

System Ready! Waiting for data...
Place finger on MAX30102 sensor
```

---

## 💾 Project Backup

After successful setup, backup these folders:
- **Core/** (all your custom code)
- **FATFS/** (modified files)
- **Middlewares/Third_Party/FatFs/src/** (modified files)
- **.ioc file** (project configuration)

---

## 🎓 Pro Tips

1. **Enable optimization:** Properties → C/C++ Build → Settings → Optimization → -O1
2. **Increase heap size:** .ioc → Project Manager → Project → Minimum Heap Size: 0x400
3. **Use SWV for debugging:** Run → Debug Configurations → Startup → Enable Serial Wire Viewer
4. **Monitor variables:** Debug view → Add variables to watch window

---

## ✅ You're Done!

Your project is now ready to:
- ✅ Receive nRF24L01 wireless data
- ✅ Read MAX30102 heart rate and SpO2
- ✅ Log combined data to SD card
- ✅ Display everything via serial monitor

**Next:** Test with real sensors and transmitter!