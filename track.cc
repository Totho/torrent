#include <czmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <assert.h>
#include <unordered_map>
#include <list>
#include <utility>
#include <iostream>
#include <algorithm>
#include <iterator>

  
using namespace std;


zctx_t *context;

unordered_map<string,unordered_map<string,list<string>>> textos; 


void trackact(string nombre,string part,string ip,zmsg_t *response){
		cout<<nombre<<endl;	 	
		textos.at(nombre).at(part).push_back(ip);
		zmsg_addstr(response,"update");	
}

const char* track_print(string sudo,string part){


	list<string> inter=textos.at(sudo).at(part);
	string aux="";			
	list<string>::iterator pos;
				pos = inter.begin();
				while(pos != inter.end()){
					aux.append(*pos);
					cout <<part+" "+aux<<endl;
					aux.clear();
					pos++;	
				}
	return aux.c_str();
	}

	
const char* track(string nombre,string num_part,string ip){
	const char *msg;
	unordered_map<string, list<string>> ips;
	list<string>direcciones;
	
	string parte;
	int cont= atoi(num_part.c_str());
	int i=1;
	direcciones.push_back(ip);
	while( i<=cont){
		parte=nombre+to_string(i);
		parte=parte+".txt";
		ips.insert(make_pair(parte,direcciones));
		i++;
	}
	textos.insert(make_pair(nombre,ips));
	msg="Hecho";
	return msg;
}


void track_upload(string nombre,string part,zmsg_t *response){
	string parte=nombre+part;
	parte=parte+".txt";
	list<string> inter=textos.at(nombre).at(parte);
	string aux="";
	zmsg_addstr(response,"down");	
	zmsg_addstr(response,nombre.c_str());
	zmsg_addstr(response,parte.c_str());
	zmsg_addstr(response,to_string(inter.size()).c_str());				
	list<string>::iterator pos;
	pos = inter.begin();
		while(pos != inter.end()){
				aux.append(*pos);
				//cout <<parte+" "+aux<<endl;
				zmsg_addstr(response,aux.c_str());
				aux.clear();
				pos++;	
		}
}

	
void dispatch(zmsg_t *msg, zmsg_t *response){
	assert(zmsg_size(msg) > 0);
	char *operation = zmsg_popstr(msg);
	
	if(strcmp(operation,"publicar") == 0){
		    string nombre = zmsg_popstr(msg);
		    string num_part = zmsg_popstr(msg);
		    string ip = zmsg_popstr(msg);
		    zmsg_addstr(response,"%s",track(nombre,num_part,ip));
		    
	}else if(strcmp(operation,"descargar") == 0){
            	    string nombre = zmsg_popstr(msg);
	    	    string parte = zmsg_popstr(msg);
		    track_upload(nombre,parte,response);	 
	}else if(strcmp(operation,"update") == 0){
		    string nombre = zmsg_popstr(msg);
		    string parte = zmsg_popstr(msg);		
            	    string dir = zmsg_popstr(msg);
		    trackact(nombre,parte,dir,response);
		    track_print(nombre,parte);	
	}else{	
		printf("unknown...\n");
		zmsg_addstr(response,"unknown!");
	}
	free(operation);
	
}
	
int main(void){

	context = zctx_new();
	void *client = zsocket_new(context, ZMQ_REP);
	int pn =zsocket_bind(client, "tcp://*:5559");
	printf("Port number %d\n", pn);
	

	while(1){
		zmsg_t *msg =zmsg_new();
		msg = zmsg_recv(client);
		zmsg_dump(msg);
		
                zmsg_t *response = zmsg_new();
		dispatch(msg,response);		
		zmsg_destroy(&msg);
		
		zmsg_send(&response,client);
		zmsg_destroy(&response);
	}
	zsocket_destroy(context,client);
	return 0;
}
