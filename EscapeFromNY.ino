#include <EEPROM.h>
#include "DigitLedDisplay.h"

//how long half a standard clock tick is (in milliseconds)
#define HALF_TICK_LENGTH 500

//accelerated clock ticks when button is pressed (in milliseconds)
#define RAPID_TICK_LENGTH 4

//time to display blinking zeros before resetting (in milliseconds)
#define RESET_TIME 10000

DigitLedDisplay ld = DigitLedDisplay(14, 10, 15);

int hours = 1;
int minutes = 0;
int seconds = 0;

int startup_selection = 0;

void setup() {

  //cycle through various startup time values
  int value = EEPROM.read(0);
  startup_selection = (value % 4);
  value++;
  EEPROM.write(0, value);

  //load the timer 
  load_timer();

  //set display brightness (0 to 15)
  ld.setBright(15);
  //total of 8 digits, indexed from 1
  ld.setDigitLimit(8);
  //debug output
  Serial.begin(115200);
  //accelerated clock pin
  pinMode(9, INPUT_PULLUP);
}

int seconds_left_digit = 0;
int seconds_right_digit = 0;
int minutes_left_digit = 0;
int minutes_right_digit = 0;
int hours_left_digit = 0;
int hours_right_digit = 0;

boolean half_tick = true;
boolean full_tick = true;

unsigned long int tick_length = 0;
unsigned long int reset_timer = 0;

void load_timer() {
  switch (startup_selection) {
    default:
    case 0:      hours = 1;      minutes = 0;      seconds = 0;      break;
    case 1:      hours = 2;      minutes = 0;      seconds = 0;      break;
    case 2:      hours = 3;      minutes = 0;      seconds = 0;      break;
    case 3:      hours = 4;      minutes = 0;      seconds = 0;      break;
  }
}

void loop() {

  //Switch to rapid ticks if pin 9 is pressed
  if (digitalRead(9)) {
    tick_length = HALF_TICK_LENGTH;
  } else {
    tick_length = RAPID_TICK_LENGTH;
  }

  //keep all ticks synced to this clock for animation consistency
  static unsigned long int half_clock_tick_time = 0;
  if (millis() - half_clock_tick_time > tick_length) {
    half_clock_tick_time += tick_length;
    half_tick = !half_tick;
    if (half_tick) full_tick = true;  //every other half tick is a full tick
  }

  if (full_tick) {
    full_tick = false;

    //if we have time left anywhere, subtract a second
    if ((seconds + hours + minutes ) > 0) seconds--;

    //borrow from minutes if needed
    if (seconds < 0) {
      seconds = 59;
      minutes--;
    }

    //borrow from hours if needed
    if (minutes < 0) {
      minutes = 59;
      hours--;
    }

    //Serial debug output
    Serial.print("H: ");
    Serial.print(hours);
    Serial.print(" M: ");
    Serial.print(minutes);
    Serial.print(" S: ");
    Serial.print(seconds);
    Serial.println(" ");

    //calculate displayed values
    seconds_left_digit = seconds / 10;
    seconds_right_digit = seconds % 10;
    minutes_left_digit = minutes / 10;
    minutes_right_digit = minutes % 10;
    hours_left_digit = hours / 10;
    hours_right_digit = hours % 10;
  }

  //if we have time anywhere, we are doing a normal countdown
  if ((seconds + hours + minutes ) > 0) {
    
    send_digit(1, seconds_right_digit);
    send_digit(2, seconds_left_digit);
    send_digit(4, minutes_right_digit);
    send_digit(5, minutes_left_digit);
    send_digit(7, hours_right_digit);
    send_digit(8, hours_left_digit);

    //blink the symbols between digits
    if (half_tick) { 
      send_digit(3, '-');
      send_digit(6, '-');
    } else {
      send_digit(3, ' ');
      send_digit(6, ' ');
    }

    //reset deadmans switch
    reset_timer = millis(); 
    
  } else { //timer has ran out, blink zeros
    
    if (half_tick) {
      send_digit(1, seconds_right_digit);
      send_digit(2, seconds_left_digit);
      send_digit(4, minutes_right_digit);
      send_digit(5, minutes_left_digit);
      send_digit(7, hours_right_digit);
      send_digit(8, hours_left_digit);
    } else {
      send_digit(1, ' ');
      send_digit(2, ' ');
      send_digit(4, ' ');
      send_digit(5, ' ');
      send_digit(7, ' ');
      send_digit(8, ' ');
    }
    //fixed symbols between digits
    send_digit(3, '-');
    send_digit(6, '-');

    //eventually reset the timer
    if (millis() - reset_timer > RESET_TIME) load_timer();
  }
}

void send_digit(int location, char input) {
  send_digit(location,  input, false);
}

void send_digit(int location, char input, bool decimal) {

  byte output = B00000000;

  // B00000001 middle
  // B00000010 upper left
  // B00000100 lower left
  // B00001000 bottom
  // B00010000 lower right
  // B00100000 upper right
  // B01000000 top
  // B10000000 decimal

  switch (input) {

    case 0:    case '0':      output = B01111110;      break;
    case 1:    case '1':      output = B00110000;      break;
    case 2:    case '2':      output = B01101101;      break;
    case 3:    case '3':      output = B01111001;      break;
    case 4:    case '4':      output = B00110011;      break;
    case 5:    case '5':      output = B01011011;      break;
    case 6:    case '6':      output = B01011111;      break;
    case 7:    case '7':      output = B01110000;      break;
    case 8:    case '8':      output = B01111111;      break;
    case 9:    case '9':      output = B01111011;      break;

    case 'o':      output = B00011101;      break;
    case '-':      output = B00000001;      break;
    case '_':      output = B00001000;      break;
    case '=':      output = B01001000;      break;
    case 'E':      output = B01001111;      break;

  case ' ':   default:      output = B00000000;      break;
  }

  //add a decimal point on if requested
  if (decimal) output |= B10000000;

  ld.write(location, output);

}