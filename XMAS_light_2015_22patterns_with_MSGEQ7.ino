//  Christmas light project

int analog60hz = A0;    // select the input pin from the 60Hz reference
int MSGEQ7analogPin = A1;  // Data from MSGEQ7
int AC_value = 0;  // variable to store the value coming from the sensor

int MSGEQ7strobePin = 3; // strobe is attached to digital pin 3
int MSGEQ7resetPin = 2; // reset is attached to digital pin 2
int MSGEQ7channel = 0; // Channel pointer
int MSGEQ7spectrumValue[7] = {0,0,0,0,0,0,0};
int MSGEQ7amplitude = 1;
int MSGEQ7mask[7] ={0,0,0,0,0,0,0};

int i,j,k,slot_counter;
long cycle_counter;

int  Pattern = 1;
//int  How_many_patterns = 5;  // %%% Adjust as necessary
int  How_many_patterns = 23;  // %%% Adjust as necessary
int  Event_counter = 0;
int  N_events;  // Single event
int  Max_Brightness;
int  Min_Brightness;
int  Rate;
int  Duration;
int  N_events_temp;
int  Duration_temp;
int  Mask;

#define N_drivers 10   // # of channels, have to adjust wait_for_50uSec() and array size
#define threshold 200  // AC threshold 
#define max_slot 120
#define multiplier 25
#define pulse_width_1ms 10
#define constant ((10*multiplier*max_slot)/60)

boolean positive_cycle;
boolean drive_window;
boolean compute_window;

int channel[N_drivers] = {13,12,11,10,9,8,7,6,5,4};
//int channel[N_drivers] = {13,12,11,10,9};
int pulse_1ms[N_drivers];
int gate_trigger[N_drivers];

boolean ramp_down[N_drivers];
int ramp_control[N_drivers]; // 0 = ramp up and down, -1 stays down, 1 stays up
int top_load[N_drivers];
int bottom_load[N_drivers];
int value[N_drivers];
int cycle_rate[N_drivers];
int cycle_begin[N_drivers];
int cycle_end[N_drivers];


void clear_drive_arrays() {
  for (i=0; i<N_drivers; i++) {
    gate_trigger[i] = max_slot/2;  // Half way there
    pulse_1ms[i] = 0;
  }
}

void clear_compute_arrays() {
  for (i=0; i<N_drivers; i++) {
    ramp_down[i] = false;
    ramp_control[i] = 0;
    value[i] = gate_trigger[i] * multiplier;
    top_load[i] = multiplier * max_slot;  // Max brightness
    bottom_load[i] = multiplier*1; // Min brightness
    cycle_rate[i] = 25; // from dim to bright in 5 second
    cycle_begin[i] = 120;
    cycle_end[i] = 300;  // 5 second duration
  }
}


void wait_for_50uSec() {
  delayMicroseconds(10);  // 10 uSec delay considering 10 channels, for 5, use 25 uSec
}


void setup() {
  
//  pinMode(2, OUTPUT); // %%%% Pin 2 is for debug, reset RS flip-flops 
//  digitalWrite(2, LOW);
//  pinMode(3, OUTPUT); // %%%% Pin 3 is for debug 
//  digitalWrite(3, LOW);

  Serial.begin(9600);
  pinMode(MSGEQ7analogPin, INPUT);
  pinMode(MSGEQ7strobePin, OUTPUT);
  pinMode(MSGEQ7resetPin, OUTPUT);
  analogReference(DEFAULT);

  digitalWrite(MSGEQ7resetPin, HIGH);
  digitalWrite(MSGEQ7strobePin, LOW);
  Serial.println("MSGEQ7 with LED lights");
  
  for (i=0; i<N_drivers; i++) {  // Define pins and set to low output
    pinMode(channel[i], OUTPUT);
    digitalWrite(channel[i], LOW);
  }
  positive_cycle = false;  // Initialise key variables
  drive_window = false;
  compute_window = false;
  clear_drive_arrays();
  clear_compute_arrays();
  slot_counter = 0;
  cycle_counter = 0;

  Pattern = 0;   //  Initialize high level variables (start with caps)
  Event_counter = 0;
  N_events = N_drivers;
  
  
}

void loop() {
  
  if (!drive_window) AC_value = analogRead(analog60hz);  // read the sinewave analog value, but not during drive cycle (quicker) 

// Set flags according to where we are in the sinewave cycle  
  
  if (AC_value > 5 && !positive_cycle) positive_cycle = true;  // Beginning of the positive cycle
  
  if (AC_value < 5 && positive_cycle) {  // When end of positive cycle is reached
    positive_cycle = false;
    drive_window = false;
    slot_counter = 0;
//      digitalWrite(3, LOW);  %%% For debug, pulse ends when here

    
  }

  if (AC_value > threshold  && positive_cycle) {  // When the voltage on the AC line is high enough to turn on LEDs
    drive_window = true;
    compute_window = false;
  }
  
    
//-------------------------------------------------------------
//   Positive cycle
//-------------------------------------------------------------

  if (drive_window) {  //  Called on positive cycle, (120 times in the middle of the positive cycle)

    for (i=0; i<N_drivers; i++) {  // Scan every channel

    if ((slot_counter == gate_trigger[i]) && (slot_counter < max_slot)) {  // If slot = gate delay
      digitalWrite(channel[i], HIGH);  // Turn 1 mSec pulse on
      pulse_1ms[i] = pulse_width_1ms;  // And set timer
    }
  
    if (pulse_1ms[i] > 0 ) {  // Is a 1 mSec pulse started?
      pulse_1ms[i] = pulse_1ms[i] - 1;  // Yes, bump timer
      if (pulse_1ms[i] == 0 ) digitalWrite(channel[i], LOW);  // When 0 reached, turn pulse off
    }  
  }  // Of FOR loop   
 
    slot_counter = slot_counter +1;  // Keep track of slots (120 in the middle of the positive cycle)
    if (slot_counter > (max_slot)) { // %%% If we reached all the slots in drive, and those for the 1 mSec pulse
//    if (slot_counter > (max_slot + pulse_width_1ms)) { // If we reached all the slots in drive, and those for the 1 mSec pulse
      drive_window = false;  // End of drive cycle
      compute_window = true;  // Next is compute cycle
//      digitalWrite(3, HIGH);  //  %%% For debug, pulse starts when drive window is closed
                                                                  
      for (i=0; i<N_drivers; i++) {  // Set all driver pins to low output
        digitalWrite(channel[i], LOW);
      }  

//    digitalWrite(2, HIGH);  //  %%% Debug Pulse line 2 to reset RS flip-flops
//    delay(1);
//    digitalWrite(2, LOW);

  digitalWrite(MSGEQ7resetPin, HIGH);  // Reset MSGEQ7 to lower filter
  digitalWrite(MSGEQ7strobePin, HIGH);
  delay(1);  
  digitalWrite(MSGEQ7resetPin, LOW);
  delayMicroseconds(100);
  digitalWrite(MSGEQ7strobePin, LOW);  // Cue to 63Hz channel 1st 


    }  
    wait_for_50uSec();  // A slot is roughly 50 uSec, delay to compensate
  }

//-------------------------------------------------------------
//   Negative cycle
//-------------------------------------------------------------

  if (compute_window) {
 
//  Handle MSGEQ7 reading, one frequency per 60Hz cycle

    // Cue to right channel    
    for (int i = 0; i < 6; i++) {
      if (i < MSGEQ7channel) {
        digitalWrite(MSGEQ7strobePin, HIGH);  // only pulse if channel is not reached
      } else {
        digitalWrite(MSGEQ7strobePin, LOW);   // else, no pulse
      }  
      delayMicroseconds(50); // to allow the output to settle
      digitalWrite(MSGEQ7strobePin, LOW);
      delayMicroseconds(50); // to allow the output to settle
    }
    MSGEQ7amplitude = analogRead(MSGEQ7analogPin) - 123 ;  // read value, substract 123 to get -123 to 1000 range
    if (MSGEQ7amplitude < 0) MSGEQ7amplitude = 0;  // Signal conditionning, remove noise from -123 to 0
 //   if (MSGEQ7amplitude > 1000) MSGEQ7amplitude = 1000;  // Not necessary, no value should exceed 1000 with above lines
    MSGEQ7spectrumValue[MSGEQ7channel] = MSGEQ7amplitude/10; // Divide by 10 to get values between 0 and 100
 
    MSGEQ7channel = MSGEQ7channel + 1;  // point to next channel
    if (MSGEQ7channel == 7) {
      MSGEQ7channel = 0;  // if reached last channel, restart from beginning
      for (int i = 0; i < 7; i++) {  // %%% Debug
        Serial.print(MSGEQ7spectrumValue[i]);  // %%% Debug
        Serial.print(" ");  // %%% Debug
      }
      Serial.println();  // %%% Debug
    }
 //  End of MSGEQ7 section
 
    for (i=0; i<N_drivers; i++) {  // Scan every channel

      if ((cycle_counter > cycle_begin[i])) {  // Wait for the lag to do anything
      
 // Case where we ramp up and down
            if (ramp_control[i] == 0) {  // 0 = up and down
              if (ramp_down[i]) {
                value[i] = value[i] - constant/cycle_rate[i];  // Compute value, considering multiplier
                if (value[i] < bottom_load[i]) {  // check lower limit
                  value[i] = bottom_load[i];  // if reached, set it
                  ramp_down[i] = false;
                }  
              } else {
                value[i] = value[i] + constant/cycle_rate[i];
                if (value[i] > top_load[i]) {  // check upper limit
                  value[i] = top_load[i];  // if reached, set it  
                  ramp_down[i] = true;
                }
             } 
           }  // Of if (ramp_control == 0)
     
  // Case where we ramp up 
           if (ramp_control[i] > 0) {
             value[i] = value[i] + constant/cycle_rate[i];
             if (value[i] > top_load[i]) {  // check upper limit
               value[i] = top_load[i];  // if reached, set it 
               ramp_down[i] = true;
             }
           }
           
  // Case where we ramp down 
           if (ramp_control[i] < 0) {
             value[i] = value[i] - constant/cycle_rate[i];
             if (value[i] < bottom_load[i]) {  // check lower limit
               value[i] = bottom_load[i];  // if reached, set it  
               ramp_down[i] = false;
             }  
           } 
           
         gate_trigger[i] = max_slot - value[i]/multiplier;  // Set gate value, based on multiplier
      }  

  // Case where we ended a sub-pattern
      if ((cycle_counter == cycle_end[i])) {  // reached end of one of the channel timer?
        pattern_sorter(i,false);  // false means "during loop"
        Event_counter = Event_counter + 1;  // keep track of event
       
      }

    } // Of For loop

// Check if end of pattern 
        if (Event_counter >= N_events) {  // Check if all the events are done in this sequence
        
//        digitalWrite(3, HIGH);
          Pattern = Pattern + 1;  // Pick next pattern
          if (Pattern > How_many_patterns) Pattern = 1; // make sure it's valid
//          Pattern = 23;  //  %%% Debug
          cycle_counter = 0;  // Reset key variables
          Event_counter = 0;          
          pattern_sorter(0,true);  // true means "begin" pattern
        }

    
    cycle_counter = cycle_counter + 1;  // Keep track of cycles (60 Hz, 60 in a second)
    compute_window = false;  // Exit compute window

  } // Of if (compute_window)
  
}  // Of LOOP

/////////////////////////////////////////////////////////////////////////////////////
//
//   Pattern routines
//
/////////////////////////////////////////////////////////////////////////////////////

void pattern_sorter(int Channel,boolean Begin_pattern) {
//  Based on the value of pattern, do a sequence
  switch (Pattern) {
    case 1:
      if (Begin_pattern) dim_up_and_down_begin(0,120,0);  // Random Rate, 60/10 second duration, up and down
//      if (Begin_pattern) dim_up_and_down_begin(5,120,0);  // Rate of 5/10 second, 60/10 second duration, up and down
      break;
      
    case 2:
      if (Begin_pattern) hold_it_begin(10); // 10/10 second duration
      break;
      
    case 3:
      if (Begin_pattern) {
        sparkle_begin(100,20); // 100 sparkles, each 20/60 of a second lit 
      } else {
        sparkle(Channel);
      }  
      break;
      
    case 4:
//      if (Begin_pattern) all_on(120, true); // Turn on all the lights for 120/60 second
      if (Begin_pattern) turn_on(0x3FF,10,30); // Turn on specific lights at a rate of 10/10 second for 30/10 seconds 
      break;
      
    case 5:
      if (Begin_pattern) {
        chenillard_begin(3,15); // 3 up and down runs, 15/60 of a second for each light to turn on 
      } else {
        chenillard(60,0); // 60/60 seconds of Off (or on) time between sequences, has to be timed 1st parameter of chenillard_begin 
                          // 0 means up and down, 1 means up, 2 means down, -1 means lit, then down, -2 means lit, then up
      }
      break;
      
    case 6:
      if (Begin_pattern) turn_on(0x3FF,20,30); // Turn on specific lights at a rate of 20/10 second for 30/10 seconds 
      break;
      
    case 7:
      if (Begin_pattern) {
        chenillard_begin(5,5); // 5 up and down runs, 5/60 of a second for each light to turn on 
      } else {
        chenillard(30,1); // 30/60 seconds of Off (or on) time between sequences, has to be timed 1st parameter of chenillard_begin 
                          // 0 means up and down, 1 means up, 2 means down, -1 means lit, then down, -2 means lit, then up
      }      
      break;
      
    case 8:
      if (Begin_pattern) {
        sparkle_begin(60,random(10,30)); // 60 sparkles, each random/60 of a second lit 
      } else {
        sparkle(Channel);
      }  
      break;

    case 9:
      if (Begin_pattern) turn_on(0x300,10,20); // Turn on specific lights at a rate of 10/10 second for 20/10 seconds 
      break;

    case 10:
      if (Begin_pattern) turn_on(0x303,10,20); // Turn on specific lights at a rate of 10/10 second for 20/10 seconds 
      break;

    case 11:
      if (Begin_pattern) turn_on(0x3C3,10,20); // Turn on specific lights at a rate of 10/10 second for 20/10 seconds 
      break;

    case 12:
      if (Begin_pattern) turn_on(0x3C7,10,20); // Turn on specific lights at a rate of 10/10 second for 20/10 seconds 
      break;

    case 13:
      if (Begin_pattern) turn_on(0x3FF,10,20); // Turn on specific lights at a rate of 10/10 second for 20/10 seconds 
      break;

    case 14:
      if (Begin_pattern) all_on(60, false); // Turn off all lights for 60/60 seconds
      break;

    case 15:
      if (Begin_pattern) dim_up_and_down_begin(5,50,0);  // Rate of 5/10 second, 50/10 second duration, up and down
      break;      

    case 16:
      if (Begin_pattern) {
        sparkle_begin(200,10); // 200 sparkles, each 10/60 of a second lit 
      } else {
        sparkle(Channel);
      }  
      break;

    case 17:
      if (Begin_pattern) {
        chenillard_begin(5,5); // 5 up and down runs, 5/60 of a second for each light to turn on 
      } else {
        chenillard(10,-1); // 60/60 seconds of Off (or on) time between sequences, has to be timed 1st parameter of chenillard_begin 
                          // 0 means up and down, 1 means up, 2 means down, -1 means lit, then down, -2 means lit, then up
      }      
      break;
      
    case 18:
      if (Begin_pattern) {
        chenillard_begin(5,5); // 5 up and down runs, 5/60 of a second for each light to turn on 
      } else {
        chenillard(10,-2); // 60/60 seconds of Off (or on) time between sequences, has to be timed 1st parameter of chenillard_begin 
                          // 0 means up and down, 1 means up, 2 means down, -1 means lit, then down, -2 means lit, then up
      }      
      break;
 
    case 19:
      if (Begin_pattern) all_on(60, true); // Turn on all lights for 60/60 seconds
      break;     

    case 20:
      if (Begin_pattern) turn_on(0x03F,10,20); // Turn on specific lights at a rate of 10/10 second for 20/10 seconds 
      break;

    case 21:
      if (Begin_pattern) turn_on(0x038,10,20); // Turn on specific lights at a rate of 10/10 second for 20/10 seconds 
      break;

    case 22:
      if (Begin_pattern) turn_on(0x000,10,20); // Turn on specific lights at a rate of 10/10 second for 20/10 seconds 
      break;

    case 23:
      if (Begin_pattern) {
        MSGEQ7_begin(200,0x003,0x00C,0x010,0x020,0x040,0x080,0x300); // Set light pattern to equilizer value (from 63Hz to 16KHz)
//        MSGEQ7_begin(200,0x001,0x002,0x004,0x008,0x010,0x020,0x040); // Set light pattern to equilizer value (from 63Hz to 16KHz)
//        MSGEQ7_begin(200,0x000,0x003,0x00C,0x030,0x0C0,0x300,0x000); // Set light pattern to equilizer value (from 63Hz to 16KHz) 
 
      } else {
        MSGEQ7(); // 50/10 seconds duration of equalizer 
      }      
      break;
      
  }
}

//------------------------------------------------------------------
//  Light show equalizer
//------------------------------------------------------------------
//

void MSGEQ7_begin(int duration_in_tenth_of_seconds,int mask0,int mask1,int mask2,int mask3,int mask4,int mask5, int mask6) {

  MSGEQ7mask[0] = mask0;
  MSGEQ7mask[1] = mask1;
  MSGEQ7mask[2] = mask2;
  MSGEQ7mask[3] = mask3;
  MSGEQ7mask[4] = mask4;
  MSGEQ7mask[5] = mask5;
  MSGEQ7mask[6] = mask6;
  
  N_events = 6 * duration_in_tenth_of_seconds;  // Create a lot of events 
  
  Duration = 2; // Display new event every 30th of a second
 
  for (j=0; j<N_drivers; j++) { 
        ramp_control[j] = 1;
        cycle_rate[j] = 1;  // fastest rate of change
        cycle_begin[j] = cycle_counter + 1;
        cycle_end[j] = 30000;  // Large value, use cycle_end[0]
  }

  cycle_end[0] = cycle_counter + Duration;  // Only set a channel to get one end of event 
}

void MSGEQ7() {

  for (j=0; j<7; j++) {
    MSGEQ7amplitude = 30 * MSGEQ7spectrumValue[j];  // Value between 0 and 3000
    Mask = 0x0001;
  
    for (k=0; k<N_drivers; k++) {
      if (MSGEQ7mask[j] & Mask) {  // If the channel is correct (could be multiple channels)
        if (MSGEQ7amplitude > value[k]) {
          top_load[k] = MSGEQ7amplitude;  // Current spectrum value for that channel
          ramp_control[k] = 1;
        } else {
          bottom_load[k] = MSGEQ7amplitude;  // Current spectrum value for that channel
          ramp_control[k] = -1;
        }
      }  
      Mask = Mask << 1; // Shift mask to next light
    }  // Of for (k=0...
  }
  cycle_end[0] = cycle_counter + 2;  // Only set a channel to get one end of event  
}


//------------------------------------------------------------------
//  Dim up and down
//------------------------------------------------------------------
//
//  Cycle on, then off the lights, at a rate equal to rate_speed/10 seconds.  If rate_speed = 0, do it randomly.
//  Do it for duration_in_tenth_of_seconds
//  If ramp = 0, up and down, ramp = 1, up then stop, ramp = -1, down then stop
//

void dim_up_and_down_begin(int rate_speed, int duration_in_tenth_of_seconds, int ramp) {

  N_events = 1;  // Single event
  
  Max_Brightness = 100;
  Min_Brightness = 1;
  Duration = 6 * duration_in_tenth_of_seconds;  // 
  
  for (j=0; j<N_drivers; j++) { 
        ramp_control[j] = ramp;
        top_load[j] = multiplier * ((Max_Brightness * max_slot)/100) ;  // Max brightness
        bottom_load[j] = multiplier * ((Min_Brightness * max_slot)/100); // Min brightness
        if (rate_speed == 0) {
          cycle_rate[j] = random(5,25); // from dim to bright in between 0.5 and 2.5 seconds
        } else {
          cycle_rate[j] = rate_speed; 
        }  
        cycle_begin[j] = cycle_counter + 1;
        cycle_end[j] = 30000;  // Large value, use cycle_end[0]
  }
  cycle_end[0] = cycle_counter + Duration;  // Only set a channel to get one end of event

}

//------------------------------------------------------------------
//  Hold it
//------------------------------------------------------------------

void hold_it_begin(int duration_in_tenth_of_seconds) {

  N_events = 1;  // Single event
  
  Duration = 6 * duration_in_tenth_of_seconds;  // 
  
  for (j=0; j<N_drivers; j++) { 
        ramp_control[j] = 1;
        top_load[j] = value[j] ;  // last value
        bottom_load[j] = value[j] ;  // last value
        cycle_begin[j] = cycle_counter + 1;
        cycle_end[j] = 30000;  // Large value, use cycle_end[0]
  }
  cycle_end[0] = cycle_counter + Duration;  // Only set a channel to get one end of event
}

//------------------------------------------------------------------
//  Sparkle
//------------------------------------------------------------------
//  Sparkle all the lignts, up to number_of_sparkles.  If number_of_sparkles = 0, do it randomly.
//  Each lamp is turned on a time equal to duration_in_sixtyth_of_seconds

void sparkle_begin(int number_of_sparkles, int duration_in_sixtyth_of_seconds) {

   if (number_of_sparkles == 0) {
          N_events = random(20,200); // Sparkle time is random
        } else {
          N_events = number_of_sparkles; 
        }  
   
  Min_Brightness = 1;
  Max_Brightness = 100;  
  Duration = duration_in_sixtyth_of_seconds;  // Short pulses

  for (j=0; j<N_drivers; j++) { 
        ramp_control[j] = 1;  // Turn on the lights at random times
        top_load[j] = multiplier * ((Max_Brightness * max_slot)/100) ;  // Max brightness
        bottom_load[j] = multiplier * ((Min_Brightness * max_slot)/100); // Min brightness
        cycle_rate[j] = 1;  // fastest rate of change
        cycle_begin[j] = cycle_counter + random(10, 40);
        cycle_end[j] = cycle_begin[j] + Duration;  // short ON second duration
  }
}  


void sparkle(int c) {
  if (ramp_control[c] == 1) {  // was lit
    ramp_control[c] = -1; // turn off if on, and vice-versa
  } else {  
    ramp_control[c] = 1;
  }       
  cycle_begin[c] = cycle_counter + random(10, 40);
  cycle_end[c] = cycle_begin[c] + Duration;  // short ON second duration
}

//------------------------------------------------------------------
//  Chenillard
//------------------------------------------------------------------
//  Chenillard 10 lamps, then off the lights, at a rate equal to rate_speed/10 seconds.  
//  Each lamp is turned on a time equal to duration_in_sixtyth_of_seconds
//  If ramp = 0, up and down, ramp = 1, up then turn off and repeat, ramp = -1, down then off and repeat


void chenillard_begin(int cycles, int duration_in_sixtyth_of_seconds) {

  all_on(10,false);  // Turn off all lights

  N_events = cycles*2 +1;  // Multiple events (add 1 to include the sequence when lights are turned off)
  
  Min_Brightness = 1;
  Max_Brightness = 100;
  Rate = duration_in_sixtyth_of_seconds; //   
  Duration = (Rate * cycles*2 * N_drivers) / 6;  // Total duration for a cycle is 10 lamps * 1/2 second per lamp

}  

void chenillard(int off_on_time, int ramp) {
//  If ramp = 0, atart by having all lights off, then move up and down, 
//     ramp = 1, atart by having all lights off, then move up from left to right, then turn off and repeat, 
//     ramp = 2, start by having all lights off, then move down from right to left, then turn off and repeat,
//     ramp = -1, start by having all lights on, then extinguish down from right to left, then turn on and repeat,
//     ramp = -2, atart by having all lights on, then extinguish up from left to right, then turn on and repeat, 
//  off_on_time is the interval of time the lights are all off or all on between sequences
//  **** Not functionnal if ramp = 0 ****

   if (ramp_control[0] == 1) {  // was lit
     if (ramp > 0) {
       save_variables();
       all_on(off_on_time,false);  // Turn off all lights
       restore_variables();       
     } else {
       for (j=0; j<N_drivers; j++) {
         ramp_control[j] = -1;
         if (ramp > -2) {
           cycle_begin[j] = cycle_counter + ((N_drivers - j - 1) * Rate) + 1; // Extinguish Right to left
         } else {
           cycle_begin[j] = cycle_counter + (j * Rate) + 1;  // Extinguish Left to right
         }  
       }
       cycle_end[0] = cycle_counter + Duration;  //equal to the time it takes to do all channels
     }

   } else {  // was off, turn it on
     if (ramp < 0) {
       save_variables();
       all_on(off_on_time,true);  // Turn on all lights
       restore_variables();
     }  else {
       for (j=0; j<N_drivers; j++) { 
         ramp_control[j] = 1;
         if (ramp < 2) {
           cycle_begin[j] = cycle_counter + (j * Rate) + 1;  // Turn on Left to right
         } else {
           cycle_begin[j] = cycle_counter + ((N_drivers - j - 1) * Rate) + 1;  // Turn on Right to left
         }
       }  
       cycle_end[0] = cycle_counter + Duration;  //equal to the time it takes to do all channels
     }  
  }  // of if (ramp_control[0] == 1)
    
}

//------------------------------------------------------------------
//  All on (or all off)
//------------------------------------------------------------------

void all_on(int duration_in_sixtyth_of_seconds, boolean on_off) {
  
   N_events = 1;  // Single event
   Duration = duration_in_sixtyth_of_seconds;  // Do it in a second
// Turn off all the lights for 1 second
  for (j=0; j<N_drivers; j++) {
    if (on_off) { 
      ramp_control[j] = 1;  // Turn on all lights
    } else {
      ramp_control[j] = -1;  // Turn off all lights
    }
    top_load[j] = multiplier * max_slot ;  // Max brightness
    bottom_load[j] = multiplier * 1; // Min brightness
    cycle_rate[j] = 2;  // do it fast
    cycle_begin[j] = cycle_counter + 1;
    cycle_end[j] = 30000;  // Large number, to only flag cycle_end[0]
  }
    cycle_end[0] = cycle_counter + Duration;
}

//------------------------------------------------------------------
//  Turn_on (or all off) specific channel(s)
//------------------------------------------------------------------
//
//  Turn on specific lights, in the patters word, 0 means off, 1 means on
//  LSB of pattern_word is the leftmost light
//  MSB (bit 9) is rightmost light
//  Rate of turn on is second argument, Duration is the third one

void turn_on(int pattern_word, int Rate, int duration_in_tenth_of_seconds) {
  
   N_events = 1;  // Single event
   Duration = 6*duration_in_tenth_of_seconds;  // Do it in a second
   Mask = 0x0001;
  
  
  for (j=0; j<N_drivers; j++) {
    if (pattern_word & Mask) { 
      ramp_control[j] = 1;  // Turn on all lights
    } else {
      ramp_control[j] = -1;  // Turn off all lights
    }
    Mask = Mask << 1; // Shift mask to next light
    top_load[j] = multiplier * max_slot ;  // Max brightness
    bottom_load[j] = multiplier * 1; // Min brightness
    cycle_rate[j] = Rate;  // do it at the right speed
    cycle_begin[j] = cycle_counter + 1;
    cycle_end[j] = 30000;  // Large number, to only flag cycle_end[0]
  }
    cycle_end[0] = cycle_counter + Duration;
}


void save_variables() {
    N_events_temp = N_events;
    Duration_temp = Duration;
}

void restore_variables() {
    N_events = N_events_temp;
    Duration = Duration_temp;
}

//  **************  Old Pattern  ************************

/*
void pattern_sorter(int Channel,boolean Begin_pattern) {
//  Based on the value of pattern, do a sequence
  switch (Pattern) {
    case 1:
      if (Begin_pattern) dim_up_and_down_begin(0,120,0);  // Random Rate, 60/10 second duration, up and down
//      if (Begin_pattern) dim_up_and_down_begin(5,120,0);  // Rate of 5/10 second, 60/10 second duration, up and down
      break;
    case 2:
      if (Begin_pattern) hold_it_begin(10); // 10/10 second duration
      break;
    case 3:
      if (Begin_pattern) {
        sparkle_begin(100,20); // 100 sparkles, each 20/60 of a second lit 
      } else {
        sparkle(Channel);
      }  
      break;
    case 4:
//      if (Begin_pattern) all_on(120, true); // Turn on all the lights for 120/60 second
      if (Begin_pattern) turn_on(0x3FF,10,30); // Turn on specific lights at a rate of 10/10 second for 30/10 seconds 
       
      break;  
    case 5:
      if (Begin_pattern) {
        chenillard_begin(3,20); // 3 up and down runs, 20/60 of a second for each light to turn on 
      } else {
        chenillard(60,0); // 60/60 seconds of Off (or on) time between sequences, has to be timed 1st parameter of chenillard_begin 
                          // 0 means up and down, 1 means up, 2 means down, -1 means lit, then down, -2 means lit, then up
      }
      break;

  }
}
*/

