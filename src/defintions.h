
                        // [Default pin] for the offical PCB, change to suit for your project
#define Vibez_pin   12  // [32]
#define Vibez       0   // PWM Channel, I think this is some black magic in the ESP32 processor
#define Out2_pin    33  // [33]
#define Out2        1   // PWM Channel (if used)
#define Out3_pin    34  // [34]
#define Out3        2   // PWM Channel
#define Out4_pin    35  // [35]
#define Out4        3   // PWM Channel

#define Stahp_pin   14  // [25] button input for stopping the current loop and moving to rest. 



// Kyle's debug code from OSSM 

#define DEBUG

#ifdef DEBUG
#define LogDebug(...) Serial.println(__VA_ARGS__)
#define LogDebugFormatted(...) Serial.printf(__VA_ARGS__)
#else
#define LogDebug(...) ((void)0)
#define LogDebugFormatted(...) ((void)0)
#endif

