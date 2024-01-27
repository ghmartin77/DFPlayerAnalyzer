#include <Arduino.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#define DEBUG_DFPLAYER_COMMUNICATION
#include "DFMiniMp3.h"

#define VERSION "1.0"

class Mp3Notify;

void initDFPlayer();
void readFolderTrackCounts();
boolean isPlaying();

SoftwareSerial softSerial(3, 2); // RX, TX

volatile boolean error = false;
volatile uint16_t errCode = 0;

volatile uint16_t lastGlobalTrackFinished = -1;

boolean sdAvailable = false;
boolean usbAvailable = false;

uint8_t cntErrorCallback = 0;
uint8_t cntPlayFinishedCallback = 0;
uint8_t cntCardOnlineCallback = 0;
uint8_t cntCardInsertedCallback = 0;
uint8_t cntCardRemovedCallback = 0;
uint8_t cntUsbOnlineCallback = 0;
uint8_t cntUsbInsertedCallback = 0;
uint8_t cntUsbRemovedCallback = 0;

unsigned long nowMs;

DFMiniMp3<SoftwareSerial, Mp3Notify> player(softSerial);

class Mp3Notify
{
  public:
    static void OnError(uint16_t errorCode) {
      Serial.print(F("--------------\n ERROR "));
      Serial.println(errorCode);
      Serial.println(F("--------------"));
      error = true;
      errCode = errorCode;
      ++cntErrorCallback;
    }

    static void OnPlayFinished(uint16_t globalTrack) {
      Serial.print(F("Callback global track finished: "));
      Serial.println(globalTrack);
      lastGlobalTrackFinished = globalTrack;
      ++cntPlayFinishedCallback;
    }

    static void OnCardOnline(uint16_t code) {
      Serial.print(F("Callback OnCardOnline: "));
      Serial.println(code);
      sdAvailable = true;
      ++cntCardOnlineCallback;
    }

    static void OnCardInserted(uint16_t code) {
      Serial.print(F("Callback OnCardInserted: "));
      Serial.println(code);
      ++cntCardInsertedCallback;
    }

    static void OnCardRemoved(uint16_t code) {
      Serial.print(F("Callback OnCardRemoved: "));
      Serial.println(code);
      ++cntCardRemovedCallback;
    }

    static void OnUsbOnline(uint16_t code) {
      usbAvailable = true;
      Serial.print(F("Callback OnUsbOnline: "));
      Serial.println(code);
      ++cntUsbOnlineCallback;
    }

    static void OnUsbInserted(uint16_t code) {
      Serial.print(F("Callback OnUsbInserted: "));
      Serial.println(code);
      ++cntUsbInsertedCallback;
    }

    static void OnUsbRemoved(uint16_t code) {
      Serial.print(F("Callback OnUsbRemoved: "));
      Serial.println(code);
      ++cntUsbRemovedCallback;
    }
};

void resetCallbackCounters() {
  cntErrorCallback = 0;
  cntPlayFinishedCallback = 0;
  cntCardOnlineCallback = 0;
  cntCardInsertedCallback = 0;
  cntCardRemovedCallback = 0;
  cntUsbOnlineCallback = 0;
  cntUsbInsertedCallback = 0;
  cntUsbRemovedCallback = 0;
}

class Test {
  public:
    virtual String getName() = 0;

    virtual void run() {
      setup();
      doRun();
      teardown();
    }
    virtual String getResult() = 0;

  protected:
    virtual void doRun() = 0;

    virtual void setup() {
      Serial.print(F("\n\n-------------------------------------------------------\n Running Test Case \""));
      Serial.print(getName());
      Serial.println(F("\"\n-------------------------------------------------------"));
      player.reset();
      // let player settle down
      busyWait(2000);
      resetCallbackCounters();
    }

    virtual void teardown() {
      Serial.print(F("-------------------------------------------------------\n Test Case \""));
      Serial.print(getName());
      Serial.println(F("\" FINISHED \n-------------------------------------------------------"));
    }

    void turnOff() {
      player.setVolume(0);
      delay(50);
      player.stop();
      delay(200);
      player.disableDac();
      delay(200);
      player.sleep();
      delay(200);
    }

    void initDFPlayer() {
      delay(100);
      player.setEq(DfMp3_Eq_Normal);
      delay(100);

      player.enableDac();
      delay(250);
      player.setVolume(1);
    }

    boolean isPlaying() {
      uint16_t state = 0;
      int retries = 3;

      error = true;
      while (retries > 0 && error) {
        --retries;
        error = false;
        state = player.getStatus();
        if (error) {
          delay(100);
        }
      }
      error = false;

      return (state & 1) == 1;
    }

    void playOrAdvertise(int fileNo) {
      if (isPlaying()) {
        player.playAdvertisement(fileNo);
      } else {
        player.playMp3FolderTrack(fileNo);
      }
    }

    void readFolderTrackCounts() {
      delay(2000);

      player.getFolderTrackCount(1);
    }

    void busyWait(long ms) {
      long startMs = millis();
      while (millis() < startMs + ms) {
        player.loop();
        delay(50);
      }
    }

};

class TestDiscoverDevices : public Test {
  public:
    String getName() {
      return "TestDiscoverDevices";
    }
    String getResult() {
      String res = F("The following devices have been discovered:");
      if (sdAnnouncedOnReset || sdDiscoveredByTrackRead || sdAnnouncedOnPlaysourceFlash) {
        res += F("\n   -> SD");
        if (sdAnnouncedOnReset) res += F(", announced on reset");
        if (sdDiscoveredByTrackRead) res += F(", discovered by getTotalTrackCount");
        if (sdAnnouncedOnPlaysourceFlash) res += F(", announced on setPlaysourceFlash");
      }
      if (usbAnnouncedOnReset || usbDiscoveredByTrackRead || usbAnnouncedOnPlaysourceFlash) {
        res += F("\n   -> USB");
        if (usbAnnouncedOnReset) res += F(", announced on reset");
        if (usbDiscoveredByTrackRead) res += F(", discovered by getTotalTrackCount");
        if (usbAnnouncedOnPlaysourceFlash) res += F(", announced on setPlaysourceFlash");
      }

      if (!usbAnnouncedOnPlaysourceFlash && !sdAnnouncedOnPlaysourceFlash) {
        res += F("\nDoes NOT react on setPlaysourceFlash");
      }

      return res;
    }
  protected:
    void doRun() {
      initDFPlayer();
      resetCallbackCounters();
      player.reset();
      busyWait(2000);
      sdAnnouncedOnReset = (cntCardOnlineCallback > 0);
      usbAnnouncedOnReset = (cntUsbOnlineCallback > 0);

      resetCallbackCounters();
      player.setPlaybackSource(DfMp3_PlaySource_Sd);
      busyWait(2000);
      sdDiscoveredByTrackRead = player.getTotalTrackCountSd() > 0;
      player.setPlaybackSource(DfMp3_PlaySource_Usb);
      busyWait(2000);
      usbDiscoveredByTrackRead = player.getTotalTrackCountUsb() > 0;

      busyWait(1000);
      resetCallbackCounters();
      player.setPlaybackSource(DfMp3_PlaySource_Flash);
      busyWait(2000);
      sdAnnouncedOnPlaysourceFlash = (cntCardOnlineCallback > 0);
      usbAnnouncedOnPlaysourceFlash = (cntUsbOnlineCallback > 0);

      sdAvailable |= sdAnnouncedOnReset || sdDiscoveredByTrackRead || sdAnnouncedOnPlaysourceFlash;
      usbAvailable |= usbAnnouncedOnReset || usbDiscoveredByTrackRead || usbAnnouncedOnPlaysourceFlash;

      // Have to set back from Flash, otherwise reset will result in an error state
      if (sdAvailable)
        player.setPlaybackSource(DfMp3_PlaySource_Sd);
      else if (usbAvailable)
        player.setPlaybackSource(DfMp3_PlaySource_Usb);
      else
        player.setPlaybackSource(DfMp3_PlaySource_Sleep);
    }

    boolean sdAnnouncedOnReset = false;
    boolean usbAnnouncedOnReset = false;
    boolean sdDiscoveredByTrackRead = false;
    boolean usbDiscoveredByTrackRead = false;
    boolean sdAnnouncedOnPlaysourceFlash = false;
    boolean usbAnnouncedOnPlaysourceFlash = false;
};

class TestGetFolderTrackCount : public Test {
  public:
    String getName() {
      return "TestGetFolderTrackCount";
    }
    String getResult() {
      String res = F("getFolderTrackCount...");
      if (sdAvailable) {
        res += F("\n   -> for SD ");
        if (folderTrackCountSdFailed) res += F("FAILED");
        else {
          res += F("returned ");
          res += folderTrackCountSd;
          res += F(" files in Folder /01/ ");
        }
      }
      if (usbAvailable) {
        res += F("\n   -> for USB ");
        if (folderTrackCountUsbFailed) res += F("FAILED");
        else {
          res += F("returned ");
          res += folderTrackCountUsb;
          res += F(" files in Folder /01/ ");
        }
      }

      return res;
    }
  protected:
    void doRun() {
      initDFPlayer();
      resetCallbackCounters();

      if (sdAvailable) {
        player.setPlaybackSource(DfMp3_PlaySource_Sd);
        busyWait(1000);
        error = false;
        folderTrackCountSd = player.getFolderTrackCount(1);
        if (error) {
          error = false;
          folderTrackCountSdFailed = false;
        }
      }

      if (usbAvailable) {
        player.setPlaybackSource(DfMp3_PlaySource_Usb);
        busyWait(1000);
        error = false;
        folderTrackCountUsb = player.getFolderTrackCount(1);
        if (error) {
          error = false;
          folderTrackCountUsbFailed = false;
        }
      }
    }

    bool folderTrackCountSdFailed = false;
    bool folderTrackCountUsbFailed = false;

    int folderTrackCountSd = -1;
    int folderTrackCountUsb = -1;
};

class TestReaction3F : public Test {
  public:
    String getName() {
      return "TestReaction3F";
    }
    String getResult() {
      String res = reactsOn3F ? F("Reacts on 0x3F queries") : F("Does NOT react on 0x3F queries");
      if (reactsOn3F) {
        res += F(", reply : ");
        res += devAnnounced;

        if (devAnnounced > 0) {
          if ((devAnnounced & 1) == 1) {
            res += F(" -> USB");
          }
          if ((devAnnounced & 2) == 2) {
            res += F(" -> SD");
          }
          if ((devAnnounced & 4) == 4) {
            res += F(" -> PC");
          }
        }
      }

      return res;
    }
  protected:
    void doRun() {
      initDFPlayer();
      resetCallbackCounters();
      int sd = player.queryStorageDevices();
      if (error) {
        error = false;
        reactsOn3F = false;
      }
      else {
        reactsOn3F = true;
        devAnnounced = sd;
      }
    }

    boolean reactsOn3F = false;
    int devAnnounced = -1;
};

class TestGetCurrentTrack : public Test {
  public:
    String getName() {
      return "TestGetCurrentTrack";
    }
    String getResult() {
      String res = F("");

      if (sdAvailable) {
        res += getCurrentTrackWorkingSd ? F("GetCurrentTrack for SD returns correct value ") : F("GetCurrentTrack for SD does NOT work");

        if (getCurrentTrackWorkingSd) {
          res += msUntilGetCurrentTrackIsUpdatedSd;
          res += F("ms after start of track");
        }
      }

      if (usbAvailable) {
        if (res.length() > 0) res += "\n";
        res += getCurrentTrackWorkingUsb ? F("GetCurrentTrack for USB returns correct value ") : F("\nGetCurrentTrack for USB does NOT work");

        if (getCurrentTrackWorkingUsb) {
          res += msUntilGetCurrentTrackIsUpdatedUsb;
          res += F("ms after start of track");
        }
      }

      return res;
    }
  protected:
    void doRun() {
      initDFPlayer();
      resetCallbackCounters();

      if (sdAvailable) {
        player.setPlaybackSource(DfMp3_PlaySource_Sd);
        busyWait(200);
        player.playFolderTrack(1, 2);
        // ensure track is fetched
        busyWait(2000);
        int wrongTrack = player.getCurrentTrack();
        if (error || wrongTrack == 0) {
          error = false;
          getCurrentTrackWorkingSd = false;
        } else {
          getCurrentTrackWorkingSd = true;
          player.playFolderTrack(1, 1);
          long nowMs;
          long msStart = millis();

          for (int i = 0; i <= 10; ++i) {
            long nowMs = millis();
            int track = player.getCurrentTrack();
            if (error)
              error = false;
            else if (track != 0 and track != wrongTrack)
            {
              msUntilGetCurrentTrackIsUpdatedSd = nowMs - msStart;
              break;
            }
            player.loop();

            delay(100 - ((millis() - msStart) % 100));
          }

          Serial.print(F("GetCurrentTrack for SD returns correct value "));
          Serial.print(msUntilGetCurrentTrackIsUpdatedSd);
          Serial.println(F("ms after start of track"));
        }
        player.stop();
      }

      if (usbAvailable) {
        player.setPlaybackSource(DfMp3_PlaySource_Usb);
        busyWait(200);
        player.playFolderTrack(1, 2);
        // ensure track is fetched
        busyWait(2000);
        int wrongTrack = player.getCurrentTrack();
        if (error || wrongTrack == 0) {
          error = false;
          getCurrentTrackWorkingUsb = false;
        } else {
          getCurrentTrackWorkingUsb = true;
          player.playFolderTrack(1, 1);
          long nowMs;
          long msStart = millis();

          for (int i = 0; i <= 10; ++i) {
            long nowMs = millis();
            int track = player.getCurrentTrack();
            if (error)
              error = false;
            else if (track != 0 and track != wrongTrack)
            {
              msUntilGetCurrentTrackIsUpdatedUsb = nowMs - msStart;
              break;
            }
            player.loop();

            delay(100 - ((millis() - msStart) % 100));
          }

          Serial.print(F("GetCurrentTrack for USB returns correct value "));
          Serial.print(msUntilGetCurrentTrackIsUpdatedUsb);
          Serial.println(F("ms after start of track"));
        }
        player.stop();
      }
    }

    bool getCurrentTrackWorkingSd = false;
    bool getCurrentTrackWorkingUsb = false;

    int msUntilGetCurrentTrackIsUpdatedSd = -1;
    int msUntilGetCurrentTrackIsUpdatedUsb = -1;
};

class TestTrackFinishedCallback : public Test {
  public:
    String getName() {
      return "TestTrackFinishedCallback";
    }
    String getResult() {
      String res = F("");

      if (sdAvailable) {
        res += F("Sends ");
        res += noCallbacksSd;
        res += F(" callback(s) on SD track end");
      }

      if (usbAvailable) {
        if (res.length() > 0) res += "\n";

        res += F("Sends ");
        res += noCallbacksUsb;
        res += F(" callback(s) on USB track end");
      }

      return res;
    }
  protected:
    void doRun() {
      initDFPlayer();
      resetCallbackCounters();

      if (sdAvailable) {
        player.setPlaybackSource(DfMp3_PlaySource_Sd);
        busyWait(1000);
        player.playFolderTrack(1, 2);
        resetCallbackCounters();
        // ensure track is fetched
        busyWait(2000);
        int track = player.getCurrentTrack();
        while (cntPlayFinishedCallback == 0) {
          busyWait(1000);
        }
        busyWait(2000);
        noCallbacksSd = cntPlayFinishedCallback;
        Serial.print(F("Received "));
        Serial.print(noCallbacksSd);
        Serial.println(F(" callback(s) on SD track end"));
      }

      if (usbAvailable) {
        player.setPlaybackSource(DfMp3_PlaySource_Usb);
        busyWait(1000);
        player.playFolderTrack(1, 2);
        resetCallbackCounters();
        // ensure track is fetched
        busyWait(2000);
        int track = player.getCurrentTrack();
        while (cntPlayFinishedCallback == 0) {
          busyWait(1000);
        }
        busyWait(2000);
        noCallbacksUsb = cntPlayFinishedCallback;
        Serial.print(F("Received "));
        Serial.print(noCallbacksUsb);
        Serial.println(F(" callback(s) on USB track end"));
      }

    }

    int noCallbacksSd = 0;
    int noCallbacksUsb = 0;
};

class TestContinuePlaybackAfterAd : public Test {
  public:
    String getName() {
      return "TestContinuePlaybackAfterAd";
    }
    String getResult() {
      String res = F("");

      if (sdAvailable) {
        res += continueAfterAdWorkingSd ? F("Continue playback after Ad for SD works") : F("Continue playback after Ad for SD does NOT work");
      }

      if (usbAvailable) {
        if (res.length() > 0) res += "\n";
        res += continueAfterAdWorkingUsb ? F("Continue playback after Ad for USB works") : F("Continue playback after Ad for USB does NOT work");
      }

      return res;
    }
  protected:
    void doRun() {
      initDFPlayer();
      resetCallbackCounters();

      if (sdAvailable) {
        lastGlobalTrackFinished = -1;
        player.setPlaybackSource(DfMp3_PlaySource_Sd);
        busyWait(1000);
        player.playFolderTrack(1, 2);
        resetCallbackCounters();
        // ensure track is fetched
        busyWait(2000);
        int track = player.getCurrentTrack();
        player.playAdvertisement(100);
        // try to get currently playing advertisement track number
        int trackAd = -1;
        int attemps = 5;
        while (attemps-- && (trackAd == -1)){
          busyWait(500);
          int cur_track = player.getCurrentTrack();
          if (cur_track)
            trackAd = cur_track;
        }
        // check if we know played track number for ADV
        if (trackAd != -1){
          while (trackAd != lastGlobalTrackFinished) {
            busyWait(1200);
          }
          busyWait(500);
          if (isPlaying()) {
            while (track != lastGlobalTrackFinished) {
              busyWait(200);
            }
          }
          continueAfterAdWorkingSd = (track == lastGlobalTrackFinished);
        } else {
          continueAfterAdWorkingSd = false;
        }
      }

      if (usbAvailable) {
        lastGlobalTrackFinished = -1;
        player.setPlaybackSource(DfMp3_PlaySource_Usb);
        busyWait(1000);
        player.playFolderTrack(1, 2);
        resetCallbackCounters();
        // ensure track is fetched
        busyWait(2000);
        int track = player.getCurrentTrack();
        player.playAdvertisement(100);
        busyWait(500);
        int trackAd = player.getCurrentTrack();
        while (trackAd != lastGlobalTrackFinished) {
          busyWait(200);
        }
        busyWait(500);
        if (isPlaying()) {
          while (track != lastGlobalTrackFinished) {
            busyWait(200);
          }
        }
        continueAfterAdWorkingUsb = (track == lastGlobalTrackFinished);
      }

    }

    bool continueAfterAdWorkingSd = false;
    bool continueAfterAdWorkingUsb = false;
};

class TestWakeupAfterSleep : public Test {
  public:
    String getName() {
      return "TestWakeupAfterSleep";
    }
    String getResult() {
      String res = F("");

      res += wakeupByResetWorks ? F("Wakeup from sleep by reset works") : F("Wakeup from sleep by reset does NOT work");

      if (sdAvailable)
      {
        res += wakeupBySetPlaysourceSdWorks ? F("\nWakeup from sleep by setting playsource SD works") : F("\nWakeup from sleep by setting playsource SD does NOT work");
      }
      if (usbAvailable)
      {
        res += wakeupBySetPlaysourceUsbWorks ? F("\nWakeup from sleep by setting playsource USB works") : F("\nWakeup from sleep by setting playsource USB does NOT work");
      }

      return res;
    }
  protected:
    void doRun() {
      initDFPlayer();
      if (sdAvailable)
        player.setPlaybackSource(DfMp3_PlaySource_Sd);
      else if (usbAvailable)
        player.setPlaybackSource(DfMp3_PlaySource_Usb);
      resetCallbackCounters();

      turnOff();

      delay(2000);
      error = false;
      player.reset();
      busyWait(2000);
      if (error) {
        error = false;
        wakeupByResetWorks = false;
      }
      else
      {
        initDFPlayer();
        if (sdAvailable)
          player.setPlaybackSource(DfMp3_PlaySource_Sd);
        else if (usbAvailable)
          player.setPlaybackSource(DfMp3_PlaySource_Usb);

        player.playFolderTrack(1, 2);
        busyWait(2000);
        wakeupByResetWorks = isPlaying();
        player.stop();
      }

      if (sdAvailable)
      {
        turnOff();

        delay(2000);
        player.setPlaybackSource(DfMp3_PlaySource_Sd);
        delay(2000);
        initDFPlayer();
        player.playFolderTrack(1, 2);
        busyWait(2000);
        wakeupBySetPlaysourceSdWorks = isPlaying();
        player.stop();
      }

      if (usbAvailable)
      {
        turnOff();

        delay(2000);
        player.setPlaybackSource(DfMp3_PlaySource_Usb);
        delay(2000);
        initDFPlayer();
        player.playFolderTrack(1, 2);
        busyWait(2000);
        wakeupBySetPlaysourceUsbWorks = isPlaying();
        player.stop();
      }
    }

    bool wakeupByResetWorks = false;
    bool wakeupBySetPlaysourceSdWorks = false;
    bool wakeupBySetPlaysourceUsbWorks = false;
};

class TestConnectivity : public Test {
  public:
    String getName() {
      return "TestConnectivity";
    }

    virtual void setup() {
      // prevent reset
    }

    String getResult() {
      return F("");
    }
  protected:
    void doRun() {
      resetCallbackCounters();
      player.getStatus();
      if (error && errCode == DfMp3_Error_RxTimeout) {
        Serial.println(F("==========================="));
        Serial.println(F("\n No communication possible"));
        Serial.println(F(" !!! CHECK YOUR WIRING !!!"));
        Serial.println(F("\n==========================="));

        while (true) delay(1000);
      }
      if (error && errCode == DfMp3_Error_Busy) {
        Serial.println(F("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="));
        Serial.println(F("\n No medium found!!!"));
        Serial.println(F(" Check SD card and/or USB stick!"));
        Serial.println(F("\n=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="));

        while (true) delay(1000);
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.print(F("\n\nDFPlayer Analyzer "));
  Serial.print(F(VERSION));
  Serial.print(F(" - Starting up...\n\n"));

  player.setACK(false);
  player.begin();

  TestConnectivity t0;
  TestDiscoverDevices t1;
  TestReaction3F t2;
  TestGetFolderTrackCount t3;
  TestGetCurrentTrack t4;
  TestTrackFinishedCallback t5;
  TestContinuePlaybackAfterAd t6;
  TestWakeupAfterSleep t7;

  Test* allTests[] {&t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7};

  for (int i = 0; i < sizeof(allTests) / sizeof(allTests[0]); ++i)
  {
    allTests[i]->run();
  }

  Serial.println(F("\n\n-------------------------------------------------------\n Profile of this DFPlayer device\n-------------------------------------------------------"));

  for (int i = 0; i < sizeof(allTests) / sizeof(allTests[0]); ++i)
  {
    String res = allTests[i]->getResult();

    if (res != NULL && res.length() > 0)
      Serial.println(res);
  }

  Serial.println(F("\n\n-------------------------------------------------------------------------------\n ALL TESTS COMPLETED!\n\n Please report your results in an issue at\n\n     https://github.com/ghmartin77/DFPlayerAnalyzer/issues\n\n Please state the player's chip name in the title of the issue entry.\n Thanks for your support!\n-------------------------------------------------------------------------------"));

  /*
      #<n> wake up after sleep possible? 2 scenarios: setPlaybackSource and reset
  */
}

void loop() {
  player.loop();
  delay(50);
}



