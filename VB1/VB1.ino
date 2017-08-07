//AD9850 DDS test
#include <UTFT.h>
#include <UTouch.h>
#define DDS_CLOCK 125000000

//------------------------------------------------------------------------------------
// Costanti
//------------------------------------------------------------------------------------
// Vref = 1.8V proveniente dall'AD8302 quindi si ha una relazione diretta tra i valori
// letti dall'ADC e magnitudine/fase
//------------------------------------------------------------------------------------
float risoluzione_fase = 0.176; // =(180°/1023)
float risoluzione_magnitudine = 0.05859375; // =(60dB/1023)
//float ins_loss=0.0; // Correttivo
// Dichiarazione font caratteri
extern uint8_t BigFont[];
extern uint8_t SmallFont[];

// Imposto i pin del display grafico
UTFT myGLCD(ITDB32S, 38, 39, 40, 41);
UTouch  myTouch( 6, 5, 4, 3, 2);

int CLOCK = 50; //Pin DDS
int LOAD = 51; //Pin DDS
int DATA = 52; //Pin DDS
int RESET = 53; //Pin DDS
int vmagPin = A0; // Pin ADC0 connesso al vmag del modulo con AD8302
int vfasePin = A1; // Pin ADC1 connesso al vphase del modulo con AD8302
int x, y; //variabili necessarie al funzionamento del touch screen
int ind = 7; //Peso posizionale relativo funzione inserimento frequenza
int op = 0; //Valore della frequenza inserita relativo funzione inserimento frequenza
int arValore[7]; //Array globale valore frequenza inserita relativo funzione inserimento frequenza
int arMagnitudo[299];
int arFase[299];



//---------------------------------------------------------------------------
// Setup iniziale
//---------------------------------------------------------------------------
void setup()
{
  pinMode (DATA,  OUTPUT);
  pinMode (CLOCK, OUTPUT);
  pinMode (LOAD,  OUTPUT);
  pinMode (RESET, OUTPUT);
  pinMode(vmagPin, INPUT);
  pinMode(vfasePin, INPUT);

  AD9850_init();
  AD9850_reset();

  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  prima_pagina();

}

//---------------------------------------------------------------------------
// Ciclo principale che controlla i pulsanti della prima pagina
//---------------------------------------------------------------------------
void loop()
{
  while (true)
  {
    if (myTouch.dataAvailable())
    {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();
      if ((y >= 170) && (y <= 230)) // Terzo pulsante
      {
        if ((x >= 20) && (x <= 80))
        {
          delay (250);
          pagina_vfo();
        }
      }
      if ((y >= 10) && (y <= 70)) // Primo pulsante
      {
        if ((x >= 20) && (x <= 80))
        {
          delay (250);
          rl_phase_swr();

        }
      }
    }
  }

}

//------------------------------------------------------------------------------
// Pagine di funzioni
//------------------------------------------------------------------------------
void prima_pagina() {
  //Disegno la prima pagina
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 0, 255);
  myGLCD.drawRect(20, 10, 80, 70); //Disegno primo pulsante
  myGLCD.drawRect(20, 90, 80, 150); // Disegno secondo pulsante
  myGLCD.drawRect(20, 170, 80, 230); // Disegno terzo pulsante
  myGLCD.setColor(0, 0, 255);
  myGLCD.setFont(BigFont);
  myGLCD.print("Antenna SWR", 90, 35);
  myGLCD.print("VNA", 90, 115);
  myGLCD.print("VFO", 90, 195);
}



// Questa funzione prende in ingresso una frequenza e analizza:
// Return Loss - Fase - SWR - Inpedenza
void rl_phase_swr() {
  int frequenza = input_touch("Inserisci frequenza");
  delay(250);
  SetFrequency(frequenza); // Do la frequenza in pasto al DDS
  delay(10);
  int magnitudineA = analogRead(vmagPin); // Leggo magnitudine
  delay(1);
  int magnitudineB = analogRead(vmagPin); // Leggo magnitudine
  delay(1);
  int magnitudineC = analogRead(vmagPin); // Leggo magnitudine
  int magnitudine = (magnitudineA+magnitudineB+magnitudineC)/3;
  delay(1);
  int faseA = analogRead(vfasePin); // Leggo fase
  delay(1);
  int faseB = analogRead(vfasePin); // Leggo fase
  delay(1);
  int faseC = analogRead(vfasePin); // Leggo fase
  int fase= (faseA+faseB+faseC)/3;

  // Calcolo il return loss ed il coefficente di riflessione
  float return_loss = magnitudine* risoluzione_magnitudine; // Traduco la lettura in dB
  return_loss = return_loss - 30; // Porto la scala da +-30 a 0-60dB
  //------fino a qui va bene... i calcoli in virgola mobile son un macello
  float esponente = return_loss / 20.0;
  float c_riflessione = pow(10.0, esponente); // Calcolo il coefficente di riflessione
  // Calcolo l'SWR
  float a = 1.0 + (1.0/c_riflessione);
  float b = 1.0 - (1.0/c_riflessione);
  float swr = a / b; // Calcolo direttamente l'SWR
  swr = abs(swr);
  //--------fino a qui tutto bene :) sistemata tutta la parte matematica

  // Calcolo l'angolo di rotazione di fase
  float angolo = fase * risoluzione_fase; // Traduco la lettura in gradi
  float F = cos(angolo / 57.296); // La divisione per 57.296 fa la conversione da gradi a radianti
  float G = sin(angolo / 57.296);
  //Ora cominciano i calcoli strani scopiazzati
  float Rr = F * (1.0/c_riflessione);
  float Ss = G * (1.0/c_riflessione);
  float Tmpsa = 1.0 - Rr;
  float Rrsqrt = Tmpsa * Tmpsa;
  float Sssqrt = Ss * Ss;
  Tmpsa = Rrsqrt + Sssqrt;
  float Tmpsb = 2.0 * Ss;
  float Tmpsc = Tmpsb / Tmpsa;
  Tmpsa = Tmpsc * 50.0;
  float Ximp = abs(Tmpsa); // Abbiamo la parte reattiva

  Tmpsa = Rr * Rr;
  Tmpsb = 1.0 - Tmpsa;
  Tmpsa = Tmpsb - Sssqrt;
  Tmpsb = Rrsqrt + Sssqrt;
  Tmpsc = Tmpsa / Tmpsb;
  Tmpsa = Tmpsc * 50.0;
  float Rimp = abs(Tmpsa); // Abbiamo la parte resistiva

  Tmpsa = Rimp * Rimp;
  Tmpsb = Ximp * Ximp;
  Tmpsc = Tmpsa + Tmpsb;
  float Zimp = sqrt(Tmpsc);
  

  // Mi preparo e visualizzo il tutto
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0 );
  myGLCD.print("Return loss [dB]: ", CENTER, 20);
  myGLCD.setColor(255, 0, 0 );
  myGLCD.printNumF(return_loss,1, CENTER, 40); //return_loss
  myGLCD.setColor(0, 255, 0 );
  myGLCD.print("SWR: ", CENTER, 60);
  myGLCD.setColor(255, 0, 0 );
  myGLCD.printNumF(swr,2, CENTER, 80);//swr
  myGLCD.setColor(0, 255, 0 );
  myGLCD.print("Inpedenza [ohm]: ", CENTER, 100);
  myGLCD.setColor(255, 0, 0 );
  myGLCD.printNumF(Zimp,1, CENTER, 120);
  myGLCD.setColor(0, 255, 0 );
  myGLCD.print("Parte resistiva: ", CENTER, 140);
  myGLCD.setColor(255, 0, 0 );
  myGLCD.printNumF(Rimp,1, CENTER, 160);
  myGLCD.setColor(0, 255, 0 );
  myGLCD.print("Parte reattiva: ", CENTER, 180);
  myGLCD.setColor(255, 0, 0 );
  myGLCD.printNumF(Ximp,1, CENTER, 200);


}


// Funzione di prova per la tracciatura del grafico SWR
void pagina_swr() {
  //Disegno e gestisco il grafico del SWR vs Freq.
  int minima = input_touch("Frequenza minima");
  delay(250);
  int massima = input_touch("Frequenza massima");
  delay(250);
  sweep(minima, massima);

  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(0, 0, 255);
  myGLCD.drawRect(15, 20, 315, 220);
  // Disegno griglia
  myGLCD.setColor(0, 255, 255);
  myGLCD.print("SWR vs Frequenza", RIGHT, 5);

  myGLCD.setColor(0, 0, 255);
  for (int i = 20; i < 240; i += 25) { //per le linee orizzontali
    myGLCD.drawLine(15, i, 314, i);
  }

  myGLCD.setColor(255, 0, 0);
  // Metto i numerini sull'asse Y
  int b = 6;
  for (int i = 15; i < 220; i += 50)
  {
    b = b - 1;
    myGLCD.printNumI(b, 5, i);
  }
  //for (int i = 0; i <= 299; i++) {
  //myGLCD.setColor(0, 255, 0);
  //myGLCD.drawPixel ((i + 15), 150 - ((arMagnitudo[i]*Adcmagres - 30) * 6));
  //myGLCD.setColor(255, 255, 0);
  //myGLCD.drawPixel ((i + 15), 200 - (arFase[i]*Adcphres));
  //
  //}
}

// Semplice VFO a frequenza immessa da tastiera touch screen
void pagina_vfo() {
  int value = input_touch("Frequenza VFO");
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(BigFont);
  myGLCD.setColor(255, 0, 0);
  myGLCD.print("Frequenza", CENTER, 5);
  myGLCD.printNumI(value, 150, 20);
  SetFrequency(value);
  delay (3000);
  pagina_vfo();


}


// Funzione di test generazione di una sweeppata
void sweep(int frq_min, int frq_max) {
  int banda = frq_max - frq_min;
  // L'asse X ha 300 pizel quindi forniremo 300 valori di frequenza al dds
  int frequenze[300];
  int incrementale = banda / 300;
  for (int i = 1; i <= 300; i++) {
    frequenze[i] = frequenze[i] + incrementale;
    SetFrequency(frequenze[i]);
    arMagnitudo[i] = analogRead(vmagPin);
    arFase[i] = analogRead(vfasePin);
  }


}

//----------------------------------------------------------------------
// Funzione "inserisci la frequenza"
//----------------------------------------------------------------------
// Prendo una frase di 20 caratteri da mostrare nell'interfaccia grafica e restituisco un valore numerico da 0 a 30000000
int input_touch(char banner[20]) {
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 0, 255);
  myGLCD.setFont(BigFont);
  // Disegno la finestrella di visualizzazione
  myGLCD.drawRect(40, 40, 280, 80);
  myGLCD.drawRect(38, 38, 282, 82);
  // Disegno la prima fila di pulsanto 1-2-3-4-5-OK
  int xo = 5;
  for (int i = 1; i <= 6; i++)
  {
    myGLCD.drawRect((i * 10) + xo, 140, (i * 10) + 40 + xo, 180 );
    xo = xo + 40;
  }
  // Disegno la seconda fila di pulsanti 6-7-8-9-0-C
  xo = 5 ;
  for (int i = 1; i <= 6; i++)
  {
    myGLCD.drawRect((i * 10) + xo, 190, (i * 10) + 40 + xo, 230 );
    xo = xo + 40;
  }

  // Metto i numerini sulla prima riga
  myGLCD.setColor(255, 0, 0);
  int b;
  b = 0;
  xo = 18;
  for (int i = 1; i <= 5; i++)
  {
    myGLCD.printNumI(b, (i * 10) + xo, 152);
    b = b + 1;
    xo = xo + 40;
  }
  myGLCD.print("OK", 270, 152);

  // Metto i numeri sulla seconda riga
  b = 5;
  xo = 18;
  for (int i = 1; i <= 5; i++)
  {
    myGLCD.printNumI(b, (i * 10) + xo, 202);
    b = b + 1;
    xo = xo + 40;
  }
  myGLCD.print("C", 279, 202);

  myGLCD.print(banner, CENTER, 120);

  // Resetto a 0 l'array per il calcolo della frequenza immessa
  ind = 7;
  for (int i = 0; i <= 7; i++) {
    arValore[i] = 0;
  }
  // Scansiono la pressione di qualche pulsante ed aggiorno l'array con il valore immesso...
  while (true)
  {
    if (myTouch.dataAvailable())
    {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();


      if ((y >= 140) && (y <= 180)) // Prima riga
      {
        if ((x >= 15) && (x <= 55)) // Button: 0
        {
          updateStr(0);
        }
        if ((x >= 65) && (x <= 105)) // Button: 1
        {
          updateStr(1);
        }
        if ((x >= 115) && (x <= 155)) // Button: 2
        {
          updateStr(2);
        }
        if ((x >= 165) && (x <= 205)) // Button: 3
        {
          updateStr(3);
        }
        if ((x >= 215) && (x <= 255)) // Button: 4
        {
          updateStr(4);
        }
        if ((x >= 265) && (x <= 305)) // Button: OK
        {
          return (op);
          ind = 7;
        }
      }
      if ((y >= 190) && (y <= 230)) // Seconda riga
      {
        if ((x >= 15) && (x <= 55)) // Button: 5
        {
          updateStr(5);
        }
        if ((x >= 65) && (x <= 105)) // Button: 6
        {
          updateStr(6);
        }
        if ((x >= 115) && (x <= 155)) // Button: 7
        {
          updateStr(7);
        }
        if ((x >= 165) && (x <= 205)) // Button: 8
        {
          updateStr(8);
        }
        if ((x >= 215) && (x <= 255)) // Button: 9
        {
          updateStr(9);
        }
        if ((x >= 265) && (x <= 305)) // Button: C
        {
          ind = 7;
          for (int i = 0; i <= 7; i++) {
            arValore[i] = 0;
          }
          op = 0;
          myGLCD.setColor(0, 0, 0);
          myGLCD.fillRect(41, 41, 279, 79);
          myGLCD.setColor(255, 0, 0);
          myGLCD.printNumI(op, 50, 54);

        }

      }

    }

  }

}

// Questa funziona gestisce l'inserimento della cifra in arrivo dal touch screen nel contesto di un array da 0 a 30Mhz
void updateStr(int val) {
  if (ind < 0) {
    ind = 7;
    for (int i = 0; i <= 7; i++) {
      arValore[i] = 0;
    }
    op = 0;

    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(41, 41, 279, 79);
    myGLCD.setColor(255, 0, 0);
    myGLCD.printNumI(op, 50, 54);
  }
  arValore[7] = arValore[6];
  arValore[6] = arValore[5];
  arValore[5] = arValore[4];
  arValore[4] = arValore[3];
  arValore[3] = arValore[2];
  arValore[2] = arValore[1];
  arValore[1] = arValore[0];
  arValore[0] = val;
  op = (arValore[7] * 10000000) + (arValore[6] * 1000000) + (arValore[5] * 100000) + (arValore[4] * 10000) + (arValore[3] * 1000) + (arValore[2] * 100) + (arValore[1] * 10) + (arValore[0] * 1);
  myGLCD.setColor(255, 0, 0);
  myGLCD.printNumI(op, 50, 54);
  ind = ind - 1;
  delay(250);
}



//-------------------------------------------------------------------------------------
// Funzioni relative al DDS
//-------------------------------------------------------------------------------------
void SetFrequency(unsigned long frequency)
{
  unsigned long tuning_word = (frequency * pow(2, 32)) / DDS_CLOCK;
  digitalWrite (LOAD, LOW);

  shiftOut(DATA, CLOCK, LSBFIRST, tuning_word);
  shiftOut(DATA, CLOCK, LSBFIRST, tuning_word >> 8);
  shiftOut(DATA, CLOCK, LSBFIRST, tuning_word >> 16);
  shiftOut(DATA, CLOCK, LSBFIRST, tuning_word >> 24);
  shiftOut(DATA, CLOCK, LSBFIRST, 0x0);
  digitalWrite (LOAD, HIGH);
}

void AD9850_init()
{

  digitalWrite(RESET, LOW);
  digitalWrite(CLOCK, LOW);
  digitalWrite(LOAD, LOW);
  digitalWrite(DATA, LOW);
}

void AD9850_reset()
{
  digitalWrite(CLOCK, LOW);
  digitalWrite(LOAD, LOW);

  digitalWrite(RESET, LOW);
  delay(5);
  digitalWrite(RESET, HIGH);  //pulse RESET
  delay(5);
  digitalWrite(RESET, LOW);
  delay(5);

  digitalWrite(CLOCK, LOW);
  delay(5);
  digitalWrite(CLOCK, HIGH);  //pulse CLOCK
  delay(5);
  digitalWrite(CLOCK, LOW);
  delay(5);
  digitalWrite(DATA, LOW);    //make sure DATA pin is LOW

  digitalWrite(LOAD, LOW);
  delay(5);
  digitalWrite(LOAD, HIGH);  //pulse LOAD
  delay(5);
  digitalWrite(LOAD, LOW);
  // Chip is RESET now
}




