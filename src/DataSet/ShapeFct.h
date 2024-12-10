#ifndef SHAPEFCT_H
#define SHAPEFCT_H

#ifdef __CPLUSPLUS
extern "C" {
#endif


/* vacuous declarations and typedef names */

/* class-like structure "DataSet_t and attributes */
struct ShapeFct_s       ; typedef struct ShapeFct_s       ShapeFct_t ;



extern ShapeFct_t* (ShapeFct_Create)              (int,int) ;
extern void        (ShapeFct_Delete)              (void*) ;
extern void        (ShapeFct_ComputeValuesAtPoint)(int,int,double*,double*,double*) ;



#include "Element.h"

#define ShapeFct_MaxNbOfNodes            (Element_MaxNbOfNodes)


#define ShapeFct_GetType(SF)             ((SF)->type)
#define ShapeFct_GetNbOfNodes(SF)        ((SF)->nn)
#define ShapeFct_GetNbOfFunctions(SF)    ((SF)->nn)
#define ShapeFct_GetCoordinate(SF)       ((SF)->a)
#define ShapeFct_GetFunction(SF)         ((SF)->h)
#define ShapeFct_GetFunctionGradient(SF) ((SF)->dh)
#define ShapeFct_GetDimension(SF)        ((SF)->dim)



struct ShapeFct_s {           /* Shape function */
  unsigned int dim ;    /* Sub-dimension (0,1,2,3) */
  unsigned int nn ;     /* Number of nodes */
  double* a ;                 /* Reference coordinates */
  double* h ;                 /* Values of shape functions */
  double* dh ;                /* Values of function gradients */
} ;



#ifdef __CPLUSPLUS
}
#endif
#endif
