#include <czmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>   
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#include<fstream>
#include<sstream>
#include<iostream>
#include<string>


#include <stdlib.h>    
#include <time.h>

#include <list>
#include <unordered_map>

using namespace std;

zctx_t* context;

unordered_map<string,list<string>> files; 
unordered_map<string,string> down_part; 
char addressBuffer[INET_ADDRSTRLEN];
string total_partes="";

void ip(){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    
    getifaddrs(&ifAddrStruct);
    const char *addr;
    
    for (ifa = ifAddrStruct->ifa_next; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
        } 
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}


int crear_torrent(string nombre){
	fstream fs;
	ifstream infile(nombre+".txt");
	string line;
	int lines=0;
	int part=1;
	int estado=0;


	while(getline(infile,line)){
		if(lines<255){
			if(estado==0){
				string arch =nombre + to_string(part);
				arch=arch+".txt";	
		  		fs.open (arch.c_str(), fstream::in | fstream::out | fstream::app);
			}
			estado=1;	
			fs <<line+"\n";	
			lines++;
		}else{
			fs <<line+"\n";
			estado=0;		
			part++;
			lines=0;
			fs.close();
		}
	}
	fstream torrent;
	line=nombre+".torrent";
	torrent.open (line.c_str(), fstream::in | fstream::out | fstream::app);
	torrent << nombre+"\n";
	torrent << to_string(part)+"\n";
	torrent.close();
	return part;
}

void publicar(zmsg_t *msg){
	string nombre;
	string port;	
	cout<< "Ingrese nombre del archivo:";
	cin>>nombre;
	cout<< "Ingrese puerto de descarga:";
	cin>>port;
	port= ":"+port;
	port=addressBuffer+port;	
	int num_part=crear_torrent(nombre);
	zmsg_addstr(msg,"publicar"); 	
	zmsg_addstr(msg,nombre.c_str());
	zmsg_addstr(msg,to_string(num_part).c_str());
	zmsg_addstr(msg,port.c_str());	
}

int buscar_lista(string arch,int rand_part){
	string aux;
	list<string> inter=files.at(arch);
	list<string>::iterator pos;
	pos = inter.begin();
	while(pos != inter.end()){
		aux.append(*pos);
		if(aux.compare(to_string(rand_part))==0){			
			return 0;		
		}
		aux.clear();
		pos++;	
	}
	return 1;	
}

void descargar(zmsg_t *msg,string nombre,string arch,string num_part){
	int rand_part;
  	srand (time(NULL));	
	while(1){
		rand_part = rand() % stoi(num_part)+1;
		if(buscar_lista(arch,rand_part)==1){
			break;
			}
		}
	string partarch =nombre + to_string(rand_part);
	partarch=partarch+".txt";
	if(down_part.count(partarch)==0){
		zmsg_addstr(msg,"descargar"); 	
		zmsg_addstr(msg,nombre.c_str());
		zmsg_addstr(msg,to_string(rand_part).c_str());
	}
}

void con_peer(string nombre,string parte,string num,zmsg_t *response,string port){
	zmsg_t *msg = zmsg_new();
	
	string file;
	
	int rand_add;
	string dir;
  	srand (time(NULL));
	rand_add = rand() % stoi(num)+1;
	for(int i=1; i<=rand_add;i++){
		dir=zmsg_popstr(response);
	}
	void *down_port = zsocket_new(context, ZMQ_REQ);
	zsocket_connect (down_port, "tcp://%s",dir.c_str());
	zmsg_addstr(msg,nombre.c_str());
	zmsg_addstr(msg,parte.c_str());

	string add= addressBuffer;
	add=add+":";
	add=add+port;

	zmsg_addstr(msg,add.c_str()); 
	zmsg_send(&msg, down_port);
	zmsg_destroy(&msg);
}


const char* upload(string part){
	ifstream infile(part);
	string line;
	string parte="";
	while(getline(infile,line)){
			parte=parte+line+"\n";
	}
	return parte.c_str();
}

int ciclo_descarga(int numparts,string nombre){
	int cont=1;
	int res=0;
	int val=0;
	while(cont<=numparts){
		string arch =nombre + to_string(cont);
		arch=arch+".txt";
		if(down_part.count(arch)==1){
			res++;
		}
		cont++;
	}
	if(res==numparts){
		val=1;
	}else{
		val=0;
	}
	return val;
}

void joinfiles(string nombre,int num){
	fstream fs;
	fs.open ("descarga.txt", fstream::in | fstream::out | fstream::app);
	string line;
	string part;
	for(int i=1;i<=num;i++){
		string part =nombre+ to_string(i);
		part=part+".txt";
		part="d"+part;
		ifstream infile(part);
		while(getline(infile,line)){
			fs <<line;
	 	}
	}
}

int main (int argc, char **argv){
	printf ("WELCOME TO BITTORRENT.....\n");
	ip();
	int flag=1;

	context = zctx_new();
	void *tracker = zsocket_new(context, ZMQ_REQ);
	zsocket_connect (tracker, "tcp://%s",argv[1]);

	void *peer = zsocket_new(context, ZMQ_REP);
	zsocket_bind (peer, "tcp://*:%s",argv[2]);

	void *peer2 = zsocket_new(context, ZMQ_REP);
	zsocket_bind (peer2, "tcp://*:%s",argv[3]);

	string opcion;


	zmq_pollitem_t items [] = {
		{peer, 0, ZMQ_POLLIN, 0},
		{peer2, 0, ZMQ_POLLIN, 0},
		{tracker, 0, ZMQ_POLLIN, 0},
		{NULL, STDIN_FILENO, ZMQ_POLLIN, 0}
	};
	
	while(1){
		if(flag==1){
			cout<<"[1] Publicar torrent"<<endl;
			cout<<"[2] Descargar"<<endl;
			cout<<"[3] Salir"<<endl;
			cout<<"Elija una opcion:"<<endl;
		
		}
		
		zmq_poll(items, 4, -1);
		if (items[0].revents & ZMQ_POLLIN) {
			zmsg_t *msg = zmsg_new();
			zmsg_t *resp = zmsg_new();
			zmsg_t *aux = zmsg_new();
			zmsg_addstr(aux,"ok");
			msg = zmsg_recv(peer);
			zmsg_send(&aux, peer);
			zmsg_dump(msg);
			string nombre = zmsg_popstr(msg);
			string parte = zmsg_popstr(msg);
			string dir = zmsg_popstr(msg);

			zmsg_addstr(resp,nombre.c_str());
			zmsg_addstr(resp,parte.c_str());					
			zmsg_addstr(resp,dir.c_str());
			zmsg_addstr(resp,upload(parte));
			//zmsg_dump(resp);

			void *up_port = zsocket_new(context, ZMQ_REQ);
			zsocket_connect (up_port, "tcp://%s",dir.c_str());
			zmsg_send(&resp, up_port);
			zmsg_destroy(&resp);
			zmsg_destroy(&msg);
		}

		if (items[1].revents & ZMQ_POLLIN) {
			zmsg_t *msg = zmsg_new();
			zmsg_t *response = zmsg_new();
			zmsg_t *response2 = zmsg_new();
			zmsg_t *aux = zmsg_new();
			zmsg_t *msg2 = zmsg_new();

			zmsg_addstr(aux,"ok");
			msg = zmsg_recv(peer2);
			zmsg_send(&aux, peer2);
			//zmsg_dump(msg);

			string nombre = zmsg_popstr(msg);
			string parte = zmsg_popstr(msg);
			string dir = zmsg_popstr(msg);
			string file = zmsg_popstr(msg);

			fstream fs;
			string arch="d";
			arch=arch+parte;	
			fs.open (arch.c_str(), fstream::in | fstream::out | fstream::app);
			fs <<file;
			fs.close();
			files.at(nombre).push_back(parte);
			down_part.insert(make_pair(parte,"ok"));
			string port=argv[2];
			string add= addressBuffer;
			add=add+":";
			add=add+port;
			
			zmsg_addstr(response,"update");
			zmsg_addstr(response,nombre.c_str());
			zmsg_addstr(response,parte.c_str());
			zmsg_addstr(response,add.c_str());


			//void *trackact = zsocket_new(context, ZMQ_REQ);
			//zsocket_connect (trackact, "tcp://%s",argv[1]);			
			zmsg_send(&response, tracker);
			response2 = zmsg_recv(tracker);
			int cantidad_par=stoi(total_partes);
			if(ciclo_descarga(cantidad_par,nombre)==0){
				int rand_part;
  				srand (time(NULL));
				rand_part = rand() % stoi(total_partes)+1;
				string partearch=nombre+to_string(rand_part)+".txt";
				while(down_part.count(partearch)==1){
					rand_part = rand() % stoi(total_partes)+1;
					partearch=nombre+to_string(rand_part)+".txt";
				}
				
				zmsg_addstr(msg2,"descargar"); 	
				zmsg_addstr(msg2,nombre.c_str());
				zmsg_addstr(msg2,to_string(rand_part).c_str());;
				//zmsg_dump(msg2);
				zmsg_send(&msg2, tracker);
			}else{
				cout<<"Descarga completa"<<endl;
				flag=1;				
				joinfiles(nombre,cantidad_par);
			}

			zmsg_destroy(&msg);
			zmsg_destroy(&msg2);
			zmsg_destroy(&response);
		}


		if (items[2].revents & ZMQ_POLLIN) {
			string port;
			zmsg_t *msg =zmsg_new();
			zmsg_t *response = zmsg_new();
			msg = zmsg_recv(tracker);

			char *operation = zmsg_popstr(msg);
		
			if(strcmp(operation,"Hecho") == 0){
				    printf("Server said: %s\n",operation);
				    
			}else if(strcmp(operation,"update") == 0){
			    	  	printf("Server said: %s\n",operation); 
			}else{		
				port=argv[3];

				string nombre = zmsg_popstr(msg);
				string parte = zmsg_popstr(msg);
				string num = zmsg_popstr(msg);
				zmsg_t *respaldo =zmsg_new();
				respaldo=zmsg_dup(msg); 
				con_peer(nombre, parte,num,respaldo,port);
				zmsg_destroy(&respaldo);
			}
			zmsg_destroy(&msg);
			zmsg_destroy(&response);	
		}

				
			
		if (items[3].revents & ZMQ_POLLIN) {
			string input;
			string port;
			cin>>input;
			if(input == "3") {
				break;
			}
			zmsg_t *msg = zmsg_new();
			char* respuesta;
			switch (stoi(input)){
				case 1: 	
					publicar(msg);
					zmsg_send(&msg, tracker);
					zmsg_destroy(&msg);	
					break;
				case 2:{
					string nombre;
					string arch;
					string num_part;
					list<string> partes;

					cout<<"Ingrese el nombre del torrent:";
					cin>>nombre;

					ifstream infile(nombre+".torrent");
					getline(infile,arch);
					getline(infile,num_part);
					if(files.count(arch)==0){
						files.insert(make_pair(arch,partes));
					}

					total_partes=num_part;

					zmsg_t *msg2 = zmsg_new();
					descargar(msg2,nombre,arch,num_part);
					zmsg_send(&msg2, tracker);
					zmsg_destroy(&msg2);
					flag=0;
					break;
				}
				default:
					break;

			}
		}	
	}		
	zsocket_destroy(context, peer);
	zsocket_destroy(context, peer2);
	zsocket_destroy(context, tracker);
	zctx_destroy(&context);
        return 0;
}
		
