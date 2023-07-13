#include <CLI/cli.h>
#include <GLOBALS/globals.h>

//*****************************************************************************
// Tasks
// Codice del Task B, terminale seriale
void doCLI(void *parameters)
{
  // contenuto del messaggio da inviare al terminale remoto
  Message rcv_msg;
  // buffer del testo ricevuto dal terminale remoto
  char cmd_buf[CMD_BUF_LEN];
  char c;
  // indice nel messaggio
  uint8_t idx = 0;
  // lunghezza del comando da riconoscere
  uint8_t cmd_len = strlen(command);
  float media;

  // reset del buffer di ricezione
  memset(cmd_buf, 0, CMD_BUF_LEN);

  // Ciclo infinito
  while (1)
  {

    // Verifica la presenza di un messaggio nella coda dei messaggi
    // da inviare al terminale remoto: se esiste, lo stampa sul terminale
    if (xQueueReceive(msg_queue, (void *)&rcv_msg, 0) == pdTRUE)
    {
      Serial.println(rcv_msg.body);
    }

    // Legge il prossimo carattere dalla seriale
    if (Serial.available() > 0)
    {
      c = Serial.read();

      if (c == '\r')
      {
        // non lo utilizzo
      }
      // Se il carattere ricevuto è INVIO, stampa un a capo
      else if (c == '\n')
      {

        Serial.print("\r\n");

        // Se il comando ricevuto è "avg", viene stampato il valore medio
        if (strncmp(cmd_buf, command, cmd_len) == 0)
        {
          portENTER_CRITICAL(&spinlock);
          media = average;
          portEXIT_CRITICAL(&spinlock);

          Serial.printf("Average: %.3f V\n", media);
        }

        // Reset del buffer di ricezione e dell'indice
        memset(cmd_buf, 0, CMD_BUF_LEN);
        idx = 0;
      }
      // Altrimenti, il carattere non è INVIO: stampa il carattere sul terminale
      else
      {

        // Se c'è spazio nel buffer del testo ricevuto, memorizza il nuovo carattere nel buffer
        if (idx < CMD_BUF_LEN - 1)
        {
          cmd_buf[idx] = c;
          idx++;
          cmd_buf[idx] = '\0';
        }

        Serial.print(c);
      }
    }

    // Per non saturare la CPU, cede il passo ad altri task per un po' di tempo
    vTaskDelay(pdMS_TO_TICKS(cli_delay));
  }
}