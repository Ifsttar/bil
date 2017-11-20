#ifndef LOAD_H
#define LOAD_H

/* vacuous declarations and typedef names */

/* class-like structure and attributes */
struct Load_s         ; typedef struct Load_s         Load_t ;



#define Load_MaxLengthOfKeyWord               (30)

#define Load_GetRegionIndex(LOAD)        ((LOAD)->reg)
#define Load_GetType(LOAD)               ((LOAD)->t)
#define Load_GetNameOfEquation(LOAD)     ((LOAD)->eqn)
#define Load_GetFunction(LOAD)           ((LOAD)->fn)
#define Load_GetField(LOAD)              ((LOAD)->ch)


#define Load_TypeIs(LOAD,TYPE)           (!strcmp(Load_GetType(LOAD),TYPE))


#include "Function.h"
#include "Field.h"


struct Load_s {               /* chargement */
  int    reg ;                /* numero de la region */
  char   *t ;                 /* type de chargement */
  char   *eqn ;               /* nom de l'equation */
  Function_t *fn ;            /* fonction du temps */
  Field_t *ch ;               /* champ */
} ;


/* Old notations which I try to eliminate little by little */
#define char_t    Load_t

#endif
