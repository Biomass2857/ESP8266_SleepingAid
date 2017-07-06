#include <math.h>
#include <Ticker.h>

const int RED = 15;
const int GREEN = 12;
const int BLUE = 13;
const int LDR = A0;
const int BUTTON = 4;
const int FANSPEED = 16;
const int FANDATA = 14;

const int btnjumpignore = 150;
const int shortpress = 800;
const int longpress = 2000;
const int tvok = 2100;

char *cstring = NULL;

bool bpressed = false;
bool breleased = false;
unsigned int timesbpressed = 0;
bool vok = false;
bool configured = false;

unsigned int currentFanState = 0;
unsigned int currentEffect = 0;
unsigned int currentFanDecay = 0;

bool crawlFstates = false;
bool crawlEstates = false;
bool crawlFanDecay = false;
bool fanMoves = false;

unsigned short fanstates[8] = { 8, 15, 20, 40, 60, 80, 0, 0 };
int data[3] = { 0 };

int cr = 0;
int cg = 0;
int cb = 0;

Ticker timer;

class Waiter
{
  public:
    Waiter()
    {
      counter = 0;
    }
  
    void wait(unsigned int ms)
    {
      counter = ms;
    }

    bool isStillWaiting()
    {
      if(counter > 0)
      {
        --counter;
      }

      return counter != 0;
    }

    void clear()
    {
      counter = 0;
    }

    unsigned int getRemainingTime()
    {
      return counter;
    }
  protected:
    unsigned int counter;
};

Waiter leff_waiter;
Waiter crawl_waiter;
Waiter dec_waiter;

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

  if(!leff_waiter.isStillWaiting())
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
    leff_waiter.wait(random(60, 150));
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

  if(!leff_waiter.isStillWaiting())
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
            leff_waiter.wait(250 / (255 / v));
          else
            leff_waiter.wait(500 / (255 / v));
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
             leff_waiter.wait(250 / (255 / v));
           else
             leff_waiter.wait(500 / (255 / v));
           blinkCounter--;
        }
        else
          state2 = (state2 + 1) % 4;
      break;
      case 3:
        if(!bahnhof)
          leff_waiter.wait((500 + random(0, 250)) / v);
         
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
}

void setFan(unsigned short speed)
{
  analogWrite(FANSPEED, 1023 * speed / 100);
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

void timerISR()
{
  static unsigned long int bpwCounter = 0;
  static unsigned int confirmCounter = 0;
  static unsigned long int buttonTriggered = 0, buttonLastSignal = 0;
  static bool isChecking = false;
  static unsigned int menu = 0;
  static bool enter = false;
  static unsigned int cdecay = 0;
  
  bpwCounter++;
  
  /*if(bpressed && bpwCounter > 400)
  {
    bpwCounter = 0;
    bpressed = false;
    selection = (selection + 1) % 2;
    ClearWait();
  }*/

  if(crawlEstates)
  {
    if(!crawl_waiter.isStillWaiting())
    {
      currentEffect = (currentEffect + 1) % 2;
      crawl_waiter.wait(5000);
    }

    switch(currentEffect)
    {
      case 0:
        fire();
      break;
      case 1:
        train();
      break;
    }
  }

  if(crawlFstates)
  {
    if(!crawl_waiter.isStillWaiting())
    {
      currentFanState = (currentFanState + 1) % 8;
      setRGB(0, 255 * fanstates[currentFanState], 0);
      setFan(fanstates[currentFanState]);
      if(currentFanState != 0)
        crawl_waiter.wait(7000);
      else
        crawl_waiter.wait(7000);
    }
    else if(crawl_waiter.getRemainingTime() < 1000)
      setRGB(0, 0, 0);
  }

  if(crawlFanDecay)
  {
    if(!crawl_waiter.isStillWaiting())
    {
      if(currentFanDecay < 100)
      {
        currentFanDecay++;
        fanMoves = true;
      }
      else if(currentFanDecay == 100)
      {
        crawl_waiter.wait(4000);
        fanMoves = false;
        currentFanDecay = 0;
      }
      else
        crawl_waiter.wait(500);
      setFan(100 - currentFanDecay);
    }
  }

  if(configured)
  {
    if(data[1] == 1)
    {
      if(!dec_waiter.isStillWaiting())
      {
        if(cdecay < fanstates[data[1]])
          cdecay++;
        dec_waiter.wait(4);
      }
    }
    setFan(fanstates[data[1]] - cdecay);
    
    switch(data[2])
    {
      case 0:
        fire();
      break;
      case 1:
        train();
      break;
    }
  }

  
  
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
      confirmCounter = 0;
      if(menu != 0)
      {
        switch(menu)
        {
          case 1:
            data[menu - 1] = currentFanState;
          break;
          case 2:
            data[menu - 1] = fanMoves ? 1 : 0;
          break;
          case 3:
            data[menu - 1] = currentEffect;
          break;
        }

        menu = 0;
        crawlFstates = false;
        crawlEstates = false;
        crawlFanDecay = false;
        setFan(0);
        setRGB(0, 0, 0);
        enter = true;
      }
      else
        timesbpressed++;
    }
    else if(offsetTime > shortpress && offsetTime < longpress)
    {
      Serial.println("[Event] reset (longpress)");
      timesbpressed = 0;
      confirmCounter = 0;
      setRGB(0, 0, 0);
      setFan(0);
      menu = 0;
      configured = false;
      crawlFstates = false;
      crawlEstates = false;
      crawlFanDecay = false;

      for(int i = 0; i < sizeof(data) / sizeof(int); i++)
      {
        data[i] = 0;
      }
    }

    buttonLastSignal = 0;
    buttonTriggered = 0;
    bpwCounter = 0;
  }
  
  if(timesbpressed > 0 || enter)
    confirmCounter++;
  
  if(!bpressed && confirmCounter > tvok && (timesbpressed > 0 || enter))
  {
    Serial.println("[Event] Virtual OK");
    confirmCounter = 0;
    enter = false;
    if(menu == 0)
    {
      menu = timesbpressed;
      timesbpressed = 0;
      switch(menu)
      {
         case 1:
          crawlFstates = true;
         break;
         case 2:
          crawlFanDecay = true;
         break;
         case 3:
          crawlEstates = true;
         break;
         case 5:
          configured = true;
          crawlFstates = false;
          crawlEstates = false;
          menu = 0;
          cdecay = 0;
          setRGB(0, 0, 0);
          setFan(0);
         break;
      }
    }
    else
    {
      
    }
  }
}

void setup()
{
  Serial.begin(9600);
  analogWriteFreq(18000);
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
