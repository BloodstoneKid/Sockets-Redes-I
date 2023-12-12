/*
 *          		S E R V I D O R
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>



#define PUERTO 6969
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_BUFFER 516
#define MAXHOST 128

extern int errno;

void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);
void errout(char *);		/* declare error out routine */

int FIN = 0;             /* Para el cierre ordenado */
void finalizar(){ FIN = 1; }

typedef struct {
    const char *pregunta;
    const char *respuesta;
} PreguntaRespuesta;


PreguntaRespuesta obtenerPreguntaAleatoria(PreguntaRespuesta *preguntas, int *numPreguntas) {
    if (*numPreguntas <= 0) {
        PreguntaRespuesta empty = {NULL, NULL};
        return empty;
    }
    int indiceAleatorio = rand() % *numPreguntas;
    PreguntaRespuesta preguntaSeleccionada = preguntas[indiceAleatorio];
    preguntas[indiceAleatorio] = preguntas[*numPreguntas - 1];
    (*numPreguntas)--;

    return preguntaSeleccionada;
}

    PreguntaRespuesta preguntas[] = {
		{"¿Cuánto es la raíz cuadrada de 64?", "8"},
        {"¿Cuál es el resultado de 2 elevado a la 5ta potencia?", "32"},
        {"¿Cuál es la suma de los primeros 50 números naturales?", "1275"},
		{"¿Cuántos lados tiene un icosaedro regular?", "20"},
		{"¿Cuál es el producto de los factores primos de 36?", "36"},
		{"¿Cuántos bits hay en un byte?", "8"},
		{"¿Cuántos grados tiene la suma de los ángulos internos de un triángulo?", "180"},
		{"¿Cuántos elementos hay en la tabla periódica de los elementos?", "118"},
		{"¿Cuántos dígitos hay en la secuencia de Fibonacci antes del 1000?", "7"}
    };
		int numPreguntasDisponibles = sizeof(preguntas) / sizeof(preguntas[0]);


/*********************************************************************************/

int main(argc, argv)
int argc;
char *argv[];
{

    int s_TCP, s_UDP;		/* connected socket descriptor */
    int ls_TCP;				/* listen socket descriptor */

    int cc;				    /* contains the number of bytes read */

    struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */

    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in clientaddr_in;	/* for peer socket address */
    int addrlen;

    fd_set readmask;
    int numfds,s_mayor;

    char buffer[BUFFERSIZE];	/* buffer for packets to be read into */

    struct sigaction vec;

		/* Create the listen socket. */
    ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
    if (ls_TCP == -1)
    {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}

	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

    addrlen = sizeof(struct sockaddr_in);

	myaddr_in.sin_family = AF_INET;

	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1)
    {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}

	if (listen(ls_TCP, 5) == -1)
    {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}


	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1)
    {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}

	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1)
    {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	}

	setpgrp();

	switch (fork())
    {
	    case -1:
		    perror(argv[0]);
		    fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		    exit(1);
	    case 0:
		    fclose(stdin);
		    fclose(stderr);

		    if ( sigaction(SIGCHLD, &sa, NULL) == -1)
           	{
            	perror(" sigaction(SIGCHLD)");
            	fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
            	exit(1);
           	}

           	vec.sa_handler = (void *) finalizar;
           	vec.sa_flags = 0;
           	if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1)
           	{
            	perror(" sigaction(SIGTERM)");
            	fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
            	exit(1);
           	}

		    while (!FIN)
           	{
            	FD_ZERO(&readmask);
            	FD_SET(ls_TCP, &readmask);
              	FD_SET(s_UDP, &readmask);

    	        if (ls_TCP > s_UDP)
              	{
                	s_mayor=ls_TCP;
              	}
              	else
              	{
                	s_mayor=s_UDP;
              	}

              	if ((numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0)
              	{
                  	if (errno == EINTR)
                  	{
                      	FIN=1;
		                close (ls_TCP);
		                close (s_UDP);
                      	perror("\nFinalizando el servidor. Se�al recibida en elect\n ");
                  	}
              	}
              	else
              	{
                  	if (FD_ISSET(ls_TCP, &readmask))
                  	{
    			        s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
    			        if (s_TCP == -1)
                      	{
                        	exit(1);
                      	}

                    	switch (fork())
                    	{
        			    	case -1:
        				    	exit(1);
        			    	case 0:
                    			close(ls_TCP);
        				    	serverTCP(s_TCP, clientaddr_in);
        				    	exit(0);
        			    	default:
        				    	close(s_TCP);
        				}
                	}
                	if (FD_ISSET(s_UDP, &readmask))
                	{
                    	cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0, (struct sockaddr *)&clientaddr_in, &addrlen);
                    	if ( cc == -1)
                    	{
                        	perror(argv[0]);
                        	printf("%s: recvfrom error\n", argv[0]);
                        	exit (1);
                    	}

                    	buffer[cc]='\0';
                    	serverUDP (s_UDP, buffer, clientaddr_in);
                	}
            	}
		    }
            close(ls_TCP);
            close(s_UDP);
            printf("\nFin de programa servidor!\n");

	    default:
		    exit(0);
	}
}


void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;		/* keeps count of number of requests */
	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		/* remote host's name string */
	char hostip[MAXHOST];  // Para almacenar la dirección IP del cliente
	unsigned int hostport;

	char mensaje[TAM_BUFFER];

	int len, len1, status,final=0;
   	struct hostent *hp;		/* pointer to host info for remote host */
   	long timevar;			/* contains time returned by time() */

   	struct linger linger;		/* allow a lingering, graceful close; */
    				                /* used when setting SO_LINGER */

    status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname, MAXHOST, NULL, 0, 0);
    
	if(status)
    {
    	strcpy(hostip, "N/A");
    }

	if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostip, MAXHOST) == NULL)
    {
      	strcpy(hostip, "N/A");      
    }


	hostport = ntohs(clientaddr_in.sin_port);


    /* Log a startup message. */
    time (&timevar);


		/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the  data sent on it has been received by the remote host.
		 */
	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) 
	{
		errout(hostname);
	}

	// Send the "Servicio Preparado" message to the client
    snprintf(mensaje, sizeof(mensaje), "220 Servicio Preparado\n");
    if (send(s, mensaje, strlen(mensaje), 0) != strlen(mensaje))
	{
		errout(hostname);
	}

	int holaReceived = 0;  // 0 representa falso
	int firstMessage = 1;  // 1 representa verdadero

	PreguntaRespuesta preguntaAleatoria;
	int intentos = 4;
	int acierto = 0;

	while (!final) 
	{
        len = recv(s, buf, TAM_BUFFER, 0);
		if (len == -1)
		{
			errout(hostname);
		} 

		char *newline = memchr(buf, '\n', len);
		char *carriage_return = memchr(buf, '\r', len);
		int message_length;

		if (newline != NULL || carriage_return != NULL)
		{
			message_length = (int)(newline != NULL ? newline - buf : carriage_return - buf);
		}			
		else 
		{
			const char *response = "500 Error de sintaxis\n";
			if (send(s, response, strlen(response), 0) != strlen(response))
			{
				errout(hostname);
			}			 
			continue; // Salir del bucle
		}

		// Almacenar el mensaje en la variable
        snprintf(mensaje, sizeof(mensaje), "%.*s", message_length, buf);

		const char *response;

		// Verificar si es el primer mensaje
		if (firstMessage) 
		{
			// El primer mensaje debe ser "HOLA"
			if (strcmp(mensaje, "HOLA") == 0) 
			{
				firstMessage = 0;
				holaReceived = 1;

				preguntaAleatoria = obtenerPreguntaAleatoria(preguntas, &numPreguntasDisponibles);	// Comprobar que no sea null -> cuando se terminen volver a llenar o algo.
				// Formatear la respuesta con el número de intentos
        		snprintf(mensaje, sizeof(mensaje), "250 %s#%d\n", preguntaAleatoria.pregunta, intentos);
				response = mensaje;
			} 
			else 
			{
				// Primer mensaje incorrecto, responder con error
				response = "500 Error de sintaxis\n";
			}
		} 
		else 
		{
			// No es el primer mensaje, verificar otros mensajes permitidos
			if (strcmp(mensaje, "ADIOS") == 0) 
			{
				response = "201 Cerrando el servicio\n";
				sleep(1);
				if (send(s, response, strlen(response), 0) != strlen(response))
				{
					errout(hostname);
				}				 
				break;
			} 
			else if (strcmp(mensaje, "+") == 0) 			
			{
				if(intentos < 1)
				{
					intentos = 4;
					snprintf(mensaje, sizeof(mensaje), "250 %s#%d\n", preguntaAleatoria.pregunta, intentos);
				}
				else if(acierto)
				{
					intentos = 4;
					preguntaAleatoria = obtenerPreguntaAleatoria(preguntas, &numPreguntasDisponibles);
					snprintf(mensaje, sizeof(mensaje), "250 %s#%d\n", preguntaAleatoria.pregunta, intentos);
					acierto = !acierto;
				}
				else
				{
					snprintf(mensaje, sizeof(mensaje), "500 Error de sintaxis\n");
				}			 

				response = mensaje;

			} 
			else if (strncmp(mensaje, "RESPUESTA ", 10) == 0) 
			{
				int n;
				char *endptr;

				// Intentar convertir la parte después de "RESPUESTA " en un número entero
				n = strtol(mensaje + 10, &endptr, 10);

				// Verificar si hay algún error en la conversión
				if (*endptr != '\0' && !isspace((unsigned char)*endptr)) 
				{
					// Hubo un error en la conversión (no se pudo convertir toda la cadena a un número)
					snprintf(mensaje, sizeof(mensaje), "500 Error de sintaxis\n");
				} 
				else 
				{
					// Comparar el número recibido con la respuesta esperada
					if (intentos-- > 0) 
					{
						if (n > atoi(preguntaAleatoria.respuesta)) 
						{
							snprintf(mensaje, sizeof(mensaje), "354 MENOR#%d\n", intentos);
						} 
						else if (n < atoi(preguntaAleatoria.respuesta)) 
						{
							snprintf(mensaje, sizeof(mensaje), "354 MAYOR#%d\n", intentos);
						} 
						else 
						{
							snprintf(mensaje, sizeof(mensaje), "350 ACIERTO\n");
							acierto = !acierto;
						}
					} 
					else 
					{
						snprintf(mensaje, sizeof(mensaje), "500 NUMERO DE INTENTOS AGOTADO\n");
					}
				}

				response = mensaje;
			} 
			else 
			{
				// Otros mensajes no permitidos
				response = "500 Error de sintaxis\n";
			}
		}

		sleep(1);
			/* Send a response back to the client. */
		if (send(s, response, strlen(response), 0) != strlen(response))
		{
			errout(hostname);
		} 

	}

	close(s);
}


void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in)
{
    int client_len = sizeof(clientaddr_in);
    char mensaje[TAM_BUFFER];
	int len,final=0;

	snprintf(mensaje, sizeof(mensaje), "220 Servicio Preparado\n");
    if (sendto(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr_in, &client_len) != strlen(mensaje)) 
	{
		errout(hostname);
	}	

	int holaReceived = 0;
	int firstMessage = 1;

	PreguntaRespuesta preguntaAleatoria;

	int intentos = 4;
	int acierto = 0;

	while (!final) 
	{
        len = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr_in *)&clientaddr_in, &client_len);
		if (len == -1)
		{
			errout(hostname);
		}	 

		char *newline = memchr(buf, '\n', len);
		char *carriage_return = memchr(buf, '\r', len);
		int message_length;

		if (newline != NULL || carriage_return != NULL)
		{
			message_length = (int)(newline != NULL ? newline - buf : carriage_return - buf);
		}			
		else 
		{
			const char *response = "500 Error de sintaxis\n";
			if (sendto(s, response, strlen(response), 0, (struct sockaddr *)&clientaddr_in, &client_len) != strlen(response)) 
			{
				errout(hostname);
			}			
			continue; // Salir del bucle
		}

		// Almacenar el mensaje en la variable
        snprintf(mensaje, sizeof(mensaje), "%.*s", message_length, buf);

		const char *response;

		// Verificar si es el primer mensaje
		if (firstMessage) 
		{
			// El primer mensaje debe ser "HOLA"
			if (strcmp(mensaje, "HOLA") == 0) 
			{
				firstMessage = 0;
				holaReceived = 1;

				preguntaAleatoria = obtenerPreguntaAleatoria(preguntas, &numPreguntasDisponibles);	// Comprobar que no sea null -> cuando se terminen volver a llenar o algo.
				// Formatear la respuesta con el número de intentos
        		snprintf(mensaje, sizeof(mensaje), "250 %s#%d\n", preguntaAleatoria.pregunta, intentos);
				response = mensaje;
			} 
			else 
			{
				// Primer mensaje incorrecto, responder con error
				response = "500 Error de sintaxis\n";
			}
		} 
		else 
		{
			// No es el primer mensaje, verificar otros mensajes permitidos
			if (strcmp(mensaje, "ADIOS") == 0) 
			{
				response = "201 Cerrando el servicio\n";
                final = 1;
				sleep(1);
				if (sendto(s, response, strlen(response), 0, (struct sockaddr *)&clientaddr_in, &client_len) != strlen(response)) errout(hostname);
				break;
			} 
			else if (strcmp(mensaje, "+") == 0) 
			{
				if(intentos < 1)
				{
					intentos = 4;
					snprintf(mensaje, sizeof(mensaje), "250 %s#%d\n", preguntaAleatoria.pregunta, intentos);
				}
				else if(acierto)
				{
					intentos = 4;
					preguntaAleatoria = obtenerPreguntaAleatoria(preguntas, &numPreguntasDisponibles);
					snprintf(mensaje, sizeof(mensaje), "250 %s#%d\n", preguntaAleatoria.pregunta, intentos);
					acierto = !acierto;
				}
				else
				{
					snprintf(mensaje, sizeof(mensaje), "500 Error de sintaxis\n");
				}				

				response = mensaje;

			} 
			else if (strncmp(mensaje, "RESPUESTA ", 10) == 0) 
			{
				int n;
				char *endptr;

				// Intentar convertir la parte después de "RESPUESTA " en un número entero
				n = strtol(mensaje + 10, &endptr, 10);

				// Verificar si hay algún error en la conversión
				if (*endptr != '\0' && !isspace((unsigned char)*endptr)) 
				{
					// Hubo un error en la conversión (no se pudo convertir toda la cadena a un número)
					snprintf(mensaje, sizeof(mensaje), "500 Error de sintaxis\n");
				} 
				else 
				{
					// Comparar el número recibido con la respuesta esperada
					if (intentos-- > 0) 
					{
						if (n > atoi(preguntaAleatoria.respuesta)) 
						{
							snprintf(mensaje, sizeof(mensaje), "354 MENOR#%d\n", intentos);
						} 
						else if (n < atoi(preguntaAleatoria.respuesta)) 
						{
							snprintf(mensaje, sizeof(mensaje), "354 MAYOR#%d\n", intentos);
						} 
						else 
						{
							snprintf(mensaje, sizeof(mensaje), "350 ACIERTO\n");
							acierto = !acierto;
						}
					} 
					else 
					{
						snprintf(mensaje, sizeof(mensaje), "500 NUMERO DE INTENTOS AGOTADO\n");
					}
				}
				response = mensaje;

			} 
			else 
			{
				// Otros mensajes no permitidos
				response = "500 Error de sintaxis\n";
			}
		}

		sleep(1);
			/* Send a response back to the client. */
		if (sendto(s, response, strlen(response), 0, (struct sockaddr *)&clientaddr_in, &client_len))
		{
			errout(hostname);
		}	 

	}
	close(s);
 }


 /*
  *	This routine aborts the child process attending the client.
  */
 void errout(char *hostname)
 {
 	printf("Connection with %s aborted on error\n", hostname);
 	exit(1);
 }
