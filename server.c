// Création du serveur 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define NB_MAX_CLIENTS 25 // nombres de clients maximals
#define LONG_MAX_MESS 1024 // longueur maximal d'un message
#define LONG_MAX_NOM 256 // longueur maximal d'un nom

pthread_t threads[NB_MAX_CLIENTS]; // tableau de thread dédié à un utilisateur souhaitant se connecter
int identif_clients[NB_MAX_CLIENTS]; //  tableau d'identifiants d'un client
int compteur_client = 0; // pour compter le nombre de clients

char messClients[NB_MAX_CLIENTS][LONG_MAX_MESS]; // matrice de chaines de caractères où les lignes sont les utilisateurs et les colonnes les messages
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // initialise mutex pour la protection de l'accés aux messages

// Cette fonction permet d'ajouter un nouveau message 
void ajouterMessage(int id_client, char *message){
    pthread_mutex_lock(&mutex);
    strcpy(messClients[id_client], message);
    pthread_mutex_unlock(&mutex);
} 

// Cette fonction permet d'envoyer un message à tous les clients
void envoyerMessage(int id_envoyer, char* message){
    for(int i=0; i<compteur_client; i++){
        if(identif_clients[i] != id_envoyer){
            write(identif_clients[i], message, strlen(message));
        }
    }
}

// Cettte fonction permet de supprimer le client de la liste des clients connectés
void supprimer(int id_client){
    //int id_client = *(int*)message;
    pthread_mutex_lock(&mutex);
    for(int i = id_client-4; i < compteur_client - 1; i++){
        identif_clients[i] = identif_clients[i + 1];
        memset(messClients[i], 0, LONG_MAX_MESS);
        strcpy(messClients[i], messClients[i + 1]);
    }
}

// Cette fonction permet de gérer les messages issues des clients
void* gestionClient(void* text){
    int id_client = *(int*)text;

    // On envoie un message de bienvenue à l'utilisateur
    char mess_bienvenue[LONG_MAX_MESS];
    sprintf(mess_bienvenue, "Soyez la bienvenue dans notre messagerie et vous êtes le client n°%d. \n", id_client-3);
    
    if(send(id_client, mess_bienvenue, strlen(mess_bienvenue), 0)<0){
        printf("Désolé, probléme d'envoie du messsage de bienvenue.\n");
    }

    char messages_clients[LONG_MAX_MESS]; // messages envoyés par le client

    while(read(id_client, messages_clients, LONG_MAX_MESS) > 0){

        ajouterMessage(id_client, messages_clients); // ajouter des messages au tableau contenant les messages envoyés par le client
        printf("Messsage reçu du client n° %d : %s \n",id_client-3, messages_clients);

        char message_envoye_clients[LONG_MAX_MESS + LONG_MAX_NOM];
        sprintf(message_envoye_clients, "utilisateur n°%d : %s", id_client-3, messages_clients);
        
        envoyerMessage(id_client, message_envoye_clients);

        memset(messages_clients, 0, LONG_MAX_MESS); // pour réinitialiser le message

    }
/*
    // Si le client ferme la connexion, supprimer le client de la liste des clients connectés
    pthread_mutex_lock(&mutex);
    for(int i = id_client-4; i < compteur_client - 1; i++){
        identif_clients[i] = identif_clients[i + 1];
        memset(messClients[i], 0, LONG_MAX_MESS);
        strcpy(messClients[i], messClients[i + 1]);
    }
*/
   supprimer(id_client);

    compteur_client--;
    pthread_mutex_unlock(&mutex);

    // Fermer la connexion
    close(id_client);
    pthread_exit(NULL);

}

int main(int argc, char** argv){

    int desc_sock, com_sock;
    struct sockaddr_un adresse_socket = {0};
    adresse_socket.sun_family = AF_UNIX;
    strcpy(adresse_socket.sun_path, "./MonStock");
    socklen_t longueur_adresse = sizeof(adresse_socket);

    //  Créer la socket serveur
    if((desc_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("échec du socket");
        exit(EXIT_FAILURE);
    }

    unlink("./MonStock");

    if(bind(desc_sock, (struct sockaddr *)&adresse_socket, longueur_adresse) == -1){
        perror("echec connexion");
        exit(EXIT_FAILURE);
    }

    // Écoute du socket server
    if(listen(desc_sock, 10) == -1) {
        perror("Ecoute");
        exit(EXIT_FAILURE);
    }

    printf("Le serveur est prêt pour accueillir une nouvelle connexion d'un client :  \n");

    while(1) {
        struct sockaddr_un adresse_client = {0};
        socklen_t long_addr_client = sizeof(adresse_client);
        com_sock = accept(desc_sock, (struct sockaddr*)&adresse_client, &long_addr_client);
        if(com_sock == -1){
            perror("Probléme d'acceptation d'une nouvelle connexion\n");
            return 1;
        }

        printf("La connexion est acceptée. \n");

        identif_clients[compteur_client] = com_sock; // identification unique pour le client

        // Création d'un nouveau thread pour le client
        if(pthread_create(&threads[compteur_client], NULL, gestionClient, &com_sock) != 0){
            perror("Probléme lors de la création du thread client");
            return 1;
        }

        compteur_client++;

    }

    // Pour fermer la socket du serveur
    close(desc_sock);

    return 0;

}