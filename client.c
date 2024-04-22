#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <signal.h>

#define LONG_MAX_MESS 1024 // longueur maximal d'un message
#define LONG_MAX_NOM 256 // longueur maximal d'un nom

char tmp_message[LONG_MAX_MESS * 25]; // stocke des messages reçus pendant l'interruption

int interrupted = 0;

void gererCommande(int signal){
    interrupted = 1;
    printf("\n Arret de l'affichage effectué. Présentez votre message et Pour avancer appuyer sur entrée.\n");
}

void* message_recue(void* mess){
    int socket_client = *(int*)mess;
    char messages[LONG_MAX_MESS];

    while(read(socket_client, messages, LONG_MAX_MESS) > 0){
        if(interrupted){
            strcat(tmp_message, messages);
            memset(messages, 0, LONG_MAX_MESS);
        }else{
            printf("%s", messages);
        }

        memset(messages, 0, LONG_MAX_MESS);
    }

    pthread_exit(NULL);
}

int main(int argc, char const *argv[]){

    int socket_client = socket(AF_UNIX, SOCK_STREAM, 0);

    // adresse de structure d'initialisation du serveur
    struct sockaddr_un adresse_serveur = {0};
    adresse_serveur.sun_family = AF_UNIX;
    strcpy(adresse_serveur.sun_path, "./MonStock"); // chemin d'accés au fichier socket

    if(connect(socket_client, (struct sockaddr*)&adresse_serveur, sizeof(adresse_serveur)) < 0){
        printf("Il y'a un probléme \n");
        return -1;
    }

    signal(SIGINT, gererCommande);

    pthread_t thread;
    pthread_create(&thread, NULL, message_recue, &socket_client);

    char Tab_message[LONG_MAX_MESS];
    while(fgets(Tab_message, LONG_MAX_MESS, stdin) != NULL) {
        if(interrupted){
            printf("%s", tmp_message);
            memset(tmp_message, 0, LONG_MAX_MESS);
            interrupted = 0;
        }
        if(write(socket_client, Tab_message, strlen(Tab_message)) < 0){
            printf("Il y'a un problème d'envoi du message");
            break;
        }
        if(strcmp(Tab_message, "AU REVOIR !!! \n") == 0){
            break;
        }

        memset(Tab_message, 0, LONG_MAX_MESS);

    }

    close(socket_client);
    pthread_cancel(thread);
    pthread_join(thread, NULL);

    return 0;
}
