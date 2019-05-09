// Vous devez modifier ce fichier pour le TP 2
// Tous votre code doit être dans ce fichier
// Jean Laprés-Chartrand
// André Lalonde
#include <stdbool.h>
#include <sys/mman.h>
#include <stdio.h>
#include "mymalloc.h"

void *myMallocPetit(short sze);
void *myMallocGros(long sze);
void *generateur_new_page();
void *searchSmall(short size, void* ptr);
short absolute(void* ptr);
void myfreePetit(void *p);
void myfreeGros(void *p);
void reuniteEmptySpace(void *ptr);

void* ptrDebutPetit = NULL;
void* ptrDebutGros = NULL;

void *mymalloc(size_t sze){
  if (sze < 3*1024){
    short size = (short)sze;
    void* mallocPointer = myMallocPetit(size);
    return mallocPointer;
  }
  else{
    long size = (long)sze;
    void* mallocPointer = myMallocGros(size);
    return mallocPointer;
  }
}

//malloc pour les objets de petites tailles
void *myMallocPetit(short size){

  //si petitBlock est null, alors on a pas encore creer de page on la cree.
  if (!ptrDebutPetit){

    //memoire du pointeur de la premiere page afin de pouvoir toujours les retrouver
    ptrDebutPetit = generateur_new_page();
    return myMallocPetit(size);
  }
  else{
    return searchSmall(size, ptrDebutPetit);
  }
}
void *myMallocGros(long sze){
  long longueur = sze+sizeof(short)+sizeof(long)+sizeof(void *);
  long *ptr = mmap(NULL, longueur, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  *ptr = sze;
  long *ptr2 = ptr+1;
  short *ptr3 = (short*)ptr2;
  *ptr3 = -4099;
  void *ptr4 = (void*)ptr3;
  ptr4 = ptr4+ sizeof(short);
  void *ptr5 = ptr4+sze;
  *(void **)ptr5 = ptrDebutGros;
  ptrDebutGros = ptr;
  return ptr4;
}

//recherche d'espace dans les pages pour eviter le plus possible les appels mmap qui sont lentes
void *searchSmall(short size, void* ptr){
  if (-(*((short*)ptr)) > size+(short)sizeof(short)+(short)sizeof(short)) {
    short tmpShort = -(*(short*)ptr);
    *(short*)ptr = size;
    short* ptrTmp = (short*)ptr;
    ptrTmp+=1;
    void* ptrTmp2 = (void*)ptrTmp;
    ptrTmp2+=size;
    *(short*)ptrTmp2 = size - tmpShort + sizeof(short) + sizeof(short);
    return (void*)ptrTmp; //on renvois le pointeur et tout va bien
  }
  //si on est a la fin d'une page
  else if((*((short*)ptr)) == 0){
    short* ptr2 = (short*)ptr+1;
    if(*(void **)ptr2==NULL){
      //le pointeur suivant la valeur zero etais null
      *(long **)ptr2=generateur_new_page();
      return searchSmall(size, *(long **)ptr2);// jai enlever un etoile ici
    }
    else{
      return searchSmall(size, (void *)(*(long **)ptr2));
    }
  }
  //si on a pas asser d'espace ou bien qu'on a une donnee, on check le prochain ptr
  else{
    return searchSmall(size, (void*)(ptr+absolute(ptr) + (short)sizeof(short)));
  }
}

//creation d'une nouvelle page si on a pas d'espace disponible
void * generateur_new_page(){
  void* ptrDebut = NULL;
  void* ptrTmp = NULL;
  //cree une nouvelle page dont qui commence au pointeur ptrTmp 
  short n = 4*1024 - (short)sizeof(short) - (short)sizeof(short) -(short)sizeof(void *);
  ptrDebut = mmap(NULL, 4*1024, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  *((short *) ptrDebut) = -n;
  ptrTmp = ptrDebut +sizeof(short) + n; 
  *(short *)ptrTmp = 0;
  ptrTmp = ptrTmp +sizeof(short);
  *(void **)ptrTmp = NULL;
  return ptrDebut;
}

//maths bro
short absolute(void* ptr){
  short value = *((short*)ptr);
  if (value < 0){
    value = -value;
  }
  return value;
}

void myfree(void *p){
  if(!p){
    return;
  }
  short *p2 = (short *)p;
  short precedant =  *(p2-1);
  if (precedant != -4099){
    myfreePetit(p);
  }
  else{
    myfreeGros(p); 
  }
  return;
}
void myfreePetit(void *p){
  void *p2 = p - sizeof(short);
  void *ptrTmp = ptrDebutPetit;
  while(ptrTmp){//tant que ptrTmp n'est pas NULL
    if(ptrTmp < p && p< (ptrTmp+4096)){//si on est sur la bonne page
      void *ptrTmp2 = ptrTmp; //début de la page
      while(ptrTmp != p2 && *(short *)ptrTmp != 0){//tant qu'on a pas trouver le short qui precede la donnee ou qu'on a trouver un short 0
        ptrTmp = ptrTmp + *(short *)ptrTmp + sizeof(short);
      }
      //si le short est negatif ou 0, on retourne sans rien faire   
      if(*(short *)ptrTmp <= 0){
        return;
      }
      else{
	
        *(short *)ptrTmp = -*(short *)ptrTmp;
        reuniteEmptySpace(ptrTmp2);
        return;
      }
    }
    else{
      ptrTmp = (*(void **)(ptrTmp+4*1024-2*(sizeof(short))-sizeof(void*)));
    }
  }
  return;
}

void myfreeGros(void *p){
  long longueurBlock = *(long *)(p-sizeof(short)-sizeof(long)) + sizeof(short)+sizeof(long)+sizeof(void *);
  if(ptrDebutGros+sizeof(short)+sizeof(long) == p){//si le pointeur a supprimer est le premier block
    void *ptrTmp1 = ptrDebutGros;
    ptrDebutGros = (void *)(*(long **)(ptrDebutGros+sizeof(short)+sizeof(long)+*(long*)ptrDebutGros));
    munmap(ptrTmp1, longueurBlock);
    return;
  }
  void *ptrTmp = ptrDebutGros;
  void *ptrTmp2 = ptrTmp;
  ptrTmp = (void *)(*(long **)(ptrTmp+sizeof(short)+sizeof(long)+*(long*)ptrTmp));
  while(ptrTmp+sizeof(short)+sizeof(long) != p && p){//tant qu'on a pas trouver le pointeur qu'on cherche ou qu'on est pas tomber sur 
                                                      //le pointeur null, on se deplace sur les gros blocks
     ptrTmp2 = ptrTmp;
     ptrTmp = (void *)(*(long **)(ptrTmp+sizeof(short)+sizeof(long)+*(long*)ptrTmp));
  }
  *(long **)(ptrTmp2+sizeof(short)+sizeof(long)+*(long*)ptrTmp2) = (void *)(*(long **)(ptrTmp+sizeof(short)+sizeof(long)+*(long*)ptrTmp));
  munmap(ptrTmp, longueurBlock);
}

void reuniteEmptySpace(void *ptr){
  void *p1 = ptr; //début de la page
  while(*(short *)p1 != 0){
    if (*(short *)p1 < 0){ //si négatif, on regarde s'il y a d'autres négatifs immédiats
      if (*(short *)(p1 - *(short *)p1 + sizeof(short)) < 0){
        *(short *)p1 = *(short *)(p1) + *(short *)(p1 - *(short *)p1 + sizeof(short)) - sizeof(short) - sizeof(short);
      }
      else{
        p1 = (p1 - *(short *)p1 + sizeof(short));
      }
    }
    else{ //si positif, on va au prochain pointeur
      p1 = (p1 + *(short *)p1 + sizeof(short));
    }
  }
  return;
}

