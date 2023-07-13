#include <PROCESSING/processing.h>
#include <GLOBALS/globals.h>

//*****************************************************************************
// Tasks
// Codice del Task A, elaborazione del buffer dei dati campionati
// Attende la notifica da parte della ISR poi calcola la media dei valori dell'ADC
void calcAverage(void *parameters)
{

  Message msg;
  float avg;

  // Ciclo infinito
  while (1)
  {

    // Attesa della notifica dalla ISR
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Calcola la media dei dati, convertiti in valori floating point in tensione
    avg = 0.0;
    for (int i = 0; i < BUF_LEN; i++)
    {
      avg += (float)read_from[i];
      // vTaskDelay( pdMS_TO_TICKS(105) ); // Togliere il commento per testare l'overrun flag
    }
    avg /= BUF_LEN;
    avg = avg * V_ref / ADC_resolution;

    // L'aggiornamento della variabile condivisa tra Task A e Task B potrebbe richiedere
    // un certo numero di istruzioni elementari della CPU, pertanto occorre proteggere
    // questa operazione mediante una sezione critica
    portENTER_CRITICAL(&spinlock);
    average = avg;
    portEXIT_CRITICAL(&spinlock);

    // Se il Task A ha impiegato troppo tempo per l'elaborazione dei dati
    // e siamo incorsi in un errore di overrun sul buffer di scrittura, allora
    // viene inviato un messaggio di errore verso il terminale seriale passando per
    // la coda di comunicazione tra Task A e Task B
    if (buf_overrun == 1)
    {
      strcpy(msg.body, "Errore: Buffer overrun. Alcuni campioni sono stati persi.");
      xQueueSend(msg_queue, (void *)&msg, 10);
    }

    // Le operazioni del Task A sui dati sono terminate.
    // Ora viene resettato il flag di overrun e viene rilasciato il semaforo "done reading".
    // Queste due operazioni devono essere racchiuse in una sezione critica per non venire interrotte.
    portENTER_CRITICAL(&spinlock);
    buf_overrun = 0;
    xSemaphoreGive(sem_done_reading);
    portEXIT_CRITICAL(&spinlock);
  }
}
