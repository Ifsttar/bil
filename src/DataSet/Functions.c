#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Message.h"
#include "DataFile.h"
#include "Functions.h"


static int    lit_fonction(Function_t*,char*) ;


Functions_t* (Functions_Create)(DataFile_t* datafile,Materials_t* materials)
{
  int n_mats = Materials_GetNbOfMaterials(materials) ;
  int    i ;
  int    i_fn ;
  Functions_t* functions   = (Functions_t*) malloc(sizeof(Functions_t)) ;
  int n_fncts ;
  
  if(!functions) arret("Functions_Create") ;

  for(i = 0 ; i < n_mats ; i++) {
    Material_t* mat = Materials_GetMaterial(materials) + i ;
    Material_GetFunctions(mat) = functions ;
  }
  
  DataFile_OpenFile(datafile,"r") ;
  
  DataFile_SetFilePositionAfterKey(datafile,"FONC,FUNC,Functions",",",1) ;
  
  Message_Direct("Enter in %s","Functions") ;
  Message_Direct("\n") ;

  {
    char* line = DataFile_ReadLineFromCurrentFilePosition(datafile) ;
  
    n_fncts = atoi(line) ;
  }
  
  Functions_GetNbOfFunctions(functions) = n_fncts ;
  Functions_GetFunction(functions) = NULL ;
  
  if(n_fncts <= 0) return(functions) ;

  {
    Function_t* function = (Function_t*) malloc(n_fncts*sizeof(Function_t)) ;
    
    if(!function) arret("Functions_Create (1) : impossible d\'allouer la memoire") ;
    
    Functions_GetFunction(functions) = function ;
  }

  for(i_fn = 0 ; i_fn < n_fncts ; i_fn++) {
    Function_t* function = Functions_GetFunction(functions) + i_fn ;
    char* line = DataFile_ReadLineFromCurrentFilePosition(datafile) ;

    
    if(!strncmp(line,"Ntimes",1)) {
      char* pline = strchr(line,'=') + 1 ;
      int n_tm ;
      int j ;
      
      n_tm = atoi(pline) ;
      
      {
        Function_t* fct = Function_New(n_tm) ;
        
        *function = *fct ;
      }
      
      /* Read the F(T) */
      for(j = 0 ; j < n_tm ; j++) {
        pline = strstr(pline,"F(") ;
        
        if(!pline) {
          pline = DataFile_ReadLineFromCurrentFilePosition(datafile) ;
          pline = strstr(pline,"F(") ;
        }
        
        if(pline) {
          sscanf(pline,"F(%lf)",Function_GetXValue(function) + j) ;
          pline = strchr(pline,'=') + 1 ;
          sscanf(pline,"%lf ",Function_GetFValue(function) + j) ;
        } else {
          arret("Functions_Create(3): not enough data") ;
        }
      }

    } else if(!strncmp(line,"File",1)) {
      char* pline = strchr(line,'=') + 1 ;
      char name[Function_MaxLengthOfFileName] ;
      
      if(strlen(pline) > Field_MaxLengthOfFileName) {
        arret("Functions_Create(5): too long file name") ;
      }
      
      sscanf(pline,"%s",name) ;
      
      i_fn += lit_fonction(function,name) - 1 ;
      
      if(i_fn >= n_fncts) arret("Functions_Create : trop de fonctions") ;
      
    } else {
      arret("Functions_Create(5): mot non connu") ;
    }
  }
  
  DataFile_CloseFile(datafile) ;
  
  return(functions) ;
}



/* Intern Functions */

int lit_fonction(Function_t* fn,char* nom)
/* Lecture des fonctions du temps dans le fichier "nom"
   retourne le nb de fonctions lues */
{
  int    n_points,n_fonctions ;
  char   line[Function_MaxLengthOfTextLine],*c ;
  FILE   *fict ;
  int    i ;
  int long pos ;

  fict = fopen(nom,"r") ;
  if(!fict) arret("lit_fonction(1) : immpossible d ouvrir le fichier") ;

  /* nb de fonctions */
  n_fonctions = 0 ;
  do {
    fgets(line,sizeof(line),fict) ;
    c = (char*) strtok(line," \n") ;
    /* } while(c != NULL && *c == '#' && !feof(fict)) ; */
  } while((c == NULL || *c == '#') && !feof(fict)) ;
  while((char*) strtok(NULL," \n") != NULL) n_fonctions++ ;

  /* nb de points */
  n_points = 1 ;
  while(!feof(fict) && fgets(line,sizeof(line),fict)) {
    c = (char*) strtok(line," \n") ;
    if(c != NULL && *c != '#') n_points++ ;
  }

  /* reservation de la memoire */
  {
    double* t = (double*) malloc(n_points*sizeof(double)) ;
    double* f = (double*) malloc(n_fonctions*n_points*sizeof(double)) ;
    if(!t) arret("lit_fonction(2) : impossible d\'allouer la memoire") ;
    if(!f) arret("lit_fonction(3) : impossible d\'allouer la memoire") ;
    
    for(i = 0 ; i < n_fonctions ;i++) {
      Function_GetNbOfPoints(fn + i) = n_points ;
      Function_GetXValue(fn + i) = t ; /* meme temps pour les fonctions */
      Function_GetFValue(fn + i) = f + i*n_points ;
    }
  }

  /* positionnement dans le fichier */
  rewind(fict) ;
  do {
    pos = ftell(fict) ;
    fgets(line,sizeof(line),fict) ;
    c = (char*) strtok(line," \n") ;
    /* } while(c != NULL && *c == '#' && !feof(fict)) ; */
  } while((c == NULL || *c == '#') && !feof(fict)) ;
  fseek(fict,pos,SEEK_SET) ;
  
  
  /* Read time and function values */
  for(i = 0 ; i < n_points ; i++) {
    double* x = Function_GetXValue(fn) ;
    int    j ;
    
    fscanf(fict,"%le",x + i) ;
    
    for(j = 0 ; j < n_fonctions ; j++) {
      double* f = Function_GetFValue(fn + j) ;
      
      fscanf(fict,"%le",f + i) ;
    }
  }

  fclose(fict) ;

  return(n_fonctions) ;
}
