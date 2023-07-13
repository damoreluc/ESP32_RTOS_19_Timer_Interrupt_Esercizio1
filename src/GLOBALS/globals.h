#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <Arduino.h>

//*****************************************************************************
// Parametri globali
// tempo di ciclo in ms del task B (interfaccia operatore)
static const uint32_t cli_delay = 20;

// numero di caratteri nel buffer dei comandi ricevuti
static const uint16_t CMD_BUF_LEN = 255;
// numero di caratteri nei messaggi verso il terminale
static const uint16_t MSG_LEN = 100;
// numero massimo di possibili messaggi contemporaneamente in uscita
static const uint16_t MSG_QUEUE_LEN = 5;

// Gestione dei buffer di acquisizione
// numero di campioni nei due buffer di acquisizione
static const uint16_t BUF_LEN = 10;
// primo buffer di acquisizione
static volatile uint16_t buf_0[BUF_LEN];
// secondo buffer di acquisizione
static volatile uint16_t buf_1[BUF_LEN];
// puntatore al buffer su cui scrivere con la ISR
static volatile uint16_t *write_to = buf_0;
// puntatore al buffer da cui leggere con il Task A
static volatile uint16_t *read_from = buf_1;
// flag di segnalazione dell'overrun su uno dei buffer di acquisizione
static volatile uint8_t buf_overrun = 0;

//*****************************************************************************
// Elenco comandi
// comando da riconoscere nel Task B
static const char command[] = "avg";

//*****************************************************************************
// Tipi di dati definiti dall'utente
// struct per i messaggi da inviare al terminale remoto
typedef struct
{
  char body[MSG_LEN];
} Message;

//*****************************************************************************
// valor medio dei dati acquisiti in un buffer
static float average = 0.0;
// tensione di riferimento dell'ADC
static const float V_ref = 3.3;
// numero di livelli dell'ADC
static const uint16_t ADC_resolution = 1 << 12;

//*****************************************************************************
// Oggetti RTOS
// semaforo di coordinamento accesso condiviso tra ISR e Task A
static SemaphoreHandle_t sem_done_reading = NULL;

// spinlock per la protezione dell'accesso alla variabile average condivisa tra Task A e Task B
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

// handler del task A per le notifiche (interrupt differito della ISR del timer)
static TaskHandle_t xTaskANotify = NULL;

// coda dei messaggi verso il terminale seriale
static QueueHandle_t msg_queue;

#endif