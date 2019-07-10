#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "Context.h"
#include "CommonModel.h"
#include "FEM.h"
#include "Plasticity.h"

#define TITLE "hydration of expansive pellet mixtures (2019)"
#define AUTHORS "Darde"

#include "PredefinedMethods.h"


/* Nb of equations */
#define NEQ     (2+dim)
/* Nb of (im/ex)plicit terms and constant terms */
#define NVI     (31)
#define NVE     (1)
#define NV0     (0)

/* Equation index */
#define E_liq    (1+dim)
#define E_liq1   (0+dim)
#define E_mec    (0)

/* Unknown index */
#define U_p_l2   (1+dim)
#define U_p_l1   (0+dim)
#define U_u      (0)

/* We define some names for implicit terms */
#define M_L           (vim   + 0)[0]
#define M_L_n         (vim_n + 0)[0]

#define M_L1          (vim   + 1)[0]
#define M_L1_n        (vim_n + 1)[0]

#define W_L           (vim   + 2)

#define SIG           (vim   + 5)
#define SIG_n         (vim_n + 5)

#define F_MASS        (vim   + 14)

#define EPS_P         (vim   + 17)
#define EPS_P_n       (vim_n + 17)

#define HARDV         (vim   + 26)[0]
#define HARDV_n       (vim_n + 26)[0]

#define CRIT          (vim   + 28)[0]

#define EPSV_1        (vim   + 29)[0]
#define EPSV_1n       (vim_n + 29)[0]

#define EPSV_2        (vim   + 30)[0]
#define EPSV_2n       (vim_n + 30)[0]


/* We define some names for explicit terms */
#define K_L           (vex + 0)[0]

/* We define some names for constant terms */


/* Functions */
static int    pm(const char *s) ;
static void   GetProperties(Element_t*) ;

static int    ComputeTangentCoefficients(FEM_t*,double,double*) ;
static int    ComputeTransferCoefficients(FEM_t*,double,double*) ;

//static Model_ComputeVariables_t             ComputeVariables ;
static double* ComputeVariables(Element_t*,void*,void*,void*,const double,const double,const int);
//static Model_ComputeSecondaryVariables_t    ComputeSecondaryVariables ;
static void  ComputeSecondaryVariables(Element_t*,double,double,double*,double*) ;
//static double* ComputeVariablesDerivatives(Element_t*,double,double*,double,int) ;


//static double pie(double,double,Curve_t*) ;
//static double dpiesdpl(double,double,Curve_t*) ;


#define ComputeFunctionGradients(...)  Plasticity_ComputeFunctionGradients(plasty,__VA_ARGS__)
#define ReturnMapping(...)             Plasticity_ReturnMapping(plasty,__VA_ARGS__)
#define CopyElasticTensor(...)         Plasticity_CopyElasticTensor(plasty,__VA_ARGS__)
#define UpdateElastoplasticTensor(...) Plasticity_UpdateElastoplasticTensor(plasty,__VA_ARGS__)


/* Material properties */
//#define SATURATION_CURVE        (Element_GetCurve(el))
//#define SaturationDegree(pc)    (Curve_ComputeValue(SATURATION_CURVE,pc))
//#define dSaturationDegree(pc)   (Curve_ComputeDerivative(SATURATION_CURVE,pc))

#define RELATIVEPERM_CURVE                (Element_GetCurve(el) + 1)
#define RelativePermeabilityToLiquid(pc)  (Curve_ComputeValue(RELATIVEPERM_CURVE,pc))

//#define CAPIHARDENING_CURVE     (Element_GetCurve(el) + 2)
//#define CapillaryHardening(pc)  (Curve_ComputeValue(CAPIHARDENING_CURVE,pc))

#define LOADINGCOLLAPSEFACTOR_CURVE     (Element_GetCurve(el) + 0)
#define LoadingCollapseFactor(s)  (Curve_ComputeValue(LOADINGCOLLAPSEFACTOR_CURVE,s))

//#define EquivalentPressure(pl,pg)   (pie(pl,pg,SATURATION_CURVE))
//#define dEquivalentPressure(pl,pg)  (dpiesdpl(pl,pg,SATURATION_CURVE))





/* Parameters */
static double  gravity ;
static double  rho_s ;
static double* sig0 ;
static double  rho_l0 ;
static double  p_g = 0 ;
static double  k_int ;
static double  mu_l ;
static double  kappa_m ;
static double  kappa_M ;
static double  kappa_s ;
static double  poisson ;
static double  alpha ;
static double  beta ;
//static double  lambda_M0 ;
static double  p_c ;
static double  p_co0 ;
static double  e0 ;
static double  A_a ;
static double  B_a ;
static double  A_b ;
static double  B_b ;
static double  A_m ;
static double  B_m ;
static double  p_l0 ;
static double  p_limit ;
static double  k_s ;
static Elasticity_t* elasty ;
static Plasticity_t* plasty ;



/* Variable indexes */
enum {
  I_U     = 0,
  I_P_L1  = I_U     + 3,
  I_P_L2,
  I_EPS,
  I_SIG   = I_EPS   + 9,
  I_EPS_P = I_SIG   + 9,
  I_Fmass = I_EPS_P + 9,
  I_M_L   = I_Fmass + 3,
  I_M_L1,
  I_EPSV_1,
  I_EPSV_2,
  I_W_L,
  I_HARDV = I_W_L   + 3,
  I_CRIT,
  I_RHO_L,
  I_PHI,
  I_K_H,
  I_GRD_P_L2,
  I_Last  = I_GRD_P_L2 + 3
} ;

#define NbOfVariables     (I_Last)
static double  Variable[NbOfVariables] ;
static double  Variable_n[NbOfVariables] ;
//static double dVariable[NbOfVariables] ;



int pm(const char *s)
{
         if(!strcmp(s,"gravity"))    { 
    return (0) ;
  } else if(!strcmp(s,"rho_s"))      { 
    return (1) ;
  } else if(!strcmp(s,"Poisson")) { 
    return (2) ;
  } else if(!strcmp(s,"rho_l"))      { 
    return (3) ;
  } else if(!strcmp(s,"k_int"))      { 
    return (4) ;
  } else if(!strcmp(s,"mu_l"))       { 
    return (5) ;
  } else if(!strcmp(s,"initial_liquid_pressure"))       {
    return (6) ;
  } else if(!strcmp(s,"initial_stress"))       {
    return(7) ;
  } else if(!strncmp(s,"initial_stress_",15))   {
    int i = (strlen(s) > 15) ? s[15] - '1' : 0 ;
    int j = (strlen(s) > 16) ? s[16] - '1' : 0 ;
    
    return(7 + 3*i + j) ;
    
    /* Cam-clay */
  } else if(!strcmp(s,"kappa_m")) {
    return(16) ;
  } else if(!strcmp(s,"kappa_M")) {
    return(17) ;
  } else if(!strcmp(s,"kappa_s"))  {
    return(18) ;
  } else if(!strcmp(s,"initial_pre-consolidation_pressure")) {
    return(19) ;
  } else if(!strcmp(s,"initial_void_ratio")) {
    return(20) ;
  } else if(!strcmp(s,"p_c")) {
    return(21) ;
  } else if(!strcmp(s,"limit_liquid_pressure")) {
    return(22) ;
  } else if(!strcmp(s,"A_a")) {
    return(23) ;
  } else if(!strcmp(s,"B_a")) {
    return(24) ;
  } else if(!strcmp(s,"A_b")) {
    return(25) ;
  } else if(!strcmp(s,"B_b")) {
    return(26) ;
  } else if(!strcmp(s,"A_m")) {
    return(27) ;
  } else if(!strcmp(s,"B_m")) {
    return(28) ;
  } else if(!strcmp(s,"slope_of_critical_state_line"))  {
    return(29) ;
  } else if(!strcmp(s,"k_s"))  {
    return(30) ;
  } else if(!strcmp(s,"alpha"))  {
    return(31) ;
  } else if(!strcmp(s,"beta"))  {
    return(32) ;
  } else if(!strcmp(s,"lambda_M0"))  {
    return(33) ;
  } else return(-1) ;
}


void GetProperties(Element_t* el)
{
  gravity = Element_GetPropertyValue(el,"gravity") ;
  rho_s   = Element_GetPropertyValue(el,"rho_s") ;
  k_int   = Element_GetPropertyValue(el,"k_int") ;
  mu_l    = Element_GetPropertyValue(el,"mu_l") ;
  rho_l0  = Element_GetPropertyValue(el,"rho_l") ;
  sig0    = &Element_GetPropertyValue(el,"initial_stress") ;
  p_co0   = Element_GetPropertyValue(el,"initial_pre-consolidation_pressure") ;
  e0      = Element_GetPropertyValue(el,"initial_void_ratio") ;
  kappa_m = Element_GetPropertyValue(el,"kappa_m") ;
  kappa_M = Element_GetPropertyValue(el,"kappa_M") ;
  kappa_s = Element_GetPropertyValue(el,"kappa_s") ;
  poisson = Element_GetPropertyValue(el,"Poisson") ;
  alpha   = Element_GetPropertyValue(el,"alpha") ;
  beta    = Element_GetPropertyValue(el,"beta") ;
//  lambda_M0 = Element_GetPropertyValue(el,"lambda_M0") ;
  p_c     = Element_GetPropertyValue(el,"p_c") ;
  A_a     = Element_GetPropertyValue(el,"A_a") ;
  B_a     = Element_GetPropertyValue(el,"B_a") ;
  A_b     = Element_GetPropertyValue(el,"A_b") ;
  B_b     = Element_GetPropertyValue(el,"B_b") ;
  A_m     = Element_GetPropertyValue(el,"A_m") ;
  B_m     = Element_GetPropertyValue(el,"B_m") ;
  p_l0    = Element_GetPropertyValue(el,"initial_liquid_pressure") ;
  p_limit = Element_GetPropertyValue(el,"limit_liquid_pressure") ;
  k_s     = Element_GetPropertyValue(el,"k_s") ;
  
  plasty  = Element_FindMaterialData(el,Plasticity_t,"Plasticity") ;
  elasty  = Plasticity_GetElasticity(plasty) ;
}



int SetModelProp(Model_t* model)
/** Set the model properties */
{
  int dim = Model_GetDimension(model) ;
  char name_eqn[3][7] = {"meca_1","meca_2","meca_3"} ;
  char name_unk[3][4] = {"u_1","u_2","u_3"} ;
  int i ;
  
  /** Number of equations to be solved */
  Model_GetNbOfEquations(model) = NEQ ;
  
  /** Names of these equations */
  Model_CopyNameOfEquation(model,E_liq,"liq") ;
  Model_CopyNameOfEquation(model,E_liq1,"pellet") ;
  for(i = 0 ; i < dim ; i++) {
    Model_CopyNameOfEquation(model,E_mec + i,name_eqn[i]) ;
  }
  
  /** Names of the main (nodal) unknowns */
  Model_CopyNameOfUnknown(model,U_p_l1,"p_l1") ;
  Model_CopyNameOfUnknown(model,U_p_l2,"p_l2") ;
  for(i = 0 ; i < dim ; i++) {
    Model_CopyNameOfUnknown(model,U_u + i,name_unk[i]) ;
  }
  
  Model_GetComputePropertyIndex(model) = pm ;
  
  Model_GetNbOfVariables(model) = NbOfVariables ;
  //Model_GetComputeSecondaryVariables(model) = ComputeSecondaryVariables ;
  
  return(0) ;
}



int ReadMatProp(Material_t* mat,DataFile_t* datafile)
/** Read the material properties in the stream file ficd 
 *  Return the nb of (scalar) properties of the model */
{
  int  NbOfProp = 34 ;
  int i ;

  /* Par defaut tout a 0 */
  for(i = 0 ; i < NbOfProp ; i++) Material_GetProperty(mat)[i] = 0. ;
  
  Material_ScanProperties(mat,datafile,pm) ;
  
  
  /* Plasticity */
  {
    plasty = Plasticity_Create() ;
      
    Material_AppendData(mat,1,plasty,Plasticity_t,"Plasticity") ;
  }
  
  /* Elastic and plastic properties */
  {
    elasty = Plasticity_GetElasticity(plasty) ;
    
    {
      /* Elasticity */
      {
        //double young    = Material_GetPropertyValue(mat,"young") ;
        //double poisson  = Material_GetPropertyValue(mat,"poisson") ;
        
        Elasticity_SetToIsotropy(elasty) ;
        //Elasticity_SetParameters(elasty,young,poisson) ;
        //Elasticity_ComputeStiffnessTensor(elasty) ;
      }
    }
    {
      /* Cam-Clay with offset*/
      {
        double lambda_M0 = Material_GetPropertyValue(mat,"lambda_M0") ;
        double M      = Material_GetPropertyValue(mat,"slope_of_critical_state_line") ;
        double pc0    = Material_GetPropertyValue(mat,"initial_pre-consolidation_pressure") ;
        
        e0     = Material_GetPropertyValue(mat,"initial_void_ratio") ;
        kappa_M  = Material_GetPropertyValue(mat,"kappa_M") ;
        
        Plasticity_SetToCamClayOffset(plasty) ;
        Plasticity_SetParameters(plasty,kappa_M,lambda_M0,M,pc0,e0) ;
      }
    }

#if 0
    {      
      Elasticity_PrintStiffnessTensor(elasty) ;
    }
#endif
  }
  
  return(NbOfProp) ;
}


int PrintModelProp(Model_t* model,FILE *ficd)
/** Print the model properties 
 *  Return the nb of equations */
{
  printf(TITLE) ;
  printf("\n") ;
  
  if(!ficd) return(0) ;

  printf("\n\
The system consists in (1 + dim) equations\n\
\t 1. The mass balance equation for water (liq)\n\
\t 2. The equilibrium equation (meca_1,meca_2,meca_3)\n") ;

  printf("\n\
The primary unknowns are\n\
\t 1. The liquid pressure (p_l)\n\
\t 2. The displacement vector (u_1,u_2,u_3)\n") ;

  printf("\n\
Example of input data\n\n") ;
  

  fprintf(ficd,"gravity = 0       # gravity\n") ;
  fprintf(ficd,"rho_s = 2350      # mass density of solid skeleton\n") ;
  fprintf(ficd,"young = 5.8e+09   # Young's modulus\n") ;
  fprintf(ficd,"poisson = 0.3     # Poisson's ratio\n") ;
  fprintf(ficd,"porosity = 0.15   # porosity\n") ;
  fprintf(ficd,"rho_l = 1000      # mass density of fluid\n") ;
  fprintf(ficd,"p_g = 0           # gas pressure\n") ;
  fprintf(ficd,"k_int = 1e-19     # intrinsic permeability\n") ;
  fprintf(ficd,"mu_l = 0.001      # viscosity of liquid\n") ;
  fprintf(ficd,"sig0_ij = -11.5e6 # initial stress sig0_ij\n") ;
  fprintf(ficd,"Curves = my_file  # file name: p_c S_l k_rl\n") ;
  
  return(0) ;
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
  return(0) ;
}



int  ComputeLoads(Element_t* el,double t,double dt,Load_t* cg,double* r)
/** Compute the residu (r) due to loads 
 *  Return 0 if succeeded and -1 if failed */
{
  int dim = Element_GetDimensionOfSpace(el) ;
  IntFct_t* fi = Element_GetIntFct(el) ;
  int nn = Element_GetNbOfNodes(el) ;
  int ndof = nn*NEQ ;
  FEM_t* fem = FEM_GetInstance(el) ;
  int    i ;

  {
    double* r1 = FEM_ComputeSurfaceLoadResidu(fem,fi,cg,t,dt) ;
  
    /* hydraulic */
    if(Element_FindEquationPositionIndex(el,Load_GetNameOfEquation(cg)) == E_liq) {
      for(i = 0 ; i < ndof ; i++) r[i] = -r1[i] ;
      
    /* other */
    } else {
      for(i = 0 ; i < ndof ; i++) r[i] = r1[i] ;
    }
  }
  
  return(0) ;
}


int ComputeInitialState(Element_t* el)
{
  double* vim0  = Element_GetImplicitTerm(el) ;
  double* vex0  = Element_GetExplicitTerm(el) ;
  double** u   = Element_ComputePointerToNodalUnknowns(el) ;
  IntFct_t*  intfct = Element_GetIntFct(el) ;
  int NbOfIntPoints = IntFct_GetNbOfPoints(intfct) ;
  DataFile_t* datafile = Element_GetDataFile(el) ;
  int    p ;
  
  /* We skip if the element is a submanifold */
  if(Element_IsSubmanifold(el)) return(0) ;

  /*
    Input data
  */
  GetProperties(el) ;
  
  /* Pre-initialization */
  for(p = 0 ; p < NbOfIntPoints ; p++) {
    
    /* storage in vim */
    {
      double* vim  = vim0 + p*NVI ;
      int    i ;

      
      /* Initial stresses, hardening variables */
      if(DataFile_ContextIsPartialInitialization(datafile)) {
      } else {
        for(i = 0 ; i < 9 ; i++) SIG[i] = sig0[i] ;
        HARDV = p_co0 ;
      }
      
      for(i = 0 ; i < 9 ; i++) EPS_P[i]  = 0 ;
    }
  }
  
    
  /* Loop on integration points */
  for(p = 0 ; p < NbOfIntPoints ; p++) {
    /* Variables */
    double* x = ComputeVariables(el,u,u,vim0,0,0,p) ;
    
    /* storage in vim */
    {
      double* vim  = vim0 + p*NVI ;
      int    i ;
      
      M_L = x[I_M_L] ;
    
      for(i = 0 ; i < 3 ; i++) W_L[i] = x[I_W_L + i] ;
    
      for(i = 0 ; i < 9 ; i++) SIG[i] = x[I_SIG + i] ;
      
      for(i = 0 ; i < 3 ; i++) F_MASS[i] = x[I_Fmass + i] ;
      
      for(i = 0 ; i < 9 ; i++) EPS_P[i]  = x[I_EPS_P + i] ;
    
      CRIT = x[I_CRIT] ;
      HARDV = x[I_HARDV] ;
    }
    
    
    /* storage in vex */
    {
      double* vex  = vex0 + p*NVE ;
      double rho_l = x[I_RHO_L] ;
      double k_h   = rho_l*k_int/mu_l ;
    
      K_L = k_h ;
    }
  }
  
  return(0) ;
}


int  ComputeExplicitTerms(Element_t* el,double t)
/** Compute the explicit terms */
{
  double* vim0 = Element_GetPreviousImplicitTerm(el) ;
  double* vex0 = Element_GetExplicitTerm(el) ;
  double** u = Element_ComputePointerToPreviousNodalUnknowns(el) ;
  IntFct_t*  intfct = Element_GetIntFct(el) ;
  int NbOfIntPoints = IntFct_GetNbOfPoints(intfct) ;
  int    p ;
  
  /* We skip if the element is a submanifold */
  if(Element_IsSubmanifold(el)) return(0) ;

  /*
    Input data
  */
  GetProperties(el) ;

  /* Loop on integration points */
  for(p = 0 ; p < NbOfIntPoints ; p++) {
    /* Variables */
    double* x = ComputeVariables(el,u,u,vim0,t,0,p) ;
    
    /* fluid mass density */
    double rho_l = x[I_RHO_L] ;
    
    /* pressures */
    double p_l2 = x[I_P_L2] ;
    double pc = p_g - p_l2 ;
    
    /* permeability */
    double k_h = rho_l*k_int/mu_l*RelativePermeabilityToLiquid(pc) ;
    
    /* storage in vex */
    {
      double* vex  = vex0 + p*NVE ;
      
      K_L = k_h ;
    }
  }
  
  return(0) ;
}



int  ComputeImplicitTerms(Element_t* el,double t,double dt)
{
  double* vim0  = Element_GetCurrentImplicitTerm(el) ;
  double* vim_n  = Element_GetPreviousImplicitTerm(el) ;
  double** u   = Element_ComputePointerToCurrentNodalUnknowns(el) ;
  double** u_n = Element_ComputePointerToPreviousNodalUnknowns(el) ;
  IntFct_t*  intfct = Element_GetIntFct(el) ;
  int NbOfIntPoints = IntFct_GetNbOfPoints(intfct) ;
  int    p ;
  
  /* We skip if the element is a submanifold */
  if(Element_IsSubmanifold(el)) return(0) ;

  /*
    Input data
  */
  GetProperties(el) ;
  
    
  /* Loop on integration points */
  for(p = 0 ; p < NbOfIntPoints ; p++) {
    /* Variables */
    double* x = ComputeVariables(el,u,u_n,vim_n,t,dt,p) ;
    
    /* storage in vim */
    {
      double* vim  = vim0 + p*NVI ;
      int    i ;
      
      M_L = x[I_M_L] ;
    
      for(i = 0 ; i < 3 ; i++) W_L[i] = x[I_W_L + i] ;
    
      for(i = 0 ; i < 9 ; i++) SIG[i] = x[I_SIG + i] ;
      
      for(i = 0 ; i < 3 ; i++) F_MASS[i] = x[I_Fmass + i] ;
      
      for(i = 0 ; i < 9 ; i++) EPS_P[i]  = x[I_EPS_P + i] ;
    
      CRIT = x[I_CRIT] ;
      HARDV = x[I_HARDV] ;
    }
  }
  
  return(0) ;
}



int  ComputeMatrix(Element_t* el,double t,double dt,double* k)
/** Compute the matrix (k) */
{
#define K(i,j)    (k[(i)*ndof + (j)])
  IntFct_t*  intfct = Element_GetIntFct(el) ;
  int nn = Element_GetNbOfNodes(el) ;
  int dim = Geometry_GetDimension(Element_GetGeometry(el)) ;
  int ndof = nn*NEQ ;
  FEM_t* fem = FEM_GetInstance(el) ;


  /* Initialization */
  {
    double zero = 0. ;
    int    i ;
    
    for(i = 0 ; i < ndof*ndof ; i++) k[i] = zero ;
  }


  /* We skip if the element is a submanifold */
  if(Element_IsSubmanifold(el)) return(0) ;
  
  
  /*
    Input data
  */
  GetProperties(el) ;


  /*
  ** Poromechanics matrix
  */
  {
    double c[IntFct_MaxNbOfIntPoints*100] ;
    int dec = ComputeTangentCoefficients(fem,dt,c) ;
    double* kp = FEM_ComputePoroelasticMatrix(fem,intfct,c,dec,1) ;
    /* The matrix kp is stored as (u for displacement, s1,s2 for pressure)
     * | Kuu  Kup2  Kup1  |
     * | Kp2u Kp2p2 Kp2p1 |
     * | Kp1u Kp1p2 Kp1p1 |
     * i.e. the displacements u are in the positions 0 to dim-1 and
     * the pressure p is in the position dim.
     * So we need to store the matrix by accounting for the right indexes.
     */
    {
      int i ;
      
      for(i = 0 ; i < ndof*ndof ; i++) {
        k[i] = kp[i] ;
      }
    }
  }
  
  /*
  ** Conduction Matrix
  */
  {
    double c[IntFct_MaxNbOfIntPoints*100] ;
    int dec = ComputeTransferCoefficients(fem,dt,c) ;
    double* kc = FEM_ComputeConductionMatrix(fem,intfct,c,dec) ;
    int    i ;
  
    for(i = 0 ; i < nn ; i++) {
      int    j ;
      
      for(j = 0 ; j < nn ; j++) {
        K(E_liq + i*NEQ,U_p_l1 + j*NEQ) += dt*kc[i*nn + j] ;
      }
    }
  }
  
  return(0) ;
#undef K
}




int  ComputeResidu(Element_t* el,double t,double dt,double* r)
/** Comput the residu (r) */
{
#define R(n,i)    (r[(n)*NEQ+(i)])
  double* vim_1 = Element_GetCurrentImplicitTerm(el) ;
  double* vim_n = Element_GetPreviousImplicitTerm(el) ;
  int nn = Element_GetNbOfNodes(el) ;
  int dim = Geometry_GetDimension(Element_GetGeometry(el)) ;
  IntFct_t*  intfct = Element_GetIntFct(el) ;
  int np = IntFct_GetNbOfPoints(intfct) ;
  int ndof = nn*NEQ ;
  FEM_t* fem = FEM_GetInstance(el) ;
  int    i ;
  double zero = 0. ;

  /* Initialization */
  for(i = 0 ; i < ndof ; i++) r[i] = zero ;

  if(Element_IsSubmanifold(el)) return(0) ;


  /* 1. Mechanics */
  
  /* 1.1 Stresses */
  {
    double* vim = vim_1 ;
    double* rw = FEM_ComputeStrainWorkResidu(fem,intfct,SIG,NVI) ;
    
    for(i = 0 ; i < nn ; i++) {
      int j ;
      
      for(j = 0 ; j < dim ; j++) R(i,E_mec + j) -= rw[i*dim + j] ;
    }
    
  }
  
  /* 1.2 Body forces */
  {
    double* vim = vim_1 ;
    double* rbf = FEM_ComputeBodyForceResidu(fem,intfct,F_MASS + dim - 1,NVI) ;
    
    for(i = 0 ; i < nn ; i++) {
      R(i,E_mec + dim - 1) -= -rbf[i] ;
    }
    
  }
  
  
  /* 2. Conservation of total mass */
  
  /* 2.1 Accumulation Terms */
  {
    double* vim = vim_1 ;
    double g1[IntFct_MaxNbOfIntPoints] ;
    
    for(i = 0 ; i < np ; i++ , vim += NVI , vim_n += NVI) g1[i] = M_L - M_L_n ;
    
    {
      double* ra = FEM_ComputeBodyForceResidu(fem,intfct,g1,1) ;
    
      for(i = 0 ; i < nn ; i++) R(i,E_liq) -= ra[i] ;
    }
  }
  
  /* 2.2 Transport Terms */
  {
    double* vim = vim_1 ;
    double* rf = FEM_ComputeFluxResidu(fem,intfct,W_L,NVI) ;
    
    for(i = 0 ; i < nn ; i++) R(i,E_liq) -= -dt*rf[i] ;
  }
  
  
  /* 3. Conservation of mass in pellets */
  
  /* 3.1 Accumulation Terms */
  {
    double* vim = vim_1 ;
    double g1[IntFct_MaxNbOfIntPoints] ;
    
    for(i = 0 ; i < np ; i++ , vim += NVI , vim_n += NVI) g1[i] = M_L1 - M_L1_n ;
    
    {
      double* ra = FEM_ComputeBodyForceResidu(fem,intfct,g1,1) ;
    
      for(i = 0 ; i < nn ; i++) R(i,E_liq1) -= ra[i] ;
    }
    
    /* */
    {
    }
  }
  
  return(0) ;
#undef R
}



int  ComputeOutputs(Element_t* el,double t,double* s,Result_t* r)
/** Compute the outputs (r) */
{
  int NbOfOutputs = 8 ;
  double* vex  = Element_GetExplicitTerm(el) ;
  double* vim  = Element_GetCurrentImplicitTerm(el) ;
  double** u   = Element_ComputePointerToCurrentNodalUnknowns(el) ;
  IntFct_t*  intfct = Element_GetIntFct(el) ;
  int np = IntFct_GetNbOfPoints(intfct) ;
  int dim = Geometry_GetDimension(Element_GetGeometry(el)) ;
  FEM_t* fem = FEM_GetInstance(el) ;

  if(Element_IsSubmanifold(el)) return(0) ;

  /* Initialization */
  {
    int    i ;
    
    for(i = 0 ; i < NbOfOutputs ; i++) {
      Result_SetValuesToZero(r + i) ;
    }
  }
  
  /*
    Input data
  */
  GetProperties(el) ;

  {
    /* Interpolation functions at s */
    double* a = Element_ComputeCoordinateInReferenceFrame(el,s) ;
    int p = IntFct_ComputeFunctionIndexAtPointOfReferenceFrame(intfct,a) ;
    /* Pressures */
    double p_l1 = FEM_ComputeUnknown(fem,u,intfct,p,U_p_l1) ;
    double p_l2 = FEM_ComputeUnknown(fem,u,intfct,p,U_p_l2) ;
    /* Displacement */
    double dis[3] = {0,0,0} ;
    /* strains */
    double eps[9] = {0,0,0,0,0,0,0,0,0} ;
    double eps_p[9] = {0,0,0,0,0,0,0,0,0} ;
    double tre,e ;
    double w_l[3] = {0,0,0} ;
    double sig[9] = {0,0,0,0,0,0,0,0,0} ;
    double hardv = 0 ;
    double k_h = 0 ;
    int    i ;
    
    for(i = 0 ; i < dim ; i++) {
      dis[i] = FEM_ComputeUnknown(fem,u,intfct,p,U_u + i) ;
    }
    
    /* Averaging */
    for(i = 0 ; i < np ; i++ , vim += NVI , vex += NVE) {
      double* def =  FEM_ComputeLinearStrainTensor(fem,u,intfct,i,U_u) ;
      int j ;
      
      for(j = 0 ; j < 3 ; j++) w_l[j] += W_L[j]/np ;

      for(j = 0 ; j < 9 ; j++) sig[j] += SIG[j]/np ;
      
      for(j = 0 ; j < 9 ; j++) eps_p[j] += EPS_P[j]/np ;
      
      for(j = 0 ; j < 9 ; j++) eps[j] += def[j]/np ;
      
      hardv += HARDV/np ;
      
      k_h += K_L/np ;
    }
    
    tre = eps[0] + eps[4] + eps[8] ;
    e   = (1 + e0) * tre ;
      
    i = 0 ;
    Result_Store(r + i++,&p_l1    ,"Pore pressure in pellets",1) ;
    Result_Store(r + i++,&p_l2    ,"Pore pressure in powder",1) ;
    Result_Store(r + i++,dis      ,"Displacements",3) ;
    Result_Store(r + i++,w_l      ,"Fluid mass flow",3) ;
    Result_Store(r + i++,sig      ,"Stresses",9) ;
    Result_Store(r + i++,&e       ,"Void ratio variation",1) ;
    Result_Store(r + i++,eps_p    ,"Plastic strains",9) ;
    Result_Store(r + i++,&hardv   ,"Hardening variable",1) ;
  }
  
  return(NbOfOutputs) ;
}



int ComputeTangentCoefficients(FEM_t* fem,double dt,double* c)
/*
**  Tangent matrix (c), return the shift (dec).
*/
{
#define T4(a,i,j,k,l)  ((a)[(((i)*3+(j))*3+(k))*3+(l)])
#define T2(a,i,j)      ((a)[(i)*3+(j)])
#define C1(i,j,k,l)    T4(c1,i,j,k,l)
#define B1(i,j)        T2(c1,i,j)
  Element_t* el  = FEM_GetElement(fem) ;
  double*  vim0  = Element_GetCurrentImplicitTerm(el) ;
//  double*  vim_n = Element_GetPreviousImplicitTerm(el) ;
//  double*  vex0  = Element_GetExplicitTerm(el) ;
  double** u     = Element_ComputePointerToCurrentNodalUnknowns(el) ;
//  double** u_n   = Element_ComputePointerToPreviousNodalUnknowns(el) ;
  int dim = Element_GetDimensionOfSpace(el) ;
  IntFct_t*  intfct = Element_GetIntFct(el) ;
  int np = IntFct_GetNbOfPoints(intfct) ;
  
  int    dec = 100 ;
  int    p ;
  double zero = 0. ;
  
  /*
  ObVal_t* obval = Element_GetObjectiveValue(el) ;
  double dxi[Model_MaxNbOfEquations] ;
  
  
  {
    int i ;
    
    for(i = 0 ; i < NEQ ; i++) {
      dxi[i] =  1.e-2*ObVal_GetValue(obval + i) ;
    }
  }
  */

  
  for(p = 0 ; p < np ; p++) {
    double* vim  = vim0 + p*NVI ;
    double* c0 = c + p*dec ;
    /* Variables */
    //double* x = ComputeVariables(el,u,u_n,vim_n,t,dt,p) ;
    
    /* Pressures */
    double p_l1 = FEM_ComputeUnknown(fem,u,intfct,p,U_p_l1) ;
    double p_l2 = FEM_ComputeUnknown(fem,u,intfct,p,U_p_l2) ;
    double s1 = p_g - p_l1 ;
    double s2 = p_g - p_l2 ;


    /* initialization */
    {
      int i ;
      
      for(i = 0 ; i < dec ; i++) c0[i] = zero ;
    }
    

    /* Mechanics */
    {
      double sig[9] ;
      int i ;
    
      for(i = 0 ; i < 9 ; i++) sig[i] = SIG[i] ;
    
      
      /* Tangent stiffness matrix */
      {
        double* c1 = c0 ;
        double crit = CRIT ;
        
        {
          double sigm    = (sig[0] + sig[4] + sig[8])/3. ;
          /* A CALCULER */
          double bulk    = 0 ; //-(1 + e0)*sigm..... ;
          double young   = 3 * bulk * (1 - 2*poisson) ;
          
          Elasticity_SetParameters(elasty,young,poisson) ;
        }

        Elasticity_ComputeStiffnessTensor(elasty,c1) ;
      
        {
          /* Criterion */
          if(crit >= 0.) {
            double lcf   = LoadingCollapseFactor(s2) ;
            double logp_co = log(p_c) + log(HARDV/p_c) * lcf ;
            double p_co    = exp(logp_co) ;
            double p_s     = k_s * s2 ;
            double hardv[2] = {p_co,p_s} ;
            double crit1 = ComputeFunctionGradients(sig,hardv) ;
            double fcg   = UpdateElastoplasticTensor(c1) ;
          
            if(fcg < 0) return(-1) ;
          }
        }
      }
      
      
      /* Coupling matrix (s1) */
      {
        double* c1 = c0 + 81 ;
        
        //for(i = 0 ; i < 3 ; i++) B1(i,i) = - xxxx ;
      }
      
      
      /* Coupling matrix (s2) */
      {
        double* c1 = c0 + 81 + 9 ;
        
        //for(i = 0 ; i < 3 ; i++) B1(i,i) = - xxxx ;
      }
    }
    
    
    /* Conservation of total mass */
    #if 0
    {
      /* Fluid mass density */
      double rho_l = rho_l0 ;
    
    
      /* Coupling matrix */
      {
        double* c1 = c0 + 81 + 9 + 9 ;
        int i ;
        
        //for(i = 0 ; i < 3 ; i++) B1(i,i) = xxx ;
      }
      
      
      /* Storage matrix */
      {
        double* c1 = c0 + 81 + 9 + 9 + 9 ;
        //double dxk   = dxi[U_p_l1] ;
        //int    k     = I_P_L ;
        //double* dx   = ComputeVariablesDerivatives(el,t,dt,x,dxk,k) ;
        /* Porosity */
        double* eps  = FEM_ComputeLinearStrainTensor(fem,u,intfct,p,U_u) ;
        double tre   = eps[0] + eps[4] + eps[8] ;
        double phi   = phi0 + tre ;
        
        c1[0] = - rho_l*phi*dSaturationDegree(pc) ;
      }
    }
    #endif
  }
  
  return(dec) ;
#undef C1
#undef B1
#undef T2
#undef T4
}



int ComputeTransferCoefficients(FEM_t* fem,double dt,double* c)
/*
**  Conduction matrix (c) and shift (dec)
*/
{
  Element_t* el = FEM_GetElement(fem) ;
  double* vex0 = Element_GetExplicitTerm(el) ;
  IntFct_t*  intfct = Element_GetIntFct(el) ;
  int np = IntFct_GetNbOfPoints(intfct) ;
  int    dec = 9 ;
  int    p ;
  double zero = 0. ;
  

  for(p = 0 ; p < np ; p++) {
    int i ;
    double* c1 = c + p*dec ;
    
    /* initialization */
    for(i = 0 ; i < dec ; i++) c1[i] = zero ;
    
    {
      double* vex  = vex0 + p*NVE ;
      
      /* Permeability tensor */
      c1[0] = K_L ;
      c1[4] = K_L ;
      c1[8] = K_L ;
    }
  }
  
  return(dec) ;
}





double* ComputeVariables(Element_t* el,void* vu,void* vu_n,void* vf_n,const double t,const double dt,const int p)
{
  IntFct_t* intfct = Element_GetIntFct(el) ;
  FEM_t*    fem    = FEM_GetInstance(el) ;
  int dim = Element_GetDimensionOfSpace(el) ;
//  Model_t*  model  = Element_GetModel(el) ;
//  double*   x      = Model_GetVariable(model,p) ;
  /* cast when type "const void*" is used 
  const double* const* u   = (const double* const*) vu ;
  const double* const* u_n = (const double* const*) vu_n ;
  const double*        f_n = (const double*) vf_n ;
  */
  double** u   = (double**) vu ;
  double** u_n = (double**) vu_n ;
  double*  f_n = (double*)  vf_n ;
  double*  x   = Variable ;
  double*  x_n = Variable_n ;
  
    
  /* Primary Variables */
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
    
    /* Pressures */
    x[I_P_L1] = FEM_ComputeUnknown(fem,u,intfct,p,U_p_l1) ;
    x[I_P_L2] = FEM_ComputeUnknown(fem,u,intfct,p,U_p_l2) ;
    
    /* Pressure gradient */
    {
      double* grd = FEM_ComputeUnknownGradient(fem,u,intfct,p,U_p_l2) ;
    
      for(i = 0 ; i < 3 ; i++) {
        x[I_GRD_P_L2 + i] = grd[i] ;
      }
      
      FEM_FreeBufferFrom(fem,grd) ;
    }
  }
  
  
  /* Needed variables to compute secondary variables */
  {
    int    i ;
    
    /* Stresses, strains at previous time step */
    {
      double* eps_n =  FEM_ComputeLinearStrainTensor(fem,u_n,intfct,p,U_u) ;
      double* vim_n = f_n + p*NVI ;
    
      for(i = 0 ; i < 9 ; i++) {
        x_n[I_EPS   + i] = eps_n[i] ;
        x_n[I_SIG   + i] = SIG_n[i] ;
        x_n[I_EPS_P + i] = EPS_P_n[i] ;
      }
      
      x_n[I_HARDV] = HARDV_n ;
      
      FEM_FreeBufferFrom(fem,eps_n) ;
    }
    
    /* Pressures at previous time step */
    x_n[I_P_L1] = FEM_ComputeUnknown(fem,u_n,intfct,p,U_p_l1) ;
    x_n[I_P_L2] = FEM_ComputeUnknown(fem,u_n,intfct,p,U_p_l2) ;
    
    /* Transfer coefficient */
    {
      double* vex0 = Element_GetExplicitTerm(el) ;
      double* vex  = vex0 + p*NVE ;
      
      x_n[I_K_H]  = K_L ;
    }
  }
    
  ComputeSecondaryVariables(el,t,dt,x_n,x) ;
  
  return(x) ;
}



void  ComputeSecondaryVariables(Element_t* el,double t,double dt,double* x_n,double* x)
{
  /* Inputs 
   * ------*/
  int dim = Element_GetDimensionOfSpace(el) ;
  /* Strains */
  double* eps   =  x   + I_EPS ;
  double* eps_n =  x_n + I_EPS ;
  /* Stresses */
  double* sig_n =  x_n + I_SIG ;
  /* Plastic strains */
  double* eps_pn = x_n + I_EPS_P ;
  /* Pressures */
  double  p_l1   = x[I_P_L1] ;
  double  p_l1n  = x_n[I_P_L1] ;
  double  p_l2   = x[I_P_L2] ;
  double  p_l2n  = x_n[I_P_L2] ;
  /* Suctions */
  double s1  = p_g - p_l1 ;
  double s1n = p_g - p_l1n ;
  double s2  = p_g - p_l2 ;
  double s2n = p_g - p_l2n ;
    

  /* Outputs 
   * ------*/

  /* Backup stresses, plastic strains */
  {
    double* sig   = x + I_SIG ;
    double* eps_p = x + I_EPS_P ;
    
    {
      double deps[9] ;
      int    i ;
      
      /* Incremental deformations */
      for(i = 0 ; i < 9 ; i++) deps[i] =  eps[i] - eps_n[i] ;
      
      /* Stresses at t_n */
      for(i = 0 ; i < 9 ; i++) sig[i] = sig_n[i] ;
    
      /* Elastic trial stresses at t */
      {
        /* A FAIRE */
      }
      /*{
        double sigm_n    = (sig_n[0] + sig_n[4] + sig_n[8])/3. ;
        double sigmeff_n = sigm_n + pp_n ;
        double trde      = deps[0] + deps[4] + deps[8] ;
        double sigmeff   = sigmeff_n*exp(-(1 + e0)*trde/kappa) ;
        double dsigmeff  = sigmeff - sigmeff_n ;
        
        for(i = 0 ; i < 9 ; i++) sig[i] = sigeff_n[i] + 2*mu*deps[i] ;
      
        sig[0] += dsigmeff - 2*mu*trde/3. ;
        sig[4] += dsigmeff - 2*mu*trde/3. ;
        sig[8] += dsigmeff - 2*mu*trde/3. ;
      }*/
    
      /* Plastic strains */
      for(i = 0 ; i < 9 ; i++) eps_p[i] = eps_pn[i] ;
    
      /* Return mapping */
      {
        double s2    = p_g - p_l2 ;
        double p_con = x_n[I_HARDV] ;
        double lcf   = LoadingCollapseFactor(s2) ;
        double logp_co = log(p_c) + log(p_con/p_c) * lcf ;
        double p_co    = exp(logp_co) ;
        double p_s     = k_s * s2 ;
        double hardv[2] = {p_co,p_s} ;
        double crit  = ReturnMapping(sig,eps_p,hardv) ;
        
        p_co = p_c * exp(log(hardv[0]/p_c) / lcf) ;
        
        x[I_CRIT]  = crit ;
        x[I_HARDV] = p_co ;
      }
    }
  }
  
  /* Backup microscopic volumetric strains epsv1 and epsv2 */
  {
    double* sig  = x + I_SIG ;
    double sigm = (sig[0] + sig[4] + sig[8])/3. ;
    
    //x[I_EPSV_1] = xxxxx ;
    //x[I_EPSV_2] = xxxxx ;
  }
  
  
  /* Backup mass flow */
  #if 0
  {
    /* Porosity */
    double tre   = eps[0] + eps[4] + eps[8] ;
    double phi   = phi0 + tre ;
    
    /* Fluid mass density */
    double rho_l = rho_l0 ;
    
    /* Fluid mass flow */
    {
      /* Transfer coefficient */
      double k_h = x[I_K_H] ;
    
      /* Pressure gradient */
      double* gpl = x + I_GRD_P_L2 ;
    
      /* Mass flow */
      double* w_l = x + I_W_L ;
      int i ;
    
      for(i = 0 ; i < 3 ; i++) w_l[i] = - k_h*gpl[i] ;
      w_l[dim - 1] += k_h*rho_l*gravity ;
    }
    
    /* Total liquid mass content and body force */
    {
      double  pc  = p_g - p_l ;
      double  m_l = rho_l*phi*sl ;
      double* f_mass = x + I_Fmass ;
      int i ;
    
      x[I_M_L]   = m_l ;
      x[I_RHO_L] = rho_l ;
      x[I_PHI]   = phi ;
      
      for(i = 0 ; i < 3 ; i++) f_mass[i] = 0 ;
      f_mass[dim - 1] = (rho_s + m_l)*gravity ;
    }
    
    /* Liquid mass content in pellets */
    {
      //x[I_M_L1]   = xxxx ;
    }
  }
  #endif
}






#if 0
double* ComputeVariablesDerivatives(Element_t* el,double t,double dt,double* x,double dxi,int i)
{
  double* dx = dVariable ;
  int j ;
  
  /* Primary Variables */
  for(j = 0 ; j < NbOfVariables ; j++) {
    dx[j] = x[j] ;
  }
  
  /* We increment the variable as (x + dx) */
  dx[i] += dxi ;
  
  ComputeSecondaryVariables(el,t,dt,x_n,dx) ;
  
  /* The numerical derivative as (f(x + dx) - f(x))/dx */
  for(j = 0 ; j < NbOfVariables ; j++) {
    dx[j] -= x[j] ;
    dx[j] /= dxi ;
  }

  return(dx) ;
}
#endif



double pie(double pl,double pg,Curve_t* cb)
{
  double  pc,sl,sg,u ;
  int     n_i = Curve_GetNbOfPoints(cb) - 1 ;
  double  pc1 = Curve_GetXRange(cb)[0] ;
  double  pc2 = Curve_GetXRange(cb)[1] ;
  double* sat = Curve_GetYValue(cb) ;
  double  dpc = (pc2 - pc1)/n_i ;
  double  zero = 0.,un = 1. ;

  pc  = pg - pl ;
  
  /* U */
  if(pc < pc1) {
    u = zero ;
    sl = sat[0] ;
    
  } else {
    int     i ;
    
    if(pc > pc2) pc = pc2 ;
    u  = zero ;
    for(i = 0 ; pc1 + (i + 1)*dpc < pc ; i++) u += sat[i] + sat[i+1] ;
    u *= dpc*0.5 ;
    u += sat[0]*pc1 ;
    sl  = sat[i] + (sat[i+1] - sat[i])/dpc*(pc - pc1 - i*dpc) ;
    u += (sat[i] + sl)*0.5*(pc - pc1 - i*dpc) - sl*pc ;
  }
  
  /* pi */
  sg  = un - sl ;
  return(sl*pl + sg*pg - 2./3.*u) ;
}


double dpiesdpl(double pl,double pg,Curve_t* cb)
{
  double pc  = pg - pl ;
  double sl  = Curve_ComputeValue(cb,pc) ;
  double dslsdpc = Curve_ComputeDerivative(cb,pc) ;
  double dusdpc  = - pc*dslsdpc ;
  
  return(sl - dusdpc/3) ;
}
