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
    Serial.begin(9600);

    // Initialize pins
    pinMode(SDpin, OUTPUT);
    digitalWrite(SDpin, HIGH);
    for (int i = minADC; i <= maxADC; i++) {
        pinMode(i, OUTPUT);
        digitalWrite(i, HIGH);
    }

    // Initialize SD and SPI
    SPI.begin();
    digitalWrite(SDpin, LOW);
    SD.begin();
    digitalWrite(SDpin, HIGH);
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));

    // Set ADC configuration registers
    for (int i = minADC; i <= maxADC; i++) {
        writeRegister(i, 0, 0x0E);
        writeRegister(i, 1, 0xC4);
        writeRegister(i, 2, 0xC0);
        writeRegister(i, 3, 0x00);
    }

    // Start conversions
    for (int i = minADC; i <= maxADC; i++) {
        startConversions(i);
    }
}

void loop() {
    // Take gauge readings
    gaugeData[0] = millis();
    for (int i = minADC; i <= maxADC; i++) {
        gaugeData[i - minADC + 1] = convertOutput(readData(i));
    }
    // Write data to SD card
    // writeToSD(gaugeData, dataLen, "data.csv");
    // Print data for testing
    for (int i = 0; i <= dataLen - 1; i++) {
        Serial.print(gaugeData[i]);
        if (i < dataLen - 1) Serial.print(", ");
    }
    Serial.println();
    delay(1000);
}

// Write integer data to SD card
void writeToSD(long* data, size_t len, String filename) {
    digitalWrite(SDpin, LOW);
    File dataFile = SD.open(filename, FILE_WRITE);
    if (dataFile != NULL) {
        for (int i = 0; i <= len - 1; i++) {
            dataFile.print(data[i]);
            if (i < len - 1) dataFile.print(",");
        }
        dataFile.println();
        dataFile.close();
    }
    digitalWrite(SDpin, HIGH);
}

// TODO: Adjust this function
// Convert output from ADC into a readable value
int convertOutput(unsigned long data) {
    // long minval = -1;
    // for (int i = 1; i <= 23; i++) minval = minval * 2;
    long maxval = 1;
    for (int i = 1; i <= 23; i++) maxval = maxval * 2;
    maxval = maxval - 1;
    return (1000 * (twosComplement(data, 24) )) / (maxval);
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

// Start ADC conversions
void startConversions(int chipSelect) {
    digitalWrite(chipSelect, LOW);
    SPI.transfer(0x08);  // Instruction to start conversions
    digitalWrite(chipSelect, HIGH);
}

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

// Convert an N-bit two's complement number to a long integer
long twosComplement(unsigned long num, unsigned char N) {
    long output;
    if (num >> (N - 1)) {
        output = -1 * (~((num - 1) << (33 - N)) >> (33 - N));
        if (output == 0) {
            output = -1;
            for (int i = 1; i <= N - 1; i++) {
                output = output * 2;
            }
        }
    } else {
        output = num;
    }
    return output;
}
