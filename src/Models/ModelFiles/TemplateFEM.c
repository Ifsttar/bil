#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "CommonModel.h"

/* Choose the finite element method */
#include "FEM.h"

#define TITLE "Short title of my model"
#define AUTHORS "Authors"

#include "PredefinedMethods.h"

/*
 * The numbers below are arbitrary and serve only as example
 */

/* Nb of equations of the model */
#define NEQ   (2)     /* Here let's consider an example with 2 equations */

/* Nb of terms per point */
#define NVI   (14)    /*  14 implicit terms per point */
#define NVE   (2)     /*  2 explicit terms per point */
#define NV0   (10)    /*  10 constant terms per point */


/* Indices of equations */
#define E_Eq1    (0)
#define E_Eq2    (1)
/* Indices of unknowns */
#define U_Unk1    (0)
#define U_Unk2    (1)


/* Value of the nodal unknown (u and el must be used as pointers below) */
#define UNKNOWN(n,i)     Element_GetValueOfNodalUnknown(el,u,n,i)


/* We define some names for nodal unknowns */
#define Unk1(n)          (UNKNOWN(n,U_Unk1))
#define Unk2(n)          (UNKNOWN(n,U_Unk2))


/* We define some names for implicit terms (vi must be used as pointer below) */
#define N_1           (vi)[0]
#define W_1           (vi + 1) /* this is a 3D vector */

#define SIG           (vi + 4) /* this a 3D tensor */

#define PHI           (vi + 13)[0]


/* We define some names for explicit terms (ve must be used as pointer below) */
#define K_1           (ve[0])
#define K_2           (ve[1])


/* We define some names for constant terms (v0 must be used as pointer below) */
#define SIG0          (v0 + 0)

#define PR_2          (v0 + 9)[0]


/* Material Properties 
 * ------------------- */
#define PCURVE1      (Element_GetCurve(el) + 0)
#define CURVE1(x)    (Curve_ComputeValue(PCURVE1,x))
#define PCURVE2      (Element_GetCurve(el) + 1)
#define CURVE2(x)    (Curve_ComputeValue(PCURVE2,x))


/* To retrieve the material properties */
#define GetProperty(a)   (Element_GetProperty(el)[pm(a)]) 


/* Intern Functions */
static int    pm(const char* s) ;
static void   GetProperties(Element_t*) ;

static double* ComputeVariables(Element_t*,double**,double**,double*,double,double,int) ;
static void    ComputeSecondaryVariables(Element_t*,double,double,double*) ;


/* Intern parameters */
static double coef1 ;
static double coef2 ;
static double coef3 ;
static double* sig0 ;


/* Locally defined intern variables  */
#define NP               IntFct_MaxNbOfIntPoints
#define NbOfVariables    (NEQ+50)
static double Variables[NP][NbOfVariables] ;
static double dVariables[NbOfVariables] ;


/* We define some indices for the local variables */
#define I_U            (U_Unk1)
#define I_P            (U_Unk2)
#define I_EPS          (NEQ+0)
#define I_SIG          (NEQ+9)
/* etc... */


int pm(const char* s)
{
  if(strcmp(s,"prop1") == 0)        return (0) ;
  else if(strcmp(s,"prop2") == 0)   return (1) ;
  else if(strcmp(s,"prop3") == 0)   return (2) ;
  else if(!strcmp(s,"sig0"))        return (3) ;
  else if(!strncmp(s,"sig0_",5)) {
     int i = (strlen(s) > 5) ? s[5] - '1' : 0 ;
     int j = (strlen(s) > 6) ? s[6] - '1' : 0 ;

     return(3 + 3*i + j) ;
  } else return(-1) ;
}


void GetProperties(Element_t* el)
{
  coef1  = GetProperty("prop1") ;
  coef2  = GetProperty("prop2") ;
  coef3  = GetProperty("prop3") ;
  sig0   = &GetProperty("sig0") ;
}


int SetModelProp(Model_t* model)
/** Set the model properties, return 0.
 *  Warning:
 *  Never call InternationalSystemOfUnits_UseAsLength() or similar
 *  to modify the units because this will also affect other models.
 */
{
  /** Number of equations to be solved */
  Model_GetNbOfEquations(model) = NEQ ;
  
  /** Names of these equations */
  Model_CopyNameOfEquation(model,E_Eq1,"first") ;
  Model_CopyNameOfEquation(model,E_Eq2,"second") ;
  
  /** Names of the main (nodal) unknowns */
  Model_CopyNameOfUnknown(model,U_Unk1,"x") ;
  Model_CopyNameOfUnknown(model,U_Unk2,"y") ;
  
  return(0) ;
}


int ReadMatProp(Material_t* mat,DataFile_t* datafile)
/** Read the material properties in the stream file ficd 
 *  Return the nb of (scalar) properties of the model */
{
  int  NbOfProp = 3 ;
  
  Material_ScanProperties(mat,datafile,pm) ;
  
  return(NbOfProp) ;
}


int PrintModelProp(Model_t* model,FILE* ficd)
/** Print the model properties 
 *  Return the nb of equations */
{
  printf(TITLE) ;
  printf("\n") ;
  
  if(!ficd) return(0) ;
  
  printf("\n") ;
  printf("The set of equations is:\n") ;
  printf("\t- First equation      (name of Eq. 1)\n") ;
  printf("\t- Second equation     (name of Eq. 2)\n") ;
  
  printf("\n") ;
  printf("The primary unknowns are:\n") ;
  printf("\t- First unknown       (name of Unk. 1)\n") ;
  printf("\t- Second unknown      (name of Unk. 2)\n") ;
  
  printf("\n") ;
  printf("Example of input data\n") ;

  fprintf(ficd,"prop1 = 0.01   # Property 1\n") ;
  fprintf(ficd,"prop2 = 1.e-3  # Property 2\n") ;
  fprintf(ficd,"prop3 = 1.e6   # Property 3\n") ;

  return(NEQ) ;
}


int DefineElementProp(Element_t* el,IntFcts_t* intfcts)
/** Define some properties attached to each element 
 *  Return 0 */
{
  IntFct_t* intfct = Element_GetIntFct(el) ;
  int NbOfIntPoints = IntFct_GetNbOfPoints(intfct) ;

  /** Define the length of tables */
  Element_GetNbOfImplicitTerms(el) = NVI*NbOfIntPoints ;
  Element_GetNbOfExplicitTerms(el) = NVE*NbOfIntPoints ;
  Element_GetNbOfConstantTerms(el) = NV0*NbOfIntPoints ;
  
  /* Skip the rest of code for basic development.
   * For advanced developments find below 
   * some examples of possible operations */
  
  /* Compute some new interpolation functions */
  {
    int dim = Element_GetDimension(el) ;
    int nn  = Element_GetNbOfNodes(el) ;
    IntFct_t* intfct = Element_GetIntFct(el) ;
    char* typ1 = IntFct_GetType(intfct) ;
    char* typ2 = IntFct_GetType(intfct+1) ;
    
    /* Replace "Type1" and "Type2" by existing types */
    if(strcmp(typ1,"Type1") || strcmp(typ2,"Type2")) {
      int i   = IntFcts_AddIntFct(intfcts,nn,dim,"Type1") ;
      int j   = IntFcts_AddIntFct(intfcts,nn,dim,"Type2") ; /* not used */
      /* here j is equal to i + 1 ! */
      Element_GetIntFct(el) = IntFcts_GetIntFct(intfcts) + i ;
    }
    /* Remove dof on some nodes 
     * (e.g. at the middle of the edges of a triangle) */
    {
      int nn  = Element_GetNbOfNodes(el) ;
      int j = 3 ; /* dof = 3 to be suppressed */
      int i ;
    
      for(i = nn/2 ; i < nn ; i++) { /* edge nodes */
        Element_GetUnknownPosition(el)[i*NEQ + j]  = -1 ;
        Element_GetEquationPosition(el)[i*NEQ + j] = -1 ;
      }
    }
  }
  return(0) ;
}



int  ComputeLoads(Element_t* el,double t,double dt,Load_t* cg,double* r)
/** Compute the residu (r) due to loads 
 *  Return 0 if succeeded and -1 if failed */
{
  IntFct_t* fi = Element_GetIntFct(el) ;
  int nn = Element_GetNbOfNodes(el) ;
  FEM_t* fem = FEM_GetInstance(el) ;
  int    i ;

  {
    double* r1 = FEM_ComputeSurfaceLoadResidu(fem,fi,cg,t,dt) ;
    for(i = 0 ; i < NEQ*nn ; i++) r[i] = -r1[i] ;
  }
  
  return(0) ;
}


int ComputeInitialState(Element_t* el,double t)
/** Compute the initial state i.e. 
 *  the constant terms,
 *  the explicit terms,
 *  the implicit terms.
 *  Return 0 if succeeded and -1 if failed
 */ 
{
  double* vi0 = Element_GetImplicitTerm(el) ;
  double* ve0 = Element_GetExplicitTerm(el) ;
  double* v0 = Element_GetConstantTerm(el) ;
  
  /* Usually we have to skip if the element is a submanifold, 
   * e.g. a surface in 3D or a line in 2D */
  if(Element_IsSubmanifold(el)) return(0) ;

  /*
    We load some input data
  */
  GetProperties(el) ;
  

  /* Compute here vi, ve and v0 for each integration points */

  /* Pre-initialization */
  for(p = 0 ; p < NbOfIntPoints ; p++) {
    
    /* storage in vi */
    {
      double* vi  = vi0 + p*NVI ;
      int    i ;
      
      /* How to account for partial initialization? */
      if(DataFile_ContextIsPartialInitialization(datafile)) {
        for(i = 0 ; i < 9 ; i++) SIG0[i] = SIG[i] ;
      } else {
        for(i = 0 ; i < 9 ; i++) SIG0[i] = sig0[i] ;
      }

      /* ... */
    }
  }


  
    
  /* Loop on integration points */
  for(p = 0 ; p < NbOfIntPoints ; p++) {
    /* Variables */
    double* x = ComputeVariables(el,u,u,vi0,t,0,p) ;
    
    /* storage in vi */
    {
      double* vi  = vi0 + p*NVI ;
      int    i ;
      
      /* ... */
    }
    
    
    /* storage in ve */
    {
      double* ve  = ve0 + p*NVE ;

      /* ... */
    }
  }

  
  return(0) ;
}


int  ComputeExplicitTerms(Element_t* el,double t)
/** Compute the (current) explicit terms.
 *  IMPORTANT: if needed use only the previous values
 *  whatever they are, nodal values or implicit terms.
 *  Return 0 if succeeded and -1 if failed */
{
  double* ve = Element_GetExplicitTerm(el) ;
  /* If you need the implicit terms, use the previous ones */
  double* vi = Element_GetPreviousImplicitTerm(el) ;
  /* If you need the nodal values, use the previous ones */
  double** u = Element_ComputePointerToPreviousNodalUnknowns(el) ;
  
  /* If needed ! */
  if(Element_IsSubmanifold(el)) return(0) ;

  /*
    We load some input data
  */
  GetProperties(el) ;
  
  
  /* Compute here ve */
  {
    /* ... */
  }
  
  return(0) ;
}


int  ComputeImplicitTerms(Element_t* el,double t,double dt)
/** Compute the (current) implicit terms 
 *  Return 0 if succeeded and -1 if failed */
{
  double* vi0  = Element_GetCurrentImplicitTerm(el) ;
  double* vi_n  = Element_GetPreviousImplicitTerm(el) ;
  double** u   = Element_ComputePointerToCurrentNodalUnknowns(el) ;
  double** u_n = Element_ComputePointerToPreviousNodalUnknowns(el) ;

  /*
    We load some input data
  */
  GetProperties(el) ;
  
  /* Compute here vi (with the help of vi_n if needed) */
  /* Loop on integration points */
  {
    IntFct_t*  intfct = Element_GetIntFct(el) ;
    int NbOfIntPoints = IntFct_GetNbOfPoints(intfct) ;
    int p ;
    
    for(p = 0 ; p < NbOfIntPoints ; p++) {
      /* Variables */
      double* x = ComputeVariables(el,u,u_n,vi_n,t,dt,p) ;
    
      /* storage in vi */
      {
        double* vi  = vi0 + p*NVI ;
        int    i ;
      
        /* ... */
      }
    }
  }

  return(0) ;
}


int  ComputeMatrix(Element_t* el,double t,double dt,double* k)
/** Compute the matrix (k) 
 *  Return 0 if succeeded and -1 if failed */
{
#define K(i,j)    (k[(i)*2*NEQ+(j)])

  /*
    We load some input data
  */
  GetProperties(el) ;

  /* Compute here the matrix K(i,j) */
  {
    /* ...*/
  }
  
  return(0) ;
#undef K
}


int  ComputeResidu(Element_t* el,double t,double dt,double* r)
/** Compute the residu (r) 
 *  Return 0 if succeeded and -1 if failed */
{
#define R(n,i)    (r[(n)*NEQ+(i)])

  /* Compute here the residu R(n,i) */
  {
    /* ... */
  }
  
  return(0) ;
#undef R
}


int  ComputeOutputs(Element_t* el,double t,double* s,Result_t* r)
/** Compute the outputs (r) 
 *  Return the nb of views (scalar, vector or tensor) */
{
  int NbOfOutputs  = 3 ;
  double scalar    = 1 ;
  double vector[3] = {1,2,3} ;
  double tensor[9] = {11,12,13,21,22,23,31,32,33} ;
  
  {
    i = 0  ;
    Result_Store(r + i++,&scalar,"NameOfView_x",1) ; /* scalar */
    Result_Store(r + i++,vector ,"NameOfView_v",3) ; /* vector */
    Result_Store(r + i++,tensor ,"NameOfView_t",9) ; /* tensor */
    
    if(i != NbOfOutputs) {
      Message_RuntimeError("ComputeOutputs: wrong number of outputs") ;
    }
  }

  return(NbOfOutputs) ;
}








double* ComputeVariables(Element_t* el,double** u,double** u_n,double* f_n,double t,double dt,int p)
/** This locally defined function compute the intern variables at
 *  the interpolation point p, from the nodal unknowns.
 *  Return a pointer on the locally defined array of the variables. */
{
  IntFct_t* intfct = Element_GetIntFct(el) ;
  FEM_t*    fem    = FEM_GetInstance(el) ;
  int dim = Element_GetDimensionOfSpace(el) ; 
  /* Variables is a locally defined array of array */
  Model_t*  model  = Element_GetModel(el) ;
  double*   x      = Model_GetVariable(model,p) ;

  /*
   * The variables are stored in the array x by using some indexes.
   * Two sets of indexes are used:
   * 1. The first set is used for the primary nodal unknowns. The
   *    notation used for these indexes begins with "U_", e.g. "U_u", 
   *    "U_p", etc.... E.g if displacements, pressure and temperature 
   *    are the nodal unknowns used in this order at each interpolation
   *    point then "U_u = 0, U_p = 3, U_T = 4". They are used to extract
   *    the value of the primary unknowns which will be used to
   *    compute the secondary variables.
   * 2. The second set is used for the secondary variables. The 
   *    notation used for these indexes begins with "I_". These
   *    secondary variables are stored in the x at these locations.
   */
  
    
  /* Load the primary variables in x */
  {
    int    i ;
    
    /* Displacements */
    for(i = 0 ; i < dim ; i++) {
      x[I_U + i] = FEM_ComputeUnknown(fem,u,intfct,p,U_u + i) ;
    }
    
    for(i = dim ; i < 3 ; i++) {
      x[I_U + i] = 0 ;
    }
    
    /* Strains */
    {
      double* eps =  FEM_ComputeLinearStrainTensor(fem,u,intfct,p,U_u) ;
    
      for(i = 0 ; i < 9 ; i++) {
        x[I_EPS + i] = eps[i] ;
      }
      
      FEM_FreeBufferFrom(fem,eps) ;
    }
    
    /* Pressure */
    x[I_P_L] = FEM_ComputeUnknown(fem,u,intfct,p,U_p_l) ;
    
    /* Pressure gradient */
    {
      double* grd = FEM_ComputeUnknownGradient(fem,u,intfct,p,U_p_l) ;
    
      for(i = 0 ; i < 3 ; i++) {
        x[I_GRD_P_L + i] = grd[i] ;
      }
      
      FEM_FreeBufferFrom(fem,grd) ;
    }
  }
  
  
  /* Needed variables to compute secondary variables */
  {
  }
    
  ComputeSecondaryVariables(el,t,dt,x) ;
  
  return(x) ;
}



void  ComputeSecondaryVariables(Element_t* el,double t,double dt,double* x)
/** Compute the secondary variables from the primary ones. */
{
  int dim = Element_GetDimensionOfSpace(el) ;
  /* Retrieve the primary variables from x */
  double* eps   =  x + I_EPS ;

  /* Compute the secondary variables in terms of the primary ones
   * and backup them in x */
  {
  }
}
