# ESP32_RTOS_19_Timer_Interrupt_Esercizio1

## FreeRTOS Esercizio 19: Campionamento con ISR, processamento dati in Task A, interfaccia utente in Task B.

Acquisizione di valori analogici dal canale 34 dell'ADC nella ISR del timer hardware, periodo 100ms
Questi dati campionati devono essere copiati in un buffer doppio lungo 10 elementi.
Ogni volta che uno dei buffer è pieno, l'ISR deve notificare il Task A.

 * Il __Task A__, quando riceve la notifica dall'ISR, dovrà attivarsi e calcolare la media dei 10 campioni
precedentemente raccolti. Per la media si adoperi una variabile di tipo float.
Si noti che durante questo periodo di calcolo, l'ISR potrebbe attivarsi nuovamente:
per questo motivo è necessario impiegare un buffer doppio così da rendere possibile
elaborare un buffer col Task A mentre l'altro viene riempito dalla ISR.

Al termine del calcolo il Task A dovrà aggiornare una variabile a virgola mobile globale contenente
la media appena calcolata. Non dare per scontato che la scrittura su questa variabile a virgola mobile
richieda un singolo ciclo di istruzioni: sarà necessario proteggere quell'azione mediante qualche strategia
di accesso protetto alla variabile.

 * Il __Task B__ implementa una semplice interfaccia operatore: riceve caratteri dalla porta seriale e ne fa
l'eco sulla stessa porta seriale. Se viene immesso il comando "avg", il Task B dovrà inviare al
terminale seriale il valore attuale della variabile media globale.
