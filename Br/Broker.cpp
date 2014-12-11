#include <bits/stdc++.h>
#include <czmq.h> 
using namespace std; 

/* 
   - Recibirá cada uno de los mensajes con los archivos sus partes o solo las partes.
   - Enviará la MasterList por búsquedas para mantener actualizado el sistema.
   -  
*/


unordered_map<string,unordered_map<string,vector<char*>>> MasterList; // Contenedor de todos los archivos [[File][[Part][DirPeers]]]
unordered_map<string,int> Totality; // [File][Numero de partes]
vector<zframe_t*>PeerList; // Contenedora id de peers




 // Para guardar la lista de servidores y clientes
typedef unordered_map<char * , zframe_t* >R; // 192.168.0.1 343532652362AF
R ServerList;
typedef unordered_map<zframe_t* , int>G; // Guardamos reproducciones globales
G ServerCounter;
int FIX1=0,FIX=0;



// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// ideal usar un mapa[mapa[vector(pairs)]] --> pair garantiza que se mande la parte "más rara" o menos saludable :D
// mapa[song:[id_parte:[dir_nodosquelacontengan,númerodenodos]]] // usar métodos de inserción en orden
// Usar otro mapa que contenga [song][numerodepartes]  --> confimo en el mapa anterior cuantas partes hay con un sub string búsqueda :D

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void RegPeer(zframe_t* id){
 
  //   Registramos Peers para reenviar listas de archivos actualizadas.
 
  cout<<":::RegPeer::::"<<endl;
  zframe_print(zframe_dup(id), "Id to add");
  zframe_t* dup = zframe_dup(id);
  zframe_print(dup, "Id copy add");
  PeerList.push_back(dup);
}

void UnRegPeer(zframe_t* id){
  //   Desregistramos Peers para Remover Participación.
 
  cout<<":::DesRegPeer::::"<<endl;
  zframe_print(zframe_dup(id), "Id to Remove");
  zframe_t* dup = zframe_dup(id);
  zframe_print(dup, "Id copy Remove");

  for(int i=0;i<PeerList.size();i++){
    if(zframe_eq(PeerList[i],id)==true){
     PeerList.erase(PeerList.begin()+i);
     cout<<":::ID eliminado:::"<<endl;
    }
  }
  
}

void PrintMasterList(){
    cout<<"Masterlist"<<endl;
    for ( auto it = MasterList.begin(); it != MasterList.end(); it++ ){
          cout << " Cancion: " << it->first<<endl;
        for ( auto it2 = it->second.begin(); it2 != it->second.end(); it2++ ){
          cout << " Parte: " << it2->first<<endl;
           for ( auto it3 : it2->second ){
          cout << " Peers: " << it3<<endl;
      
      }
    }
  }
}


void ListCompleteFiles(zmsg_t* incmsg,char * DirPeer){
  /*
      - Lista y guarda en MasterList todas los archivos de cada nodo.
  */

    cout<<"List Complete Files"<<endl;
    zmsg_print(incmsg);
    int size=zmsg_size(incmsg)/2; // Tamaño del mensaje en frames (número de canciones) si el mensaje = 0 es que es una vil sanguijuela

    while(size>0){

    string File(zmsg_popstr(incmsg));
    string algo(zmsg_popstr(incmsg));
    int Pieces= stoi(algo);
    Totality[File]=Pieces;
    // Sacamos la extensión
    int Est=File.find(".");
      if (Est != std::string::npos){
          string Aux=File;
          Aux.erase(Est, Aux.length());
                
      for(int i =1;i<=Pieces;i++){
          if(i<10){
            string Part = "00"+to_string(i); // formateamos la entrada 
            string Str = Aux+".7z."+Part; // Se arma la lista 
            MasterList[File][Str].push_back(DirPeer);
          }

          if(i>=10 && i<100){
            string Part = "0"+to_string(i); // formateamos la entrada 
            string Str = Aux+".7z."+Part; // Se arma la lista 
            MasterList[File][Str].push_back(DirPeer);
          }

           if(i>=100 && i<1000){         
            string Str = Aux+".7z."+to_string(i); // Se arma la lista 
            MasterList[File][Str].push_back(DirPeer);
          }                 
            
      }
    }
    
    size--;    
  }     

 PrintMasterList();
}


void RemovePartsOfFiles(zmsg_t* incmsg,char * DirPeer){
    
    cout<<"List Files To Remove"<<endl;
    zmsg_print(incmsg);
    int size=zmsg_size(incmsg)/2; // Tamaño del mensaje en frames (número de canciones) si el mensaje = 0 es que es una vil sanguijuela
    char * DirP=DirPeer;
    
    while(size>0){

    string File(zmsg_popstr(incmsg));
    string algo(zmsg_popstr(incmsg));
    int Pieces= stoi(algo);
    
    // Sacamos la extensión
    int Est=File.find(".");
      if (Est != std::string::npos){
          string Aux=File;
          Aux.erase(Est, Aux.length());
                
      for(int i =1;i<=Pieces;i++){
          
          if(i<10){
            string Part = "00"+to_string(i); // formateamos la entrada 
            string Str = Aux+".7z."+Part; // Se arma la lista 
            if(MasterList[File][Str].size()==1){Totality.erase(File);MasterList.erase(File);              

            } // removemos la entrada si se desconecta el único peer que contiene ese archivo
            
           else{   



            for(int j=0;j<MasterList[File][Str].size();j++){
              if(MasterList[File][Str].size()==1){
                MasterList[File].erase(Str);
              }
              
              cout<<"----->Count: "<<MasterList[File][Str][j]<<endl;
              if(strcmp(MasterList[File][Str][j],DirPeer)==0){
                cout<<"Se cumplio :D"<<endl;
                cout<<"vec"<<MasterList[File][Str][j]<<endl;
                cout<<"Dir"<<DirPeer<<endl;
                MasterList[File][Str].erase(MasterList[File][Str].begin()+j);   
              }
            }
          }  
           
        }

          if(i>=10 && i<100){
            string Part = "0"+to_string(i); // formateamos la entrada 
            string Str = Aux+".7z."+Part; // Se arma la lista 
            if(MasterList[File][Str].size()==1){Totality.erase(File);} // removemos la entrada si se desconecta el único peer que contiene ese archivo
            
         
            
          }

           if(i>=100 && i<1000){         
            string Str = Aux+".7z."+to_string(i); // Se arma la lista 
            if(MasterList[File][Str].size()==1){Totality.erase(File);} // removemos la entrada si se desconecta el único peer que contiene ese archivo
                        
           
          }  
      }
    }
    
    size--;    
  }     
 PrintMasterList();
}


void Unicast(zmsg_t* msg ,void* Peers,zframe_t * PeerId){
      zmsg_t* msg2 = zmsg_dup(msg);
      zmsg_prepend(msg2,&PeerId);
      zmsg_print(msg2);
      zmsg_send(&msg2,Peers);
      zmsg_destroy(&msg2);
}


void SearchResult(zmsg_t* msg,void* Peers,zframe_t* id){
  zmsg_t* nmsg=zmsg_new();  
  string Key=zmsg_popstr(msg); // sacamos criterio de búsqueda
  zmsg_addstr(nmsg,"SearchResult");


  for (auto it = Totality.begin(); it != Totality.end(); it++ ){          
          int Est=it->first.find(Key);
          if (Est != std::string::npos){
              cout << " Archivo: " << it->first<<endl;
              zmsg_addstr(nmsg,it->first.c_str());
            }

      }

      Unicast(nmsg,Peers,id); // difundimos lista de archivos según su criterio al cliente 

}


void BuildFileList(string File,void* Peers,zframe_t* id){
  // Función que crea la lista de partes de un archivo 
  zmsg_t* nmsg=zmsg_new();
  zmsg_addstr(nmsg,"FileList");
  zmsg_addstr(nmsg,File.c_str());  

  for(auto it = MasterList[File].begin(); it != MasterList[File].end(); it++ ){       
           for(auto it3 : it->second ){
             zmsg_addstr(nmsg, it->first.c_str());
             zmsg_addstr(nmsg, it3);
               
      }
    }
  
  Unicast(nmsg,Peers,id); // difundimos lista de archivos según su criterio al cliente    
}

void RegNewParts(zmsg_t* msg){
  char* DirPeer = zmsg_popstr(msg);
  string File = zmsg_popstr(msg);
  string Part = zmsg_popstr(msg);

  MasterList[File][Part].push_back(DirPeer);
  cerr<<"Agregada nueva parte de "<<DirPeer<<endl;

}

void handlePeersMessage(zmsg_t* msg,void* Peers){
 /*
    - Lista de archivos global: 
        Se reenviarán a los clientes la lista de los archivos entregada los peers[solo 1 vez].
        zmsg [id_cliente|@Nombre_de_la_cancion *servido(es) ..... ]
  */
  cout << "Handling the following Peer" << endl;
  zmsg_print(msg);
  zmsg_t * incmsg=zmsg_dup(msg);

  zframe_t* id = zmsg_pop(incmsg); 
  char* opcode = zmsg_popstr(incmsg);

  if(strcmp(opcode, "PeerReg") == 0){ 
      RegPeer(id);     
      char* DirPeer = zmsg_popstr(incmsg);    
      ListCompleteFiles(incmsg,DirPeer);

  }else if(strcmp(opcode, "Search") == 0){
     SearchResult(incmsg,Peers,zframe_dup(id));  

  } else if(strcmp(opcode, "ListFile") == 0){
      // Generamos La lista [File][Part][Node] y se la enviamos al nodo que la pidio
    string File(zmsg_popstr(incmsg));
    BuildFileList(File,Peers,zframe_dup(id));

  } else if(strcmp(opcode, "NewPart") == 0){
      // Reg nuevas partes.
      RegNewParts(incmsg);

  } else if(strcmp(opcode, "DiscNode") == 0){
      // Eliminamos partes de un nodo.
      UnRegPeer(id);     
      char* DirPeer = zmsg_popstr(incmsg);    
      RemovePartsOfFiles(incmsg,DirPeer);

  }else {
    cout << "Unhandled message" << endl;
  }
    cout << "End of handling" << endl;

  zframe_destroy(&id);
  zmsg_destroy(&msg);


}   



int main(int argc, char** argv){

   if (argc != 2) {
    cerr << "Wrong call\n";
    cout<<argc<<endl;
    return 1;
  }

  string port1 =argv[1]; 


  string PeerSite="tcp://*:"+ port1;
  


  zctx_t* context = zctx_new();
  void* Peers = zsocket_new(context, ZMQ_ROUTER);
  int PsPort = zsocket_bind(Peers, PeerSite.c_str());
  cout << "Listen to Peers at: "
       << "localhost:" << PsPort << endl;

  zmq_pollitem_t items[] = {{Peers, 0, ZMQ_POLLIN, 0}};

  cout << "Listening!" << endl;


  while (true) {
    zmq_poll(items, 1, 10 * ZMQ_POLL_MSEC);
    if (items[0].revents & ZMQ_POLLIN) {
      cerr << "From Peers\n";
      zmsg_t* msg = zmsg_recv(Peers);
      handlePeersMessage(msg,Peers);
    }
 
  }
  cout<<":::::::::::::::::::::::::::"<<endl;
  zctx_destroy(&context);
	return 0;
}
//          Ppeers 
// ./Broker 5555 
