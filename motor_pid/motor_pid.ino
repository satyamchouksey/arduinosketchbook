#define ZC 2
#define STATUS_LED 12
#define RUN_LED 11
#define TRIAC 3
#define SPEED A0
#define RPM 5
#include <PID_v1.h>
#define EXT_RUN A1
#define EXT_SPD A2
#include <LiquidCrystal.h>

LiquidCrystal lcd(4, 6, 7, 8, 9, 10);


boolean run = false;
boolean led_status;
//closet to green led: blue red white green
int pulse_length;
int min_pulse;
int max_pulse;
double target_rpm, Input, Output;
PID myPID(&Input, &Output, &target_rpm,0.010,0.001,0,REVERSE);

void setup()
{
  
  target_rpm = 0;
  //turn the PID on
  myPID.SetMode(AUTOMATIC);
    noInterrupts();           // disable all interrupts
  pinMode(RPM,INPUT);
//  digitalWrite(RPM,HIGH);
  pinMode(TRIAC,OUTPUT);
  digitalWrite(TRIAC,LOW);
  pinMode(ZC,INPUT);

  pinMode(RUN_LED,OUTPUT);
  pinMode(STATUS_LED,OUTPUT);
  digitalWrite(STATUS_LED,HIGH);
  Serial.begin(9600);

  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("motor pid");

  //setup for the RPM counter on timer 1
  //clear the control registers
  TCCR1A = 0;
  TCCR1B = 0;
  
  TCCR1B |= (1 << CS12) | ( 1<< CS11 ); //external pin 5 count
  TCCR1B |= (1 << ICNC1 ); //noise canceller

  //setup for the triac control on timer 2
  TCCR2A = 0;
  TCCR2B = 0;
  
  //all this is using timer1
  TCCR2B |= (1 << CS22) | (1<< CS21) | (1<<CS20);    // 1024 prescaler 
  TIMSK2 |= (1 << TOIE2);   // enable timer  2 overflow interrupt

  //want this to vary between 0 and 10ms (100hz)
  //16mhz / 1024 / 100hz = 156
  //156 will be the slowest, 0 the fastest
  //so timer-reset varies from 0 to 156
  max_pulse = 156 + 5; //little longer
  min_pulse = 1;
  pulse_length = max_pulse;
  myPID.SetOutputLimits(min_pulse,max_pulse);

  delay(500);    
  attachInterrupt(0,ZC_INT,FALLING); //set the interrupt handler for the RPM counter
  interrupts();             // enable all interrupts
}

//timer 2 overflow interrupt - used to turn on the triac
ISR(TIMER2_OVF_vect)        
{
 digitalWrite(TRIAC,HIGH);
}

//zc interrupt
void ZC_INT()
{
  digitalWrite(TRIAC,LOW);
  TCNT2 = 256 - pulse_length;   // preload timer
}

double last_print = 0;
void loop()
{
   delay(100);
  //   target_rpm = map(analogRead(SPEED),0,1024,0,30000);
  target_rpm = map(analogRead(EXT_SPD),0,1000,0,30000);
  run = digitalRead(EXT_RUN);
  Input = getRPM();
  if( run )
  {
   digitalWrite(RUN_LED,HIGH); 

   myPID.Compute();
   pulse_length = Output;
   //pulse_length = map(analogRead(SPEED),0,1024,min_pulse,max_pulse);
  }
  else
  {
    //safely off 
   digitalWrite(RUN_LED,LOW);
    pulse_length = max_pulse;
  }
  lcd.setCursor(0,0 );
  lcd.print("rpm: ");
  lcd.print(Input);
  
  lcd.setCursor(0, 1);
  lcd.print("target: ");
  lcd.print(target_rpm);


  
  if( millis() - last_print > 1000)
  {
    last_print = millis();
    Serial.print("out : " );


   Serial.println(Output);
    Serial.print("target : ");
   Serial.println(target_rpm);
    Serial.print("rpm : " );
   Serial.println(Input);


  }
}

//if 50 counts in 100ms, then that's 500 counts in 1s
//500 * 60 = 30000RPM
double last_get = 0;

float getRPM()
{
  double now = millis();  

  int count = TCNT1;
  TCNT1 = 0;
 
  int interval = now - last_get;
  float rpm = (60000 * count / interval );
  
  last_get = now;  
  return rpm;
}
