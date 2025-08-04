#include <SPI.h>
#include <SD.h>

#define numGauges 2  // Number of gauges should be <= number of ADCs * 2

// Chip select (CS) pins
#define SDpin 9
#define minADC A0
#define maxADC A0

// Create data array
const size_t dataLen = numGauges + 1;  // Length = number of gauges + 1 for time stamp
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

    // // Reset ADCs
    // for (int i = minADC; i <= maxADC; i++) {
    //     digitalWrite(i, LOW);
    //     SPI.transfer(0x06);  // Instruction to reset ADC
    //     digitalWrite(i, HIGH);
    // }
    // delay(250);

    // Set ADC configuration registers
    for (int i = minADC; i <= maxADC; i++) {
        digitalWrite(i, LOW);
        SPI.transfer(0x43);  // Instruction to write 4 bytes starting at register 0
        SPI.transfer(0x0E);  // Settings for register 0
        SPI.transfer(0xC4);  // Settings for register 1
        SPI.transfer(0xC0);  // Settings for register 2
        SPI.transfer(0x00);  // Settings for register 3
        digitalWrite(i, HIGH);
        // Serial.println(readConfig(i), HEX);
    }

    // // Start conversions
    // for (int i = minADC; i <= maxADC; i++) {
    //     digitalWrite(i, LOW);
    //     SPI.transfer(0x08);  // Instruction to start conversions
    //     digitalWrite(i, HIGH);
    // }
}

void loop() {
}

// Write integer data to SD card
void writeToSD(long* data, size_t len, String filename) {
    digitalWrite(SDpin, LOW);
    File dataFile = SD.open(filename, FILE_WRITE);
    if (dataFile != NULL) {
        for (int i = 0; i <= len - 1; i++) {
            dataFile.print(data[i]);
            if (i < len - 1) dataFile.print(", ");
        }
        dataFile.println();
        dataFile.close();
    }
    digitalWrite(SDpin, HIGH);
}

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

// Read ADC configuration registers
unsigned long readConfig(int chipSelect) {
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
