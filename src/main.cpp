#include <Arduino.h>
#include <math.h>
#include <ESP32Servo.h>
#include <stdlib.h>

#include <WiFi.h>
#include <WebServer.h>

using namespace std;

//////////////////////////////////////////////////////////////
const char* ssid = "spider_bot";
const char* password = "disabledmakri";

WebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Robot Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { 
      text-align: center; 
      font-family: Arial, sans-serif; 
      background-color: #f4f4f9; 
      margin-top: 50px;
    }
    .d-pad { 
      display: grid; 
      grid-template-columns: repeat(3, 80px); 
      grid-gap: 15px; 
      justify-content: center; 
      margin-top: 30px;
    }
    .action-pad {
      margin-top: 20px;
    }
    .btn { 
      background-color: #007BFF; 
      border: none; 
      color: white; 
      padding: 20px 0; 
      font-size: 16px; 
      font-weight: bold;
      border-radius: 10px; 
      cursor: pointer; 
      user-select: none;
      touch-action: manipulation;
      transition: background-color 0.1s;
    }
    .btn:active, .btn.active-key { background-color: #0056b3; }
    .btn-stop { background-color: #dc3545; }
    .btn-stop:active, .btn-stop.active-key { background-color: #a71d2a; }
    .btn-action { padding: 15px 40px; background-color: #28a745; margin: 0 5px; }
    .btn-action:active, .btn-action.active-key { background-color: #1e7e34; }
    .empty { background: transparent; }
    .slider-container { margin-top: 30px; }
    .slider { width: 80%; max-width: 300px; cursor: pointer; }
    .speed-label { font-size: 18px; font-weight: bold; color: #333; }
  </style>
  <script>
    function sendCommand(cmd) {
      fetch('/' + cmd);
    }

    function updateSpeed(val) {
      document.getElementById('speed-display').innerText = val;
      fetch('/speed?v=' + val);
    }

    // Listen for key presses
    document.addEventListener('keydown', function(event) {
      if (event.repeat) return; // Prevent multiple triggers if key is held down
      
      const key = event.key.toLowerCase();
      if (key === 'w') {
        sendCommand('forward');
        document.getElementById('btn-fwd').classList.add('active-key');
      } else if (key === 'a') {
        sendCommand('left');
        document.getElementById('btn-left').classList.add('active-key');
      } else if (key === 's') {
        sendCommand('backward');
        document.getElementById('btn-rev').classList.add('active-key');
      } else if (key === 'd') {
        sendCommand('right');
        document.getElementById('btn-right').classList.add('active-key');
      } else if (key === 'c') {
        sendCommand('crouch');
        document.getElementById('btn-crouch').classList.add('active-key');
      } else if (key === 'v') {
        sendCommand('wave');
        document.getElementById('btn-wave').classList.add('active-key');
      }
    });

    // Listen for key releases to stop the motors or reset button state
    document.addEventListener('keyup', function(event) {
      const key = event.key.toLowerCase();
      
      // Stop command is only sent for movement keys
      if (['w', 'a', 's', 'd'].includes(key)) {
        sendCommand('stop');
      }
      
      if (['w', 'a', 's', 'd', 'c', 'v'].includes(key)) {
        document.getElementById('btn-fwd').classList.remove('active-key');
        document.getElementById('btn-left').classList.remove('active-key');
        document.getElementById('btn-rev').classList.remove('active-key');
        document.getElementById('btn-right').classList.remove('active-key');
        document.getElementById('btn-crouch').classList.remove('active-key');
        document.getElementById('btn-wave').classList.remove('active-key');
      }
    });
  </script>
</head>
<body>
  <h2>ESP32 Controller</h2>
  <p>Use W, A, S, D, C, V keys or click the buttons.</p>
  <div class="d-pad">
    <div class="empty"></div>
    <button id="btn-fwd" class="btn" 
            onmousedown="sendCommand('forward')" onmouseup="sendCommand('stop')" 
            ontouchstart="sendCommand('forward')" ontouchend="sendCommand('stop')">FWD</button>
    <div class="empty"></div>
    
    <button id="btn-left" class="btn" 
            onmousedown="sendCommand('left')" onmouseup="sendCommand('stop')" 
            ontouchstart="sendCommand('left')" ontouchend="sendCommand('stop')">LEFT</button>
    <button id="btn-stop" class="btn btn-stop" onclick="sendCommand('stop')">STOP</button>
    <button id="btn-right" class="btn" 
            onmousedown="sendCommand('right')" onmouseup="sendCommand('stop')" 
            ontouchstart="sendCommand('right')" ontouchend="sendCommand('stop')">RIGHT</button>
            
    <div class="empty"></div>
    <button id="btn-rev" class="btn" 
            onmousedown="sendCommand('backward')" onmouseup="sendCommand('stop')" 
            ontouchstart="sendCommand('backward')" ontouchend="sendCommand('stop')">REV</button>
    <div class="empty"></div>
  </div>

  <div class="action-pad">
    <button id="btn-crouch" class="btn btn-action" onclick="sendCommand('crouch')">Dance</button>
    <button id="btn-wave" class="btn btn-action" onclick="sendCommand('wave')">WAVE</button>
  </div>

  <div class="slider-container">
    <label class="speed-label">Speed: <span id="speed-display">7</span></label><br><br>
    <input type="range" min="1" max="40" value="7" class="slider" oninput="updateSpeed(this.value)">
  </div>

</body>
</html>
)rawliteral";

//////////////////////////////////////////////////////////////

struct Node{
  Node* next;
  int arm;
  float x,y,z;
  bool relative;
};
typedef struct Node Node;

class Arm{
  private:
    static constexpr float l1=80,l2=110;
    const float e=1;

    float x0,y0,z0;
    float x2,y2,z2;

    float tempx,tempy,tempz;
    float distance=10;
    
    int servopins[3]={};
    bool right=true;

  public:
    float o1,o2,o3;
    float x1,y1,z1;
    Servo servos[3];
    bool moving;
    float speed=0.07;
    short arm;

    Arm(){
      x1=x2=tempx=y1=y2=tempy=z1=z2=tempz=0.0;
    }

    Arm(int servopins[3],float x0,float y0,float z0,bool right,short arm){
      for(int i=0;i<3;i++){
        this->servopins[i]=servopins[i];
      }

      moving=false;

      x1=x2=tempx=y1=y2=tempy=z1=z2=tempz=0.0;

      this->x0=x0;
      this->y0=y0;
      this->z0=z0;

      this->right=right;
      this->arm=arm;

      calculateAngles(0,0,0);
    }

    void move(float x,float y,float z){
      x2=x;
      y2=y;
      z2=z;

      if(max(fabs(x2-x1),max(fabs(y2-y1),fabs(z2-z1))) > e) moving=true;
      distance = sqrt(pow(x2-tempx,2)+pow(y2-tempy,2)+pow(z2-tempz,2));
    }

    void calculateAngles(float x,float y,float z){
      x+=x0;
      y+=y0;
      z+=z0;

      float h = sqrt(x*x+y*y);
      float l = sqrt(h*h+z*z);

      o3 = (5.0*PI/4.0)-(acos(constrain((l1*l1 + l2*l2 - l*l)/(2*l1*l2),-1.0,1.0)));
      o2 = PI/2.0 + (acos(constrain((l*l + l1*l1 - l2*l2)/(2*l*l1),-1.0,1.0)) + atan2(z,h));
      o1 = PI/2.0 + (atan2(x,y) * (right?1:-1));
    }

    void interpolate(){
      if(max(fabs(x2-x1),max(fabs(y2-y1),fabs(z2-z1))) < e){
        tempx=x1;
        tempy=y1;
        tempz=z1;

        moving=false;
        return;
      }

      x1+=(x2-tempx)/distance * speed;
      y1+=(y2-tempy)/distance * speed;
      z1+=(z2-tempz)/distance * speed;

      calculateAngles(x1,y1,z1);
      
      moving=true;

      writeServos();
    }

    void writeServos(){
      servos[0].write(o1*180/PI);
      servos[1].write(o2*180/PI);
      servos[2].write(o3*180/PI);
    }

};

////////////////////////////////////////////////////////////////

void addToQueue(int,int,float,float,float,bool);
void goThroughQueue(int);
void serialInput();
void updateState();
void stateLogic();
void startWalkingF();
void startWalkingB();
void handleCommands();
bool queuesEmpty();

Node* heads[4] = {nullptr};
Node* tails[4] = {nullptr};

int svps[4][3]={{14,27,26},
                {25,33,32},
                {4,16,17},
                {18,19,21}};


Arm arms[4]={Arm(svps[0],40,30,-150,true,0),
             Arm(svps[1],-40,30,-150,true,1),
             Arm(svps[2],40,30,-150,false,2),
             Arm(svps[3],-40,30,-150,false,3)};

enum BOT_STATES{IDLE,WALKINGF,WALKINGB,RIGHT,LEFT,CROUCH,WAVING};
BOT_STATES State=IDLE;

long long int now=0;
float arr[5]={};
bool sign[5]={};
int z=0;

////////////////////////////////////////////////////////////////

void setup(){
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for(int i=0;i<4;i++){
    for(int j=0;j<3;j++){
      arms[i].servos[j].setPeriodHertz(50);

      arms[i].servos[j].attach(svps[i][j],500,2400);
    }
    arms[i].writeServos();
  }
  delay(1000);

  Serial.begin(9600);

  WiFi.softAP(ssid,password);
  delay(1000);
  IPAddress myIP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  Serial.println(myIP);

  handleCommands();
  server.begin();
}

void loop(){
  server.handleClient();

  for(int i=0;i<4;i++){
    goThroughQueue(i);
  }

  for(int i=0;i<4;i++){
    arms[i].interpolate();
  }
  
  // if(millis()-now>=50){
  //   for(int i=0;i<4;i++){
  //     Serial.print(arms[i].x1);
  //     Serial.print(" ");
  //     Serial.print(arms[i].o1 *180/PI);
  //     Serial.print(" ");
  //     // Serial.print(arms[i].y1);
  //     // Serial.print(" ");
  //     // Serial.print(arms[i].z1);
  //     // Serial.print(" ");
  //   }
  //   Serial.println();

  //   now=millis();
  // }

  // updateState();
  stateLogic();

  // serialInput();

}

///////////////////////////////////////////////////////////////////////

void addToQueue(int queue,int arm,float x,float y,float z,bool relative=false){
  Node* node = (Node*) malloc(sizeof(Node));

  node->arm=arm;
  node->x=x;
  node->y=y;
  node->z=z;
  node->next=nullptr;
  node->relative=relative;

  if(heads[queue]==nullptr){
    heads[queue]=tails[queue]=node;
    return;
  }

  tails[queue]->next=node;
  tails[queue]=node;
}

bool execute[4]={1,1,1,1};
void goThroughQueue(int queue){
  if(heads[queue]==nullptr) return;

  if(execute[queue]){
    int armn=heads[queue]->arm;

    if(heads[queue]->relative)
      arms[armn].move(heads[queue]->x+arms[armn].x1,heads[queue]->y+arms[armn].y1,heads[queue]->z+arms[armn].z1);
    else
      arms[heads[queue]->arm].move(heads[queue]->x,heads[queue]->y,heads[queue]->z);

    execute[queue]=false;
  }

  if(!arms[heads[queue]->arm].moving){
    Node *temp = heads[queue];

    heads[queue]=heads[queue]->next;
    free(temp);

    execute[queue]=true;
  }
}

void serialInput(){
  while(Serial.available()){
    int val=Serial.read();

    if(val==(int)' '){
      z++;
      continue;
    }

    if(val==(int)'-'){
      sign[z]=true;
      continue;
    }

    if(val==10 || val==(int)'n'){
      for(int j=0;j<5;j++){
        arr[j]*= sign[j]?-1:1;
      }
      addToQueue((int)arr[0],(int)arr[1],arr[2],arr[3],arr[4]);

      for(int j=0;j<5;j++){
        arr[j]=0;
        sign[j]=false;
      }

      z=0;
      break;
    }

    arr[z]=(arr[z]*10)+val-48;
  }
}

void updateState(){
  // if(State==IDLE) startWalking();
}

short cvs[6]={0,0,0,0,0};
void stateLogic(){
  if(State==WALKINGF){
    if(queuesEmpty()){
      if(cvs[4]==0 || cvs[4]==2){
        // Serial.println("PULLBACK");
        addToQueue(0,0,40,0,0,true);
        addToQueue(1,1,40,0,0,true);
        addToQueue(2,2,40,0,0,true);
        addToQueue(3,3,40,0,0,true);
      }
      else if(cvs[4]==1){
        // Serial.println("LEFT");
        addToQueue(0,2,-20,0,40);
        addToQueue(0,2,-40,0,0);

        addToQueue(0,3,-20,0,40);
        addToQueue(0,3,-40,0,0);
      }
      else if(cvs[4]==3){
        // Serial.println("RIGHT");
        addToQueue(0,0,-20,0,40);
        addToQueue(0,0,-40,0,0);

        addToQueue(0,1,-20,0,40);
        addToQueue(0,1,-40,0,0);
      }

      cvs[4]++;
      if(cvs[4]==4) cvs[4]=0;
    }
  }
  else if(State==WALKINGB){
    if(queuesEmpty()){
      if(cvs[0]==0 || cvs[0]==2){
        // Serial.println("PULLBACK");
        addToQueue(0,0,-40,0,0,true);
        addToQueue(1,1,-40,0,0,true);
        addToQueue(2,2,-40,0,0,true);
        addToQueue(3,3,-40,0,0,true);
      }
      else if(cvs[0]==1){
        // Serial.println("LEFT");
        addToQueue(0,2,20,0,40);
        addToQueue(0,2,40,0,0);

        addToQueue(0,3,20,0,40);
        addToQueue(0,3,40,0,0);
      }
      else if(cvs[0]==3){
        // Serial.println("RIGHT");
        addToQueue(0,0,20,0,40);
        addToQueue(0,0,40,0,0);

        addToQueue(0,1,20,0,40);
        addToQueue(0,1,40,0,0);
      }

      cvs[0]++;
      if(cvs[0]==4) cvs[0]=0;
    }
  }
  else if(State==LEFT){
    if(queuesEmpty()){
      if(cvs[1]==0){
        // Serial.println("PULLBACK");
        addToQueue(0,0,20,0,5);
        addToQueue(0,0,40,0,0);

        addToQueue(0,1,20,0,5);
        addToQueue(0,1,40,0,0);

        addToQueue(0,2,-20,0,5);
        addToQueue(0,2,-40,0,0);

        addToQueue(0,3,-20,0,5);
        addToQueue(0,3,-40,0,0);
      }
      else if(cvs[1]==1){
        // Serial.println("LEFT");
        addToQueue(0,0,-40,0,0,true);
        addToQueue(1,1,-40,0,0,true);
        addToQueue(2,2,40,0,0,true);
        addToQueue(3,3,40,0,0,true);
      }
      cvs[1]=1-cvs[1];
    }
  }
  else if(State==RIGHT){
    if(queuesEmpty()){
      if(cvs[2]==0){
        // Serial.println("PULLBACK");
        addToQueue(0,0,-20,0,5);
        addToQueue(0,0,-40,0,0);

        addToQueue(0,1,-20,0,5);
        addToQueue(0,1,-40,0,0);

        addToQueue(0,2,20,0,5);
        addToQueue(0,2,40,0,0);

        addToQueue(0,3,20,0,5);
        addToQueue(0,3,40,0,0);
      }
      else if(cvs[2]==1){
        // Serial.println("LEFT");
        addToQueue(0,0,40,0,0,true);
        addToQueue(1,1,40,0,0,true);
        addToQueue(2,2,-40,0,0,true);
        addToQueue(3,3,-40,0,0,true);
      }
      cvs[2]=1-cvs[2];
    }
  }
  else if(State==CROUCH){
    if(queuesEmpty()){
      addToQueue(0,0,0,0,cvs[3]?0:30);
      addToQueue(1,1,0,0,cvs[3]?0:30);
      addToQueue(2,2,0,0,cvs[3]?0:30);
      addToQueue(3,3,0,0,cvs[3]?0:30);
      cvs[3]=1-cvs[3];
    }
  }
  else if(State==IDLE){
    if(queuesEmpty()){
      addToQueue(0,0,0,0,0);
      addToQueue(1,1,0,0,0);
      addToQueue(2,2,0,0,0);
      addToQueue(3,3,0,0,0);
    }
  }
  else if(State==WAVING){
    if(queuesEmpty()){
      addToQueue(0,2,-10,0,5);
      addToQueue(0,2,-40,0,0);
      addToQueue(0,1,0,-10,5);
      addToQueue(0,1,0,-30,0);
      
      addToQueue(0,3,0,0,350);
      addToQueue(0,3,0,20,350);
      addToQueue(0,3,0,-20,350);
      addToQueue(0,3,0,20,350);
      addToQueue(0,3,0,-20,350);
      addToQueue(0,3,0,20,350);
      addToQueue(0,3,0,-20,350);
      addToQueue(0,3,0,0,0);

      State=IDLE;
    }
  }
}

void startWalkingF(){
  addToQueue(0,0,-5,0,5);
  addToQueue(0,0,-10,0,0);

  addToQueue(0,1,-5,0,5);
  addToQueue(0,1,-10,0,0);

  State=WALKINGF;
}

void startWalkingB(){
  addToQueue(0,0,5,0,5);
  addToQueue(0,0,10,0,0);

  addToQueue(0,1,5,0,5);
  addToQueue(0,1,10,0,0);

  State=WALKINGB;
}

bool queuesEmpty(){
  for(int i=0;i<4;i++){
    if(heads[i]!=nullptr) return false;
  }
  return true;
}

void handleCommands(){
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });

  server.on("/forward", []() {
    startWalkingF();
    server.send(200, "text/plain", "OK");
  });

  server.on("/backward", []() {
    startWalkingB();
    server.send(200, "text/plain", "OK");
  });

  server.on("/left", []() {
    cvs[1]=0;
    State=LEFT;
    server.send(200, "text/plain", "OK");
  });

  server.on("/right", []() {
    cvs[2]=0;
    State=RIGHT;
    server.send(200, "text/plain", "OK");
  });

  server.on("/crouch", []() {
    State=CROUCH;
    server.send(200, "text/plain", "OK");
  });

  server.on("/wave", []() {
    State=WAVING;
    server.send(200, "text/plain", "OK");
  });

  server.on("/stop", []() {
    State=IDLE;
    server.send(200, "text/plain", "OK");
  });

  server.on("/speed", []() {
    if(server.hasArg("v")){
      String s = server.arg("v");

      float newSpeed= s.toFloat()/100.0;

      for(int i=0;i<4;i++){
        arms[i].speed = newSpeed;
      }
    }

    server.send(200, "text/plain", "OK");
  });
}