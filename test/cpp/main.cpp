// Copyright (c) Chris Hafey.
// SPDX-License-Identifier: MIT

#include "../../src/JpegXLDecoder.hpp"
#include "../../src/JpegXLEncoder.hpp"


#include <fstream>
#include <iostream>
#include <vector>
#include <iterator>
#include <time.h> 
#include <algorithm>


void readFile(std::string fileName, std::vector<uint8_t>& vec) {
    // open the file:
    std::ifstream file(fileName, std::ios::in | std::ios::binary);
    // Stop eating new lines in binary mode!!!
    file.unsetf(std::ios::skipws);

    // get its size:
    std::streampos fileSize;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // reserve capacity
    vec.reserve(fileSize);

    // read the data:
    vec.insert(vec.begin(),
                std::istream_iterator<uint8_t>(file),
                std::istream_iterator<uint8_t>());

    //std::istreambuf_iterator iter(file);
    //std::copy(iter.begin(), iter.end(), std::back_inserter(vec));
}

void writeFile(std::string fileName, const std::vector<uint8_t>& vec) {
    std::ofstream file(fileName, std::ios::out | std::ofstream::binary);
    std::copy(vec.begin(), vec.end(), std::ostreambuf_iterator<char>(file));
}

enum { NS_PER_SECOND = 1000000000 };

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td)
{
    td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
    td->tv_sec  = t2.tv_sec - t1.tv_sec;
    if (td->tv_sec > 0 && td->tv_nsec < 0)
    {
        td->tv_nsec += NS_PER_SECOND;
        td->tv_sec--;
    }
    else if (td->tv_sec < 0 && td->tv_nsec > 0)
    {
        td->tv_nsec -= NS_PER_SECOND;
        td->tv_sec++;
    }
}

int decodeFile(const char* path) {
    JpegXLDecoder decoder;
    std::vector<uint8_t>& encodedBytes = decoder.getEncodedBytes();
    readFile(path, encodedBytes);

    timespec start, finish, delta;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    int result = decoder.decode();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);
    sub_timespec(start, finish, &delta);
    const double ms = (double)delta.tv_sec * 1000.0 + (double)(delta.tv_nsec) / 1000000.0;
    printf("Decode of %s took %f ms and returned %d\n", path, ms, result);
    return result;
}

void encodeFile(const char* inPath, const FrameInfo frameInfo, int offset, const char* outPath) {
    JpegXLEncoder encoder;
    std::vector<uint8_t>& rawBytes = encoder.getDecodedBytes(frameInfo);
    readFile(inPath, rawBytes);
    short* pOffset = (short*)rawBytes.data();
    for(int i=0; i < rawBytes.size() /2; i++) {
        *pOffset += offset;
        pOffset++;
    }

    timespec start, finish, delta;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    encoder.encode();

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);
    sub_timespec(start, finish, &delta);
    const double ms = (double)delta.tv_sec * 1000.0 + (double)(delta.tv_nsec) / 1000000.0;
    printf("Encode of %s took %f ms (%ld bytes, %f:1 compression ratio)\n", inPath, ms, encoder.getEncodedBytes().size(), (float)rawBytes.size() / encoder.getEncodedBytes().size() );

    if(outPath) {
        const std::vector<uint8_t>& encodedBytes = encoder.getEncodedBytes();
        writeFile(outPath, encodedBytes);
    }
}

void roundTrip(const char* inPath, const FrameInfo frameInfo, int offset, const char* outPath)
{
    JpegXLEncoder encoder;
    encoder.setEffort(1);
    std::vector<uint8_t>& rawBytes = encoder.getDecodedBytes(frameInfo);
    readFile(inPath, rawBytes);
    short* pOffset = (short*)rawBytes.data();
    for(int i=0; i < rawBytes.size() /2; i++) {
        *pOffset += offset;
        pOffset++;
    }

    timespec start, finish, delta;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    encoder.encode();

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);
    sub_timespec(start, finish, &delta);
    double ms = (double)delta.tv_sec * 1000.0 + (double)(delta.tv_nsec) / 1000000.0;
    printf("Encode of %s took %f ms (%ld bytes, %f:1 compression ratio)\n", inPath, ms, encoder.getEncodedBytes().size(), (float)rawBytes.size() / encoder.getEncodedBytes().size() );

    JpegXLDecoder decoder;
    const std::vector<uint8_t>& encEncodedBytes = encoder.getEncodedBytes();
    std::vector<uint8_t>& decEncodedBytes = decoder.getEncodedBytes();
    decEncodedBytes.resize(encEncodedBytes.size());
    for(int i=0; i < encEncodedBytes.size(); i++) {
        decEncodedBytes[i] = encEncodedBytes[i];
    }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    int result = decoder.decode();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);
    sub_timespec(start, finish, &delta);
    ms = (double)delta.tv_sec * 1000.0 + (double)(delta.tv_nsec) / 1000000.0;
    printf("Decode of %s took %f ms and returned %d\n", inPath, ms, result);

    const std::vector<uint8_t>& decodedBytes = decoder.getDecodedBytes();
    unsigned short* pDecodedBytes = (unsigned short*)decodedBytes.data();
    unsigned short* pRawBytes = (unsigned short*)rawBytes.data();

    for(int i=0; i < decodedBytes.size() / 2; i++) {
        if(*pRawBytes != *pDecodedBytes) {
            printf("mismatch at offset %d\n", i);
            return;
        }
        pDecodedBytes++;
        pRawBytes++;
    }

}

int main(int argc, char** argv) {
    roundTrip("test/fixtures/raw/CT1.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 2000, "test/fixtures/jxl/CT1.jxl");
    roundTrip("test/fixtures/raw/CT2.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 2048, "test/fixtures/jxl/CT2.jxl");
    roundTrip("test/fixtures/raw/MG1.RAW", {.width = 3064, .height = 4664, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MG1.jxl");
    roundTrip("test/fixtures/raw/MR1.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MR1.jxl");
    roundTrip("test/fixtures/raw/MR2.RAW", {.width = 1024, .height = 1024, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MR2.jxl");
    roundTrip("test/fixtures/raw/MR3.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MR3.jxl");
    roundTrip("test/fixtures/raw/MR4.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MR4.jxl");
    roundTrip("test/fixtures/raw/NM1.RAW", {.width = 256, .height = 1024, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/NM1.jxl");
    roundTrip("test/fixtures/raw/RG1.RAW", {.width = 1841, .height = 1955, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/RG1.jxl");
    roundTrip("test/fixtures/raw/RG2.RAW", {.width = 1760, .height = 2140, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/RG2.jxl");
    roundTrip("test/fixtures/raw/RG3.RAW", {.width = 1760, .height = 1760, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/RG3.jxl");
    roundTrip("test/fixtures/raw/SC1.RAW", {.width = 2048, .height = 2487, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/SC1.jxl");
    roundTrip("test/fixtures/raw/XA1.RAW", {.width = 1024, .height = 1024, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/XA1.jxl");

    /*
    encodeFile("test/fixtures/raw/CT1.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 2000, "test/fixtures/jxl/CT1.jxl");
    encodeFile("test/fixtures/raw/CT2.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 2048, "test/fixtures/jxl/CT2.jxl");
    encodeFile("test/fixtures/raw/MG1.RAW", {.width = 3064, .height = 4664, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MG1.jxl");
    encodeFile("test/fixtures/raw/MR1.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MR1.jxl");
    encodeFile("test/fixtures/raw/MR2.RAW", {.width = 1024, .height = 1024, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MR2.jxl");
    encodeFile("test/fixtures/raw/MR3.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MR3.jxl");
    encodeFile("test/fixtures/raw/MR4.RAW", {.width = 512, .height = 512, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/MR4.jxl");
    encodeFile("test/fixtures/raw/NM1.RAW", {.width = 256, .height = 1024, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/NM1.jxl");
    encodeFile("test/fixtures/raw/RG1.RAW", {.width = 1841, .height = 1955, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/RG1.jxl");
    encodeFile("test/fixtures/raw/RG2.RAW", {.width = 1760, .height = 2140, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/RG2.jxl");
    encodeFile("test/fixtures/raw/RG3.RAW", {.width = 1760, .height = 1760, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/RG3.jxl");
    encodeFile("test/fixtures/raw/SC1.RAW", {.width = 2048, .height = 2487, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/SC1.jxl");
    encodeFile("test/fixtures/raw/XA1.RAW", {.width = 1024, .height = 1024, .bitsPerSample = 16, .componentCount = 1}, 0, "test/fixtures/jxl/XA1.jxl");
    */
    /*
    decodeFile("test/fixtures/jxl/CT1.jxl");
    decodeFile("test/fixtures/jxl/CT2.jxl");
    decodeFile("test/fixtures/jxl/MG1.jxl");
    decodeFile("test/fixtures/jxl/MR1.jxl");
    decodeFile("test/fixtures/jxl/MR2.jxl");
    decodeFile("test/fixtures/jxl/MR3.jxl");
    decodeFile("test/fixtures/jxl/MR4.jxl");
    decodeFile("test/fixtures/jxl/NM1.jxl");
    decodeFile("test/fixtures/jxl/RG1.jxl");
    decodeFile("test/fixtures/jxl/RG2.jxl");
    decodeFile("test/fixtures/jxl/RG3.jxl");
    decodeFile("test/fixtures/jxl/SC1.jxl");
    decodeFile("test/fixtures/jxl/XA1.jxl");
    */
    return 0;
}
