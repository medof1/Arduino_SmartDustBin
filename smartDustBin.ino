//Ultrasonic
int echo[4] = {4, 2, 6, 8}; // 0=pintu, 1= kering, 2= Basah, 3= Metal
int trig[4] = {5, 2, 7, 9};
long duration[4] = {0, 0, 0, 0};
long cm[4] = {0, 0, 0, 0};

//Servo
#include <Servo.h>
Servo myservo0;
Servo myservo1;
Servo myservo2;
int pos[3] = {0, 90, 90}; // 0= Pintu, 1= Servo Atas, 2= Servo Bawah
int serv[3] = {0, 0, 0};
int deg[3] = {0, 90, 90};

//Proximity
int prox[2]; // 0= Inductive, 1= Capacitive

//Photo
int photo = 0;

//Harvesting
int pin[2] = {A5, A4};
int switching = 0;
int val = 0;

//Common
int a = 0; // memilih ultrasonik dan servo
int b = 0; // pembacaan ultrasonik pintu
int c = 1; // pembacaan ultrasonik kapasitas sampah
int jarak = 20; // jarak buka pintu
int safety = 0;
int seq = 0; // sequence
int seq1 = 11;
const long Interval[4] = {5000, 3000, 1000, 2}; // 0= Kapasitas, 1= Pintu, 2= Servo, 3= Servo Step
unsigned long PrevMillis[3] = {0, 0, 0}; // 0= Kapasitas, 1= Servo, 2= Servo Step

void setup() {
  Serial.begin(115200);

  //Ultrasonic
  for (a = 0; a <= 3; a++) { // EchoTrig 4 Ultrasonic
    pinMode(trig[a], OUTPUT);
    pinMode(echo[a], INPUT);
  }
  
  //Kapasitas awal Sampah
  for (a = 1; a <= 3; a++) {
    ultrasonic();
  }

  //Servo
  myservo0.attach(10); //  Pintu
  myservo1.attach(12); // Servo Atas
  myservo2.attach(11); // Servo Bawah
  myservo0.write(deg[0]);
  myservo1.write(deg[1]);
  myservo2.write(deg[2]);

  //Proximity
  pinMode(A1, INPUT); // Inductive
  pinMode(A2, INPUT); // Capacitive

  //Harvessting
  batre();
}

void loop() {
  harvesting();
  sequence();
  prin();
}

void harvesting() {
  val = analogRead(A0) ;
  if (val >= 1000 ) { //4.8v
    solar();
  }
  if (val <= 703 ) { // 3.4V
    batre();
  }
}

void batre() {
  digitalWrite(pin[1], HIGH);
  digitalWrite(pin[0], LOW);
  switching = 0;
}

void solar() {
  digitalWrite(pin[0], HIGH);
  digitalWrite(pin[1], LOW);
  switching = 1;
}

void sequence() {

  // Setiap 10s membaca kapasitas tong sampah
  if (millis() - PrevMillis[0] >= Interval[0] and seq == 0) {
    PrevMillis[0] = millis();
    a = c;
    ultrasonic();
    c = c + 1;
    if (c >= 4) {
      c = 1;
    }
  }

  //Safety buka pintu
  if (millis() - PrevMillis[1] >= 100 and safety < 10 and (seq == 0 or seq == 1)) {
    PrevMillis[1] = millis();
    a = 0;
    ultrasonic();
    if (seq == 0) {
      if (cm[0] <= jarak) {
        safety = safety + 1;
      } else {
        safety = 0;
      }
    }
    if (seq == 1) {
      if (cm[0] > jarak) {
        safety = safety + 1;
      } else {
        safety = 0;
      }
    }
  }
  //safety = 15;

  //Sequence 0 ( Buka Pintu )
  if (seq == 0 and millis() - PrevMillis[1] >= Interval[2] and safety >= 10) {
    pos[0] = 90;
    if (millis() - PrevMillis[2] >= Interval[3]) {
      PrevMillis[2] = millis();
      if (pos[0] > deg[0]) {
        deg[0] = deg[0] + 1 ;
        servo0();
      } else {
        seq = 1;
        safety = 0;
        PrevMillis[1] = millis();
      }
    }
  }

  //Sequence 1 ( Tutup Pintu )
  if (seq == 1 and millis() - PrevMillis[1] >= Interval[1] and safety >= 10) {
    pos[0] = 0;
    if (millis() - PrevMillis[2] >= Interval[3]) {
      PrevMillis[2] = millis();
      if (pos[0] < deg[0]) {
        deg[0] = deg[0] - 1;
        servo0();
      } else {
        seq = 2;
        safety = 0;
        proximity();
        PrevMillis[1] = millis();
      }
    }
  }

  //prox[0] = 100;
  //Sequence 2 ( Deteksi Sampah Metal )
  if (seq == 2 and millis() - PrevMillis[1] >= Interval[2]) {
    // If sampah = Metal
    if (prox[0] <= 700) {
      pos[1] = 0;
      if (millis() - PrevMillis[2] >= Interval[3]) {
        PrevMillis[2] = millis();
        if (pos[1] < deg[1]) {
          deg[1] = deg[1] - 1;
          servo1();
        } else {
          seq = 3;
          PrevMillis[1] = millis();
        }
      }
    }
    // If sampah Bukan Metal
    if (prox[0] > 700) {
      pos[1] = 180;
      if (millis() - PrevMillis[2] >= Interval[3]) {
        PrevMillis[2] = millis();
        if (pos[1] > deg[1]) {
          deg[1] = deg[1] + 1;
          servo1();
        } else {
          seq = 4;
          PrevMillis[1] = millis();
        }
      }
    }
  }

  //Sequence 3 ( Return Servo 1 dan kembali ke seq 0 )
  if (seq == 3 and millis() - PrevMillis[1] >= Interval[2]) {
    pos[1] = 90;
    if (millis() - PrevMillis[2] >= Interval[3]) {
      PrevMillis[2] = millis();
      if (pos[1] > deg[1]) {
        deg[1] = deg[1] + 1;
        servo1();
      } else {
        seq = 0;
        PrevMillis[1] = millis();
      }
    }
  }

  //Sequence 4 ( Deteksi Sampah Non Metal )
  if (seq == 4 and millis() - PrevMillis[1] >= Interval[2]) {
    pos[1] = 90;
    if (millis() - PrevMillis[2] >= Interval[3]) {
      PrevMillis[2] = millis();
      if (pos[1] < deg[1]) {
        deg[1] = deg[1] - 1;
        servo1();
      } else {
        // If sampah Kering
        if (prox[1] < 700) {
          seq = 5;
        }
        // If sampah basah
        if (prox[1] >= 700) {
          seq = 6;
        }
        PrevMillis[1] = millis();
      }
    }
  }

  //Sequence 5 ( Sampah kering )
  if (seq == 5 and millis() - PrevMillis[1] >= Interval[2]) {
    pos[2] = 0;
    if (millis() - PrevMillis[2] >= Interval[3]) {
      PrevMillis[2] = millis();
      if (pos[2] < deg[2]) {
        deg[2] = deg[2] - 1;
        servo2();
      } else {
        seq = 7;
        PrevMillis[1] = millis();
      }
    }
  }

  //Sequence 6 ( Sampah basah )
  if (seq == 6 and millis() - PrevMillis[1] >= Interval[2]) {
    pos[2] = 180;
    if (millis() - PrevMillis[2] >= Interval[3]) {
      PrevMillis[2] = millis();
      if (pos[2] > deg[2]) {
        deg[2] = deg[2] + 1;
        servo2();
      } else {
        seq = 8;
        PrevMillis[1] = millis();
      }
    }
  }

  //Sequence 7 ( Sampah kering, Servo2 return )
  if (seq == 7 and millis() - PrevMillis[1] >= Interval[2]) {
    pos[2] = 90;
    if (millis() - PrevMillis[2] >= Interval[3]) {
      PrevMillis[2] = millis();
      if (pos[2] > deg[2]) {
        deg[2] = deg[2] + 1;
        servo2();
      } else {
        seq = 0;
        PrevMillis[1] = millis();
      }
    }
  }

  //Sequence 8 ( Sampah basah, Servo2 return )
  if (seq == 8 and millis() - PrevMillis[1] >= Interval[2]) {
    pos[2] = 90;
    if (millis() - PrevMillis[2] >= Interval[3]) {
      PrevMillis[2] = millis();
      if (pos[2] < deg[2]) {
        deg[2] = deg[2] - 1;
        servo2();
      } else {
        seq = 0;
        PrevMillis[1] = millis();
      }
    }
  }
}

void servo0() {
  myservo0.write(deg[0]);
}
void servo1() {
  myservo1.write(deg[1]);
}
void servo2() {
  myservo2.write(deg[2]);
}

void ultrasonic() {
  digitalWrite(trig[a], LOW);
  delayMicroseconds(5);
  digitalWrite(trig[a], HIGH);
  delayMicroseconds(10);
  digitalWrite(trig[a], LOW);

  duration[a] = pulseIn(echo[a], HIGH);
  cm[a] = (duration[a] / 2) / 29.1;
}

void proximity() {
  prox[0] = analogRead(A1);
  prox[1] = analogRead(A2);
}

void prin() {
  Serial.print("*");
  Serial.print(cm[0]);
  Serial.print("/");
  Serial.print(cm[1]);
  Serial.print("/");
  Serial.print(cm[2]);
  Serial.print("/");
  Serial.print(cm[3]);
  Serial.print("/");
  Serial.print(switching);
  Serial.print("/");
  Serial.print(val);
  Serial.print("#");
  Serial.print(" Sequence : ");
  Serial.print(seq);
  Serial.print(" Servo 0 : ");
  Serial.print(deg[0]);
  Serial.print(" Servo 1 : ");
  Serial.print(deg[1]);
  Serial.print(" Servo 3 : ");
  Serial.print(deg[2]);
  Serial.print(" Proxy 1 : ");
  Serial.print(prox[0]);
  Serial.print(" Proxy 2 : ");
  Serial.print(prox[1]);
  Serial.print(" Solar : ");
  Serial.print(val);
  Serial.print(" Safety : ");
  Serial.println(safety);
}
