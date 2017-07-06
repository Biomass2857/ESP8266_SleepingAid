#include <math.h>
#include <Ticker.h>

const int RED = 15;
const int GREEN = 12;
const int BLUE = 13;
const int LDR = A0;
const int BUTTON = 4;
const int FANSPEED = 16;
const int FANDATA = 14;

const int btnjumpignore = 200;
const int shortpress = 600;
const int longpress = 2000;
const int tvok = 2100;

int depth1 = -1;
int depth2 = -1;
int depth3 = -1;

int depth[10];
unsigned int depthIndex = 0;

bool bpressed = false;
bool breleased = false;
unsigned int timesbpressed = 0;
bool vok = false;
bool configured = false;

int cr = 0;
int cg = 0;
int cb = 0;

unsigned int selection = 0;
Ticker timer;

static unsigned int waitCounter = 0;
static bool doIHaveToWait = false;

void setWait(unsigned int ms)
{
  doIHaveToWait = true;
  waitCounter = ms;
}

void doIStillHaveToWait()
{
  if(--waitCounter == 0)
    doIHaveToWait = false;
}

void ClearWait()
{
  waitCounter = 0;
  doIHaveToWait = false;
}

void setRGB(int r, int g, int b)
{
  analogWrite(RED, r * 4);
  analogWrite(GREEN, g * 4);
  analogWrite(BLUE, b * 4);
}

void fire()
{
  const static int ir = 255;
  const static int ig = 80;
  
  const static int dir = 230;
  const static int dig = 30;
  
  static int cr = 0;
  static int cg = 0;
  static int cb = 0;
  
  static int action = 0;

  if(!doIHaveToWait)
  {
    action = random(0, 2);
        
    if(action == 0) // Heller
    {
      cr += random(0, ir - cr);
      cg += random(0, ig - cg);
    }
    else if(action == 1) // dunkler
    {
      cr -= random(0, cr - dir);
      cg -= random(0, cg - dig);
    }
    else if(action == 2)
    {
      cr = random(0, 40);
      cg = random(0, 10);
    }
      
    setRGB(cr, cg, cb);
    setWait(random(60, 150));
  }
  else
  {
    doIStillHaveToWait();
  }
}

void train()
{
  const static int bhfProb = 7;
  const static int pollerFehlerProb = 20;
  static int zyklen = 0;
  static int zyklenBeendet = 0;
  static double grmult = 1, bmult = 0;
  static bool bahnhof = false;
  static int v = 1;
  static int mode = 2;
  static int blinkCounter = 0;
  static int state2 = 0;

  if(!doIHaveToWait)
  {
    switch(state2)
     {
      case 0:
        mode = random(0, bhfProb);
        v = 15; //random(3, 7);
        bahnhof = (mode == 0);
        zyklenBeendet = 0;
        if(bahnhof)
        {
          zyklen = random(60 / v, 120 / v);
          grmult = 1;
          bmult = 0;
        }
        else
        {
          zyklen = random(40 / v, 90 / v);
          grmult = 0.7;
          bmult = 1;
        }
        state2 = (state2 + 1) % 4;
      break;
      case 1:
        if(blinkCounter < (255 / v))
        {
          setRGB(blinkCounter * v * grmult, blinkCounter * v * grmult, blinkCounter * v * bmult);
          
          if(bahnhof)
            setWait(250 / (255 / v));
          else
            setWait(500 / (255 / v));
          blinkCounter++;
        }
        else
          state2 = (state2 + 1) % 4;
      break;
      case 2:
        if(blinkCounter > 0)
        {
           setRGB(blinkCounter * v * grmult, blinkCounter * v * grmult, blinkCounter * v * bmult);
      
           if(bahnhof)
             setWait(250 / (255 / v));
           else
             setWait(500 / (255 / v));
           blinkCounter--;
        }
        else
          state2 = (state2 + 1) % 4;
      break;
      case 3:
        if(!bahnhof)
          setWait((500 + random(0, 250)) / v);
         
        if(zyklenBeendet < zyklen)
        {
          zyklenBeendet++;
          state2 = 1;
        }
        else
          state2 = (state2 + 1) % 4;
      break;
     }
  }
  else
  {
    doIStillHaveToWait();
  }
}

void setFan(int speed)
{
  analogWrite(FANSPEED, ((speed * 68 / 100) + 32) * 1023 / 100);
}

void button_change_isr()
{
  if(!bpressed)
  {
    bpressed = true;
    setRGB(255, 0, 0);
  }
  else
  {
    breleased = true;
    setRGB(0, 0, 0);
  }
}

void button_release_isr()
{
  bpressed = false;
}

void timerISR()
{
  static unsigned long int bpwCounter = 0;
  static unsigned int confirmCounter = 0;
  static unsigned long int buttonTriggered = 0, buttonLastSignal = 0;
  static bool isChecking = false;
  static bool firePreview = false, trainPreview = false;
  
  bpwCounter++;
  
  /*if(bpressed && bpwCounter > 400)
  {
    bpwCounter = 0;
    bpressed = false;
    selection = (selection + 1) % 2;
    ClearWait();
  }*/

  if(firePreview)
  {
    fire();
  }
  else if(trainPreview)
  {
    train();
  }

  if(configured)
  {
    switch(selection)
    {
      case 0:
        train();
      break;
      case 1:
        fire();
      break;
    }
  }
  else
  {
    if(bpressed && !isChecking)
    {
      bpwCounter = 0;
      isChecking = true;
      confirmCounter = 0;
    }
    else if(bpressed && isChecking && !breleased)
    {
      buttonLastSignal = bpwCounter;
      confirmCounter = 0;
    }
    else if(breleased && isChecking)
    {
      isChecking = false;

      bpressed = false;
      breleased = false;
      
      long int offsetTime = buttonLastSignal - buttonTriggered;

      if(offsetTime <= shortpress && offsetTime > btnjumpignore)
      {
        Serial.println("[Event] Shortpress");
        timesbpressed++;
        confirmCounter = 0;
      }
      else if(offsetTime > shortpress && offsetTime < longpress)
      {
        Serial.println("[Event] reset (longpress)");
        timesbpressed = 0;
        confirmCounter = 0;
        setRGB(0, 0, 0);
        setFan(0);
        firePreview = false;
        trainPreview = false;
        depthIndex = 0;
      }

      buttonLastSignal = 0;
      buttonTriggered = 0;
      bpwCounter = 0;
    }
    
    if(timesbpressed > 0)
      confirmCounter++;
    
    if(!bpressed && confirmCounter > tvok && timesbpressed > 0)
    {
      confirmCounter = 0;
      vok = true;
      setRGB(100, 100, 100);
    }

    if(vok)
    {
      vok = false;
      if(depth1 < 0)
      {
        depth1 = timesbpressed;
        timesbpressed = 0;
        switch(depth1)
        {
          case 1:
            setRGB(255, 0, 255);
          break;
          case 2:
            setRGB(0, 255, 0);
          break;
          case 3:
            setRGB(0, 0, 255);
          break;
          default:
            depth1 = -1;
          break;
        }
      }
      else if(depth2 < 0)
      {
        depth2 = timesbpressed;
        timesbpressed = 0;
        switch(depth1)
        {
          case 1:
            switch(depth2)
            {
              case 1:
                setFan(25);
              break;
              case 2:
                setFan(50);
              break;
              case 3:
                setFan(100);
              break;
              default:
                depth2 = -1;
              break;
            }
          break;
          case 2:
            switch(depth2)
            {
              case 1:
                firePreview = true;
              break;
              case 2:
                trainPreview = true;
              break;
              default:
                depth2 = -1;
              break;
            }
          break;
          default:
            setFan(0);
          break;
        }
      }
      else if(depth3 < 0)
      {
        depth3 = timesbpressed;
        timesbpressed = 0;
      }

      if(depth1 >= 0 && depth2 >= 0 && depth3 >= 0)
      {
        switch(depth1)
        {
          case 1:
            if(depth2 >= 0)
            {
              switch(depth2)
              {
                case 0:
                  
                break;
                case 1:

                break;
                default:
                  depth1 = -1;
                  depth2 = -1;
                break;
              }
            }
          break;
          case 2:
            if(depth2 >= 0)
            {
              switch(depth2)
              {
                case 0:
                    
                break;
                case 1:
  
                break;
                default:
                  depth1 = -1;
                  depth2 = -1;
                break;
              }
            }
          break;
          default:
            depth1 = -1;
          break;
        }
      }
    }
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(LDR, INPUT);
  pinMode(BUTTON, INPUT);
  pinMode(FANSPEED, OUTPUT);
  pinMode(FANDATA, INPUT);
  attachInterrupt(BUTTON, button_change_isr, CHANGE);
  timer.attach_ms(1, timerISR);
  setRGB(0, 0, 0);
}

char *cstring = NULL;

void loop()
{
  if(cstring == NULL)
    cstring = (char*) malloc(10 * sizeof(char));
  
  static int counter = 0;
  
  if(Serial.available() > 0)
  {
    char c = Serial.read();
    if(c != 37)
      *(cstring + counter++) = c;
    else
    {
      *(cstring + counter++) = '\0';
      Serial.print("Set to ");
      Serial.print(cstring);
      Serial.println("\% Speed");
      analogWrite(FANSPEED, ((atoi(cstring) * 68 / 100) + 32) * 1023 / 100);
      free(cstring);
      cstring = NULL;
      counter = 0;
    }
  }
}
