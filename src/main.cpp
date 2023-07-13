/*
 * FreeRTOS Esercizio 19: Campionamento con ISR, processamento dati in Task A, interfaccia utente in Task B.
 *
 * Acquisizione di valori analogici dal canale 34 dell'ADC nella ISR del timer hardware, periodo 100ms
 * Questi dati campionati devono essere copiati in un buffer doppio lungo 10 elementi.
 * Ogni volta che uno dei buffer è pieno, l'ISR deve notificare il Task A.
 *
 * Il Task A, quando riceve la notifica dall'ISR, dovrà attivarsi e calcolare la media dei 10 campioni
 * precedentemente raccolti. Per la media si adoperi una variabile di tipo float.
 * Si noti che durante questo periodo di calcolo, l'ISR potrebbe attivarsi nuovamente:
 * per questo motivo è necessario impiegare un buffer doppio così da rendere possibile
 * elaborare un buffer col Task A mentre l'altro viene riempito dalla ISR.
 *
 * Al termine del calcolo il Task A dovrà aggiornare una variabile a virgola mobile globale contenente
 * la media appena calcolata. Non dare per scontato che la scrittura su questa variabile a virgola mobile
 * richieda un singolo ciclo di istruzioni: sarà necessario proteggere quell'azione mediante qualche strategia
 * di accesso protetto alla variabile.
 *
 * Il Task B implementa una semplice interfaccia operatore: riceve caratteri dalla porta seriale e ne fa
 * l'eco sulla stessa porta seriale. Se viene immesso il comando "avg", il Task B dovrà inviare al
 * terminale seriale il valore attuale della variabile media globale.
 *
 *
 * Nota: nel file soc.h sono definiti i riferimenti ai due core della ESP32:
 *   #define PRO_CPU_NUM (0)
 *   #define APP_CPU_NUM (1)
 *
 * Qui viene adoperata la APP_CPU
 *
 */

#include <Arduino.h>
#include <GLOBALS/globals.h>
#include <CLI/cli.h>
#include <PROCESSING/processing.h>

//*****************************************************************************
// Impostazioni del programma
// pin driver del Led
static const int pinLed = GPIO_NUM_23;
// pin del canale analogico
static const int pinAdc = GPIO_NUM_34;
// divisore del timer hardware (per tick = 1us)
static const uint16_t timer_divider = 80;
// costante di tempo del timer hardware (100ms)
static const uint64_t timer_max_count = 100000;

//*****************************************************************************
// variabili globali
// handler del timer hardware
static hw_timer_t *timer = NULL;


//*****************************************************************************
// Funzioni richiamabili da qualsiasi punto di questo file

// Scambia il valore dei puntatori del doppio buffer write_to e read_from
// Questa funzione viene richiamata solo dalla ISR, pertanto non c'è bisogno di renderla thread-safe
void IRAM_ATTR swap()
{
  volatile uint16_t *temp_ptr = write_to;
  write_to = read_from;
  read_from = temp_ptr;
}

//*****************************************************************************
// Interrupt Service Routines (ISRs)

// ISR del timer hardware, eseguita quando il timer raggiunge il valore timer_max_count
void IRAM_ATTR onTimer()
{
  BaseType_t task_woken = pdFALSE;
  static uint16_t idx = 0;

  // Toggle LED
  int pin_state = digitalRead(pinLed);
  digitalWrite(pinLed, !pin_state);

  // Acquisizione del valore dall'ADC
  // Se il buffer non è in overrun, pone il campiore nel prossimo elemento libero del buffer.
  // Se il buffer è in overrun, il campione viene saltato.
  if ((idx < BUF_LEN) && (buf_overrun == 0))
  {
    write_to[idx] = analogRead(pinAdc);
    idx++;
  }

  // Controllo se il buffer è pieno
  if (idx >= BUF_LEN)
  {

    // Se il Task A non ha completato la lettura, imposta il flag di overrun.
    // Il semaforo sem_done_reading viene liberato dal Task A quando ha terminato l'elaborazione dei
    // dati del buffer di lettura.
    // Non c'è bisogno di proteggere con una sezione critica questa operazione sul flag di overrun
    // poiché non ci sono altri task o ISR che potrebbero interrompere o modificare il valore del flag.
    if (xSemaphoreTakeFromISR(sem_done_reading, &task_woken) == pdFALSE)
    {
      buf_overrun = 1;
    }

    // Se il buffer è pieno e il flag di overrun non è attivo
    // si scambiano i puntatori ai due buffer e invia la notifica al Task A
    if (buf_overrun == 0)
    {

      // Reset dell'indice e scambio dei puntatori ai due buffer
      idx = 0;
      swap();

      // Invia la notifica al Task A, affinché proceda con l'elaborazione dei nuovi dati
      vTaskNotifyGiveFromISR(xTaskANotify, &task_woken);
    }
  }

  // Uscita dalla ISR (per il FreeRTOS normale)
  // portYIELD_FROM_ISR(task_woken);

  // Uscita dalla ISR (per il porting di FreeRTOS di ESP-IDF)
  // richiesta di context switch se il semaforo o la notifica ha sbloccato il task
  if (task_woken)
  {
    portYIELD_FROM_ISR();
  }
}

//*****************************************************************************
// Main (sul core 1, con priorità 1)

// configurazione del sistema
void setup()
{
  // Configurazione della seriale
  Serial.begin(115200);

  // breve pausa
  vTaskDelay(pdMS_TO_TICKS(1000));
  Serial.println();
  Serial.println("FreeRTOS Software timer: Esempio 1 - Sample and Process");

  // Crea il semaforo di coordinamento tra ISR e Task A
  sem_done_reading = xSemaphoreCreateBinary();

  // Forza il reboot della ESP32 in caso di errore
  if (sem_done_reading == NULL)
  {
    Serial.println("Errore: impossibile creare il semaforo");
    ESP.restart();
  }

  // All'inizio il semaforo sem_done_reading deve essere libero
  xSemaphoreGive(sem_done_reading);

  // Crea la coda dei messaggi in uscita
  msg_queue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message));

  // Crea il Task B dell'interfaccia utente
  // con priorità più alta di quella del task setup&loop come richiesto per il context switch nella ISR
  xTaskCreatePinnedToCore(
      doCLI,        // Funzione da eseguire
      "Do CLI",     // Nome del task
      3072,         // Stack del task
      NULL,         // parametri per il task
      3,            // Livello di priorità aumentato
      NULL,         // Puntatore al task
      APP_CPU_NUM); // Core su sui eseguire il task

  // Crea il Task A di elaborazione dei dati
  // con priorità più bassa del task B; viene salvato l'handler del task A per le notifiche nella ISR
  xTaskCreatePinnedToCore(
      calcAverage,         // Funzione da eseguire
      "Calculate average", // Nome del task
      1024,                // Stack del task
      NULL,                // parametri per il task
      2,                   // Livello di priorità aumentato
      &xTaskANotify,       // Puntatore al task
      APP_CPU_NUM);        // Core su sui eseguire il task

  // Configurazione del pin del led
  pinMode(pinLed, OUTPUT);

  // Crea e avvia il timer hardware num. 0 (num, divider, countUp)
  timer = timerBegin(0, timer_divider, true);

  // Associa la ISR al timer (timer, function, edge)
  timerAttachInterrupt(timer, &onTimer, true);

  // Imposta il valore di conteggio al quale eseguire la ISR (timer, count, autoreload)
  timerAlarmWrite(timer, timer_max_count, true);

  // Abilita la generazione degli interrupt del timer
  timerAlarmEnable(timer);

  // Elimina il task con "Setup e Loop"
  vTaskDelete(NULL);
}

void loop()
{
  // lasciare vuoto
}