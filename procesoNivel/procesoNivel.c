#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<commons/collections/list.h>
#include<commons/config.h>
#include<string.h>
#include<configuracion.h>
#include<header_clie_serv.h>
#include<tad_items.h>

//#define DIRECCION "192.168.1.1"
#define DIRECCION INADDR_ANY   //INADDR_ANY representa la direccion de cualquier interfaz conectada con la computadora
#define BUFF_SIZE 1024
#define MAXLINE		512
typedef struct {
	int socket;
	char IDPersonaje;
} socket_IDPersonaje;
typedef struct {
	char IDRecurso;
	char IDPersonaje;
} IDRecurso_IDPersonaje;

//PROTOTIPOS

t_config_nivel* leerConfiguracion(char*,int*);
int haceteServidor(ITEM_NIVEL* ListaItems,t_list* ListaIDs,t_list* ListaRecursosAsignados,t_config_nivel* niv);
//int haceteCliente(void);
void ingresarAlOrquestador(char*);
ITEM_NIVEL* cargarCajasALaLista(ITEM_NIVEL* ListaItems,t_config_nivel* niv);
int puerto(char*);
char *ip(char*);
/*int atender_cliente(ITEM_NIVEL* ListaItems,t_list* ListaIDs,t_list* ListaRecursosAsignados,t_config_nivel* niv,int unSocket,char buffer[],int nbytes);*/
t_list* CrearID(t_list* ListaIDs,int unSocket,char id);
char BuscarID(t_list* ListaIDs, int unSocket);
t_list* BorrarID(t_list* ListaIDs, int unSocket);
t_list* AsignarRecurso(t_list* ListaRecursosAsignados,char IDRecurso,char IDPersonaje);
/*void DesasignarRecursos(ITEM_NIVEL* ListaItems,t_list* ListaRecursosAsignados,t_config_nivel* niv,char IDPersonaje);*/

//MAIN
int main(int argc,char *argv[]){
	int ok;
	if (argc!=2){
		printf("Debe tener un argumento con el nombre del archivo de Configuracion\n");
	}
	else{
	t_config_nivel* niv;
	niv = leerConfiguracion(argv[1],&ok);
	if(ok){
		ITEM_NIVEL* ListaItems = NULL;
		t_list* ListaIDs;
		t_list* ListaRecursosAsignados;

		ListaIDs = list_create();
		ListaRecursosAsignados = list_create();
		ListaItems = cargarCajasALaLista(ListaItems,niv);
		nivel_gui_inicializar();
		nivel_gui_dibujar(ListaItems);
		//ingresarAlOrquestador("127.0.0.1:7077");
		haceteServidor(ListaItems,ListaIDs,ListaRecursosAsignados,niv);
		//nivel_gui_terminar();
	}
	}
	return 0;
}

//DEFINICIONES

t_config_nivel *leerConfiguracion(char* nombreArchivo,int* ok){
	t_config_nivel *niv=(t_config_nivel*)malloc(sizeof(t_config_nivel));
	t_config *nivel;

	 nivel = asociarArchivoFisico(nombreArchivo);
	 	 if (dictionary_size(nivel->properties)){
	 niv->nombre = obtenerNombre(nivel);
	 niv->orquestador = obtenerOrquestador(nivel);
	 niv->planificador = obtenerPlanificador(nivel);
	 niv->tiempoChequeoDeadlock = obtenerTiempoChequeoDeadLock(nivel);
	 niv->recovery = obtenerRecovery(nivel);
	 niv->cajaN = obtenerCajas(nivel);
	 niv->puerto = obtenerPuerto(nivel);
	 niv->cantCajas = config_keys_amount(nivel)-6;
	 *ok=1;
	 	 }
	 		 else{
	 			printf("No se a encontrado: %s\n",nombreArchivo);
	 			*ok=0;
	 		 }
	 destruirEstructura(nivel);
	 return niv;
}

int haceteServidor(ITEM_NIVEL* ListaItems,t_list* ListaIDs,t_list* ListaRecursosAsignados,t_config_nivel* niv){
 	fd_set sockets_maestro; // conjunto maestro de descriptores de fichero TODOS LOS SOCKETS VIGENTES
 	fd_set sockets_lectura; // conjunto temporal de descriptores de fichero para select() LOS SOCKETS QUE PERCIBAN DATOS ENTRANTES
 	int maximo_d; // número máximo de descriptores de fichero
 	int sock_escucha; // descriptor de socket a la escucha
 	char buffer[BUFF_SIZE]; // buffer para datos del cliente
  	int i;
	int nbytes;
 	FD_ZERO(&sockets_maestro); // borra los conjuntos maestro y temporal
 	FD_ZERO(&sockets_lectura);


	sock_escucha=obtener_socket_escucha(niv->puerto);// obtener socket a la escucha
 	FD_SET(sock_escucha, &sockets_maestro);// añadir listener al conjunto maestro
 	maximo_d = sock_escucha; // seguir la pista del descriptor de fichero mayor, por ahora es éste

 	for(;;) {  // bucle principalint i;
		sockets_lectura = sockets_maestro; // copiamos el maestro en lectura.. asi el select trabaja sobre TODOS los disponibles
 		if (select(maximo_d+1, &sockets_lectura, NULL, NULL, NULL) == -1) {
 			perror("select");
 			exit(1);
 		}
 		// explorar conexiones existentes en busca de datos que leer
 		for(i = 0; i <= maximo_d; i++) {
 			if (FD_ISSET(i, &sockets_lectura)) { // ¡¡tenemos datos!!

				if (i == sock_escucha) {//sock escucha estaba en listen, si se "movio" es una nueva conexion
					gestionar_nueva_conexion(sock_escucha,&sockets_maestro,&maximo_d);
					// gestionar nuevas conexiones


 				} else {
 					// gestionar datos de un cliente EXISTENTE
 					if (recibir(i,buffer,BUFF_SIZE,&nbytes)!=0) //Cerro conexion
 					{
 						IDRecurso_IDPersonaje* aux;
 						int j,k;
 					  	char IDPersonaje;
 					  	IDPersonaje = BuscarID(ListaIDs,i);
						//DesasignarRecursos(ListaItems,ListaRecursosAsignados,niv,IDPersonaje);
 						for(j=0;j<list_size(ListaRecursosAsignados);j++){
							aux = (IDRecurso_IDPersonaje*) list_get(ListaRecursosAsignados,j);
							if (aux->IDPersonaje == IDPersonaje){
								AumentarRecurso(ListaItems,aux->IDRecurso);
								nivel_gui_dibujar(ListaItems);
								k = 0;
								while (niv->cajaN[k]->simbolo != aux->IDRecurso){
									k = k + 1;
								}
								if (niv->cajaN[k]->simbolo == aux->IDRecurso){ //tecnicamente no haria falta
									niv->cajaN[k]->instancias = niv->cajaN[k]->instancias + 1;
								}
								list_remove(ListaRecursosAsignados,j);
								free(aux);
								j = j - 1;
							}
						}
 						ListaIDs = BorrarID(ListaIDs,i);
						BorrarItem(&ListaItems,IDPersonaje);
						nivel_gui_dibujar(ListaItems);
						close(i); // ¡Hasta luego!
						FD_CLR(i, &sockets_maestro); // eliminar del conjunto maestro
 					//} else atender_cliente(ListaItems,ListaIDs,ListaRecursosAsignados,niv,i,buffer,nbytes);
 					} else {
 						char px[3],py[3];
 						char IDPersonaje,IDRecurso;
 						int unSocket,posicionX,posicionY,j;
 						unSocket = i;
 						//printf("Buf: %s\n",buffer);
 						//sleep(1);
 						switch(buffer[0]){//solicitud del personaje
 							case '1':{//personaje informa que se conecto
 								ListaIDs = CrearID(ListaIDs,unSocket,buffer[1]);//guardo en la listaIds socket de personaje junto con su id
 								CrearPersonaje(&ListaItems, buffer[1], 1, 1);
 								break;
 							}
 							case '2':{//personaje informa nueva posicion
 								IDPersonaje = BuscarID(ListaIDs,unSocket);
 								py[0]=buffer[1];
 								py[1]=buffer[2];
 								py[2]='\0';
 								px[0]=buffer[3];
 								px[1]=buffer[4];
 								px[2]='\0';
 								MoverPersonaje(ListaItems, IDPersonaje,atoi(px), atoi(py));
 								break;
 							}
 							case '3':{//personaje solicita posicion recurso
 								IDRecurso = buffer[1];
 								j = 0;
 								while (niv->cajaN[j]->simbolo != IDRecurso){
 									j = j + 1;
 								}
 								if (niv->cajaN[j]->simbolo == IDRecurso){ //tecnicamente no haria falta
 									posicionX=niv->cajaN[j]->posicionX;
 									posicionY=niv->cajaN[j]->posicionY;
 								}
 								if(posicionX>9)
 									sprintf(px,"%d",posicionX);
 									else sprintf(px,"0%d",posicionX);
 								if(posicionY>9)
 									sprintf(py,"%d",posicionY);
 									else sprintf(py,"0%d",posicionY);

 								sprintf(buffer,"%s%s",px,py);
 								if (send(unSocket, buffer, strlen(buffer), 0) == -1){
 									perror("Error al enviar datos. Cliente no encontrado");
 								}
 								break;
 							}
 							case '4':{//personaje solicita otorgamiento de recurso: 1 si se lo doy o 0 si no
 								char otorgamiento[3];
 								IDRecurso = buffer[1];
 								IDPersonaje = BuscarID(ListaIDs,unSocket);
 								j = 0;
 								while (niv->cajaN[j]->simbolo != IDRecurso){
 									j = j + 1;
 								}
 								if (niv->cajaN[j]->simbolo == IDRecurso){ //tecnicamente no haria falta
 									if (niv->cajaN[j]->instancias > 0){
 										niv->cajaN[j]->instancias = niv->cajaN[j]->instancias - 1;
 										RestarRecurso(ListaItems, IDRecurso);
 										ListaRecursosAsignados = AsignarRecurso(ListaRecursosAsignados,IDRecurso,IDPersonaje);
 										//buffer[0]='1'; //se lo doy
 										//buffer[1]='\0';
 		 								sprintf(otorgamiento,"%d",1);
 									} else {
 										//buffer[0]='0'; //no se lo doy
 										//buffer[1]='\0';
 		 								sprintf(otorgamiento,"%d",0);
 									}
 								}

 								sprintf(buffer,"%s",otorgamiento);
 								if (send(unSocket, buffer, strlen(buffer), 0) == -1){
 									perror("Error al enviar datos. Cliente no encontrado");
 								}
 								break;
 							}
 						}
 						nivel_gui_dibujar(ListaItems);
 					}
 				} //fin del IF (Es cliente nuevo o existente?)
 			}//fin del IF hay CAMBIOS EN EL DESCRIPTOR? (Tenemos Datos)
 		}//fin del FOR que busca los cambios en descriptores
 	}//fin del ciclo infinito
 	return 0;
 }

void ingresarAlOrquestador(char* dirOrquestador){
	int nbytes;
	char buffer[1024];
	int socketOrquestador;

	/*	char* dirOrquestador="127.0.0.1:7077"; */


	socketOrquestador = conectar_a_servidor("127.0.0.1",7077);
		strcpy(buffer,"Me quiero conectar al Orquestador!");
		if (send(socketOrquestador, buffer, strlen(buffer), 0) >= 0) {
			} else {
			perror("Error al enviar datos. Server no encontrado.\n");
			}

		if ((nbytes = recv(socketOrquestador, buffer, sizeof(buffer), 0)) <= 0) {
 						if (nbytes == 0) {
 							printf("cliente: socket %d terminó conexion\n", socketOrquestador);
 						} else {
 							perror("recv");
 						}
  					} else {

  						printf("Te conectaste al Orquestador! \n");
					}

		//Me quedo conectado al orquestador por si hay que avisar interbloqueo...
}


/*int haceteCliente(){
	int unSocket;
	int nbytes;
	char buffer[BUFF_SIZE];

	unSocket=conectar_a_servidor(DIRECCION,niv->puerto);

	while (1) {
		gets(buffer);
		// Enviar los datos leidos por teclado a traves del socket.
		if (strcmp(buffer, "fin") == 0){
		printf("Cliente cerrado correctamente.\n");
		break;
		} else {
			if (send(unSocket, buffer, strlen(buffer), 0) >= 0) {
				printf("Datos enviados!: %s : %d\n",buffer,strlen(buffer));
			} else {
			perror("Error al enviar datos. Server no encontrado.\n");
			break;
			}


 		//acá recibe su pedido...
		if ((nbytes = recv(unSocket, buffer, sizeof(buffer), 0)) <= 0) {
 					// error o conexión cerrada por el server
 						if (nbytes == 0) {
 						// conexión cerrada
 							printf("cliente: socket %d terminó conexion\n", unSocket);
 						} else {
 							perror("recv");
 						}
 						close(unSocket); // ¡Hasta luego!
  					} else {
						printf("Mensaje recibido: ");
						fwrite(buffer, 1, nbytes, stdout);
						printf("\n");
						printf("Tamanio del buffer %d bytes!\n", nbytes);
						fflush(stdout); //limpiar buffer de salida estandar
					}

	}
	}

	close(unSocket);

	return EXIT_SUCCESS;

}*/

ITEM_NIVEL* cargarCajasALaLista(ITEM_NIVEL* ListaItems,t_config_nivel* niv){
	int i;
	for(i=0; i < niv->cantCajas; i++){
		CrearCaja(&ListaItems,niv->cajaN[i]->simbolo,niv->cajaN[i]->posicionY,niv->cajaN[i]->posicionX,niv->cajaN[i]->instancias);
	}
	return ListaItems;
}

//OJO -> IDEM PERSONAJE
int puerto(char* cadena){
	int i,puerto;
	for(i=0;*(cadena+i)!=':';i++);
		puerto=atoi(cadena+(i+1));
	return puerto;
}

//OJO -> IDEM PERSONAJE
char* ip(char* cadena){
	int i;
	char *ip=(char*)malloc(sizeof(char)*15);
	for(i=0;*(cadena+i)!=':';i++)
		*(ip+i)=*(cadena+i);
	return ip;
}

/*int atender_cliente(ITEM_NIVEL* ListaItems,t_list* ListaIDs,t_list* ListaRecursosAsignados,t_config_nivel* niv,int unSocket,char buffer[],int nbytes){
	char px[3],py[3];
	char IDPersonaje,IDRecurso;
	int i,posicionX,posicionY;
	switch(buffer[0]){//solicitud del personaje
		case '1':{//personaje informa que se conecto
			ListaIDs = CrearID(ListaIDs,unSocket,buffer[1]);//guardo en la listaIds socket de personaje junto con su id
			CrearPersonaje(&ListaItems, buffer[1], 1, 1);
			break;
		}
		case '2':{//personaje informa nueva posicion
			IDPersonaje = BuscarID(ListaIDs,unSocket);
			py[0]=buffer[1];
			py[1]=buffer[2];
			py[2]='\0';
			px[0]=buffer[3];
			px[1]=buffer[4];
			px[2]='\0';
			MoverPersonaje(ListaItems, IDPersonaje,atoi(px), atoi(py));
			break;
		}
		case '3':{//personaje solicita posicion recurso
			IDRecurso = buffer[1];
			i = 0;
			while (niv->cajaN[i]->simbolo != IDRecurso){
				i = i + 1;
			}
			if (niv->cajaN[i]->simbolo == IDRecurso){ //tecnicamente no haria falta
				posicionX=niv->cajaN[i]->posicionX;
				posicionY=niv->cajaN[i]->posicionY;
			}
			if(posicionX>9)
				sprintf(px,"%d",posicionX);
				else sprintf(px,"0%d",posicionX);
			if(posicionY>9)
				sprintf(py,"%d",posicionY);
				else sprintf(py,"0%d",posicionY);
			sprintf(buffer,"%s%s",px,py);
			if (send(unSocket, buffer, strlen(buffer), 0) == -1){
				perror("Error al enviar datos. Cliente no encontrado");
			}
			break;
		}
		case '4':{//personaje solicita otorgamiento de recurso: 1 si se lo doy o 0 si no
			IDRecurso = buffer[1];
			IDPersonaje = BuscarID(ListaIDs,unSocket);
			i = 0;
			while (niv->cajaN[i]->simbolo != IDRecurso){
				i = i + 1;
			}
			if (niv->cajaN[i]->simbolo == IDRecurso){ //tecnicamente no haria falta
				if (niv->cajaN[i]->instancias > 0){
					niv->cajaN[i]->instancias = niv->cajaN[i]->instancias - 1;
					RestarRecurso(ListaItems, IDRecurso);
					ListaRecursosAsignados = AsignarRecurso(ListaRecursosAsignados,IDRecurso,IDPersonaje);
					buffer[0]='1'; //se lo doy
				} else {
					buffer[0]='0'; //no se lo doy
				}
			}
			if (send(unSocket, buffer, 1, 0) == -1){
				perror("Error al enviar datos. Cliente no encontrado");
			}
			break;
		}
	}
	nivel_gui_dibujar(ListaItems);
	return 0;
}*/

t_list* CrearID(t_list* ListaIDs,int unSocket,char id){
	socket_IDPersonaje* aux;
	aux = (socket_IDPersonaje*)malloc(sizeof(socket_IDPersonaje));
	aux->socket = unSocket;
	aux->IDPersonaje = id;
	list_add(ListaIDs,(socket_IDPersonaje*)aux);
	//free(aux);
	return ListaIDs;
}

char BuscarID(t_list* ListaIDs, int unSocket){
	socket_IDPersonaje* aux;
	int i;
	//aux = (socket_IDPersonaje*)malloc(sizeof(socket_IDPersonaje));
	for(i=0;i<list_size(ListaIDs);i++){
		aux = (socket_IDPersonaje*)list_get(ListaIDs,i);
		if (aux->socket == unSocket){
			break;
		}
	}
	return aux->IDPersonaje;
}

t_list* BorrarID(t_list* ListaIDs, int unSocket){
	socket_IDPersonaje* aux;
	int i;
	//aux = (socket_IDPersonaje*)malloc(sizeof(socket_IDPersonaje));
	for(i=0;i<list_size(ListaIDs);i++){
		aux = (socket_IDPersonaje*)list_get(ListaIDs,i);
		if (aux->socket == unSocket){
			break;
		}
	}
	list_remove(ListaIDs,i);
	free(aux);
	return ListaIDs;
}

t_list* AsignarRecurso(t_list* ListaRecursosAsignados,char IDRecurso,char IDPersonaje){
	IDRecurso_IDPersonaje* aux;
	aux = (IDRecurso_IDPersonaje*)malloc(sizeof(IDRecurso_IDPersonaje));
	aux->IDRecurso = IDRecurso;
	aux->IDPersonaje = IDPersonaje;
	list_add(ListaRecursosAsignados,(void*)aux);
	//free(aux);
	return ListaRecursosAsignados;
}

/*void DesasignarRecursos(ITEM_NIVEL* ListaItems,t_list* ListaRecursosAsignados,t_config_nivel* niv,char IDPersonaje){
	IDRecurso_IDPersonaje* aux;
	int i,j;
	for(i=0;i<=list_size(ListaRecursosAsignados);i++){
		aux = list_get(ListaRecursosAsignados,i);
		if (aux->IDPersonaje == IDPersonaje){
			AumentarRecurso(ListaItems,aux->IDRecurso);
			j = 0;
			while (niv->cajaN[i]->simbolo != aux->IDRecurso){
				j = j + 1;
			}
			if (niv->cajaN[i]->simbolo == aux->IDRecurso){ //tecnicamente no haria falta
				niv->cajaN[i]->instancias = niv->cajaN[i]->instancias + 1;
			}
			list_remove_and_destroy_element(ListaRecursosAsignados,i,(void*)aux);

			i = -1;
		}
	}
}*/
