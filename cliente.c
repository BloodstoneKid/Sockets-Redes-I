/*
 *			C L I E N T C P
 *
 *	This is an example program that demonstrates the use of
 *	stream sockets as an IPC mechanism.  This contains the client,
 *	and is intended to operate in conjunction with the server
 *	program.  Together, these two programs
 *	demonstrate many of the features of sockets, as well as good
 *	conventions for using these features.
 *
 *
 */
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#define PUERTO 6969
#define TAM_BUFFER 516

void TCPCliente(char * hostname, char * fichero);
void UDPCliente(char * hostname, char * fichero);

/*
 *			M A I N
 *
 *	This routine is the client which request service from the remote.
 *	It creates a connection, sends a number of
 *	requests, shuts down the connection in one direction to signal the
 *	server about the end of data, and then receives all of the responses.
 *	Status will be written to stdout.
 *
 *	The name of the system to which the requests will be sent is given
 *	as a parameter to the command.
 */
int main(argc, argv)
int argc;
char *argv[];
{

	if (argc != 2) {
		fprintf(stderr, "Uso:  %s <host> <TCP/UDP> <fichero de ordenes>\n", argv[0]);
		exit(1);
	}

    		if (!strcmp("TCP",argv[2]))
		{
			TCPCliente(argv[1], argv[3]);
		}
		
		else if (!strcmp("UDP", argv[2]))
		{
			UDPCliente(argv[1], argv[3]);
		}
		
		else {
			fprintf(stderr, "Protocolo No Reconocido <TCP-UDP>\n");
			exit(1);
		}

}

void TCPCliente(char * hostname, char * fichero){

    int s;				/* connected socket descriptor */
   	struct addrinfo hints, *res;
    long timevar;			/* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
	int addrlen, i, j, errcode;
    /* This example uses TAM_BUFFER byte messages. */
	char buf[TAM_BUFFER];
    FILE *archivo;
        archivo = fopen(fichero,"r");
    if(archivo==NULL){
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE );
    }

	/* Create the socket. */
	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	/* Set up the peer address to which we will connect. */
	servaddr_in.sin_family = AF_INET;
	
	/* Get the host information for the hostname that the
	 * user passed in. */
      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
 	 /* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo (hostname, NULL, &hints, &res); 
    if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], hostname);
		exit(1);
        }
    else {
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	    }
    freeaddrinfo(res);

    /* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);

		/* Try to connect to the remote server at the address
		 * which was just built into peeraddr.
		 */
	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
		exit(1);
	}
		/* Since the connect call assigns a free address
		 * to the local end of this connection, let's use
		 * getsockname to see what it assigned.  Note that
		 * addrlen needs to be passed in as a pointer,
		 * because getsockname returns the actual length
		 * of the address.
		 */
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	 }

	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("Connected to %s on port %u at %s",
			argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

    while (fgets(buf, sizeof(buf), archivo)) {

        buf[strcspn(buf, "\n")] = '\r';
		buf[strcspn(buf, "\r")+1] = '\n';
        if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
			fprintf(stderr, "%s: Connection aborted on error ",	argv[0]);
			fprintf(stderr, "on send number %d\n", i);
			exit(1);
		}
    }    

	if (shutdown(s, 1) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
		exit(1);
	}


	char mensaje[TAM_BUFFER];

	while (i = recv(s, buf, TAM_BUFFER, 0)) {
		if (i == -1) {
            perror(argv[0]);
			fprintf(stderr, "%s: error reading result\n", argv[0]);
			exit(1);
		}

			char *newline = memchr(buf, '\n', i);
			char *carriage_return = memchr(buf, '\r', i);
			int message_length;

			if (newline != NULL || carriage_return != NULL)
				message_length = (int)(newline != NULL ? newline - buf : carriage_return - buf);
			else printf("CLIENTE: No se encontró un carácter de nueva línea o retorno de carro");

			// Almacenar el mensaje en la variable
            snprintf(mensaje, sizeof(mensaje), "%.*s", message_length, buf);

			/* Print out message indicating the identity of this reply. */
		printf("%s\n",mensaje);
	}

    fclose(archivo);
}

void UDPCliente(char * hostname, char * fichero){
    int i, errcode;
	int retry = RETRIES;		/* holds the retry count */
    int s;				/* socket descriptor */
    long timevar;                       /* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
    struct in_addr reqaddr;		/* for returned internet address */
    int	addrlen, n_retry;
    struct sigaction vec;
   	struct addrinfo hints, *res;

    FILE *archivo;
        archivo = fopen(fichero,"r");
    if(archivo==NULL){
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE );
    }

	/* Create the socket. */
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		perror("CLIENTE");
		fprintf(stderr, "%s: unable to create socket\n", "CLIENTE");
		exit(1);
	}
	
    /* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	
			/* Bind socket to some local address so that the
		 * server can send the reply back.  A port number
		 * of zero will be used so that the system will
		 * assign any available port number.  An address
		 * of INADDR_ANY will be used so we do not have to
		 * look up the internet address of the local host.
		 */
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror("CLIENTE");
		fprintf(stderr, "%s: unable to bind socket\n", "CLIENTE");
		exit(1);
	   }
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
            perror("CLIENTE");
            fprintf(stderr, "%s: unable to read socket address\n", "CLIENTE");
            exit(1);
    }

            /* Print out a startup message for the user. */
    time(&timevar);
            /* The port number must be converted first to host byte
             * order before printing.  On most hosts, this is not
             * necessary, but the ntohs() call is included here so
             * that this program could easily be ported to a host
             * that does require it.
             */
    printf("Connected to %s on port %u at %s", hostname, ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

	/* Set up the server address. */
	servaddr_in.sin_family = AF_INET;
		/* Get the host information for the server's hostname that the
		 * user passed in.
		 */
      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
 	 /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo (hostname, NULL, &hints, &res); 
    if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				"CLIENTE", hostname);
		exit(1);
      }
    else {
			/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	 }
     freeaddrinfo(res);
     /* puerto del servidor en orden de red*/
	 servaddr_in.sin_port = htons(PUERTO);

   /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
    vec.sa_handler = (void *) handler;
    vec.sa_flags = 0;
    if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGALRM)");
            fprintf(stderr,"%s: unable to register the SIGALRM signal\n", "CLIENTE");
            exit(1);
        }
	
    n_retry=RETRIES;

	char cadena[TAM_BUFFER];
	sprintf(cadena, "%s", " \r\n");
    if (sendto (s, cadena, TAM_BUFFER, 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) 
	{
			perror("CLIENTE");
			fprintf(stderr, "%s: unable to send request\n", "CLIENTE");
			exit(1);
	}

	struct sockaddr_in socketAsociado;
	int contador = 0, flag = 0;

	while (n_retry > 0) {
		/* Send the request to the nameserver. */

		/* Set up a timeout so I don't hang in case the packet
		 * gets lost.  After all, UDP does not guarantee
		 * delivery.
		 */
	    alarm(TIMEOUT);
		/* Wait for the reply to come in. */

        if (recvfrom (s, &socketAsociado, addrlen, 0, (struct sockaddr *)&servaddr_in, &addrlen) == -1) {
										
			// DA IGUAL NO RECIBIR NADA
    		if (errno == EINTR) {
    				/* Alarm went off and aborted the receive.
    				 * Need to retry the request if we have
    				 * not already exceeded the retry limit.
    				 */
 		         printf("attempt %d (retries %d).\n", n_retry, RETRIES);
  	 		     n_retry--; 
                    } 
            else  {
				printf("Unable to get response from");
				exit(1); 
                }
              } 

        else {

			alarm(0);

			do
			{
			
					

					cadena[strcspn(cadena, "\n")] = '\r';
					cadena[strcspn(cadena, "\r")+1] = '\n';

					if (sendto (s, cadena, TAM_BUFFER, 0, (struct sockaddr *)&socketAsociado, sizeof(struct sockaddr_in)) == -1) 
					{
							perror("CLIENTE");
							fprintf(stderr, "%s: unable to send request\n", "CLIENTE");
							exit(1);
					}

					cadena[strcspn(cadena, "\r")] = '\0';

                char mensaje[TAM_BUFFER];
				i=recvfrom (s, cadena, TAM_BUFFER, 0, (struct sockaddr *)&socketAsociado, &addrlen);
				char *newline = memchr(cadena, '\n', i);
			char *carriage_return = memchr(cadena, '\r', i);
			int message_length;

			if (newline != NULL || carriage_return != NULL)
				message_length = (int)(newline != NULL ? newline - buf : carriage_return - buf);
			else printf("CLIENTE: No se encontró un carácter de nueva línea o retorno de carro");

			// Almacenar el mensaje en la variable
            snprintf(mensaje, sizeof(mensaje), "%.*s", message_length, cadena);

			/* Print out message indicating the identity of this reply. */
		printf("%s\n",mensaje);
			}
			while(fgets(cadena, TAM_BUFFER-1, archivo));

            /* Print out response. */
			// DIRECCIÓN NO ENCONTRADA
            if (reqaddr.s_addr == ADDRNOTFOUND) 
               printf("Host %s unknown by nameserver %s\n", cadena, hostname);
            else {
                /* inet_ntop para interoperatividad con IPv6 */
                if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
                   perror(" inet_ntop \n");
                printf("Address for %s is %s\n", cadena, hostname);
                }	
            break;	
            }
  }

    if (n_retry == 0)
	{
       printf("Unable to get response from");
       printf(" %s after %d attempts.\n", hostname, RETRIES);
    }
    fclose(archivo);
}
