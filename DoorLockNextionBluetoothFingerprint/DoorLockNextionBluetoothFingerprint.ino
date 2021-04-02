#include <String.h>
#include <FPS_GT511C3.h> //fingerprint library
#include <SoftwareSerial.h>
#include <Nextion.h> //nextion library
#include <Wire.h> //i2c library
#include <RTClib.h> //RTC library

RTC_DS1307 RTC; // add an object as RTC
String cmd; // for reading the commands from serial port
int wrongCount; //counter for fail attempts
SoftwareSerial nextion(3, 2);// Nextion TX to pin 3 and RX to pin 2 of Arduino for arduino uno or mega
#define buzzPin 6 //for buzzer
#define lockPin 7 //for locker solenoid
Nextion myNextion(nextion, 9600); // for uno or mega use nextion instead Seril1
FPS_GT511C3 fps(4, 5); // fingerprint object to pin 8 and 9
int lastState[2];// for checking minute to update the clock

void setup()
{
  delay(5);
  Serial.begin(9600);
  delay(5);
  pinMode(buzzPin, OUTPUT);
  pinMode(lockPin, OUTPUT);
  digitalWrite(lockPin, LOW);
  myNextion.init(); //start the LCD
  fps.SetLED(true); //start FPS
  Wire.begin(); // start i2c
  RTC.begin(); //start RTC
//  if (! RTC.isrunning()) // if RTC didn't work then it will be setted to system clock
    RTC.adjust(DateTime(__DATE__, __TIME__));
  
  delay(500);
}

void loop()
{

  ///Fingerprint

  if (fps.IsPressFinger()) //have a finger preesed on the finger pad
  {
    fps.CaptureFinger(false); //stop reading finger print, take a picture
    int id = fps.Identify1_N(); // find the id from data base

    if (id < 200) // if exists in database
    {
      wrongCount = 0;//no more fail attempts
      myNextion.sendCommand("page ApprovedPage"); //show the approved page
      fps.SetLED(false);//stop FPS
      digitalWrite(lockPin, HIGH); //open the locker
      digitalWrite(buzzPin, HIGH); //start the buzzer
      delay(1000);
      digitalWrite(buzzPin, LOW); //stop the buzzer
      delay(9000);
      fps.SetLED(true);//start FPS
      digitalWrite(lockPin, LOW);//close the locker
      myNextion.sendCommand("page Home"); //go home
    }
    else //if unable to recognize
    { 
      if (++wrongCount > 5) //check if its more than 5 fail attempts
      {
        myNextion.sendCommand("page DisaprovedPage");// show dissaproved page
        fps.SetLED(false);//stop FPS
        for (int i = 0; i <= 20; i++) // warrn using buzzer
        {
          digitalWrite(buzzPin, HIGH);
          delay(1000);
          digitalWrite(buzzPin, LOW);
          delay(500);
        }
        fps.SetLED(true);//start FPS 
        myNextion.sendCommand("page Home"); //go home
        wrongCount = 0;// DELET AFTER 30 SECONDS
      }
      else
      {
        myNextion.sendCommand("page FingerError"); // no problem, try again
        fps.SetLED(false);//stop FPS 
        digitalWrite(buzzPin, HIGH);
        delay(200);
        digitalWrite(buzzPin, LOW);
         delay(200);
        digitalWrite(buzzPin, HIGH);
        delay(200);
        digitalWrite(buzzPin, LOW);
        delay(1400);
        fps.SetLED(true);//start FPS 
        myNextion.sendCommand("page Home");// go home
      }

    }
  }
  //
  //  // Bluetooth and Nextin LCD
  //
  if (Serial.available()) //there is a signal
  {
    Serial.setTimeout(5);//Faster responce for Serial.readString();
    cmd = Serial.readString();// read the command from serial port

    if (cmd == "Approved!") //Approved!
    {
      wrongCount = 0; //reset
      myNextion.sendCommand("page ApprovedPage");//show the approved page
      fps.SetLED(false);//stop FPS
      digitalWrite(lockPin, HIGH);//open locker
      digitalWrite(buzzPin, HIGH);//start buzzer
      delay(1000);
      digitalWrite(buzzPin, LOW);//stop buzzer
      delay(9000);
      fps.SetLED(true);//start FPS
      digitalWrite(lockPin, LOW);// lock it
      myNextion.sendCommand("page Home");// go home
    }

    if (cmd == "Disapproved!") //Disapproved!
    {
      myNextion.sendCommand("page DisaprovedPage"); // show disapproved page
      fps.SetLED(false);//stop FPS
      for (int i = 0; i <= 20; i++) // warning effect
      {
        digitalWrite(buzzPin, HIGH);
        delay(1000);
        digitalWrite(buzzPin, LOW);
        delay(500);
      }
      wrongCount = 0;// DELET AFTER 30 SECONDS
      fps.SetLED(true);//start fps
      myNextion.sendCommand("page Home");// go home
    }
  }

  //RTC Clock
  updateClock();

}

void updateClock()
{
  DateTime now = RTC.now(); //read the time
  lastState[0] = now.minute(); 
  if (lastState[0] != lastState[1])// check if it's next minute
  {
    if (now.hour() > 12) // make it am, pm
    {
      myNextion.setComponentText("Home.txtClockH", " " + String(now.hour() - 12)); // set the hour
      myNextion.setComponentText("Home.savedH", " " + String(now.hour() - 12));// to make it stable i saved it toanother varible in the LCD
    }
    else
    {
      myNextion.setComponentText("Home.txtClockH", String(now.hour()));
      myNextion.setComponentText("Home.savedH", String(now.hour()));
    }
    if (now.minute() < 10)
    {
      myNextion.setComponentText("Home.txtClockM", "0" + String(now.minute()));// set the minute
      myNextion.setComponentText("Home.savedM", "0" + String(now.minute()));// to make it stable i saved it toanother varible in the LCD
    }
    else
    {
      myNextion.setComponentText("Home.txtClockM", String(now.minute()));
      myNextion.setComponentText("Home.savedM", String(now.minute()));
    }

    myNextion.setComponentText("Home.txtDate", dateStringFormat(now.year(), now.month(), now.day()));// set the date
    myNextion.setComponentText("Home.savedDate",  dateStringFormat(now.year(), now.month(), now.day()));// to make it stable i saved it toanother varible in the LCD
    lastState[1] = lastState[0]; // save the minute fo next time
  }
}

String dateStringFormat(int year, int month, int day)
{

  int m = month;         // Month Entry
  int d = day;          // Day Entry
  int yy;         // Last 2 digits of the year (ie 2016 would be 16)
  int yyyy = year;     // Year Entry
  int c;          // Century (ie 2016 would be 20)
  int mTable;     // Month value based on calculation table
  int SummedDate; // Add values combined in prep for Mod7 calc
  int DoW;        // Day of the week value (0-6)
  int leap;       // Leap Year or not
  int cTable;     // Century value based on calculation table
  String dateFormated;
  // Leap Year Calculation
  if ((fmod(yyyy, 4) == 0 && fmod(yyyy, 100) != 0) || (fmod(yyyy, 400) == 0))
    leap = 1;
  else
    leap = 0;

  while (yyyy > 2299)
    yyyy = yyyy - 400;
  while (yyyy < 1900)
    yyyy = yyyy + 400;

  c = yyyy / 100;

  yy = fmod(yyyy, 100);

  if (c == 19)
    cTable = 1;
  if (c == 20)
    cTable = 0;
  if (c == 21)
    cTable = 5;
  if (c == 22)
    cTable = 3;

  if (m == 1)
  {
    if (leap == 1)
      mTable = 6;
    else
      mTable = 0;
  }
  if (m == 2)
  {
    if (leap == 1)
      mTable = 2;
    else
      mTable = 3;
  }

  if (m == 10)
    mTable = 0;
  if (m == 8)
    mTable = 2;
  if (m == 3 || m == 11)
    mTable = 3;
  if (m == 4 || m == 7)
    mTable = 6;
  if (m == 5)
    mTable = 1;
  if (m == 6)
    mTable = 4;
  if (m == 9 || m == 12)
    mTable = 5;

  SummedDate = d + mTable + yy + (yy / 4) + cTable;
  DoW = fmod(SummedDate, 7);

  switch (DoW) // week days
  {
    case 0:
      dateFormated = "Saturday,";
      break;
    case 1:
      dateFormated = "Sunday,";
      break;
    case 2:
      dateFormated = "Monday,";
      break;
    case 3:
      dateFormated = "Tuesday,";
      break;
    case 4:
      dateFormated = "Wednesday,";
      break;
    case 5:
      dateFormated = "Thursday,";
      break;
    case 6:
      dateFormated = "Friday,";
      break;
    default:
      break;
  }

  switch (month) // month names
  {
    case 1:
      dateFormated += "Jan. " + String(day);
      break;
    case 2:
      dateFormated += "Feb. " + String(day);
      break;
    case 3:
      dateFormated += "Mar. " + String(day);
      break;
    case 4:
      dateFormated += "Apr. " + String(day);
      break;
    case 5:
      dateFormated += "May " + String(day);
      break;
    case 6:
      dateFormated += "Jun. " + String(day);
      break;
    case 7:
      dateFormated += "Jul. " + String(day);
      break;
    case 8:
      dateFormated += "Aug. " + String(day);
      break;
    case 9:
      dateFormated += "Sept. " + String(day);
      break;
    case 10:
      dateFormated += "Oct. " + String(day);
      break;
    case 11:
      dateFormated += "Nov. " + String(day);
      break;
    case 12:
      dateFormated += "Dec. " + String(day);
      break;
    default:
      break;
  }
  return dateFormated; // retrn the date.
}


