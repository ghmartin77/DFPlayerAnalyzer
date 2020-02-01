# DFPlayerAnalyzer
Test driver to analyze different makes of DFPlayerMini and reveal specials in behavior.

Connect your Arduino/DFPlayer model like this:
```
D2 ---- 1k ----- RX(DFPlayer)
D3 ------------- TX(DFPlayer)
VCC ------------ VCC(DFPlayer)
GND ------------ GND(DFPlayer
```

Optional: Connect USB Stick to D+/D- of DFPlayer (and VCC/GND accordingly).

Upload the sketch to an Arduino (e.g. Nano), configure console for 115200 Baud, run the sketch, collect the output and [create an issue for your player version](https://github.com/ghmartin77/DFPlayerAnalyzer/issues?q=is%3Aissue+is%3Aopen+sort%3Aupdated-desc) if not already available.

Setup of SD card (minimum):
```
/01/001.mp3 (arbitrary MP3, 10-15 secs)
/01/002.mp3 (arbitrary MP3, 10-15 secs)
/ADVERT/0100.mp3 (arbitrary MP3, 10-15 secs)
```

Setup of USB stick (minimum):
Same as for SD card.

Beside all the nitty-gritty communication details of the test run, you'll get a summary like this:
```
-------------------------------------------------------
 Profile of this DFPlayer device
-------------------------------------------------------
The following devices have been discovered:
   -> SD, discovered by getTotalTrackCount
   -> USB, announced on reset, discovered by getTotalTrackCount, announced on setPlaysourceFlash
Does NOT react on 0x3F queries
getFolderTrackCount...
   -> for SD returned 3 files in Folder /01/ 
   -> for USB returned 55 files in Folder /01/ 
GetCurrentTrack for SD returns correct value 0ms after start of track
GetCurrentTrack for USB returns correct value 0ms after start of track
Sends 1 callback(s) on SD track end
Sends 1 callback(s) on USB track end
Continue playback after Ad for SD works
Continue playback after Ad for USB works
Wakeup from sleep by reset works
Wakeup from sleep by setting playsource SD works
Wakeup from sleep by setting playsource USB works


-------------------------------------------------------------------------------
 ALL TESTS COMPLETED!
```

The driver library used is a slightly modified version of this great implementation: https://github.com/Makuna/DFMiniMp3
