
#include <werkstatt_non_blocking.h>
#include <TimerOne.h>

#define ONE_SEC_IN_US 60000
#define MAX_BPM 400
#define MIN_BPM 60
#define CONTROL_PERIOD_US 500
#define MAX_STEPS 15 // max steps indexed from 0
#define UNITY_SCALING_FACTOR 5 // multiply by 5 to get the arduino pwm value necessary
#define NO_NOTE 32 // the 5th bit of the note byte will tell us if the sequence note is active or not

// Serial Masks
#define serial

#define HEADER_POSITION_SEL_MSK 0x0F // position lives in lower nibble of header byte
#define HEADER_RESERVED_MSK 0x0F // reserved section lives in upper nibble of header byte

int control_updated = 0; // control updated flag
arpeggiator arpeggiator(6); //initiate and name the arpeggiator class (Output pin)


interface bpm{A0}; // initiate and name the bpm interface class (input pin)
int bpm_value=MAX_BPM;

uint8_t unity_data[3]; // will hold our received unity data
uint8_t *newDataBuffer = unity_data; // point to first character in unity array

/* 
notes are assigned in intervals: tonic, minor2nd, major2nd, minor3rd,
major3rd, fourth, tritone, fifth, minor6th, major6th, minor7th,
major7th, octave. 
*/
int notes[MAX_STEPS] = {fifth   , fifth,   fifth,     fifth,    fifth,    fifth,  fifth}; // VCO EXP config
int note_values[MAX_STEPS] = {q,     q,      q,        q,         q,       q,      q,     }; //VCO EXP config

int sequence_length = 0;
int *outBufferNotes = notes; // points to current note in sequence
int *outBufferNoteValues = note_values; // points to current note length in sequence
int step_index = 0;
int output_note = 0;

long dummy_period_us = 0;
int process_unity = 0;
int unity_index = 0;


//Read analog control pins every ms or so     
ISR(TIMER2_OVF_vect)
{
     bpm_value = bpm.potentiometer(MIN_BPM, MAX_BPM); // Update bpm value  
}
 
void output_note_ISR()
{
    Timer1.stop();
    output_note = 1;
}

void setup() 
{ 
  // clear previous sequence

  for(int i=0; i < MAX_STEPS; i++){
    notes[i] = -1
    note_values[i] = q  
  }
   
  // Init analog knob update timer
  TIMSK2 = (TIMSK2 & B11111110) | 0x01; //use mask so that only the least significant bit is affected
  
  // Init first note being played
  bpm_value = bpm.potentiometer(MIN_BPM, MAX_BPM); // Read potentiometer on A0 to get initial bpm value
  dummy_period_us = arpeggiator.note_time_us(bpm_value, *outBufferNoteValues); // Set sequencer to start on that bpm

  arpeggiator.play(*outBufferNotes);

  outBufferNotes = notes + step_index; // Increment our pointer and allow it to wrap around
  outBufferNoteValues = note_values + step_index; // Increment our pointer and allow it to wrap around

  Serial.begin( 115200 ); // Instead of 'Serial1'
  Serial.println("Enter data to echo:");
//  Serial.print();

  Timer1.initialize(dummy_period_us); // set up timer with first value in array
  Timer1.attachInterrupt(output_note_ISR);
}      


void loop() 
{
  // I am ashamed
  if(Serial.available() == 3){
    Serial.readBytes(newDataBuffer, (byte) 3);
 
    // Lets parse this shit
    uint8_t header;
    uint8_t note_byte;
    uint8_t duration_byte;
    uint8_t modify_index;
    
    header =  *(newDataBuffer)- 48 ;
    modify_index = header & MAX_STEPS ; // Lower byte contains index to change
    note_byte = *(newDataBuffer+1)- 48; 
    duration_byte = *(newDataBuffer+2) - 48;
//    
    notes[modify_index] = (int) note_byte * UNITY_SCALING_FACTOR;
    note_values[modify_index] = (int) duration_byte;
    
//    *outBufferNotes = (int) note_byte;
//    *outBufferNoteValues = (int) duration_byte;
    Serial.print((int) modify_index);
    Serial.print(',');
    Serial.print((int) note_byte);
    Serial.print(',');
    Serial.print((int) duration_byte);
    Serial.print('\n');

    
//    step_index++;
//    if(step_index > (sizeof(notes)/sizeof(int))-1){
//      step_index = 0;  
//    }
//    Serial.end();
  }
  
    

  if(output_note){
    // update sequencer pointers
    outBufferNotes = notes + step_index; // Increment our pointer and allow it to wrap around
    outBufferNoteValues = note_values + step_index; // Increment our pointer and allow it to wrap around
    
    // start playing then change note   
    arpeggiator.play(*outBufferNotes);

    step_index++;
    if(step_index > (sizeof(notes)/sizeof(int))-1){
      step_index = 0;  
    }
    
    dummy_period_us = arpeggiator.note_time_us(bpm_value, *outBufferNoteValues); // Set sequencer to start on that bpm
    Timer1.setPeriod(dummy_period_us);
    Timer1.attachInterrupt(output_note_ISR);
    Timer1.resume();
      
    output_note = 0;
  }
}



