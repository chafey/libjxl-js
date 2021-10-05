let libjxljs = require('../../dist/libjxljs.js');
const fs = require('fs')

function decode(pathToJPEGXLFile, iterations = 1) {
  const encodedBitStream = fs.readFileSync(pathToJPEGXLFile);
  const decoder = new libjxljs.JpegXLDecoder();
  const encodedBuffer = decoder.getEncodedBuffer(encodedBitStream.length);
  encodedBuffer.set(encodedBitStream);

  // do the actual benchmark
  const beginDecode = process.hrtime();
  for(var i=0; i < iterations; i++) {
    decoder.decode();
  }
  const decodeDuration = process.hrtime(beginDecode); // hrtime returns seconds/nanoseconds tuple
  const decodeDurationInSeconds = (decodeDuration[0] + (decodeDuration[1] / 1000000000));
  
  // Print out information about the decode
  console.log("Decode of " + pathToJPEGXLFile + " took " + ((decodeDurationInSeconds / iterations * 1000)) + " ms");
  const frameInfo = decoder.getFrameInfo();
  console.log('  frameInfo = ', frameInfo);
  var decoded = decoder.getDecodedBuffer();
  console.log('  decoded length = ', decoded.length);

  decoder.delete();
}

function encode(pathToUncompressedImageFrame, imageFrame, iterations = 1) {
  const uncompressedImageFrame = fs.readFileSync(pathToUncompressedImageFrame);
  const encoder = new libjxljs.JpegXLEncoder();
  encoder.setEffort(1)
  const decodedBytes = encoder.getDecodedBuffer(imageFrame);
  decodedBytes.set(uncompressedImageFrame);

  const encodeBegin = process.hrtime();
  for(var i=0; i < iterations;i++) {
    encoder.encode();
  }
  const encodeDuration = process.hrtime(encodeBegin);
  const encodeDurationInSeconds = (encodeDuration[0] + (encodeDuration[1] / 1000000000));
  
  // print out information about the encode
  console.log("Encode of " + pathToUncompressedImageFrame + " took " + ((encodeDurationInSeconds / iterations * 1000)) + " ms");
  const encodedBytes = encoder.getEncodedBuffer();
  console.log('  encoded length=', encodedBytes.length)

  // cleanup allocated memory
  encoder.delete();
}

libjxljs.onRuntimeInitialized = async _ => {

  encode('../fixtures/raw/CT2.RAW', {width: 512, height: 512, bitsPerSample: 16, componentCount: 1});

  decode('../fixtures/jxl/CT1.jxl');
  decode('../fixtures/jxl/CT2.jxl');
  decode('../fixtures/jxl/MG1.jxl');
  decode('../fixtures/jxl/MR1.jxl');
  decode('../fixtures/jxl/MR2.jxl');
  decode('../fixtures/jxl/MR3.jxl');
  decode('../fixtures/jxl/MR4.jxl');
  decode('../fixtures/jxl/NM1.jxl');
  decode('../fixtures/jxl/RG1.jxl');
  decode('../fixtures/jxl/RG2.jxl');
  decode('../fixtures/jxl/RG3.jxl');
  decode('../fixtures/jxl/SC1.jxl');
  decode('../fixtures/jxl/XA1.jxl');
}
