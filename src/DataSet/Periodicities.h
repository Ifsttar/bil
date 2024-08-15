#ifndef PERIODICITIES_H
#define PERIODICITIES_H

#ifdef __CPLUSPLUS
extern "C" {
#endif

/* vacuous declarations and typedef names */

/* class-like structures */
struct Periodicities_s       ; typedef struct Periodicities_s      Periodicities_t ;



#include "DataFile.h"
#include "Mesh.h"
#include "Graph.h"


extern Periodicities_t* (Periodicities_Create)(DataFile_t*) ;
extern Periodicities_t* (Periodicities_New)(const int) ;
extern void             (Periodicities_Delete)(void*) ;
extern void             (Periodicities_EliminateMatrixRowColumnIndexes)(Mesh_t*) ;
extern void             (Periodicities_UpdateMatrixRowColumnIndexes)(Mesh_t*) ;
extern void             (Periodicities_UpdateGraph)(Mesh_t*,Graph_t*) ;


#define Periodicities_GetNbOfPeriodicities(PS) ((PS)->nbperiod)
#define Periodicities_GetPeriodicity(PS)       ((PS)->periodicity)



#include "Periodicity.h"


struct Periodicities_s {            /* Periodicities */
  unsigned int   nbperiod ;         /* Nb of periodicities */
  Periodicity_t* periodicity ;      /* Periodicity */
} ;



#ifdef __CPLUSPLUS
}
#endif
#endif
