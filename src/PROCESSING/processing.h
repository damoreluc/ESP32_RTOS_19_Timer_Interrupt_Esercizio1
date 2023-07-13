#ifndef _PROCESSING_H
#define _PROCESSING_H

#include <Arduino.h>

//*****************************************************************************
// Tasks
// Codice del Task A, elaborazione del buffer dei dati campionati
// Attende la notifica da parte della ISR poi calcola la media dei valori dell'ADC
void calcAverage(void *parameters);

#endif