#define ESP32 true

#include <Arduino.h>
#include <defintions.h>   // mostly pin definitions for reference board
#include <FS.h>           // file system for ESPUI
#include <ESPUI.h>        // for building webpage UI
#include <WiFi.h>         // lets make this network
#include <DNSServer.h>    // will have to see why this is needed for ESPui?


/////////////
// Setup How This Feels
/////////////


#define vibrator_PWM  40  // 40Hz makes a Magic Wand way more rumbly
#define vibrator_max  135 // Sets top vibration (0-255) Default is lower to compensate for using 12v to supply a 10v motor
#define vibrator_low  35  // Tune this value to be just below where there are vibrations that can be felt 

#define flick_warn_time     75 // Time in milliseconds that the vibrator will be on as a reminder
#define flick_warn_interval 33 // [30]  Time in SECONDS Interval between reminders   

#define red_time  360     // Time in MINUTES for the red light (locked up)

int sleep_low = 35;  // [20] Time in MINUTES for the minimum amount of pause before next round of teasing
int sleep_high = 70;  // [45] Time in MINUTES for the maximum amount of pause before next round of teasing
 
#define tease_power_start     35  // Integer (0-255) - PWM power of vibrator at lowest tease
int tease_power_inc = 13;         // Starting from "vibrator_low", how much more intese will each step be
int tease_interval_start = 120;   // [120] Time in SECONDS - between full tease
int tease_interval_dec = 18;      // Time in SECONDS - speed up the interval by this each loop
#define tease_wait            500 // [500] Time in millis  - time between ramp steps when teasing
#define tease_ramp_rate       1   // how much to add to the PWM each loop
int tease_session_qty = 10;       // How many times does speed / intesinty increase for each before resetting

//                                  
//                                                      (tease_power_start)   ->  /|
//                      tease_power_start -> /|         (+ tease_power_inc)      / |
//                                          / |                                 /  |
//                                         /  |                                /   |
//  ______________________________________/   |_______________________________/    |____________ 
//   ^ tease_interval_start ^                    ^ (tease_interval_start) ^ 
//                                                 ( -tease_interval_dec)                              
//                               /|
//                              / |
//                             /  |
//                         tease_wait (time between steps)
//                           /    |
//                   -->    /     |   <--  Height of each step = tease_ramp_rate
//                         /      | 
//         _______________/       |________________ 


/////////////
// Initialally set some things you want to initialize
/////////////

#define EEPROM_size 12 

int k = 1000;            // I'm lazy and need to convert millis to seconds often
int kM = 1000 * 60;      // even more lazy, milliseconds to minutes



volatile bool vibez_on = true;     // tracks if currently allowed to vibe (Not in sleep mode, not stopped by another process)
int  current_mood = 0;    // Current output for the vibrator
int  current_best_mood = tease_power_start;
int  ti = 0;              // Loop increment for teasing

int next_flick = millis() + (flick_warn_interval * k);  // First flick will be the interval after now
int next_tease = millis() + (tease_interval_start * k); // First tease session will start in x seconds (default start is 120 seconds) 
int red_over = millis() + (red_time * kM); // defines red time, for a new one reset the counter


volatile int sleep_time = 0;


/////////////
// Make some functions here that'll help out in the future
/////////////

//////////////////////
// Here's where we pause all activites (when STAHP is smashed)
void ICACHE_RAM_ATTR sleep_baby_sleep(){

  if(vibez_on){
    vibez_on = false;
    current_mood = 0;
    current_best_mood = tease_power_start;
    red_over = millis() + (red_time * kM);
    ledcWrite(Vibez,0);
    sleep_time = millis() + (random(sleep_low, sleep_high) * kM);
    LogDebug("[STAHP] Sleepy time little baby");
  }

} //sleep_baby_sleep
//////////////////////

//////////////////////
// Makes a slow saw tooth ramp on the vibrator for 30 cycles -- gives about 4 minutes to cum
void cum_now(){ 
    LogDebug("[Cum Now, Please?] Ohhhh wow, hope you're ready to cum");

      for (int i = 0; i <= 30; i++) {
          for (int x = vibrator_low; x <= vibrator_max; x++) {
            ledcWrite(Vibez, x);
            delay(100);
            if(!vibez_on){ break;};
        }
        if(!vibez_on){ break;};
      }

} //cum_now
//////////////////////

//////////////////////
// Makes a quick blip of the vibrator every interval as an indicator the timer is running
int flick_warn(){

  LogDebug("[Flick Warn] Hey! no time to rest yet");
  int next_flick = 0;

          if(!current_mood){ // no need to flick warn if running

            // Vibrator full on, wait, off
            ledcWrite(Vibez,vibrator_max);
            delay(flick_warn_time);
            ledcWrite(Vibez,0);

          } 

  /////////////////////////////////////////////////////////////////
  // Hidding this reporting section here saved many lines of code. 
  float red_time_left = constrain((red_time - millis()) / kM,0,sleep_high); 
  float next_tease_minutes = constrain((next_tease - millis())/ kM,0,tease_interval_start);
  
  Serial.println("Vibez: " + String(vibez_on) + " Red time left: " + String(red_time_left) + " minutes");
  Serial.println("Current Mood: " + String(current_mood) + " / " + String(current_best_mood) + " tease increment: " + String(ti) );
  Serial.println("Next Tease: " + String(next_tease) + " current millis(): " + String(millis())); 
  Serial.println("Thank you for flying Air Canada!");
  Serial.println("");
  /////////////////////////////////////////////////////////////////



  next_flick = millis() + (flick_warn_interval * k); // Figure out when next to flick that vibe
  return next_flick; // time to next flick
} //flick_warn
//////////////////////

//////////////////////
// Checks to see if current timer is in the past
bool after_now(int incoming_time){ // have to do a lot of time checks so that things aren't blocking, this seems easier
  bool output = false;

  if (millis() > incoming_time){
    output = true;
  }

  return output;
} // after_now
//////////////////////

//////////////////////
// Helps work with an incrament in a looping value
int i_constrain_loop(int current, int max){
current = current + 1;  // advance by one
if(current > max){      // Shit... too far?    
    current = 0;        // Start again.
  }
return current;
} // i_constrain_loop
//////////////////////



/////////////
// ESPUI and Friends
/////////////
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;

const char* ssid = "KM_Sex_toy_69_420";
const char* password = "gofuckyourself";
const char* hostname = "KM_Sex_toy";


uint16_t slider_sleep_min = 0;
uint16_t slider_sleep_max = 0;
uint16_t slider_lockup_time = 0;
uint16_t slider_tease_qty = 0;



void slider(Control* sender, int type)
{
    Serial.print("Slider: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
    // Like all Control Values in ESPUI slider values are Strings. To use them as int simply do this:
    int sliderValueWithOffset = sender->value.toInt() + 100;
    Serial.print("SliderValue with offset");
    Serial.println(sliderValueWithOffset);
}

void set_red(Control* sender, int type)
{

  int new_red_time = sender->value.toInt();
  LogDebug("[Red Time Adjustment] Your new lock up time in this session is: " + String(new_red_time) + " [Minutes]");
  red_over = millis() + (new_red_time * kM); // defines red time, for a new one reset the counter
  LogDebug(String(millis()) + "/" + String(red_over));

}

void set_sleep_min(Control* sender, int type)
{
  int new_sleep_time = sender->value.toInt();
  LogDebug("[Sleep MIN Adjustment] Your new minimum sleep is: " + String(new_sleep_time) + " [Minutes]");

  if(new_sleep_time>sleep_high){

    LogDebug("Sleep HIGH was GREATER than Sleep LOW... correcting.");
    sleep_high = new_sleep_time;
    ESPUI.updateSlider(slider_sleep_max,sleep_high);
    sleep_low = new_sleep_time;
   }else{
    LogDebug("Sleep low was less than sleep high");
    sleep_low = new_sleep_time;
   }

  LogDebug("Sleep Min: " + String(sleep_low) + " --- Sleep Max: " + String(sleep_high));
}


void set_sleep_max(Control* sender, int type)
{
  int new_sleep_time = sender->value.toInt();
  LogDebug("[Sleep MIN Adjustment] Your new minimum sleep is: " + String(new_sleep_time) + " [Minutes]");

  if(new_sleep_time<sleep_low){

    LogDebug("Sleep HIGH was LESS THAN than sleep LOW... correcting.");
    sleep_low = new_sleep_time;
    ESPUI.updateSlider(slider_sleep_min,sleep_low);
    sleep_high = new_sleep_time;
   }else{
    LogDebug("Sleep low was less than sleep high");
    sleep_high = new_sleep_time;
   }

  LogDebug("Sleep Min: " + String(sleep_low) + " --- Sleep Max: " + String(sleep_high));
}

void set_tease_qty(Control* sender, int type)
{
  int new_tease_qty = sender->value.toInt();

  tease_session_qty = new_tease_qty;

  tease_power_inc = constrain((vibrator_max-vibrator_low)/tease_session_qty,1,vibrator_max-vibrator_low);
  tease_interval_dec = constrain((tease_interval_start-5)/tease_session_qty,5,tease_interval_start);
  
  LogDebug("New tease session quantity: " + String(tease_session_qty) + " loops " );
  LogDebug("New how much more intense is the next tease: " + String(tease_power_inc) + " / " + String(vibrator_max) );
  LogDebug("New how much less time before the next tease: " + String(tease_interval_dec) + " / " + String(tease_interval_start) );

  current_best_mood = constrain(vibrator_low + (ti * tease_power_inc),vibrator_low,vibrator_max); // Sets the upper bounds on the next round
  next_tease = millis() + (constrain(tease_interval_start - (ti * tease_interval_dec), 10, tease_interval_start) * k);  // When do we next tease?

}






// VOID SETUP --- VOID SETUP --- VOID SETUP --- VOID SETUP --- Why do I always miss you when looking?

void setup() {

  tease_power_inc = constrain((vibrator_max-vibrator_low)/tease_session_qty,1,vibrator_max-vibrator_low);
  tease_interval_dec = constrain((tease_interval_start-5)/tease_session_qty,5,tease_interval_start);

  ESPUI.setVerbosity(Verbosity::VerboseJSON);
  Serial.begin(115200); // I see you talking back to me (over the serial monitor)


  /////////////
  // Assign PWM channels
  /////////////

  ledcAttachPin(Vibez_pin,Vibez); // Setups PWM on channel 0 (Vibez in defintions)
  ledcSetup(Vibez, vibrator_PWM, 8); // SLOW PWM, more rumbly (Pin,Refresh Rate,Bits of Resolution)

  ledcWrite(Vibez,(vibrator_max - vibrator_low)); //Lets try this new output
  delay(500);
  ledcWrite(Vibez,0);

  //ledcAttachPin(Out2_pin,Out2); //Same as above, uncomment if you want these to be PWM channels
  //ledcAttachPin(Out3_pin,Out3);
  //ledcAttachPin(Out4_pin,Out4);

  /////////////
  // Assign non PWM outputs
  /////////////

   pinMode(Out2_pin,OUTPUT); // "Red Light" output, on = locked up
   LogDebug("[Start Time] Welp, you're now locked up for : " + String(red_time));
   digitalWrite(Out2_pin,HIGH);


  /////////////
  // Assign Input Button
  /////////////

    pinMode(Stahp_pin,INPUT_PULLUP);                             // setup the input, it has an external pull-up
    attachInterrupt(Stahp_pin,sleep_baby_sleep,FALLING);  // because of the pull-up, we trigger on falling (start of the press)

  /////////////
  // ESPUI things
  /////////////

  WiFi.setHostname(hostname);
  WiFi.begin();
  LogDebug("[WiFi Startup] Lets Get sending those wavey packets");

  uint8_t timeout = 10;

  WiFi.mode(WIFI_AP);
            delay(100);
            WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
            uint32_t chipid = 0;
            for (int i = 0; i < 17; i = i + 8)
            {
                chipid |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
            }

  char ap_ssid[25];
            snprintf(ap_ssid, 26, "KM_Sex_Toy-%08X", chipid);
            WiFi.softAP(ap_ssid);

            timeout = 5;

            do
            {
                delay(500);
                Serial.print(".");
                timeout--;
            } while (timeout);
        
    dnsServer.start(DNS_PORT, "*", apIP);

    Serial.println("\n\nWiFi parameters:");
    Serial.print("Mode: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
    Serial.print("IP address: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());

    ESPUI.separator("Session Controls");

    ESPUI.label("Session 'Lockup' Time is the ammount of time output #2 is on, we recommend a red light to signal the duration of the session. At the end of this time output #2 will release, and a 4 minute oscillating ramp will try to make you cum",ControlColor::Turquoise);
    slider_lockup_time = ESPUI.slider("Session 'Lockup' time [Minutes]", &set_red, ControlColor::Alizarin,red_time,15,360);
    ESPUI.label("When hitting the STAHP button the system to goto sleep (teasing is off, and so is the flick warn) for a random ammount of time between these two values.",ControlColor::Turquoise);
    slider_sleep_min = ESPUI.slider("Sleep Minimum [Minutes]", &set_sleep_min, ControlColor::Alizarin,sleep_low,15,90);
    slider_sleep_max = ESPUI.slider("Sleep Maximum [Minutes]", &set_sleep_max, ControlColor::Alizarin,sleep_high,15,360);

 

    ESPUI.separator("Teasing values");
    ESPUI.label("When not in a sleep period, we will try to tease an orgasm out of you. With a decreasing wait period between slow ramps up to increasing values. Fine tune the fun here.",ControlColor::Turquoise);
    slider_tease_qty = ESPUI.slider("Number of times the tease pattern gets more intense before resetting", &set_tease_qty, ControlColor::Alizarin,tease_session_qty,5,20);
    





    ESPUI.begin("KM - 4 Channel Sex Toy");

    int ESPUI_stop = millis() + (90 * k); // shut it down after 90s



}



//  VOID LOOP --- LOOP LOOP LOOP --- VOID LOOP --- LOOP LOOP LOOP --- I sometimes miss you too

void loop() {

if(after_now(next_flick) && vibez_on){next_flick = flick_warn();}           // Flick Warn every interval
if(after_now(red_over)){digitalWrite(Out2_pin,LOW);if(!vibez_on){vibez_on = true;}; cum_now();}             // After the red time is over, turn off the red light "lock up" output (Output 2) and give one last hurrah
if(after_now(sleep_time) && !vibez_on){vibez_on = true; next_tease = millis() + (2*k);}  //sleep has concluded, get ready to start teasing
//if(after_now(ESPUI_stop)){ESPUI.}


if(!vibez_on){
  LogDebug("Sleep Time: " + String(sleep_time - millis()));
  delay(1000);
}


if(after_now(next_tease) && vibez_on){              // Run a teasing ramp 
  if(current_mood < current_best_mood){             // Have we gone as far as we can yet?
    Serial.println(current_mood);
    current_mood = current_mood + tease_ramp_rate;  // Not far enough yet, bump the mood up a bit
    next_tease = millis() + tease_wait;             // wait before next bump
  } else{
    LogDebug("[Tease fully Ramp'd] We've reached the limt of what mood enhancers can do");
    current_mood = 0;           // Shut it down, no time for fun now
    current_best_mood = constrain(vibrator_low + (ti * tease_power_inc),vibrator_low,vibrator_max); // Sets the upper bounds on the next round
    next_tease = millis() + (constrain(tease_interval_start - (ti * tease_interval_dec), 10, tease_interval_start) * k);  // When do we next tease?
    ti = i_constrain_loop(ti,tease_session_qty);  // Loop inc that decides how frequently and how hard the vibrator turns on
    
  }
}

ledcWrite(Vibez,current_mood);  // Finally, get those good vibez after we figured out what we wanted. 

}