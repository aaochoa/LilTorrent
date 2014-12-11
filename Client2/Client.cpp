#include <bits/stdc++.h>
#include <czmq.h> 
#include <dirent.h>
#include <SFML/Audio.hpp>

using namespace std; 

const char * Tpath;
char* Path1; // se usará
string Dir; // se usará 
int Time;
int Cont1=0; // Total partes a descargar
int Cont2=1; // Total partes descargadas
mutex Save;

/* 
    -El cliente enviará al comenzar su primera ejecución una petición para conocer la lista de reproducción general.
    -La lista de reproducción se guardará según un usuario y se hará de manera local(por ahora sin criptar) --> pensar en el sistema
    -Deberá tener hilos para que; mientras escuche una canción pueda buscar otra o accer de a un menú de opciones.
    -Si se tiene un archivo completo se debe enviar el nombre y las partes que constituyen al archivo.

*/
 
typedef vector<string> Pl; // PlayList.
Pl Playlist;
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
unordered_map<string,int> DownloadList; // Se guardara [FILE][Npartes]  se mantendrán las partes que se descargan
vector<string>PartList; // Guardo las partes individuales sin archivos completos
unordered_map<string,unordered_map<string,vector<char*>>> MasterList;
unordered_map<string,vector<char *>> PartDirList; // 
vector<pair<string,int>> DownloadQueue; // [File.7z.00][partes]

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
vector<string>FileList;
sf::Music music;
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

typedef vector<string> A ; // Guardamos lo
A ListAux;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
template<template <typename> class P = std::less >
struct compare_pair_second {
    template<class T1, class T2> bool operator()(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right) {
        return P<T2>()(left.second, right.second);
    }
};
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


void MyCompleteFiles(zmsg_t* msg){ // complejidad muuuuuuuuuy alta, reducir con cola de prioridad o alguna otra estructura
  // Funcion que determina cuantas partes posee un archivo, si este está completo.
  int cont=0;
  for(int i=0;i<FileList.size();i++){    
       int Pos=FileList[i].find(".7z.");
          if(Pos==string::npos){
            string File=FileList[i];
            zmsg_addstr(msg, FileList[i].c_str());
            int Est=File.find(".");
             if (Est != string::npos){
                          string Aux=File;
                          Aux.erase(Est, Aux.length());                          

                          for(int j=0;j<FileList.size();j++){ 

                            if(FileList[j].find(Aux+".7z.")!=string::npos){
                                cont++;
                              }
                    }
              }                      
              
                     zmsg_addstr(msg, to_string(cont).c_str()); 
                     cont=0; 
          }
    } 
}


void FindParts(zmsg_t* msg){ //complejidad alta :|. ----> arreglada
  // Función que lista las partes de un archivo no completo 

  for(int i=0;i<FileList.size();i++){
      string Compare=FileList[i];
      zmsg_t* msg2 = zmsg_dup(msg);
      int size=zmsg_size(msg2);

      int Est=FileList[i].find(".");
      if(Est != string::npos){
          string Aux=FileList[i];
          Aux.erase(Est, Aux.length()); 

       if(size==0){ // caso en que todo sea partes
           for(int j=0;j<FileList.size();i++){
           PartList.push_back(FileList[i]);
         }
           break;
      }
        int cont=0;
        string File=Aux;
        while(size>0){          
          string AuxFile(zmsg_popstr(msg2));
          string Numbers(zmsg_popstr(msg2));
          int Ast=AuxFile.find(File);           
          if(Ast != string::npos){
            cont++;
          }           
          size=size-2;
        }

        if(cont==0){
          PartList.push_back(FileList[i]);
          cout<<"Partes Sin completar "<<FileList[i]<<endl;
        }
        cont=0;    
       
    }
  }



}


void RegPeer(void* Tracker,string DirNode){
  
  // Enviamos mensaje para que el Traker sepa de nuestra existencia, además enviar nuestra dirección.
  // Nota : Dirnode es ip:puerto
  
  zmsg_t* regmsg = zmsg_new();
  zmsg_addstr(regmsg, "PeerReg");
  zmsg_addstr(regmsg, DirNode.c_str()); 
  MyCompleteFiles(regmsg); // enviamos el archivo con las partes que lo contienen // revisar en el tracker si está vacio esta parte del mensaje
  FindParts(regmsg); // por si tiene partes cuando inicia el programa.
  zmsg_send(&regmsg, Tracker);
  zmsg_destroy(&regmsg);
}


void QuerySearch(void* Tracker){
  // Función que pide resultados a partir de un criterio de búsqueda.
  cout<<":::::::::::::::::::::::::::::::::::"<<endl;
  cout<<"::  Ingrese Indicio de búsqueda  ::"<<endl;
  cout<<":::::::::::::::::::::::::::::::::::"<<endl;
  string op;
  cin>> op;
  zmsg_t* msg= zmsg_new();
  zmsg_addstr(msg,"Search");
  zmsg_addstr(msg,op.c_str());
  zmsg_send(&msg, Tracker);
  zmsg_destroy(&msg); 

}


void QueryListFile(void* Tracker){
  cout<<":::::::::::::::::::::::::::::::::::"<<endl;
  cout<<"::  Ingrese Archivo A Descargar  ::"<<endl;
  cout<<":::::::::::::::::::::::::::::::::::"<<endl;
  string op;
  cin>> op;
  zmsg_t* msg= zmsg_new();
  zmsg_addstr(msg,"ListFile");
  zmsg_addstr(msg,op.c_str());
  zmsg_send(&msg, Tracker);
  zmsg_destroy(&msg); 

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


void BuildMasterList(zmsg_t* msg){

  string File(zmsg_popstr(msg));    
  int size=zmsg_size(msg)/2; //Debido a que es un par  [Song|Dir]  
  DownloadList[File]=size; // Partes de un archivo.

  while(size>0){
    string Part(zmsg_popstr(msg));   
    char * Peer=zmsg_popstr(msg);   
    MasterList[File][Part].push_back(Peer);
    PartDirList[Part].push_back(Peer); // Auxiliar Para disminuir complejidad en búsquedas.     
    size--;    
  } 
  PrintMasterList();
  zmsg_destroy(&msg);
 
}  



/*

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// ideal usar un mapa[mapa[vector(pairs)]] --> pair garantiza que se mande la parte "más rara" o menos saludable :D
// mapa[song:[id_parte:[dir_nodosquelacontengan,númerodenodos]]] // usar métodos de inserción en orden
// Usar otro mapa que contenga [song][numerodepartes]  --> confimo en el mapa anterior cuantas partes hay con un sub string búsqueda :D

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

*/


void SendFile(void* Peers,string Fname,string path1 , string path2 , zframe_t* id ){
  
    // Se enviará el archivo a cada peer 
      
  int tim=rand() % 10;
  cout<<"---------------Rand segundos: "<<tim<<endl;
  sleep(tim);
  


  zmsg_t* Reqmsg= zmsg_new();
  cout << "Requested fname: " << Fname << endl;

  zfile_t *file = zfile_new(path1.c_str(),Fname.c_str());
  //cout << "File is readable? " << zfile_digest(file) << endl;
  zfile_close(file);  
  zchunk_t *chunk = zchunk_slurp(path2.c_str(),0);
  if(!chunk) {
    cout << "Cannot read file!" << endl;
    //return 1;
  }
  cout << "Chunk size: " << zchunk_size(chunk) << endl;

  zframe_t *frame = zframe_new(zchunk_data(chunk), zchunk_size(chunk));
  zmsg_prepend(Reqmsg,&frame);
  zmsg_prepend(Reqmsg,&id);
  zmsg_send(&Reqmsg,Peers);
  //zframe_send(&frame, Peers, 0);
 
  //return 0;
}


void QueryFile(zctx_t* context,string Part,string File,void * Tracker,string DirPeer){ 

      int VecSize=PartDirList[Part].size();      
      int index=rand() % VecSize  ;   
      char * PeerDir = PartDirList[Part][index];       
      void* Peer = zsocket_new(context,ZMQ_DEALER);
      int o = zsocket_connect(Peer,PeerDir);     
      
      zmsg_t* sendmsg= zmsg_new();
      zmsg_addstr(sendmsg,Part.c_str());
      zmsg_print(sendmsg);
      zmsg_send(&sendmsg,Peer);
      
      zfile_t *download = zfile_new("./Temp", Part.c_str());
      zfile_output(download);

      int i = 0;
      while(true){       
        zmsg_t* Recmsg = zmsg_recv(Peer);             
        zframe_t *filePart = zmsg_pop(Recmsg);
        cout << "Recv " << i << " of size " << zframe_size(filePart) << endl;
        zchunk_t *chunk = zchunk_new(zframe_data(filePart), zframe_size(filePart)); 
        zfile_write(download, chunk, 0);
        if(!zframe_more(filePart)) break;
        i++;
      }
      zfile_close(download);
      cout << "Complete " << zfile_digest(download) << endl;
     
      Save.lock();      
      cout<<"::::::::::::::::::::::::::::::::::::"<<endl;
      cout<<"!!Parte: "<<Cont2++<<"!!De: "<<Cont1<<endl;      
      cout<<"::::::::::::::::::::::::::::::::::::"<<endl;
      if(Cont1<Cont2){Cont1=0;Cont2=0;}
      Save.unlock();

      // ----------------------------- Mensaje de confirmación 
      zmsg_t* Regpartmsg= zmsg_new();
      zmsg_addstr(Regpartmsg,"NewPart");
      zmsg_addstr(Regpartmsg,DirPeer.c_str());
      zmsg_addstr(Regpartmsg,File.c_str());
      zmsg_addstr(Regpartmsg,Part.c_str());
      //zmsg_print(Regpartmsg);
      zmsg_send(&Regpartmsg,Tracker);
      zsocket_disconnect(Peer,PeerDir);

}


void HealthyPart(){
  // Función que llena el vector DownloadQueue con el número de peer que contienen todas las partes.
    Save.lock();
    for ( auto it = MasterList.begin(); it != MasterList.end(); it++ ){
          //cout << " Cancion: " << it->first<<endl;
        for ( auto it2 = it->second.begin(); it2 != it->second.end(); it2++ ){
             DownloadQueue.emplace_back(it2->first,it2->second.size());             
      }
    }
    Save.unlock();
    // Ordenamos vector para descargar las partes menos saludables de todos nuestros archivos.
    sort(DownloadQueue.begin(), DownloadQueue.end(), compare_pair_second<>());

 
 //   for(int i = 0; i < DownloadQueue.size(); i++){
   //  cout << DownloadQueue[i].first << ", " << DownloadQueue[i].second << endl;
    //}
}
     

void QuerySend(zctx_t* context,string File,void * Tracker,string DirPeer){
  // Función que genera hilos a partir de cada parte de un archivo

  Save.lock();
  int TamaArchivos = DownloadQueue.size(); 
  Save.unlock();

  for(int i=0;i<TamaArchivos;i++){ // Creamos tantos hilos como partes XD
      // hamos pop para asegurar integridad y que no se repitan descargas
      
      thread Concurrency1(QueryFile,context,DownloadQueue[i].first,File,Tracker,DirPeer);
      Concurrency1.detach();
      
      // Eliminamos luego de crear hilos los elementos del vector
    
     // QueryController[i]=Concurrency1;
    }
    // ojo 
    Cont1=Cont1+TamaArchivos;
    DownloadQueue.clear(); 
}


void handlePeersMessage(zmsg_t* msg ,void* Tracker,void* Peers){
  /* 
    - 
  */

  cout << "Handling the following Peer" << endl;
  zmsg_print(msg);
  zmsg_t * incmsg=zmsg_dup(msg);

  zframe_t* id = zmsg_pop(incmsg);
  string Fname= zmsg_popstr(incmsg);
  
  thread A(SendFile,Peers,Fname,"./Temp/", "./Temp/"+Fname , zframe_dup(id));
  A.detach();
  Fname.clear();
  zmsg_destroy(&incmsg);
  zmsg_destroy(&msg);

}


void handleTrackerMessage(zmsg_t* msg,zctx_t* context,void* Tracker,string DirPeer){
 /*
    - Recibe Lista de Archivos global: 
      [Song|DirServer...........] 
    - Recibe luego de una consulta para reanudación o descarga el número de partes de un archivo 
  */

  cout << "Handling the following Tracker" << endl;
  zmsg_print(msg);
  char* opcode = zmsg_popstr(msg);
  if (strcmp(opcode, "SearchResult") == 0) {    
  // Se mostrará lo que corresponda a las búsquedas
    
    cout<<"::::::::::::::::::::::::::::::::::::::"<<endl;
    cout<<":::Lista de Archivos Con Su Indicio:::"<<endl;
    cout<<"::::::::::::::::::::::::::::::::::::::"<<endl;
    zmsg_print(msg);

  } else if(strcmp(opcode, "FileList") == 0){  
    BuildMasterList(zmsg_dup(msg));
    // empezamos a descargar
    string File(zmsg_popstr(msg));  
    HealthyPart();
    QuerySend(context,File,Tracker,DirPeer);

  }else{
    cout << "Unhandled message" << endl;
  }
  cout << "End of handling" << endl;
  free(opcode);
  zmsg_destroy(&msg);
 
}   


void SplitFiles(string Dir){
  for(int i=0;i<FileList.size();i++){
      int Pos=FileList[i].find(".7z."); // para que no parta algo que ya está dividido
          if(Pos==string::npos){
              // le sacamos la extensión a todos los archivos completos
              int Est=FileList[i].find(".");
              if (Est != std::string::npos){
                  string Aux=FileList[i];
                  Aux.erase(Est, Aux.length());
                
              string sentence = "7z a -v1m -mx0 "+ Dir+"/"+Aux+".7z "+Dir+"/"+FileList[i];
              system(sentence.c_str());
              cout<<"sentence: "<<sentence<<endl;
              //  system("7z a -v1m -mx0 song.7z Song.mp3 ");             
              }
              
           } 
      }
}


void Disconnect(void* Tracker, string DirNode){
  zmsg_t* Discmsg = zmsg_new();
  zmsg_addstr(Discmsg, "DiscNode");
  zmsg_addstr(Discmsg, DirNode.c_str()); 
  MyCompleteFiles(Discmsg); // enviamos el archivo con las partes que lo contienen // revisar en el tracker si está vacio esta parte del mensaje
  //FindParts(regmsg); // por si tiene partes cuando inicia el programa.
  zmsg_send(&Discmsg, Tracker);
  zmsg_destroy(&Discmsg);
}

// Reanudar descarga : mirar si no hay un archivo completo, sincronizar con el traquer, esperar lista con partes faltantes , recibir partes 
 


int DirFiles(string path){ 
  /*
      - Lista y Envía la lista de canciones, sus partes : [Canción][Parte001][Canción][Parte002]
      Problemas = sacar de una parte el nombre de la canción
  */
  FileList.clear();
  DIR *dir;
  struct dirent *ent;
    if ((dir = opendir (path.c_str())) != NULL) {
        while((ent = readdir (dir)) != NULL) {
                string Part_Name = ent->d_name;               
                if(Part_Name.compare(".")!=0 && Part_Name.compare("..")!=0){
                   if(Part_Name.compare(".mediaartlocal")!=0){
                      cout<<Part_Name<<endl;                     
                      FileList.push_back(Part_Name);

                      }                               
                  }         
                
              }
              closedir (dir);
               
      } else {             
              perror ("");
              return EXIT_FAILURE;
             }

        cout<<"Archivos Listados!!...."<<endl;            
}

void PollItems(void * Tracker , void * NodeListener ,zmq_pollitem_t items[],zctx_t* context,string DirPeer){

    while(true){
    zmq_poll(items, 2, 10 * ZMQ_POLL_MSEC);
    if(items[0].revents & ZMQ_POLLIN){
      cerr << "From Tracker \n";
      zmsg_t* msg = zmsg_recv(Tracker);
      handleTrackerMessage(msg,context,Tracker,DirPeer);
    }
     if(items[1].revents & ZMQ_POLLIN){
      cerr << "From Peers\n";
      zmsg_t* msg = zmsg_recv(NodeListener);
      handlePeersMessage(msg,Tracker,NodeListener);
    }
  }

}


int main(int argc, char** argv){

     if (argc != 7) {
    cerr << "Wrong call\n";
    return 1;
  }
  
  // [Dirtracker][PortTracker][NodoActualDir][NodoActualPort][DirFiles][Delay]
  // ./Client localhost 5555 localhost 6666 Temp 5
  string TrackerDir=argv[1];
  string TrackerPort=argv[2];
  string NodeDir=argv[3];
  string NodePort=argv[4];

  string TrackerConnect="tcp://"+TrackerDir+":"+TrackerPort; 
  string NodeListenerConnect="tcp://*:"+NodePort;
  string NodeDirSite="tcp://"+NodeDir+":"+NodePort; 

  Tpath= argv[5];
  cout<<"Path: "<<Tpath<<endl;
  int Time = atoi(argv[6]);

  zctx_t* context = zctx_new();

  void* Tracker = zsocket_new(context, ZMQ_DEALER);
  int a = zsocket_connect(Tracker, TrackerConnect.c_str());
  cout << "connecting to Tracker: "<<TrackerConnect << (a == 0 ? " OK" : "ERROR") << endl;
  cout << "Listening! Tracker" << endl;

  void* NodeListener = zsocket_new(context, ZMQ_ROUTER);
  int b = zsocket_bind(NodeListener,NodeListenerConnect.c_str());
  cout << "Listening! Nodes at : "<<NodeListenerConnect << (b == 0 ? " OK" : "ERROR") << endl; 
  

  zmq_pollitem_t items[] = {{Tracker, 0, ZMQ_POLLIN, 0},
                            {NodeListener, 0, ZMQ_POLLIN, 0}};

  DirFiles(Tpath);                          
  SplitFiles(Tpath);
  DirFiles(Tpath);                           
  RegPeer(Tracker,NodeDirSite);

  thread Poll(PollItems,Tracker,NodeListener,items,context,NodeDirSite);
  Poll.detach();
  
  for(int i=0;i<FileList.size();i++){
      cout<<"Item ["<<i<<"]: "<<FileList[i]<<endl;
     }
  int op=0;   
  while(op!=7){   
  cout<<"LL      iii lll  '     TTTTTTT                                     tt"<<endl;    
  cout<<"LL          lll '''      TTT    oooo  rr rr  rr rr    eee  nn nnn  tt"<<endl;    
  cout<<"LL      iii lll ''       TTT   oo  oo rrr  r rrr  r ee   e nnn  nn tttt"<<endl;  
  cout<<"LL      iii lll          TTT   oo  oo rr     rr     eeeee  nn   nn tt"<<endl;    
  cout<<"LLLLLLL iii lll          TTT    oooo  rr     rr      eeeee nn   nn  tttt"<<endl; 
  cout<<"::::::::::::::::::::::::::::::"<<endl;
  cout<<"::::::::     MENU    :::::::::"<<endl;
  cout<<"::::::::::::::::::::::::::::::"<<endl;
  cout<<":: 1-> Search :::"<<endl;
  cout<<":: 2-> Download :::"<<endl;
  cout<<":: 3-> Play Song (sin extensión) :::"<<endl;
  cout<<":: 4-> Pause :::"<<endl;
  cout<<":: 5-> Stop :::"<<endl;
  cout<<":: 6-> Play in memory :::"<<endl;
  cout<<":: 7-> Exit :::"<<endl;  
  
  cin>>op;

  
  switch (op){
    case 1 :
    QuerySearch(Tracker);
    break;
    
    case 2 :
    QueryListFile(Tracker);
    break;

    case 3 :
    Save.lock();
    if(Cont1!=Cont2){
      cout<<"No se puede reproducir, canción incompleta , \n ¡espera hasta que se termine de descargar!"<<endl;
      }else{
        string Song;
        cin>>Song;
        string sentence= "7z x Temp/"+Song+".7z.001";
        system(sentence.c_str());
        if(music.openFromFile("Temp/"+Song+".ogg")){
          music.stop();
          music.play();
        }
      }
    Save.unlock();
    break;
    
    case 4 :
    music.pause();
    break;
    
    case 5 :
    music.stop();
    break;


    case 6 :
    music.play();
    break;
         
  }
  
}
  DirFiles(Tpath);        
  Disconnect(Tracker,NodeDirSite);
  

  Poll.~thread();
 
  zctx_destroy(&context);
  return 0;
 
}


