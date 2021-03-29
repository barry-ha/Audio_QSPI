# Audio_QSPI
A library to open and play Microsoft WAV audio PCM files from Quad-SPI memory on Feather M4 Express.

This is the audio programming guide and library that I wanted (needed!) when I started using WAV files on the Feather M4 Express. It was a long process to pull together all the different parts of:

   - creating sound bites, 
   - storing large audio on the Feather,
   - decoding WAV file headers, and 
   - playing audio via the built-in DAC (digital to analog converter).

I hope this library is useful or at least illustrates some techniques. Drop me an email if you like it or need a feature. The information presented here is available if you look long enough on the web, but it's spread out or tantalizingly inapplicable. For example, the Audio library by Paul Stoffringeng is a brilliant and highly polished set of features but it only works on the Teensy (not the Feather M4).

To keep it simple, this library is designed for short audio clips of less than 2 seconds each. It plays WAV files on the main level and blocks the main thread until it finishes. It does not use interrupts. It has only been tested with 16-bit PCM audio files at 16 KHz.

This library was developed for the Griduino project, a device for a vehicle's dashboard. Along with many other features, it announces its location by speaking letters and numbers using the NATO Military Phonetic Alphabet.

   - Griduino runs on the Feather M4 Express, https://www.adafruit.com/product/3857
   - Griduino software is an open-source project, https://github.com/barry-ha/Griduino
   - Griduino kits are available, https://www.griduino.com.

## How to Prepare Audio Files
Prepare a WAV file to 16 kHz mono:

1. Install free open-source [Audacity software](https://www.audacityteam.org/download/) <img align="right" src="img/audacity_logo.png" width="10%" height="10%" alt="logo" title="Audacity Logo"/>
1. Open Audacity
1. Open a project, e.g. \Documents\Arduino\Griduino\work-in-progress\Spoken Word Originals\Barry
1. Select "Project rate" of 16000 Hz
1. Select an audio fragment, such as spoken word "Charlie"
1. Menu bar > Effect > Normalize:
   1. Remove DC offset
   1. Normalize peaks -1.0 dB
1. Menu bar > File > Export > Export as WAV
   1. Save as type: WAV (Microsoft)
   1. Encoding: Signed 16-bit PCM
   1. Filename = e.g. "c-bwh-16.wav"
1. The output file contains 2-byte integer numbers in the range -32767 to +32767

## How to Transfer Audio Files
1. Format QSPI file system to CircuitPy format (one time).<br/>Formatting is only done once; it erases everything on the memory chip and then the file system will remain compatible with both CircuitPy and Arduino IDE frameworks thereafter.

1. To save files from Windows onto the QSPI memory chip on Feather M4 Express:
   1. Temporarily load CircuitPy onto the Feather
   1. Drag-and-drop files from within Windows to Feather
   1. Then load your Arduino sketch again.
   
## How to Examine Flash File System on Feather M4
After transferring files to the Quad-SPI memory chip, you'll want to confirm what has been stored in the SD file system. Here's how...

_todo - link to the Griduino example "Flash-file-directory-list"_

## How to Read WAV File Header
Some Arduino programs may want to read attributes from the WAV file. For example, it may want to show the file size, bit rate, mono/stereo, and other characteristics. This is useful for at least debugging purposes. 

Call the **getInfo()** method to fetch metadata from a WAV file. Here's how...

_todo - show example usage of audio-qspi.getInfo(&info, myfile);_

## How to Play WAV Files
This might be what you really came for: reading the WAV file from Flash memory and playing it on the Feather M4.

Call the **play()** method to send PCM sampled audio to the Feather's DAC pin. Here's how...

_todo - show example usage of audio-qspi.play(myfile);_


