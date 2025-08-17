
#include <SPI.h>
#include <SD.h>

// Chip select (CS) pins
#define SDpin 9
#define minADC A0
#define maxADC A1

// Create data array
const size_t dataLen = (1 + maxADC - minADC) + 1;  // Length = number of ADCs, + 1 for time stamp
long* gaugeData = (long*) calloc(dataLen, sizeof(long));

void setup() {
    // Serial.begin(9600);
    // Serial.println();

    // Initialize pins
    pinMode(SDpin, OUTPUT);
    digitalWrite(SDpin, HIGH);
    for (int i = A0; i <= A1; i++) {
        pinMode(i, OUTPUT);
        digitalWrite(i, HIGH);
    }

    // Initialize SD and SPI
    SPI.begin();
    digitalWrite(SDpin, LOW);
    SD.begin();
    digitalWrite(SDpin, HIGH);
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE1));

    // Set ADC configuration registers
    for (int i = minADC; i <= maxADC; i++) {
        writeRegister(i, 0, 0x6E);
        writeRegister(i, 1, 0xC4);
        writeRegister(i, 2, 0xC0);
        writeRegister(i, 3, 0x00);
    }

    // Start conversions
    for (int i = minADC; i <= maxADC; i++) {
        startConversions(i);
    }

    // Mark start of new recording session in data file
    digitalWrite(SDpin, LOW);
    File dataFile = SD.open("data.csv", FILE_WRITE);
    if (dataFile != NULL) {
        dataFile.println("----------------------------------------------------------------------------------------------------");
        dataFile.close();
    }
    digitalWrite(SDpin, HIGH);
}

void loop() {
    // Save gauge readings in array
    gaugeData[0] = millis();
    for (int i = minADC; i <= maxADC; i++) {
        gaugeData[1 + i - minADC] = toLong(readData(i));
    }
    // Write data to SD card
    writeToSD(SDpin, gaugeData, dataLen, "data.csv");
    // Print data to serial port (for debugging)
    // printToPort(gaugeData, dataLen);
}


//------------------------- Functions for handling SPI devices and debugging -------------------------

// Write to an ADC configuration register
void writeRegister(int chipSelect, unsigned char reg, unsigned long settings) {
    if (reg >= 0 && reg <= 3) {
        digitalWrite(chipSelect, LOW);
        SPI.transfer(0x40 + (reg << 2));  // Instruction to write one byte at register "reg"
        SPI.transfer(settings);  // Settings for register "reg"
        digitalWrite(chipSelect, HIGH);
    }
}

// Read ADC configuration registers
unsigned long readRegisters(int chipSelect) {
    unsigned long output;
    digitalWrite(chipSelect, LOW);
    SPI.transfer(0x23);  // Instruction to read 4 bytes starting at register 0
    for (int i = 1; i <= 4; i++) {
        output = (output << 8) + SPI.transfer(0);
    }
    digitalWrite(chipSelect, HIGH);
    return output;
}

// Start ADC conversions
void startConversions(int chipSelect) {
    digitalWrite(chipSelect, LOW);
    SPI.transfer(0x08);  // Instruction to start conversions
    digitalWrite(chipSelect, HIGH);
}

// Read raw data from ADC
unsigned long readData(int chipSelect) {
    digitalWrite(chipSelect, LOW);
    SPI.transfer(0x10);  // Instruction to read data
    unsigned long data = 0;
    for (int i = 1; i <= 3; i++) {  // Get 3 data bytes
        data = (data << 8) + SPI.transfer(0);
    }
    digitalWrite(chipSelect, HIGH);
    return data;
}

// Convert N-bit two's complement number to long (32-bit) integer
long toLong(unsigned long num) {
    unsigned char N = 24;
    unsigned long output;
    if (N >= 32) {  // N must be less than 32
        output = -999999999;
    } else {
        if (num >> (N - 1)) {  // Check if 1st bit (i.e. sign bit) is 1
            if (!(num << (33 - N))) {  // If all other bits are 0, number is -2^(N-1)
                output = -1;
                for (int i = 1; i <= N - 1; i++) {
                    output = output * 2;
                }
            } else {  // Otherwise, do the procedure
                output = num - 1;  // Subtract 1
                output = ~output;  // Invert bits
                output = (output << (33 - N)) >> (33 - N);  // Set extra leading bits to 0
                output = output * -1;  // Make negative
            }
        } else {  // If sign bit is 0, number stays the same
            output = num;
        }
    }
    return output;
}

// Write (long) integer data array to SD card
void writeToSD(int chipSelect, long* data, size_t len, String filename) {
    digitalWrite(chipSelect, LOW);
    File file = SD.open(filename, FILE_WRITE);
    if (file != NULL) {
        for (int i = 0; i <= len - 1; i++) {
            file.print(data[i]);
            if (i < len - 1) file.print(",");
        }
        file.println();
        file.close();
    }
    digitalWrite(chipSelect, HIGH);
}

// Print (long) integer data array to serial port (for debugging)
void printToPort(long* data, size_t len) {
    for (int i = 0; i <= len - 1; i++) {
        Serial.print(data[i]);
        if (i < len - 1) Serial.print(",");
    }
    Serial.println();
}

