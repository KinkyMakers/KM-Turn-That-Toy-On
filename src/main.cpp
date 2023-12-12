#define ESP32 true

#include <Arduino.h>
#include <defintions.h>   // mostly pin definitions for reference board




/////////////
// Setup How This Feels
/////////////


#define vibrator_PWM  40  // 40Hz makes a Magic Wand way more rumbly
#define vibrator_max  110 // Sets top vibration (0-255) Default is lower to compensate for using 12v to supply a 10v motor
#define vibrator_low  18  // Tune this value to be just below where there are vibrations that can be felt 

#define flick_warn_time     75 // Time in milliseconds that the vibrator will be on as a reminder
#define flick_warn_interval 33 // [30]  Time in SECONDS Interval between reminders   

#define red_time  45    // Time in MINUTES for the red light (locked up)
 
#define tease_power_start     35  // Integer (0-255) - PWM power of vibrator at lowest tease
int tease_power_inc = 13;         // Starting from "vibrator_low", how much more intese will each step be
int tease_interval_start = 120;   // [120] Time in SECONDS - between full tease
int tease_interval_dec = 18;      // Time in SECONDS - speed up the interval by this each loop
#define tease_wait            500 // [500] Time in millis  - time between ramp steps when teasing
#define tease_ramp_rate       1   // how much to add to the PWM each loop
int tease_session_qty = 12;       // How many times does speed / intesinty increase for each before resetting

int vibe_setup_required = true;             // tracks if the vibe was running before being shut off  


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

int hold_her = false; 
int was_running = false;
int hold_her_inesnity = 0;
int next_her = 0;

volatile bool vibez_on = true;     // tracks if currently allowed to vibe (not stopped by another process)



  int  current_mood =       0;          // Current output for the vibrator
  int  current_best_mood =  0;
  int  ti =                 0;          // Loop increment for teasing

  int next_flick = 0;
  int next_tease = 0; // First tease session will start in x seconds (default start is 120 seconds) 
  int red_over = 0; // defines red time, for a new one reset the counter

  int current_tube_led = 0; // Current output for the LED tube
  #define tube_led_max 125 // Max brightness for the LED tube
  int tube_led_time = 0; // Time to incriment the LED tube brightness


void setup_vibe(){
  current_mood = 0;
  current_best_mood = tease_power_start;
  ti = 0;

  next_flick = millis() + (flick_warn_interval * k);  // First flick will be the interval after now
  next_tease = millis() + (tease_interval_start * k); // First tease session will start in x seconds (default start is 120 seconds)
  red_over = millis() + (red_time * kM); // defines red time, for a new one reset the counter
}



/////////////
// Make some functions here that'll help out in the future
/////////////







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
  float red_time_left = constrain((red_time - millis()) / kM,0,red_time); 
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
int i_constrain_loop(int current, int max, int min = 0 ){
current = current + 1;  // advance by one
if(current > max){      // Shit... too far?    
    current = min;        // Start again.
  }
return current;
} // i_constrain_loop
//////////////////////







// VOID SETUP --- VOID SETUP --- VOID SETUP --- VOID SETUP --- Why do I always miss you when looking?

void setup() {

  tease_power_inc = constrain((vibrator_max-vibrator_low)/tease_session_qty,1,vibrator_max-vibrator_low);
  tease_interval_dec = constrain((tease_interval_start-5)/tease_session_qty,5,tease_interval_start);

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
  ledcAttachPin(Out4_pin,Out4);
  ledcSetup(Out4, 1000, 8); //Fast PWM for the LED Tube
  ledcWrite(Out4,tube_led_max); // Make the tube pretty

  /////////////
  // Assign non PWM outputs
  /////////////

   pinMode(Out3_pin,OUTPUT); // "Red Light" output, on = locked up
   LogDebug("[Start Time] Welp, you're now locked up for : " + String(red_time));
   digitalWrite(Out3_pin,HIGH);

   pinMode(Out2_pin,OUTPUT);


  /////////////
  // Assign Input Button
  /////////////

    pinMode(Button1,INPUT_PULLUP);                              // setup the input, it has an external pull-up
    pinMode(Button2,INPUT_PULLUP);                              // setup the input, it has an external pull-up
    pinMode(Button3,INPUT_PULLUP);                              // setup the input, it has an external pull-up




}



//  VOID LOOP --- LOOP LOOP LOOP --- VOID LOOP --- LOOP LOOP LOOP --- I sometimes miss you too

void loop() {

if(!digitalRead(Button2)){

  if(vibe_setup_required){
    setup_vibe();
    LogDebug("[Vibe Setup] Vibe setup required");
    vibe_setup_required = false;
    flick_warn();
  }

  if(after_now(next_flick) && vibez_on){next_flick = flick_warn();}           // Flick Warn every interval
  if(after_now(red_over)){digitalWrite(Out3_pin,LOW);if(!vibez_on){vibez_on = true;}; cum_now();}             // After the red time is over, turn off the red light "lock up" output (Output 2) and give one last hurrah



  if(after_now(next_tease)){              // Run a teasing ramp 
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

  

} else {

  vibe_setup_required = true;   // We're not running the vibe now, so we need to set it up again


}

if(!digitalRead(Button1) && after_now(next_her) && !digitalRead(Button3)){

  current_tube_led = constrain(current_tube_led - 5,0,tube_led_max); // decriment the LED brightness by 5 until 0

  current_mood = hold_her_inesnity; 
  digitalWrite(Out2_pin,true);
  was_running = true;
  hold_her_inesnity = i_constrain_loop(hold_her_inesnity,vibrator_max,vibrator_low);
  delay(200);

} else {

  digitalWrite(Out2_pin,false);
  if(was_running){ current_mood = 0; was_running = false; next_her = millis() + 5000; LogDebug("[Hold Her] Hold her for 5 seconds");}
  hold_her_inesnity = 0;
  
  if(after_now(tube_led_time)){
    current_tube_led = constrain(current_tube_led + 2,0,tube_led_max); // incriment the LED brightness by 5 until max
    tube_led_time = millis() + 200;
  }

}

ledcWrite(Out4,current_tube_led);
ledcWrite(Vibez,current_mood);  // Finally, get those good vibez after we figured out what we wanted. 

if(after_now(tube_led_time)){LogDebug("[LED Tube] Current LED Tube Brightness: " + String(current_tube_led) + " / " + String(tube_led_max));}

}

