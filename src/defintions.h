
                        // [Default pin] for the offical PCB, change to suit for your project
#define Vibez_pin   32  // [32] --> Default Vibe Pin
#define Vibez       0   // PWM Channel, I think this is some black magic in the ESP32 processor
#define Out2_pin    33  // [33] --> Default Air Valve Pin
#define Out2        1   // PWM Channel (if used)
#define Out3_pin    27  // [27] --> Default Lock Pins
#define Out3        2   // PWM Channel
#define Out4_pin    26 // [26]
#define Out4        3   // PWM Channel

#define Button1     25  // [25] button input for triggering the vibe and closing the valve 
#define Button2     34
#define Button3     35


// Kyle's debug code from OSSM 

#define DEBUG

#ifdef DEBUG
#define LogDebug(...) Serial.println(__VA_ARGS__)
#define LogDebugFormatted(...) Serial.printf(__VA_ARGS__)
#else
#define LogDebug(...) ((void)0)
#define LogDebugFormatted(...) ((void)0)
#endif

