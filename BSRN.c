#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>



// Diese Methode soll einen Trennstrich und einen Zeilenumbruch generieren
void binde_strich() {
    for (int i = 0; i < 45; i++) {
        printf("-");
    }
    printf("\n");
}



// Das Auslesen aus dem /Proc-Verzeichnis, wobei /proc/[$pid]/statm verwendet wird, um den Speicher eines Prozesses zu ermitteln
static void get_memory_usage(int pid) {
    binde_strich();

    printf("Speicherbelegung aus dem /Proc-Verzeichnis\n");
    // Erstellt den Pfad zur statm-Datei des Prozesses, statm_path dient als Puffer
    char statm_path[20];
    sprintf(statm_path, "/proc/%d/statm", pid);

    // Oeffnet die statm-Datei des Prozesses zum Lesen
    FILE* fp = fopen(statm_path, "r");
    if (fp == NULL) {
        perror("fopen()");
        exit(EXIT_FAILURE);
    }

    // Variablen zur Speicherung der gelesenen Werte
    unsigned long groesse, speicherteile, shared, seiten, bib, daten, dt;

    // Liest die Werte aus der statm-Datei und speichert sie in den Variablen
    if (fscanf(fp, "%lu %lu %lu %lu %lu %lu %lu", &groesse, &speicherteile, &shared, &seiten, &daten, &bib, &dt) != 7) {
        fprintf(stderr, "Konnte Speicherbelegung nicht ermitteln.\n");
        exit(EXIT_FAILURE);
    }

    // Schliesst die statm-Datei
    fclose(fp);

    printf("fuer PID %d:\n", pid);
    printf("Gesamtgroesse des Programms: %lu\n", groesse);
    printf("Groesse von Speicherteilen: %lu\n", speicherteile);
    printf("Anzahl von Pages: %lu\n", shared);
    printf("Anzahl Seiten von Programmcode: %lu\n", seiten);
    printf("Anzahl Seiten von Stack/Daten: %lu\n", daten);
    printf("Anzahl von Bibliotheksprogrammcode: %lu\n", bib);
    printf("Anzahl von dirty Pages: %lu\n", dt);

    // Nach dem Einlesen der Werte aus der statm-Datei
    unsigned long actual_ram_usage = (speicherteile) * 4096;  // Konvertierung von Seiten (resident set size) in Bytes
    printf("Tatsaechliche RAM-Nutzung: %lu kB\n", actual_ram_usage / 1024);  // Konvertierung von Bytes in Kilobyte und Ausgabe


    binde_strich();
}



// Funktion zur Visualisierung der Prozesserzeugung mit fork()
void visualize_fork() {
    int pid_des_Kindes = fork();

    // Es kam zu einem Fehler -> Programmabbruch
    if (pid_des_Kindes < 0) {
        printf("\nEs kam beim fork zu einem Fehler!");
        exit(1);
    }

    // Elternprozess
    if (pid_des_Kindes > 0) {


        // Der Vater- bzw. Elternprozess ruft pstree auf, um die Beziehung zwischen Vater- und Kindprozess anzuzeigen. Command ist als Puffer da.
        // Hierbei wird nur die Beziehung zwischen Eltern- und Kindprozess ausgegeben, indem "- pstree -p %pid" aufgerufen wird, wobei der pstree vom pid des Vaters ausgeht.
        char command[50];
        sprintf(command, "pstree -p %d", getpid());

        FILE* fp = popen(command, "r");
        if (fp == NULL) {
            printf("Fehler beim Oeffnen des Commands!\n");
            exit(1);
        }
 
        // Hier wird nur eine Zeile vom pstree gespeichert und ausgegeben, sodass die Beziehung zwischen dem Vater- und Kindprozess visualisiert wird
        char output[1000];
        while (fgets(output, sizeof(output), fp) != NULL) {
            printf("%s", output);
        }
        pclose(fp);
        
        binde_strich();
        //Randnotiz: Der pstree wird vor dem wait(NULL)-Aufruf aufgerufen => Dies ermoeglicht, dass der Kindprozess noch im pstree auftaucht, bevor er, der Kindprozess, beendet wird!
    
        // Der Vater- bzw. Elternprozess wartet nun, bis das Kind fertig ist
        wait(NULL);
    

        printf("\n[Elternprozess] PID: %i", getpid());
        printf("\n[Elternprozess] PPID: %i", getppid());
        printf("\n[Elternprozess] reale UID %i \teffektive UID %i", getuid(), geteuid());
        printf("\n[Elternprozess] reale GID %i \teffektive UID %i \n", getgid(), getegid());

        get_memory_usage(getpid());  // Speichernutzung des Elternprozesses abrufen

    
    }

    // Kindprozess
    if (pid_des_Kindes == 0) {
        printf("\n[Kindprozess] PID: %i", getpid());
        printf("\n[Kindprozess] PPID: %i", getppid());
        printf("\n[Kindprozess] reale UID %i \teffektive UID %i", getuid(), geteuid());
        printf("\n[Kindprozess] reale GID %i \teffektive UID %i \n", getgid(), getegid());

        get_memory_usage(getpid());  // Speichernutzung des Kindprozesses abrufen
        exit(EXIT_SUCCESS); // Beenden des Kindprozesses nach der Ausgabe
    }
}



// Funktion zur Visualisierung der Prozessverzweigung mit exec()
void visualize_exec() {
    int pid = fork();

    if (pid < 0) {
        printf("Es kam beim fork zu einem Fehler!\n");
        exit(1);
    }

    if (pid > 0) {
        // Elternprozess
        wait(NULL); // Auf Beendigung des Kindprozesses warten

        printf("\n[Eltern] Eigene PID:     %d\n", getpid());
        printf("[Eltern] PID des Kindes: %d\n", pid);

        char choice;
        printf("--- Moechten Sie zusaetzliche Prozessinformationen anzeigen lassen? (j/n)---:  ");
        scanf(" %c", &choice);

        if (choice == 'j' || choice == 'J') {
            get_memory_usage(getpid()); // Speichernutzung des Elternprozesses abrufen
        }
    }

    if (pid == 0) {
        // Kindprozess
        printf("[Kind]   Eigene PID:     %d\n", getpid());
        printf("[Kind]   PID des Vaters: %d\n", getppid());

        char choice;
        printf("--- Moechten Sie zusaetzliche Prozessinformationen anzeigen lassen? (j/n)---:  ");
        scanf(" %c", &choice);

        if (choice == 'j' || choice == 'J') {
            get_memory_usage(getpid()); // Speichernutzung des Kindprozesses abrufen
        }
         printf("Vor dem exec()-Aufruf:\n");
                
         /*Hier wird nur eine Zeile vom pstree gespeichert und ausgegeben, sodass
         die Beziehung zwischen dem Vater- und Kindprozess vor dem exec()-Aufruf visualisiert wird ähnl. wie bei visualize_fork() */
         char command[50];
         sprintf(command, "pstree -p %d", getppid());
         FILE* fp = popen(command, "r");
                 if (fp == NULL) {
                     printf("Fehler beim Oeffnen des Commands!\n");
                        exit(1);
                 }
                     char output[10];
                    while (fgets(output, sizeof(output), fp) != NULL) {
                printf("%s", output);
            }
            pclose(fp);

         printf("Der Kindprozess wird ersetzt....\n");
         sleep(5);
         printf("Nach dem exec()-Aufruf: \n");
   
          char ParentPID[10];//Hier wird der execlp-Argument definiert = pstree soll den Kindprozess ersetzen, wobei vom PID des Vater ausgegangen wird
          sprintf(ParentPID, "%d",getppid());
        // Aktuelles Programm durch "pstree -p" ersetzen
        if (execlp("pstree", "pstree", "-p", ParentPID, NULL) == -1) {
            printf("Fehler beim Ausfuehren von execl!\n");
            exit(1);
        }
    }

    printf("[Eltern] '%d': Programmende.\n",getpid());
}





int main() {
    // Menue zur Auswahl der Funktionen
    int choice;
    printf("\n--- Menü ---\n");
    printf("1. Visualisierung der Prozesserzeugung\n");
    printf("2. Visualisierung der Prozessverzweigung\n");
    printf("3. Beenden\n");

    do {
        if (scanf("%d", &choice) != 1) {
            // Ungueltige Eingabe
            printf("\nUngueltige Auswahl. Bitte versuchen Sie es erneut.\n");
            while (getchar() != '\n');  // Eingabepuffer leeren
        } else {
            switch (choice) {
                case 1:
                    printf("\nVisualisierung der Prozesserzeugung:\n");
                    visualize_fork();
                    break;
                case 2:
                    printf("\nVisualisierung der Prozessverzweigung:\n");
                    visualize_exec();
                    break;
                case 3:
                    printf("\nDas Programm wird beendet.\n");
                    break;
                default:
                    printf("\nUngueltige Auswahl. Bitte versuchen Sie es erneut.\n");
                    break;
            }
        }
    } while (choice != 3);
    return EXIT_SUCCESS;
}

