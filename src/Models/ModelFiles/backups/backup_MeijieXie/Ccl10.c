/* General features of the model:
 * Curves for CSH:
 *   - C/S ratio
 *   - H/S ratio
 *   - Molar Volume
 * Alkalis (as sodium and potassium compounds)
 * Dissolution kinetics for CH based on spherical crystal 
 * coated by a calcite layer.
 * Dissolution and continuous decalcification of CSH
 * Precipitation/Dissolution of CC
 * Use of Zeta unknowns for Calcium and Silicon
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "CommonModel.h"

/* The Finite Volume Method */
#include "FVM.h"

/* Cement chemistry */
#include "HardenedCementChemistry.h"
#include "CementSolutionDiffusion.h"

#define TITLE   "Carbonation Of CBM (2017)"
#define AUTHORS "Morandeau-Thiery-Dangla"

#include "PredefinedMethods.h"


/* Nb of equations */
#define NEQ    	  (8)


/* Nb of nodes (el must be used below) */
#define NN     Element_GetNbOfNodes(el)


/* Nb of terms */
#define NVE    	  ((9 + CementSolutionDiffusion_NbOfConcentrations)*NN)
#define NVI       (8*NN*NN + 2*NN)
#define NV0       (2)


/* Indices of equations */
#define E_C       (0)
#define E_q       (1)
#define E_mass    (2)
#define E_Ca      (3)
#define E_Na      (5)
#define E_K       (6)
#define E_Si      (4)
#define E_Cl      (7)



/* Indices of unknowns */
#define U_C_CO2   (0)
#define U_P_L     (2)
#define U_ZN_Ca_S (3)
#define U_PSI     (1)
#define U_C_Na    (5)
#define U_C_K     (6)
#define U_ZN_Si_S (4)
#define U_C_Cl    (7)


#define NOLOG_U   1
#define LOG_U     2
#define Ln10      Math_Ln10
#define U_CO2     LOG_U



/* Value of the nodal unknown (u, u_n and el must be used below) */
#define UNKNOWN(n,i)     Element_GetValueOfNodalUnknown(el,u,n,i)
#define UNKNOWNn(n,i)    Element_GetValueOfNodalUnknown(el,u_n,n,i)





/* Names of nodal unknowns */
#if (U_CO2 == LOG_U)
#define LogC_CO2(n)	  (UNKNOWN(n,U_C_CO2))
#define LogC_CO2n(n)	(UNKNOWNn(n,U_C_CO2))
#define C_CO2(n)	    (pow(10,UNKNOWN(n,U_C_CO2)))
#define C_CO2n(n)	    (pow(10,UNKNOWNn(n,U_C_CO2)))
#else
#define C_CO2(n)	    (UNKNOWN(n,U_C_CO2))
#define C_CO2n(n)	    (UNKNOWNn(n,U_C_CO2))
#define LogC_CO2(n)	  (log10(UNKNOWN(n,U_C_CO2)))
#define LogC_CO2n(n)	(log10(UNKNOWNn(n,U_C_CO2)))
#endif

#define ZN_Si_S(n)    (UNKNOWN(n,U_ZN_Si_S))
#define ZN_Si_Sn(n)   (UNKNOWNn(n,U_ZN_Si_S))

#define ZN_Ca_S(n)    (UNKNOWN(n,U_ZN_Ca_S))
#define ZN_Ca_Sn(n)   (UNKNOWNn(n,U_ZN_Ca_S))

#define P_L(n)        (UNKNOWN(n,U_P_L))
#define P_Ln(n)       (UNKNOWNn(n,U_P_L))

#define PSI(n)        (UNKNOWN(n,U_PSI))
#define PSIn(n)       (UNKNOWNn(n,U_PSI))

#if (U_Na == LOG_U)
  #define LogC_Na(n)    (UNKNOWN(n,U_C_Na))
  #define LogC_Nan(n)   (UNKNOWNn(n,U_C_Na))
  #define C_Na(n)       (pow(10,UNKNOWN(n,U_C_Na)))
  #define C_Nan(n)      (pow(10,UNKNOWNn(n,U_C_Na)))
#else
  #define C_Na(n)       (UNKNOWN(n,U_C_Na))
  #define C_Nan(n)      (UNKNOWNn(n,U_C_Na))
  #define LogC_Na(n)    (log10(UNKNOWN(n,U_C_Na)))
  #define LogC_Nan(n)   (log10(UNKNOWNn(n,U_C_Na)))
#endif

#if (U_K == LOG_U)
  #define LogC_K(n)     (UNKNOWN(n,U_C_K))
  #define LogC_Kn(n)    (UNKNOWNn(n,U_C_K))
  #define C_K(n)        (pow(10,UNKNOWN(n,U_C_K)))
  #define C_Kn(n)       (pow(10,UNKNOWNn(n,U_C_K)))
#else
  #define C_K(n)        (UNKNOWN(n,U_C_K))
  #define C_Kn(n)       (UNKNOWNn(n,U_C_K))
  #define LogC_K(n)     (log10(UNKNOWN(n,U_C_K)))
  #define LogC_Kn(n)    (log10(UNKNOWNn(n,U_C_K)))
#endif

#if (U_Cl == LOG_U)
  #define LogC_Cl(n)     (UNKNOWN(n,U_C_Cl))
  #define LogC_Cln(n)    (UNKNOWNn(n,U_C_Cl))
  #define C_Cl(n)        (pow(10,UNKNOWN(n,U_C_Cl)))
  #define C_Cln(n)       (pow(10,UNKNOWNn(n,U_C_Cl)))
#else
  #define C_Cl(n)        (UNKNOWN(n,U_C_Cl))
  #define C_Cln(n)       (UNKNOWNn(n,U_C_Cl))
  #define LogC_Cl(n)     (log10(UNKNOWN(n,U_C_Cl)))
  #define LogC_Cln(n)    (log10(UNKNOWNn(n,U_C_Cl)))
#endif


/* Names used for implicit terms */
#define MassAndFlux(f,i,j)  ((f)[((i)*NN + (j))])

#define NW_C          (f)
#define NW_Cn         (f_n)
#define N_C(i)        MassAndFlux(NW_C,i,i)
#define N_Cn(i)       MassAndFlux(NW_Cn,i,i)
#define W_C(i,j)      MassAndFlux(NW_C,i,j)

#define NW_q          (f   + NN*NN)
#define NW_qn         (f_n + NN*NN)
#define N_q(i)        MassAndFlux(NW_q,i,i)
#define N_qn(i)       MassAndFlux(NW_qn,i,i)
#define W_q(i,j)      MassAndFlux(NW_q,i,j)

#define MW_tot        (f   + 2*NN*NN)
#define MW_totn       (f_n + 2*NN*NN)
#define M_tot(i)      MassAndFlux(MW_tot,i,i)
#define M_totn(i)     MassAndFlux(MW_totn,i,i)
#define W_tot(i,j)    MassAndFlux(MW_tot,i,j)

#define NW_Ca         (f   + 3*NN*NN)
#define NW_Can        (f_n + 3*NN*NN)
#define N_Ca(i)       MassAndFlux(NW_Ca,i,i)
#define N_Can(i)      MassAndFlux(NW_Can,i,i)
#define W_Ca(i,j)     MassAndFlux(NW_Ca,i,j)

#define NW_Na         (f   + 4*NN*NN)
#define NW_Nan        (f_n + 4*NN*NN)
#define N_Na(i)       MassAndFlux(NW_Na,i,i)
#define N_Nan(i)      MassAndFlux(NW_Nan,i,i)
#define W_Na(i,j)     MassAndFlux(NW_Na,i,j)

#define NW_K          (f   + 5*NN*NN)
#define NW_Kn         (f_n + 5*NN*NN)
#define N_K(i)        MassAndFlux(NW_K,i,i)
#define N_Kn(i)       MassAndFlux(NW_Kn,i,i)
#define W_K(i,j)      MassAndFlux(NW_K,i,j)

#define NW_Si         (f   + 6*NN*NN)
#define NW_Sin        (f_n + 6*NN*NN)
#define N_Si(i)       MassAndFlux(NW_Si,i,i)
#define N_Sin(i)      MassAndFlux(NW_Sin,i,i)
#define W_Si(i,j)     MassAndFlux(NW_Si,i,j)

#define NW_Cl         (f   + 7*NN*NN)
#define NW_Cln        (f_n + 7*NN*NN)
#define N_Cl(i)       MassAndFlux(NW_Cl,i,i)
#define N_Cln(i)      MassAndFlux(NW_Cln,i,i)
#define W_Cl(i,j)     MassAndFlux(NW_Cl,i,j)

#define N_CH(i)       (f   + 8*NN*NN)[i]
#define N_CHn(i)      (f_n + 8*NN*NN)[i]

#define C_OH(i)       (f   + 8*NN*NN + NN)[i]
#define C_OHn(i)      (f_n + 8*NN*NN + NN)[i]



/* Names used for explicit terms */
#define TransferCoefficient(va,n)  ((va) + (n)*NN)

#define KF_CO2          TransferCoefficient(va,0)

#define KD_L            TransferCoefficient(va,1)
#define KD_C_L          TransferCoefficient(va,2)
#define KD_Ca_L         TransferCoefficient(va,3)
#define KD_Na_L         TransferCoefficient(va,4)
#define KD_K_L          TransferCoefficient(va,5)
#define KD_Si_L         TransferCoefficient(va,6)
#define KD_Cl_L         TransferCoefficient(va,7)

#define TORTUOSITY      TransferCoefficient(va,8)

#define CONCENTRATION(i)  (TransferCoefficient(va,9) + (i)*CementSolutionDiffusion_NbOfConcentrations)



/* Names used for constant terms */
#define V_S0(n)         (v0[(0+n)])




/* Units
 * ----- */
#include "InternationalSystemOfUnits.h"
/* Shorthands of some units */
#define dm    (0.1*InternationalSystemOfUnits_OneMeter)
#define cm    (0.01*InternationalSystemOfUnits_OneMeter)
#define dm2   (dm*dm)
#define dm3   (dm*dm*dm)
#define cm3   (cm*cm*cm)
#define MPa   (1.e6*InternationalSystemOfUnits_OnePascal)
#define GPa   (1.e3*MPa)
#define mol   InternationalSystemOfUnits_OneMole
#define sec   InternationalSystemOfUnits_OneSecond


#define TEMPERATURE  (293.15)




#include "MolarMassOfMolecule.h"


/* Water property
 * -------------- */
#define M_H2O          MolarMassOfMolecule(H2O)


/* CO2 gas properties
 * ------------------ */
#define M_CO2          MolarMassOfMolecule(CO2)
/* Henry's laww constant for the solubility of CO2 gas */
#define k_h           (0.9983046)                /* CO2(g) = CO2(aq) (T = 293K)*/




/* Material Properties
 * ------------------- */
#define SaturationDegree(p)              (Curve_ComputeValue(Element_GetCurve(el),p))
#define RelativePermeabilityToLiquid(p)  (Curve_ComputeValue(Element_GetCurve(el) + 1,p))




/* Calcium Silicate Hydrate Properties (C-S-H)
 * ------------------------------------------- */
#define M_CaO          MolarMassOfMolecule(CaO)
#define M_SiO2         MolarMassOfMolecule(SiO2)
//#define MolarVolumeOfCSH(s_ch)           (Curve_ComputeValue(Element_GetCurve(el) + 4,s_ch))
#define V_CSH         (78 * cm3)
#define V_SH          (43 * cm3)
#define MolarVolumeOfCSH(x)    ((x)/1.7*V_CSH + (1 - (x)/1.7)*V_SH)



/* Calcium Hydroxide (Portlandite) Properties (CH)
 * ----------------------------------------------- */
#define M_CaOH2        MolarMassOfMolecule(CaO2H2)
/* Molar volume of CH solid (dm3/mole) */
#define V_CH	    	  (33 * cm3)



/* Calcium Carbonate (Calcite) Properties (CC)
 * ------------------------------------------- */
#define M_CaCO3        MolarMassOfMolecule(CaCO3)
/* Molar volume of CC (dm3/mole) */
#define V_CC	      	(37 * cm3)


/* Chloride adsorption curve 
 * ------------------------- */
#define AlphaCoef(x) \
        (Curve_ComputeValue(Element_GetCurve(el) + 2,x))
#define BetaCoef(x) \
        (Curve_ComputeValue(Element_GetCurve(el) + 3,x))
#define AdsorbedChloridePerUnitMoleOfCSH(c_cl,x) \
        (AlphaCoef(x) * (c_cl) / (1. + BetaCoef(x) * (c_cl)))

/* Sodium adsorption curve 
 * ------------------------- */
#define RNa(x) \
        (Curve_ComputeValue(Element_GetCurve(el) + 4,x))
#define AdsorbedSodiumPerUnitMoleOfCSH(c_na,x) \
        ((c_na < 0.3) ? (RNa(x) * (c_na)) : (RNa(x) * 0.3))

/* Potassium adsorption curve 
 * ------------------------- */
#define RK(x) \
        (Curve_ComputeValue(Element_GetCurve(el) + 5,x))
#define AdsorbedPotassiumPerUnitMoleOfCSH(c_k,x) \
        ((c_k < 0.3) ? (RK(x) * (c_k)) : (RK(x) * 0.3)) 


/* Element contents in solid phases  */
#define n_ca_ref                           (n_ch0)
#define n_si_ref                           (n_csh0)
#define CalciumContentInCHAndCC(zn_ca_s)   (n_ca_ref*MAX(zn_ca_s,0.))
#define SiliconContentInCSH(zn_si_s)       (n_si_ref*MAX(zn_si_s,0.))



/* Access to Properties */
#define GetProperty(a)                   (Element_GetProperty(el)[pm(a)])



static int     pm(const char* s) ;
static void    GetProperties(Element_t*) ;

static double* ComputeVariables(Element_t*,double**,double*,double,int) ;
static Model_ComputeSecondaryVariables_t    ComputeSecondaryVariables ;
static double* ComputeVariableDerivatives(Element_t*,double,double*,double,int) ;

static double  dn1_caoh2sdt(double,double) ;

static void    ComputeTransferCoefficients(Element_t*,double**,double*) ;
static double* ComputeVariableFluxes(Element_t*,double**,int,int) ;
static double* ComputeFluxes(Element_t*,double*,int,int) ;

static int     TangentCoefficients(Element_t*,double,double*) ;


static double  PermeabilityCoefficient(Element_t*,double) ;
static void    ComputePhysicoChemicalProperties(double) ;

static void concentrations_oh_na_k(double,double,double,double,double,double) ;


/* Internal parameters */
static double phi0 ; 
static double phimin ;
static double k_int,frac,phi_r ;
static double a_2,c_2 ;
static double tau_ch ;
static double n_ch0,n_csh0,c_na0,c_k0 ;
static double p_g = 0. ;

static double d_co2 ;

static double mu_l ;

static CementSolutionDiffusion_t* csd = NULL ;
static HardenedCementChemistry_t* hcc = NULL ;



#include "WaterViscosity.h"
#include "DiffusionCoefficientOfMoleculeInAir.h"


void ComputePhysicoChemicalProperties(double TK)
{

  /* Diffusion Coefficient Of Molecules In Air (dm2/s) */
  d_co2      = DiffusionCoefficientOfMoleculeInAir(CO2,TK) ;
  
  /* Viscosity (Pa.s) */
  mu_l       = WaterViscosity(TK) ;
}


#define NbOfVariables    (29)
static double Variables[Element_MaxNbOfNodes][NbOfVariables] ;
static double dVariables[NbOfVariables] ;


#define I_C_CO2        (8)
#define I_P_L          (9)

#define I_N_C          (10)
#define I_N_Ca         (11)
#define I_N_Si         (12)
#define I_N_K          (13)
#define I_N_Na         (14)
#define I_Mass         (15)
#define I_N_Cl         (16)
#define I_N_Q          (17)
#define I_N_CC         (18)
#define I_N_Si_S       (19)
#define I_N_Ca_S       (20)

#define I_N_CH         (21)
#define I_V_S          (22)

#define I_N_CHn        (23)
#define I_V_S0         (24)

#define I_Phi          (25)

#define I_V_CSH        (26)

#define I_C_OH         (27)
#define I_C_OHn        (28)


#define NbOfVariableFluxes    (8)
static double VariableFluxes[Element_MaxNbOfNodes][NbOfVariableFluxes] ;

#define I_W_C           (0)
#define I_W_Ca          (1)
#define I_W_Si          (2)
#define I_W_Na          (3)
#define I_W_K           (4)
#define I_W_tot         (5)
#define I_W_Cl          (6)
#define I_W_q           (7)


int pm(const char* s)
{
  if(strcmp(s,"porosity") == 0)     return (0) ;
  else if(strcmp(s,"k_int") == 0)   return (1) ;
  else if(strcmp(s,"N_CH") == 0) return (2) ;
  else if(strcmp(s,"N_CSH") == 0)    return (4) ;
  else if(strcmp(s,"C_K") == 0)     return (5) ;
  else if(strcmp(s,"C_Na") == 0)    return (6) ;
  else if(strcmp(s,"A_2") == 0)     return (8) ;
  else if(strcmp(s,"C_2") == 0)     return (9) ;
  else if(strcmp(s,"Radius_CH") == 0) return (10) ;
  else if(strcmp(s,"D") == 0) 	    return (11) ;
  else if(strcmp(s,"Tau") == 0)     return (12) ;
  else if(strcmp(s,"frac") == 0)    return (13) ;
  else if(strcmp(s,"phi_r") == 0)   return (14) ;
  else if(strcmp(s,"phimin") == 0)   return (15) ;
  else return(-1) ;
}


void GetProperties(Element_t* el)
{
  phi0     = GetProperty("porosity") ;
  k_int    = GetProperty("k_int") ;
  a_2      = GetProperty("A_2") ;
  c_2      = GetProperty("C_2") ;
  n_ch0    = GetProperty("N_CH") ;
  n_csh0   = GetProperty("N_CSH") ;
  c_na0    = GetProperty("C_Na") ;
  c_k0     = GetProperty("C_K") ;
  frac     = GetProperty("frac") ;
  phi_r    = GetProperty("phi_r") ;
  tau_ch   = GetProperty("Tau") ;
  phimin   = GetProperty("phimin") ;
}


int SetModelProp(Model_t* model)
{
  Model_GetNbOfEquations(model) = NEQ ;
  
  Model_CopyNameOfEquation(model,E_C   ,"carbone") ;
  Model_CopyNameOfEquation(model,E_q   ,"charge") ;
  Model_CopyNameOfEquation(model,E_mass,"masse") ;
  Model_CopyNameOfEquation(model,E_Ca  ,"calcium") ;
  Model_CopyNameOfEquation(model,E_Na  ,"sodium") ;
  Model_CopyNameOfEquation(model,E_K   ,"potassium") ;
  Model_CopyNameOfEquation(model,E_Si  ,"silicon") ;
  Model_CopyNameOfEquation(model,E_Cl  ,"chlore") ;
  
  
#if (U_CO2 == LOG_U)
  Model_CopyNameOfUnknown(model,U_C_CO2 ,"logc_co2") ;
#else
  Model_CopyNameOfUnknown(model,U_C_CO2 ,"c_co2") ;
#endif

  Model_CopyNameOfUnknown(model,U_ZN_Si_S,"z_si") ;
  Model_CopyNameOfUnknown(model,U_P_L    ,"p_l") ;
  Model_CopyNameOfUnknown(model,U_ZN_Ca_S,"z_ca") ;
  Model_CopyNameOfUnknown(model,U_PSI    ,"psi") ;

#if (U_Na == LOG_U)
  Model_CopyNameOfUnknown(model,U_C_Na   ,"logc_na") ;
#else
  Model_CopyNameOfUnknown(model,U_C_Na   ,"c_na") ;
#endif
#if (U_K == LOG_U)
  Model_CopyNameOfUnknown(model,U_C_K    ,"logc_k") ;
#else
  Model_CopyNameOfUnknown(model,U_C_K    ,"c_k") ;
#endif
#if (U_Cl == LOG_U)
  Model_CopyNameOfUnknown(model,U_C_Cl   ,"logc_cl") ;
#else
  Model_CopyNameOfUnknown(model,U_C_Cl   ,"c_cl") ;
#endif

  Model_GetNbOfVariables(model) = NbOfVariables ;
  Model_GetNbOfVariableFluxes(model) = NbOfVariableFluxes ;
  Model_GetComputeSecondaryVariables(model) = ComputeSecondaryVariables ;
  
  return(0) ;
}


int ReadMatProp(Material_t* mat,DataFile_t* datafile)
/* Lecture des donnees materiaux dans le fichier ficd */
{
  int  NbOfProp = 16 ;
  
  InternationalSystemOfUnits_UseAsLength("decimeter") ;
  InternationalSystemOfUnits_UseAsMass("hectogram") ;

  Material_GetProperty(mat)[pm("phimin")] = 0. ;

  Material_ScanProperties(mat,datafile,pm) ;
    
  /* Default initialization */
  {
    double h   = 5.6e-6 * (mol/dm2/sec) ;  /* (mol/dm2/s) these MT p 223 */
    double R_0 = Material_GetProperty(mat)[pm("Radius_CH")] ; /* (dm) */
    double D   = Material_GetProperty(mat)[pm("D")] ; /* (mol/dm/s) */
    
    if(R_0 == 0.) R_0 = 40.e-5 * dm ;
    
    if(D == 0.) D = 7e-15 * (mol/dm/sec) ;
    
    /* Initial (or reference) CH molar content */
    n_ch0 = Material_GetProperty(mat)[pm("N_CH")] ;
    
    {
      double t_ch = Material_GetProperty(mat)[pm("Tau")] ; /* (s) */
      
      if(t_ch == 0) {
        t_ch = R_0/(3*h*V_CH) ;     /* (s) approx 721.5 s */
        /* t_ch = R_0*R_0/(3*V_CH*D) ; */ /* (s) approx 2.3e8 s */
        Material_GetProperty(mat)[pm("Tau")] = t_ch ;
      }
      
      a_2 = n_ch0/t_ch ;  /* (mol/dm3/s) M. Thiery, PhD thesis, p 227 */
    }
    
    c_2 = h*R_0/D ;     /* (no dim) M. Thiery, PhD thesis p 228 */
  
    Material_GetProperty(mat)[pm("A_2")] = a_2 ;
    Material_GetProperty(mat)[pm("C_2")] = c_2 ;
  }
  
  {
    /* Initial (or reference) CSH molar content */
    n_csh0 = Material_GetProperty(mat)[pm("N_CSH")] ;
    if(n_csh0 == 0) n_csh0 = 1. ;
    
    Material_GetProperty(mat)[pm("N_CSH")] = n_csh0 ;
  }
  
  {
    frac = Material_GetProperty(mat)[pm("frac")] ;
    if(frac == 0) frac = 0.8 ;
    
    Material_GetProperty(mat)[pm("frac")] = frac ;
  }
  
  {
    phi_r = Material_GetProperty(mat)[pm("phi_r")] ;
    if(phi_r == 0) phi_r = 0.7 ;
    
    Material_GetProperty(mat)[pm("phi_r")] = phi_r ;
  }
  
  
  ComputePhysicoChemicalProperties(TEMPERATURE) ;

  {
    if(!csd) csd = CementSolutionDiffusion_Create() ;
    if(!hcc) hcc = HardenedCementChemistry_Create() ;
    
    HardenedCementChemistry_SetRoomTemperature(hcc,TEMPERATURE) ;
    
    CementSolutionDiffusion_SetRoomTemperature(csd,TEMPERATURE) ;
  }
  
  return(NbOfProp) ;
}


int PrintModelChar(Model_t* model,FILE *ficd)
/* Saisie des donnees materiaux */
{
  
  printf(TITLE) ;
  
  if(!ficd) return(NEQ) ;
  
  printf("\n") ;
  printf("The set of 8 equations is:\n") ;
  printf("\t- Mass balance of C      (carbone)\n") ;
  printf("\t- Mass balance of Ca     (calcium)\n") ;
  printf("\t- Mass balance of Si     (silicon)\n") ;
  printf("\t- Mass balance of Na     (sodium)\n") ;
  printf("\t- Mass balance of K      (potassium)\n") ;
  printf("\t- Mass balance of Cl     (chlore)\n") ;
  printf("\t- Total mass balance     (mass)\n") ;
  printf("\t- Charge balance         (charge)\n") ;
  
  printf("\n") ;
  printf("The 8 primary unknowns are:\n") ;
  printf("\t- Liquid pressure                  (p_l)\n") ;
  printf("\t- Electric potential               (psi) \n") ;
  printf("\t- Carbon dioxide gas concentration (c_co2)\n") ;
  printf("\t- Potassium concentration          (c_k)\n") ;
  printf("\t- Sodium concentration             (c_na)\n") ;
  printf("\t- Chloride concentration           (c_cl)\n") ;
  printf("\t- Zeta unknown for calcium         (z_ca)\n") ;
  printf("\t   \t z_ca is defined as:\n") ;
  printf("\t   \t z_ca = n_ch/n0 + log(s_ch)  for c_co2 < c_co2_eq\n") ;
  printf("\t   \t z_ca = n_cc/n0 + log(s_cc)  for c_co2 > c_co2_eq\n") ;
  printf("\t- Zeta unknown for silicon         (z_si)\n") ;
  printf("\t   \t z_si is defined as:\n") ;
  printf("\t   \t z_si = n_si/n0 + log(s_sh/s_sh_eq)\n") ;
  
  printf("\n") ;
  printf("PAY ATTENTION to units : \n") ;
  printf("\t length    : dm !\n") ;
  printf("\t time      : s !\n") ;
  printf("\t mass      : hg !\n") ;
  printf("\t pressure  : Pa !\n") ;
  
  printf("\n") ;
  printf("Example of input data\n") ;


  fprintf(ficd,"porosity = 0.38   # Porosity\n") ;
  fprintf(ficd,"k_int = 1.4e-17   # Intrinsic permeability (dm2)\n") ;
  fprintf(ficd,"N_CH = 3.9     # Initial content in Ca(OH)2 (mol/L)\n") ;
  fprintf(ficd,"Radius_CH = 40.e-5  # Portlandite crystal radius \n") ;
  fprintf(ficd,"N_CSH = 2.4        # Initial content in CSH (mol/L)\n") ;
  fprintf(ficd,"C_Na = 0.019      # Total content in Na (mol/L)\n") ;
  fprintf(ficd,"C_K  = 0.012      # Total content in K  (mol/L)\n") ;
  fprintf(ficd,"D = 7.e-15        # Diffusion coef in CC (dm/mol/s)\n") ;
  fprintf(ficd,"A_2 = 1e-2        # Kinetic coef 2 (dm/mol/s)\n") ;
  fprintf(ficd,"frac = 0.8        # Fractionnal length of pore bodies\n") ;
  fprintf(ficd,"phi_r = 0.7       # Porosity for which permeability vanishes\n") ;
  fprintf(ficd,"Curves = my_file  # File name: p_c S_l k_rl C/S H/S V_csh\n") ;  

  return(NEQ) ;
}


int DefineElementProp(Element_t* el,IntFcts_t* intfcts)
{
  Element_GetNbOfImplicitTerms(el) = NVI ;
  Element_GetNbOfExplicitTerms(el) = (Element_IsSubmanifold(el)) ? 0 : NVE ;
  Element_GetNbOfConstantTerms(el) = NV0 ;
  return(0) ;
}



int  ComputeLoads(Element_t* el,double t,double dt,Load_t* cg,double* r)
/* Residu du aux chargements (r) */
{
  int nn = Element_GetNbOfNodes(el) ;
  FVM_t* fvm = FVM_GetInstance(el) ;
  int    i ;

  {
    double* r1 = FVM_ComputeSurfaceLoadResidu(fvm,cg,t,dt) ;
    
    for(i = 0 ; i < NEQ*nn ; i++) r[i] = -r1[i] ;
  }
  
  return(0) ;
}


int ComputeInitialState(Element_t* el)
/* Initialise les variables du systeme (f,va) */ 
{
  double* f  = Element_GetImplicitTerm(el) ;
  double* v0 = Element_GetConstantTerm(el) ;
  int nn = Element_GetNbOfNodes(el) ;
  double** u = Element_ComputePointerToNodalUnknowns(el) ;
  
  /*
    Input data
  */
  GetProperties(el) ;
  
  
  /* Pre-initialization */
  {
    double c_na_tot 	= c_na0 ;
    double c_k_tot  	= c_k0 ;
    int i ;

    for(i = 0 ; i < nn ; i++) {
      double c_na       = c_na_tot ;
      double c_k        = c_k_tot ;
      double c_co2      = C_CO2(i) ;
      double zn_ca_s    = ZN_Ca_S(i) ;
      double zn_si_s    = ZN_Si_S(i) ;
      double c_cl       = C_Cl(i) ;

      /* Compute the concentrations of alkalis Na and K */
      concentrations_oh_na_k(c_co2,zn_ca_s,zn_si_s,c_cl,c_na_tot,c_k_tot) ;
  
      c_na = HardenedCementChemistry_GetAqueousConcentrationOf(hcc,Na) ;
      c_k  = HardenedCementChemistry_GetAqueousConcentrationOf(hcc,K) ;


#if (U_Cl == LOG_U)
      LogC_Cl(i)  = log10(c_cl) ;
#else
      C_Cl(i)     = c_cl ;
#endif
#if (U_Na == LOG_U)
      LogC_Na(i)  = log10(c_na) ;
#else
      C_Na(i)     = c_na ;
#endif
#if (U_K == LOG_U)
      LogC_K(i)   = log10(c_k) ;
#else
      C_K(i)      = c_k ;
#endif

      C_OH(i)     = HardenedCementChemistry_GetAqueousConcentrationOf(hcc,OH) ;
      
      /* Solid contents */
      {
        double s_ch       = HardenedCementChemistry_GetSaturationIndexOf(hcc,CH) ;
        double s_cc       = HardenedCementChemistry_GetSaturationIndexOf(hcc,CC) ;
        /*double s_ch   = HardenedCementChemistry_GetSaturationIndexOfCH(hcc) ;
        double s_cc   = HardenedCementChemistry_GetSaturationIndexOfCC(hcc) ; */
        double n_ch_cc    = CalciumContentInCHAndCC(zn_ca_s) ;
        //double n_ch       = (z_co2 > 1) ? 0       : n_ch_cc ;
        //double n_cc       = (z_co2 > 1) ? n_ch_cc : 0 ;
        double n_ch       = (s_cc > s_ch) ? 0       : n_ch_cc ;
        double n_cc       = (s_cc > s_ch) ? n_ch_cc : 0 ;
        double n_si_s     = SiliconContentInCSH(zn_si_s) ;
        double x_csh      = HardenedCementChemistry_GetCalciumSiliconRatioInCSH(hcc) ;
        double v_csh      = MolarVolumeOfCSH(x_csh) ;
        double v_s0       = V_CH*n_ch + V_CC*n_cc + v_csh*n_si_s ;
        
        V_S0(i)    = v_s0 ;
        N_CH(i)    = n_ch ;
      }
    }
  }
  

  {
    int i ;
    
    for(i = 0 ; i < nn ; i++) {
      /* Variables */
      double* x   = ComputeVariables(el,u,f,0,i) ;
      double* mui = CementSolutionDiffusion_GetPotentialAtPoint(csd,i) ;
    
      HardenedCementChemistry_CopyChemicalPotential(hcc,mui) ;
    
      /* Back up */
      N_C(i)  = x[I_N_C] ;
      N_Ca(i) = x[I_N_Ca] ;
      N_Na(i) = x[I_N_Na] ;
      N_Si(i) = x[I_N_Si] ;
      N_K(i)  = x[I_N_K] ; 
      N_Cl(i) = x[I_N_Cl] ; 
      M_tot(i) = x[I_Mass] ;
      N_q(i)  = x[I_N_Q] ;
      
      /* Concentrations */
      C_OH(i) = x[I_C_OH] ;
    }
  }
  
  
  if(Element_IsSubmanifold(el)) return(0) ;

  /* Coefficient de transfert */
  ComputeTransferCoefficients(el,u,f) ;


  /* Flux */
  {
    int i ;
    
    for(i = 0 ; i < nn ; i++) {
      int j ;
      
      for(j = i + 1 ; j < nn ; j++) {
        double* w = ComputeVariableFluxes(el,u,i,j) ;
        
        W_C(i,j)     = w[I_W_C] ;
        W_Ca(i,j)    = w[I_W_Ca] ;
        W_Na(i,j)    = w[I_W_Na] ;
        W_Si(i,j)    = w[I_W_Si] ;
        W_q(i,j)     = w[I_W_q] ;
        W_K(i,j)     = w[I_W_K] ;
        W_Cl(i,j)    = w[I_W_Cl] ;
        W_tot(i,j)   = w[I_W_tot] ;
        
        W_C(j,i)     = - w[I_W_C] ;
        W_Ca(j,i)    = - w[I_W_Ca] ;
        W_Na(j,i)    = - w[I_W_Na] ;
        W_Si(j,i)    = - w[I_W_Si] ;
        W_q(j,i)     = - w[I_W_q] ;
        W_K(j,i)     = - w[I_W_K] ;
        W_Cl(j,i)    = - w[I_W_Cl] ;
        W_tot(j,i)   = - w[I_W_tot] ;
      }
    }
  }
  return(0) ;
}


int  ComputeExplicitTerms(Element_t* el,double t)
/* Thermes explicites (va)  */
{
  double*  f = Element_GetPreviousImplicitTerm(el) ;
  double** u = Element_ComputePointerToPreviousNodalUnknowns(el) ;
  
  if(Element_IsSubmanifold(el)) return(0) ;
  
  /*
    Input data
  */
  GetProperties(el) ;
  
  /*
    Coefficients de transfert
  */
  ComputeTransferCoefficients(el,u,f) ;

  return(0) ;
}


int  ComputeImplicitTerms(Element_t* el,double t,double dt)
{
  double* f   = Element_GetCurrentImplicitTerm(el) ;
  double* f_n = Element_GetPreviousImplicitTerm(el) ;
  int nn = Element_GetNbOfNodes(el) ;
  double** u = Element_ComputePointerToNodalUnknowns(el) ;
  
  /*
    Input data
  */
  GetProperties(el) ;
  
  
  /* Molar contents */
  {
    int i ;
    
    for(i = 0 ; i < nn ; i++) {
      /* Variables */
      double* x   = ComputeVariables(el,u,f_n,dt,i) ;
      double* mui = CementSolutionDiffusion_GetPotentialAtPoint(csd,i) ;
    
      HardenedCementChemistry_CopyChemicalPotential(hcc,mui) ;
    
      /* Back up */
      N_C(i)  = x[I_N_C] ;
      N_Ca(i) = x[I_N_Ca] ;
      N_Na(i) = x[I_N_Na] ;
      N_Si(i) = x[I_N_Si] ;
      N_K(i)  = x[I_N_K] ; 
      N_Cl(i) = x[I_N_Cl] ; 
      M_tot(i) = x[I_Mass] ;
      N_q(i)  = x[I_N_Q] ;

      /* Solid contents */
      N_CH(i) = x[I_N_CH] ;
      
      /* Concentrations */
      C_OH(i) = x[I_C_OH] ;

      {
        double c_co2      = x[I_C_CO2] ;
        
        double c_h2o = HardenedCementChemistry_GetAqueousConcentrationOf(hcc,H2O) ;
        double c_oh = HardenedCementChemistry_GetAqueousConcentrationOf(hcc,OH) ;
        double c_ca= HardenedCementChemistry_GetAqueousConcentrationOf(hcc,Ca) ;
        double c_na= HardenedCementChemistry_GetAqueousConcentrationOf(hcc,Na) ;
        double c_k = HardenedCementChemistry_GetAqueousConcentrationOf(hcc,K) ;
        
        double n_si_s     = x[I_N_Si_S] ;
        double n_ch       = x[I_N_CH] ;
      
        if(c_co2 < 0 || c_oh <= 0 || c_h2o <= 0 || c_na < 0 || c_k < 0 || c_ca < 0 || n_si_s < 0. || n_ch < 0.) {
          double x0 = Element_GetNodeCoordinate(el,i)[0] ;
          double n_cc       = x[I_N_CC] ;
          double c_naoh    	= HardenedCementChemistry_GetAqueousConcentrationOf(hcc,NaOH) ;
          double c_nahco3  	= HardenedCementChemistry_GetAqueousConcentrationOf(hcc,NaHCO3) ;
          double c_naco3 	  = HardenedCementChemistry_GetAqueousConcentrationOf(hcc,NaCO3) ;
          printf("\n") ;
          printf("en x     = %e\n",x0) ;
          printf("c_co2    = %e\n",c_co2) ;
          printf("c_oh     = %e\n",c_oh) ;
          printf("c_h2o    = %e\n",c_h2o) ;
          printf("n_cc     = %e\n",n_cc) ;
          printf("c_na     = %e\n",c_na) ;
          printf("c_k      = %e\n",c_k) ;
          printf("c_ca     = %e\n",c_ca) ;
          printf("n_si_s   = %e\n",n_si_s) ;
          printf("c_naoh   = %e\n",c_naoh) ;
          printf("c_nahco3 = %e\n",c_nahco3) ;
          printf("c_naco3  = %e\n",c_naco3) ;
          return(1) ;
        }
      }
    }
  }
  
  if(Element_IsSubmanifold(el)) return(0) ;
  

  /* Flux */
  {
    int i ;
    
    for(i = 0 ; i < nn ; i++) {
      int j ;
      
      for(j = i + 1 ; j < nn ; j++) {
        double* w = ComputeVariableFluxes(el,u,i,j) ;
        
        W_C(i,j)     = w[I_W_C] ;
        W_Ca(i,j)    = w[I_W_Ca] ;
        W_Na(i,j)    = w[I_W_Na] ;
        W_Si(i,j)    = w[I_W_Si] ;
        W_q(i,j)     = w[I_W_q] ;
        W_K(i,j)     = w[I_W_K] ;
        W_Cl(i,j)    = w[I_W_Cl] ;
        W_tot(i,j)   = w[I_W_tot] ;
        
        W_C(j,i)     = - w[I_W_C] ;
        W_Ca(j,i)    = - w[I_W_Ca] ;
        W_Na(j,i)    = - w[I_W_Na] ;
        W_Si(j,i)    = - w[I_W_Si] ;
        W_q(j,i)     = - w[I_W_q] ;
        W_K(j,i)     = - w[I_W_K] ;
        W_Cl(j,i)    = - w[I_W_Cl] ;
        W_tot(j,i)   = - w[I_W_tot] ;
      }
    }
  }

  return(0) ;
}



int  ComputeMatrix(Element_t* el,double t,double dt,double* k)
{
#define K(i,j)    (k[(i)*ndof + (j)])
  int nn = Element_GetNbOfNodes(el) ;
  int ndof = nn*NEQ ;
  FVM_t* fvm = FVM_GetInstance(el) ;
  double c[4*NEQ*NEQ] ;
  int    i ;
  
  /*
    Initialisation 
  */
  for(i = 0 ; i < ndof*ndof ; i++) k[i] = 0. ;

  if(Element_IsSubmanifold(el)) return(0) ;
  
  /*
    Input data
  */
  GetProperties(el) ;
  
  TangentCoefficients(el,dt,c) ;
  {
    double* km = FVM_ComputeMassAndIsotropicConductionMatrix(fvm,c,NEQ) ;
    for(i = 0 ; i < ndof*ndof ; i++) k[i] = km[i] ;
  }

#if (U_Cl == LOG_U)
  {
    double** u = Element_ComputePointerToNodalUnknowns(el) ;
    
    for(i = 0 ; i < 2*NEQ ; i++){
      K(i,U_C_Cl)     *= Ln10*C_Cl(0) ;
      K(i,U_C_Cl+NEQ) *= Ln10*C_Cl(1) ;
    }
  }
#endif

#if (U_Na == LOG_U)
  {
    double** u = Element_ComputePointerToNodalUnknowns(el) ;
    
    for(i = 0 ; i < 2*NEQ ; i++){
      K(i,U_C_Na)     *= Ln10*C_Na(0) ;
      K(i,U_C_Na+NEQ) *= Ln10*C_Na(1) ;
    }
  }
#endif

#if (U_K == LOG_U)
  {
    double** u = Element_ComputePointerToNodalUnknowns(el) ;
    
    for(i = 0 ; i < 2*NEQ ; i++){
      K(i,U_C_K)     *= Ln10*C_K(0) ;
      K(i,U_C_K+NEQ) *= Ln10*C_K(1) ;
    }
  }
#endif


#if (U_CO2 == LOG_U)
  {
    double** u = Element_ComputePointerToNodalUnknowns(el) ;
    
    for(i = 0 ; i < 2*NEQ ; i++){
      K(i,U_C_CO2)     *= Ln10*C_CO2(0) ;
      K(i,U_C_CO2+NEQ) *= Ln10*C_CO2(1) ;
    }
  }
#endif

  return(0) ;

#undef K
}


int  ComputeResidu(Element_t* el,double t,double dt,double* r)
{
#define R(n,i)    (r[(n)*NEQ+(i)])
  double* f   = Element_GetCurrentImplicitTerm(el) ;
  double* f_n = Element_GetPreviousImplicitTerm(el) ;
  int nn = Element_GetNbOfNodes(el) ;
  FVM_t* fvm = FVM_GetInstance(el) ;
  int    i ;
  double zero = 0. ;
  
  /*
    Initialization
  */
  for(i = 0 ; i < NEQ*nn ; i++) r[i] = zero ;

  if(Element_IsSubmanifold(el)) return(0) ;

  
  /*
    Conservation of element C: (N_C - N_Cn) + dt * div(W_C) = 0
  */
  {
    double* r1 = FVM_ComputeMassBalanceEquationResidu(fvm,NW_C,NW_Cn,dt) ;
      
    for(i = 0 ; i < nn ; i++) {
      R(i,E_C) -= r1[i] ;
    }
  }
  
  /*
    Conservation of charge: div(W_q) = 0
  */
  {
    double g[Element_MaxNbOfNodes*Element_MaxNbOfNodes] ;
    
    for(i = 0 ; i < nn ; i++) {
      int j ;
      
      for(j = 0 ; j < nn ; j++) {
        if(i == j) {
          g[i*nn + i] = 0 ;
        } else {
          g[i*nn + j] = dt * W_q(i,j) ;
        }
      }
    }
    
    {
      double* r1 = FVM_ComputeMassAndFluxResidu(fvm,g) ;
      
      for(i = 0 ; i < nn ; i++) {
        R(i,E_q) -= r1[i] ;
      }
    }
  }
  
  /*
    Conservation of total mass: (M_tot - M_totn) + dt * div(W_tot) = 0
  */
  {
    double* r1 = FVM_ComputeMassBalanceEquationResidu(fvm,MW_tot,MW_totn,dt) ;
      
    for(i = 0 ; i < nn ; i++) {
      R(i,E_mass) -= r1[i] ;
    }
  }
  
  /*
    Conservation of element Ca: (N_Ca - N_Can) + dt * div(W_Ca) = 0
  */
  {
    double* r1 = FVM_ComputeMassBalanceEquationResidu(fvm,NW_Ca,NW_Can,dt) ;
    
    for(i = 0 ; i < nn ; i++) {
      R(i,E_Ca) -= r1[i] ;
    }
  }
  
  /*
    Conservation of element Na: (N_Na - N_Nan) + dt * div(W_Na) = 0
  */
  {
    double* r1 = FVM_ComputeMassBalanceEquationResidu(fvm,NW_Na,NW_Nan,dt) ;
      
    for(i = 0 ; i < nn ; i++) {
      R(i,E_Na) -= r1[i] ;
    }
  }
  
  /*
    Conservation of element K: (N_K - N_Kn) + dt * div(W_K) = 0
  */
  {
    double* r1 = FVM_ComputeMassBalanceEquationResidu(fvm,NW_K,NW_Kn,dt) ;
      
    for(i = 0 ; i < nn ; i++) {
      R(i,E_K) -= r1[i] ;
    }
  }

  /*
    Conservation of element Si: (N_Si - N_Sin) + dt * div(W_Si) = 0
  */
  {
    double* r1 = FVM_ComputeMassBalanceEquationResidu(fvm,NW_Si,NW_Sin,dt) ;
      
    for(i = 0 ; i < nn ; i++) {
      R(i,E_Si) -= r1[i] ;
    }
  }
  
  /*
    Conservation of element Cl: (N_Cl - N_Cln) + dt * div(W_Cl) = 0
  */
  {
    double* r1 = FVM_ComputeMassBalanceEquationResidu(fvm,NW_Cl,NW_Cln,dt) ;
    
    for(i = 0 ; i < nn ; i++) {
      R(i,E_Cl) -= r1[i] ;
    }
  }

  return(0) ;
#undef R
}



int  ComputeOutputs(Element_t* el,double t,double* s,Result_t* r)
{
  double* f = Element_GetCurrentImplicitTerm(el) ;
  double* va = Element_GetExplicitTerm(el) ;
  FVM_t* fvm = FVM_GetInstance(el) ;
  double** u = Element_ComputePointerToNodalUnknowns(el) ;
  int    nso = 57 ;
  int    i ;

  if(Element_IsSubmanifold(el)) return(0) ;
  
  /*
    Input data
  */
  GetProperties(el) ;
  

  /* Initialization */
  for(i = 0 ; i < nso ; i++) {
    Result_SetValuesToZero(r + i) ;
  }

  {
    int j = FVM_FindLocalCellIndex(fvm,s) ;
    double* x = ComputeVariables(el,u,f,0,j) ;
    /* Concentrations */
#define ptC(CPD)   &(HardenedCementChemistry_GetAqueousConcentrationOf(hcc,CPD))
#define ptS(CPD)   &(HardenedCementChemistry_GetSaturationIndexOf(hcc,CPD))
#define ptPSI      &(HardenedCementChemistry_GetElectricPotential(hcc))
#define ptX_CSH    &(HardenedCementChemistry_GetCalciumSiliconRatioInCSH(hcc))
#define ptZ_CSH    &(HardenedCementChemistry_GetWaterSiliconRatioInCSH(hcc))

    double p_l        = x[I_P_L] ;
    double p_c        = p_g - p_l ;
    /* saturation */
    double s_l        = SaturationDegree(p_c) ;
    double c_co2      = x[I_C_CO2] ;

    /* densite de charge */
    double c_q = x[I_N_Q] ;

    /* contenus solides */
    double n_si_s     = x[I_N_Si_S] ;
    double n_ca_s     = x[I_N_Ca_S] ;
    double n_cc 	    = x[I_N_CC] ;
    double n_ch       = x[I_N_CH] ;
    double s_ch       = HardenedCementChemistry_GetSaturationIndexOf(hcc,CH) ;
    
    double v_csh      = x[I_V_CSH] ;
    double v_solide_csh   = v_csh*n_si_s ;
    double v_solide_ch    = V_CH*n_ch ;
    double v_solide_cc    = V_CC*n_cc ;

    double n_chn      = x[I_N_CHn] ;
    double av         = 1 - n_chn/n_ch0 ;
    double dn1sdt     = a_2*dn1_caoh2sdt(av,c_2) ;
    double dn_chsdt   = dn1sdt*log(s_ch) ;
    double coeff_dnCH = log(s_ch) ;
  
    double CsurS      = (n_ca_s/n_si_s) ;
    

    double zn_ca_s    = x[U_ZN_Ca_S] ;  /* mj */
    double zn_si_s    = x[U_ZN_Si_S] ;  /* mj */


    /* porosity */
    double phi        = x[I_Phi] ;

    double n_cl_s     = n_si_s * AdsorbedChloridePerUnitMoleOfCSH(*ptC(Cl  ),*ptX_CSH) ; /* Added by mj */
    // double n_Cl       = 0.5*(N_Cl(0) + N_Cl(1)) ; /* Added by mj */
    double n_Cl       = n_cl_s + *ptC(Cl  ) * phi * s_l ; /* Added by mj */

    double n_na_s     = n_si_s * AdsorbedSodiumPerUnitMoleOfCSH(*ptC(Na ),*ptX_CSH) ; /* Added by mj */
    double n_k_s      = n_si_s * AdsorbedPotassiumPerUnitMoleOfCSH(*ptC(K ),*ptX_CSH) ; /* Added by mj */

    /* double n_Na = 0.5*(N_Na(0) + N_Na(1)) ; */ /* mj */
    double n_Ca = 0.5*(N_Ca(0) + N_Ca(1)) ;
    double n_Si = 0.5*(N_Si(0) + N_Si(1)) ; 


    /* Transferts */
    double x1 = Element_GetNodeCoordinate(el,1)[0] ;
    double x0 = Element_GetNodeCoordinate(el,0)[0] ;
    double dx        = x1 - x0 ;
    double grd_psi   = (PSI(1) - PSI(0))/dx ;
    
    double coeff_permeability = PermeabilityCoefficient(el,phi) ;
	  double k_l  = (k_int/mu_l)*RelativePermeabilityToLiquid(p_c)*coeff_permeability ; 

    double tortuosity = 0.5 * (TORTUOSITY[0] + TORTUOSITY[1]) ;

    /* quantites exploitees */
    i = 0 ;
    
    Result_Store(r + i++,&c_co2,"c_co2",1) ;
    
    {
      double c_h   = *(ptC(H)) ;
      double ph    = - log10(c_h) ;
      
      Result_Store(r + i++,&ph,"ph",1) ;
    }
    
    Result_Store(r + i++,&n_si_s,"n_Si_s",1) ;
    Result_Store(r + i++,&phi,"porosity",1) ;
    Result_Store(r + i++,&n_ch,"n_CH",1) ;
    
    
    Result_Store(r + i++,ptC(Ca  ),"c_ca",1) ;
    
    Result_Store(r + i++,ptC(CO3 ),"c_co3",1) ;
    Result_Store(r + i++,ptC(HCO3),"c_hco3",1) ;
    
    Result_Store(r + i++,&n_cc,"n_CC",1) ;
    
    Result_Store(r + i++,ptC(H ),"c_h",1) ;
    Result_Store(r + i++,ptC(OH),"c_oh",1) ;
    
    Result_Store(r + i++,&s_l,"saturation",1) ;
    Result_Store(r + i++,&grd_psi,"grad_psi",1) ;
    Result_Store(r + i++,&c_q,"charge",1) ;
    
    Result_Store(r + i++,ptC(Na  ),"c_na",1) ;
    Result_Store(r + i++,ptC(NaOH),"c_naoh",1) ;
    
    Result_Store(r + i++,ptC(NaHCO3),"c_nahco3",1) ;
    Result_Store(r + i++,ptC(NaCO3),"c_naco3",1) ;
    

    Result_Store(r + i++,ptC(K  ),"c_k",1) ;
    Result_Store(r + i++,ptC(KOH),"c_koh",1) ;
    
    
    Result_Store(r + i++,ptC(CaOH),"c_caoh",1) ;
    
    Result_Store(r + i++,ptC(CaHCO3),"c_cahco3",1) ;
    Result_Store(r + i++,ptC(CaCO3),"c_caco3aq",1) ;
    Result_Store(r + i++,ptC(CaO2H2),"c_caoh2aq",1) ;
    
    Result_Store(r + i++,&p_l,"p_l",1) ;
    
    Result_Store(r + i++,ptC(H3SiO4),"c_h3sio4",1) ;
    
    Result_Store(r + i++,x + I_N_Na,"n_Na",1) ;
    /* Result_Store(r + i++,&n_Na,"n_Na",1) ; */
    Result_Store(r + i++,&n_Ca,"n_Ca",1) ;
    Result_Store(r + i++,&n_Si,"n_Si",1) ;
    Result_Store(r + i++,&n_ca_s,"n_Ca_s",1) ;
    
    
    Result_Store(r + i++,ptC(CaH2SiO4),"c_cah2sio4",1) ;
    Result_Store(r + i++,ptC(CaH3SiO4),"c_cah3sio4",1) ;
    
    Result_Store(r + i++,&CsurS,"CsurS",1) ;
    
    Result_Store(r + i++,ptC(H4SiO4),"c_h4sio4",1) ;
    Result_Store(r + i++,ptC(H2SiO4),"c_h2sio4",1) ;
    
    Result_Store(r + i++,ptC(Cl  ),"c_cl",1) ;
    Result_Store(r + i++,&n_cl_s,"n_cl_s",1) ;  /* Added by mj */
    Result_Store(r + i++,&n_Cl,"n_Cl",1) ;   /* Added by mj */
    Result_Store(r + i++,&n_na_s,"n_na_s",1) ;  /* Added by mj */
    Result_Store(r + i++,&n_k_s,"n_k_s",1) ;  /* Added by mj */   

    {
      double I = HardenedCementChemistry_GetIonicStrength(hcc) ;
      
      Result_Store(r + i++,&I,"I",1) ;
    }
    
    /* Added by A. Morandeau */
    Result_Store(r + i++,ptX_CSH,"x_csh",1) ;
    
    Result_Store(r + i++,&n_si_s,"n_si_s",1) ;
    
    Result_Store(r + i++,ptS(CH),"s_ch",1) ;
    Result_Store(r + i++,ptS(SH),"s_sh",1) ;
    
    Result_Store(r + i++,&k_l,"k_l",1) ;
    Result_Store(r + i++,&coeff_permeability,"verma-pruess",1) ;
    Result_Store(r + i++,&dn_chsdt,"dn_chsdt",1) ;
    Result_Store(r + i++,&dn1sdt,"dn1sdt",1) ;
    Result_Store(r + i++,&coeff_dnCH,"coeff_dnCH",1) ;
    Result_Store(r + i++,&v_solide_csh,"v_csh",1) ;
    Result_Store(r + i++,&v_solide_ch,"v_ch",1) ;
    Result_Store(r + i++,&v_solide_cc,"v_cc",1) ;
    Result_Store(r + i++,&tortuosity,"tortuosityiff",1) ; /* Added by mj */
    Result_Store(r + i++,x + I_N_K,"n_K",1) ;   /* Added by mj */
      
    Result_Store(r + i++,&zn_ca_s,"ZN_Ca_S",1) ;     /* Added by mj */
    Result_Store(r + i++,&zn_si_s,"ZN_Si_S",1) ;  /* Added by mj */
  }
  
  
  if(i != nso) arret("ComputeOutputs") ;
  return(nso) ;
}


void ComputeTransferCoefficients(Element_t* el,double** u,double* f)
/* Termes explicites (va)  */
{
  double* va = Element_GetExplicitTerm(el) ;
  int nn = Element_GetNbOfNodes(el) ;
  int    i ; 

  /* initialization */
  for(i = 0 ; i < NVE ; i++) va[i] = 0. ;
  
  /*
    Transfer coefficients
  */
  for(i = 0 ; i < nn ; i++) {
    /* Variables */
    double* x = ComputeVariables(el,u,f,0,i) ;
    /* pressions */
    double p_l     = P_L(i) ;
    double p_c     = p_g - p_l ;
    /* saturation */
    double s_l     = SaturationDegree(p_c) ;

    /* Porosity */
    double phi        = x[I_Phi] ;
	
    /* Permeability */
    double coeff_permeability = PermeabilityCoefficient(el,phi) ; /* Change here */
    double k_l  = (k_int/mu_l)*RelativePermeabilityToLiquid(p_c)*coeff_permeability;  
    
    
    /* tortuosite gaz */
    double s_g = 1 - s_l ;
    double phi_g = phi*s_g ;
   /* double tau  = pow(phi,1/3)*pow(s_g,7/3) ; */
    double tau  = pow(phi,1.74)*pow(s_g,3.20) ;

    
    /* tortuosite liquide */

    double vca = 0.397 ;
    double vfa = 0.294 ;
    double vpa = 0.31  ;
    double fac = (1.-1.5*vca)/(1.-0.86*vfa)*vpa ;
    double fporo = (phi > 0.18) ? 0.001+0.07*phi*phi+1.8*(phi-0.18)*(phi-0.18) : 0.001+0.07*phi*phi ;
    double coefsa = pow(s_l,6.) ; 
    double iff = fac * fporo * coefsa ;   /* mj */

    /* double phi_cap = phi/2  ;
    double phi_c   = 0.17 ; /*Percolation capilar porosity*/
    
    /*Diffusivity according to Oh and Jang, CCR203*/
    /* double n = 2.7 ; 		/* OPC n = 2.7  --------------  Fly ash n = 4.5 */
    /* double ds_norm = 5e-5 ;	/* OPC ds_norm = 1e-4 --------  Fly ash ds_norm = 5e-5 */
    /* double m_phi = 0.5*( pow(ds_norm,1/n) + phi_cap/(1-phi_c)*(1 - pow(ds_norm,1/n)) - phi_c/(1-phi_c)) ;
    /* double iff =  pow( m_phi + sqrt( m_phi*m_phi +  pow(ds_norm,1/n)*phi_c/(1-phi_c)),n)*pow(s_l,4.5) ; */
    
    /*Diffusivity : ITZ for mortars and concrete */

    /*Diffusivity according to Bazant et Najjar */
		/*double iff    = 0.00029*exp(9.95*phi)/(1+625*pow((1-s_l),4)) ;*/

    /* Humidit\E9 relative */
    /* double hr = exp(-p_c*M_H2O/(RT*rho_l)) ; */
    
    
    /* Diffusion */
    {
      KF_CO2[i]      = phi_g*tau*d_co2 ;
      /* KF_CO2[i]    	= (1.6e-3)*pow(phi,1.8)*pow(1-hr,2.2) ; */
    }
    
    /* Liquid tortuosity */
    {
      TORTUOSITY[i] = iff ;
    }
    
    /* Concentrations */
    {
      double* c = HardenedCementChemistry_GetAqueousConcentration(hcc) ;
      int nbc = HardenedCementChemistry_NbOfConcentrations ;
      int j ;
    
      for(j = 0 ; j < nbc ; j++) {
        CONCENTRATION(i)[j] = c[j] ;
      }
    }
    
    /* Permeabilities */
    {
  
      double rho_l  = HardenedCementChemistry_GetLiquidMassDensity(hcc) ;
      double c_c_l  = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,C) ;
      double c_ca_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,Ca) ;
      double c_na_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,Na) ;
      double c_k_l  = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,K) ;
      double c_si_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,Si) ;
      double c_cl_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,Cl) ;
    
      KD_L[i]    = rho_l * k_l ;
            
      KD_C_L[i]  = c_c_l  * k_l ;
      KD_Ca_L[i] = c_ca_l * k_l ;
      KD_Na_L[i] = c_na_l * k_l ;
      KD_K_L[i]  = c_k_l  * k_l ;
      KD_Si_L[i] = c_si_l * k_l ;
      KD_Cl_L[i] = c_cl_l * k_l ;
    }
  }
}



double* ComputeVariableFluxes(Element_t* el,double** u,int i,int j)
{
  double* grdij = dVariables ;

  /* Gradients */
  {
    int nn = Element_GetNbOfNodes(el) ;
    FVM_t* fvm   = FVM_GetInstance(el) ;
    double* dist = FVM_ComputeIntercellDistances(fvm) ;
    double dij  = dist[nn*i + j] ;
    
    {
      double* xi = Variables[i] ;
      double* xj = Variables[j] ;
      int k ;
    
      for(k = 0 ; k < NbOfVariables ; k++)  {
        grdij[k] = (xj[k] - xi[k]) / dij ;
      }
    }

    {
      double* g = CementSolutionDiffusion_GetGradient(csd) ;
      double* mui = CementSolutionDiffusion_GetPotentialAtPoint(csd,i) ;
      double* muj = CementSolutionDiffusion_GetPotentialAtPoint(csd,j) ;
      int n = CementSolutionDiffusion_NbOfConcentrations ;
      int k ;
      
      for(k = 0 ; k < n ; k++) {
        g[k] = (muj[k] - mui[k]) / dij ;
      }
    }
  }

  /* Fluxes */
  {
    double* w = ComputeFluxes(el,grdij,i,j) ;
    
    return(w) ;
  }
}



double* ComputeFluxes(Element_t* el,double* grdij,int i,int j)
{
  double* va = Element_GetExplicitTerm(el) ;
  double* w  = VariableFluxes[i] ;
  
  /* Diffusion in the cement solution */
  {
    /* Gradients */
    {
      double* g = CementSolutionDiffusion_GetGradient(csd) ;
      int n = CementSolutionDiffusion_NbOfConcentrations ;
      double* ci = CONCENTRATION(i) ;
      double* cj = CONCENTRATION(j) ;
      double tortuosity = 0.5 * (TORTUOSITY[i] + TORTUOSITY[j]) ;
      int k ;
      
      for(k = 0 ; k < n ; k++) {
        double rho = 0.5 * (ci[k] + cj[k]) ;
      
        g[k] *= tortuosity * rho ;
      }
    }
    /* Fluxes */
    {
      CementSolutionDiffusion_ComputeFluxes(csd) ;
      
      w[I_W_C]   = CementSolutionDiffusion_GetElementFluxOf(csd,C) ;
      w[I_W_Ca]  = CementSolutionDiffusion_GetElementFluxOf(csd,Ca) ;
      w[I_W_Si]  = CementSolutionDiffusion_GetElementFluxOf(csd,Si) ;
      w[I_W_Na]  = CementSolutionDiffusion_GetElementFluxOf(csd,Na) ;
      w[I_W_K ]  = CementSolutionDiffusion_GetElementFluxOf(csd,K) ;
      w[I_W_Cl]  = CementSolutionDiffusion_GetElementFluxOf(csd,Cl) ;
      w[I_W_q ]  = CementSolutionDiffusion_GetIonCurrent(csd) ;
    }
  }
  
  /* Advection */
  {
    /* Gradients */
    double grd_p_l = grdij[I_P_L] ;
      
    /* Transfer terms */
    double kd_l     = 0.5 * (KD_L[i]     + KD_L[j]) ;
    double kd_c_l   = 0.5 * (KD_C_L[i]   + KD_C_L[j]) ;
    double kd_ca_l  = 0.5 * (KD_Ca_L[i]  + KD_Ca_L[j]) ;
    double kd_si_l  = 0.5 * (KD_Si_L[i]  + KD_Si_L[j]) ;
    double kd_na_l  = 0.5 * (KD_Na_L[i]  + KD_Na_L[j]) ;
    double kd_k_l   = 0.5 * (KD_K_L[i]   + KD_K_L[j]) ;
    double kd_cl_l  = 0.5 * (KD_Cl_L[i]  + KD_Cl_L[j]) ;

    /* Fluxes */
    w[I_W_tot]   = - kd_l    * grd_p_l  ;
   
    w[I_W_C  ]  += - kd_c_l  * grd_p_l  ;
    w[I_W_Ca ]  += - kd_ca_l * grd_p_l  ;
    w[I_W_Si ]  += - kd_si_l * grd_p_l  ;
    w[I_W_Na ]  += - kd_na_l * grd_p_l  ;
    w[I_W_K  ]  += - kd_k_l  * grd_p_l  ;
    w[I_W_Cl ]  += - kd_cl_l * grd_p_l  ;
  }
  
  /* Diffusion of CO2 in gas phase */
  {
    /* Gradients */
    double grd_co2 = grdij[I_C_CO2] ;
      
    /* Transfer terms */
    double kf_co2   = 0.5 * (KF_CO2[i]   + KF_CO2[j]) ;

    /* Fluxes */
    double w_co2_g = - kf_co2 * grd_co2  ;
  
    w[I_W_tot]  +=  M_CO2 * w_co2_g ;
    w[I_W_C  ]  +=  w_co2_g ;
  }
    
  return(w) ;
}




int TangentCoefficients(Element_t* el,double dt,double* c)
/**  Tangent matrix coefficients (c) */
{
  double* f_n = Element_GetPreviousImplicitTerm(el) ;
  int nn = Element_GetNbOfNodes(el) ;
  int ndof = nn*NEQ ;
  ObVal_t* obval = Element_GetObjectiveValue(el) ;
  double** u   = Element_ComputePointerToNodalUnknowns(el) ;
  double** u_n = Element_ComputePointerToPreviousNodalUnknowns(el) ;
  int    dec = NEQ*NEQ ;
  double dui[NEQ] ;
  FVM_t* fvm   = FVM_GetInstance(el) ;
  double* dist = FVM_ComputeIntercellDistances(fvm) ;
  int    i ;
  
  /* Initialization */
  for(i = 0 ; i < ndof*ndof ; i++) c[i] = 0. ;

  if(Element_IsSubmanifold(el)) return(0) ;
  
  
  for(i = 0 ; i < NEQ ; i++) {
    dui[i] =  1.e-2 * ObVal_GetValue(obval + i) ;
  }

  dui[U_C_CO2  ] =  1.e-4 * ObVal_GetValue(obval + U_C_CO2) ;
  dui[U_C_Na   ] =  1.e-3 * ObVal_GetValue(obval + U_C_Na) ;
  dui[U_C_K    ] =  1.e-3 * ObVal_GetValue(obval + U_C_K) ;
  dui[U_ZN_Ca_S] =  1.e-4 * ObVal_GetValue(obval + U_ZN_Ca_S) ;
  dui[U_ZN_Si_S] =  1.e-4 * ObVal_GetValue(obval + U_ZN_Si_S) ;
  dui[U_P_L    ] =  1.e-4 * ObVal_GetValue(obval + U_P_L) ;
  dui[U_PSI    ] =  1.e+0 * ObVal_GetValue(obval + U_PSI) ;
  dui[U_C_Cl   ] =  1.e-3 * ObVal_GetValue(obval + U_C_Cl) ;
  
  
  for(i = 0 ; i < nn ; i++) {
    double* xi = ComputeVariables(el,u,f_n,dt,i) ;
    double* mui = CementSolutionDiffusion_GetPotentialAtPoint(csd,i) ;
    int k ;
    
    HardenedCementChemistry_CopyChemicalPotential(hcc,mui) ;

    dui[U_C_CO2  ] =  1.e-4*ObVal_GetValue(obval + U_C_CO2) ;
    dui[U_C_Na   ] =  1.e-3 * ObVal_GetValue(obval + U_C_Na) ;
    dui[U_C_K    ] =  1.e-3 * ObVal_GetValue(obval + U_C_K) ;
    dui[U_C_Cl   ] =  1.e-3 * ObVal_GetValue(obval + U_C_Cl) ;
    dui[U_ZN_Ca_S] =  1.e-4*ObVal_GetValue(obval + U_ZN_Ca_S) ;
    dui[U_ZN_Si_S] =  1.e-4*ObVal_GetValue(obval + U_ZN_Si_S) ;
    
    dui[U_ZN_Si_S] *= ((xi[U_ZN_Si_S] > ZN_Si_Sn(i)) ? 1 : -1) ; 
    dui[U_ZN_Ca_S] *= ((xi[U_ZN_Ca_S] > ZN_Ca_Sn(i)) ? 1 : -1) ;
    
    #if (U_CO2 == LOG_U)
    dui[U_C_CO2  ] *=  C_CO2n(i) ;
    #endif
    
    #if (U_Cl == LOG_U)
    dui[U_C_Cl  ] *=  C_Cln(i) ;
    #endif
    #if (U_Na == LOG_U)
    dui[U_C_Na   ] *=  C_Nan(i) ;
    #endif
    #if (U_K == LOG_U)
    dui[U_C_K    ] *=  C_Kn(i) ;
    #endif
    
    for(k = 0 ; k < NEQ ; k++) {
      double  dui_k = dui[k] ;
      double* dxi = ComputeVariableDerivatives(el,dt,xi,dui_k,k) ;
      
      /* Content terms at node i */
      {
        double* cii = c + (i*nn + i)*NEQ*NEQ ;
    
        cii[E_C*NEQ    + k] = dxi[I_N_C] ;
        cii[E_Ca*NEQ   + k] = dxi[I_N_Ca] ;
        cii[E_Na*NEQ   + k] = dxi[I_N_Na] ;
        cii[E_Si*NEQ   + k] = dxi[I_N_Si] ;
        cii[E_K*NEQ    + k] = dxi[I_N_K] ;
        cii[E_mass*NEQ + k] = dxi[I_Mass] ;
        cii[E_Cl*NEQ   + k] = dxi[I_N_Cl] ;
      }

      /* Transfer terms from node i to node j: d(wij)/d(ui_k) */
      {
        int j ;
        
        for(j = 0 ; j < nn ; j++) {
          if(j != i) {
            
            {
              double* g = CementSolutionDiffusion_GetGradient(csd) ;
              double* muj = CementSolutionDiffusion_GetPotentialAtPoint(csd,j) ;
              int n = CementSolutionDiffusion_NbOfConcentrations ;
              int l ;
    
              /* On output ComputeVariableDerivatives has computed 
               * mui + d(mui) (through hcc). Then it is copied into muj */
              HardenedCementChemistry_CopyChemicalPotential(hcc,muj) ;

              /* The derivatives d(mui)/d(ui_k) */
              for(l = 0 ; l < n ; l++) {
                g[l] = (muj[l] - mui[l]) / dui_k ;
              }
            }
            
            {
              double* cij = c + (i*nn + j)*NEQ*NEQ ;
              double dij  = dist[nn*i + j] ;
              double dtdij = dt/dij ;
              double* dw = ComputeFluxes(el,dxi,i,j) ;
        
              cij[E_C*NEQ    + k] = - dtdij*dw[I_W_C] ;
              cij[E_Ca*NEQ   + k] = - dtdij*dw[I_W_Ca] ;
              cij[E_Na*NEQ   + k] = - dtdij*dw[I_W_Na] ;
              cij[E_Si*NEQ   + k] = - dtdij*dw[I_W_Si] ;
              cij[E_K*NEQ    + k] = - dtdij*dw[I_W_K] ;
              cij[E_mass*NEQ + k] = - dtdij*dw[I_W_tot] ;
              cij[E_q*NEQ    + k] = - dtdij*dw[I_W_q] ;
              cij[E_Cl*NEQ   + k] = - dtdij*dw[I_W_Cl] ;
            }
          }
        }
      }
    }
  }

  return(dec) ;
}



double* ComputeVariables(Element_t* el,double** u,double* f_n,double dt,int n)
{
  double* v0 = Element_GetConstantTerm(el) ;
  double* x = Variables[n] ;
  
  /* Primary Variables */
  x[U_C_CO2  ] = C_CO2(n) ;
  x[U_C_Na   ] = C_Na(n) ;
  x[U_C_K    ] = C_K(n) ;
  x[U_ZN_Ca_S] = ZN_Ca_S(n) ;
  x[U_ZN_Si_S] = ZN_Si_S(n) ;
  x[U_P_L    ] = P_L(n) ;
  x[U_PSI    ] = PSI(n) ;
  x[U_C_Cl   ] = C_Cl(n) ;
  
  /* Needed variables to compute secondary components */
  x[I_N_CHn]  = N_CHn(n) ;
  x[I_V_S0 ]  = V_S0(n) ;
  x[I_C_OHn]  = C_OHn(n) ;
  
  ComputeSecondaryVariables(el,0,dt,x) ;
  return(x) ;
}


double* ComputeVariableDerivatives(Element_t* el,double dt,double* x,double du_i,int i)
{
  double* dx = dVariables ;
  int j ;
  
  /* Primary Variables */
  for(j = 0 ; j < NbOfVariables ; j++) {
    dx[j] = x[j] ;
  }
  
  /* We increment the variable as (x + dx) */
  dx[i] += du_i ;
  
  ComputeSecondaryVariables(el,0,dt,x) ;
  
  /* The numerical derivative as (f(x + dx) - f(x))/dx */
  for(j = 0 ; j < NbOfVariables ; j++) {
    dx[j] -= x[j] ;
    dx[j] /= du_i ;
  }

  return(dx) ;
}



void  ComputeSecondaryVariables(Element_t* el,double t,double dt,double* x)
{
  double c_co2      = x[U_C_CO2  ] ;
  double zn_ca_s    = x[U_ZN_Ca_S] ;
  double zn_si_s    = x[U_ZN_Si_S] ;
  double p_l        = x[U_P_L    ] ;
  double c_cl       = x[U_C_Cl   ] ;
  double c_na       = x[U_C_Na   ] ;
  double c_k        = x[U_C_K    ] ;

  /* Liquid components */
  double c_co2aq    = k_h*c_co2 ;
    
  /* Solve cement chemistry */
  {
    double c_na       = x[U_C_Na   ] ;
    double c_k        = x[U_C_K    ] ;
    double logc_co2aq = log10(c_co2aq) ;
    double logc_na    = log10(c_na) ;
    double logc_k     = log10(c_k) ;
    double psi        = x[U_PSI] ;
    double c_oh       = x[I_C_OHn] ;
    double logc_oh    = log10(c_oh) ;
    double logc_cl    = log10(c_cl) ;
  
    HardenedCementChemistry_SetInput(hcc,SI_CH_CC,MIN(zn_ca_s,0)) ;
    /*HardenedCementChemistry_GetInput(hcc,SI_Ca) = MIN(zn_ca_s,0) ; */
    HardenedCementChemistry_SetInput(hcc,SI_CSH,MIN(zn_si_s,0)) ;
    /*HardenedCementChemistry_GetInput(hcc,SI_Si) = MIN(zn_si_s,0) ;*/
    HardenedCementChemistry_GetInput(hcc,LogC_CO2)    = logc_co2aq ;
    HardenedCementChemistry_GetInput(hcc,LogC_Na)     = logc_na ;
    HardenedCementChemistry_GetInput(hcc,LogC_K)      = logc_k ;
    HardenedCementChemistry_GetInput(hcc,LogC_OH)     = logc_oh ;
    HardenedCementChemistry_GetElectricPotential(hcc) = psi ;
    
    HardenedCementChemistry_GetAqueousConcentrationOf(hcc,Cl) = c_cl ;
    HardenedCementChemistry_GetLogAqueousConcentrationOf(hcc,Cl) = logc_cl ;
  
    HardenedCementChemistry_ComputeSystem(hcc,CaO_SiO2_Na2O_K2O_CO2) ;
      
    HardenedCementChemistry_SolveElectroneutrality(hcc) ;
  }
  
  
  
  /* Backup */
  
  double c_q  = HardenedCementChemistry_GetLiquidChargeDensity(hcc) ;
  
  double rho_l  = HardenedCementChemistry_GetLiquidMassDensity(hcc) ;
  
  double c_c_l  = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,C) ;
  double c_ca_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,Ca) ;
  double c_na_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,Na) ;
  double c_k_l  = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,K) ;
  double c_si_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,Si) ;
  double c_cl_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,Cl) ;
  
  double s_ch       = HardenedCementChemistry_GetSaturationIndexOf(hcc,CH) ;
  double s_cc       = HardenedCementChemistry_GetSaturationIndexOf(hcc,CC) ;
  /*double s_ch   = HardenedCementChemistry_GetSaturationIndexOfCH(hcc) ;
  double s_cc   = HardenedCementChemistry_GetSaturationIndexOfCC(hcc) ; */
       
    
  /* Solid contents */
  /* ... as components: CH, CC, CSH */
  double n_ch_cc    = CalciumContentInCHAndCC(zn_ca_s) ;
  double n_chn      = x[I_N_CHn] ;
  double av         = 1 - n_chn/n_ch0 ;
  double dn1sdt     = a_2*dn1_caoh2sdt(av,c_2) ;
  double dn_chsdt   = dn1sdt*log(s_ch) ; /* Kinetics */
  double n_ch_ki    = MAX(n_chn + dt*dn_chsdt , 0.) ;
  double n_ch       = (s_cc > s_ch) ? n_ch_ki : n_ch_cc ;
  double n_cc       = (s_cc > s_ch) ? n_ch_cc - n_ch_ki : 0 ;
  
  /* ... as elements: C, Ca, Si, Cl */
  //double x_csh      = CalciumSiliconRatioInCSH(s_ch) ;
  double x_csh      = HardenedCementChemistry_GetCalciumSiliconRatioInCSH(hcc) ;
  double n_si_s     = SiliconContentInCSH(zn_si_s) ;
  double n_ca_s     = n_ch + n_cc + x_csh*n_si_s ;
  double n_c_s      = n_cc ;
  double c_oh       = HardenedCementChemistry_GetAqueousConcentrationOf(hcc,OH) ;
  double n_cl_s     = n_si_s * AdsorbedChloridePerUnitMoleOfCSH(c_cl,x_csh) ;
  double n_na_s     = n_si_s * AdsorbedSodiumPerUnitMoleOfCSH(c_na,x_csh) ; /* Added by mj */
  double n_k_s      = n_si_s * AdsorbedPotassiumPerUnitMoleOfCSH(c_k,x_csh) ; /* Added by mj */

  /* ... as mass */
  //double z_csh      = WaterSiliconRatioInCSH(s_ch) ;
  double z_csh      = HardenedCementChemistry_GetWaterSiliconRatioInCSH(hcc) ;
  double m_csh      = (M_CaO*x_csh + M_SiO2 + M_H2O*z_csh)*n_si_s ;
  double m_s        = M_CaOH2*n_ch + M_CaCO3*n_cc + m_csh ;
  
  /* ... as volume */
  double v_csh      = MolarVolumeOfCSH(x_csh) ;
  double v_s        = V_CH*n_ch + V_CC*n_cc + v_csh*n_si_s ;
  
  
  /* Porosity */
  double v_s0     = x[I_V_S0] ;
  double phith    = phi0 + v_s0 - v_s ;
  double phi      = (phith > phimin) ? phith : phimin ;
  
  /* Saturation */
  double p_c      = p_g - p_l ;
  double s_l      = SaturationDegree(p_c) ;
  double s_g      = 1 - s_l ;
  
  
  /* Liquid contents */
  double phi_l    = phi*s_l ;
  /* ... as elements: C, Ca, Si */
  double n_c_l  = phi_l*c_c_l ;
  double n_ca_l = phi_l*c_ca_l ;
  double n_na_l = phi_l*c_na_l ;
  double n_k_l  = phi_l*c_k_l ;
  double n_si_l = phi_l*c_si_l ;
  double n_cl_l = phi_l*c_cl_l ;
  /* ... as mass */
  double m_l    = phi_l*rho_l ;
       
       
  /* Gas contents */
  double phi_g  = phi*s_g ;
  /* ... as elements */
  double n_c_g  = phi_g*c_co2 ;
  /* ... as mass */
  double rho_g  = M_CO2*c_co2 ;
  double m_g    = phi_g*rho_g ;


  /* Back up */
  

  /* Gas components */
  x[I_C_CO2     ] = c_co2 ;
  
  /* Liquid components */
  x[I_P_L       ] = p_l ;
  
  /* Solid components */
  x[I_N_CH    ] = n_ch ;
  x[I_V_S     ] = v_s ;
  x[I_N_Si_S  ] = n_si_s ;
  x[I_N_Ca_S  ] = n_ca_s ;
  x[I_N_CC    ] = n_cc ;
  x[I_V_CSH   ] = v_csh ;
  
  /* Porosity */
  x[I_Phi     ] = phi ;
  
  /* Element contents */
  x[I_N_C ]  = n_c_l  + n_c_s  + n_c_g ;
  x[I_N_Ca]  = n_ca_l + n_ca_s ;
  x[I_N_Na]  = n_na_l + n_na_s; 
  x[I_N_K ]  = n_k_l  + n_k_s;
  x[I_N_Si]  = n_si_l + n_si_s ;
  x[I_N_Cl]  = n_cl_l + n_cl_s ;
  
  /* Total mass */
  x[I_Mass]  = m_g + m_l + m_s ;
  
  /* Charge density */
  x[I_N_Q]   = c_q ;
  
  /* OH concentration */
  x[I_C_OH]  = c_oh ;
  
    
  return ;
}



void concentrations_oh_na_k(double c_co2,double zn_ca_s,double zn_si_s,double c_cl,double c_na_tot,double c_k_tot)
{
/* Solve a set of 3 equations:
 * 1. Electroneutralilty
 * 2. Mass balance of Na
 * 3. Mass balance of K
 * Unknowns: c_oh, c_na, c_k.
 * On input, c_na_tot and c_k_tot are the total contents of Na and K
 */
  
  /* Initialization */
  double c_na = c_na_tot ;
  double c_k  = c_k_tot ;
  double c_oh0 = c_na + c_k ;
  double c_oh = c_oh0 ;
  
  /* c_na_tot =  c_na * (A_Na + B_Na*c_oh + C_Na*c_oh*c_oh) */
  //double A_Na = 1 ;
  //double B_Na = k_naoh/k_e + k_nahco3*k_h*c_co2/k_1 ;
  //double C_Na = k_naco3*k_h*c_co2/(k_1*k_e) ;

  /* c_k_tot =  c_k * (A_K + B_K*c_oh) */
  //double A_K = 1 ;
  //double B_K = k_koh/k_e  ;
  
  double err,tol = 1.e-8 ;
  

  /* Solve cement chemistry */
  {
    double c_co2aq    = k_h*c_co2 ;
    double logc_co2aq = log10(c_co2aq) ;
    double logc_cl    = log10(c_cl) ;
  
    HardenedCementChemistry_SetInput(hcc,SI_CH_CC,MIN(zn_ca_s,0)) ;
    /*HardenedCementChemistry_GetInput(hcc,SI_Ca) = MIN(zn_ca_s,0) ; */
    HardenedCementChemistry_SetInput(hcc,SI_CSH,MIN(zn_si_s,0)) ;
    /*HardenedCementChemistry_GetInput(hcc,SI_Si) = MIN(zn_si_s,0) ;*/
    HardenedCementChemistry_GetInput(hcc,LogC_CO2) = logc_co2aq ;
    HardenedCementChemistry_GetAqueousConcentrationOf(hcc,Cl) = c_cl ;
    HardenedCementChemistry_GetLogAqueousConcentrationOf(hcc,Cl) = logc_cl ;
  }
  
  int i = 0 ;
    
  
  do {
    double dc_oh = - c_oh ;
    double logc_na    = log10(c_na) ;
    double logc_k     = log10(c_k) ;
    
    HardenedCementChemistry_GetInput(hcc,LogC_Na)  = logc_na ;
    HardenedCementChemistry_GetInput(hcc,LogC_K)   = logc_k ;
    HardenedCementChemistry_GetInput(hcc,LogC_OH)  = -7 ;
  
    HardenedCementChemistry_ComputeSystem(hcc,CaO_SiO2_Na2O_K2O_CO2) ;
      
    HardenedCementChemistry_SolveElectroneutrality(hcc) ;
    
    {
      double c_na_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,Na) ;
      
      c_na *= c_na_tot/c_na_l ;
    }
    
    //c_na = c_na_tot/(A_Na + B_Na*c_oh + C_Na*c_oh*c_oh) ;
    
    {
      double c_k_l = HardenedCementChemistry_GetElementAqueousConcentrationOf(hcc,K) ;
      
      c_k *= c_k_tot/c_k_l ;
    }
    
    //c_k  = c_k_tot/(A_K + B_K*c_oh) ;
    
    c_oh = HardenedCementChemistry_GetAqueousConcentrationOf(hcc,OH) ;
    
    dc_oh += c_oh ;
    
    err = fabs(dc_oh/c_oh) ;
    
    if(i++ > 20) {
      printf("c_na_tot = %e\n",c_na_tot) ;
      printf("c_na     = %e\n",c_na) ;
      printf("c_k_tot  = %e\n",c_k_tot) ;
      printf("c_k      = %e\n",c_k) ;
      printf("c_oh0    = %e\n",c_oh0) ;;
      printf("c_oh     = %e\n",c_oh) ;
      arret("concentrations_oh_na_k : non convergence") ;
    }

  } while(err > tol || c_oh < 0) ;
  
  /*
  {
    printf("\n") ;
    printf("c_oh = %e \n", c_oh) ;
    printf("c_na = %e \n", c_na) ;
    printf("c_k  = %e \n", c_k) ;
    printf("c_na(kcc) = %e \n", HardenedCementChemistry_GetAqueousConcentrationOf(hcc,Na)) ;
    printf("c_k(hcc)  = %e \n", HardenedCementChemistry_GetAqueousConcentrationOf(hcc,K)) ;
  }
  */
  
}



double dn1_caoh2sdt(double av0,double c)
{
  double av = ((av0 > 0) ? ((av0 < 1) ? av0 : 1) : 0) ; /* av = 1 - n_ch/n_ch0 */
  double rp = (av < 1) ? pow(1 - av,1./3) : 0 ; /* rp = Rp/R0 */
  double rc = pow(1 - av + V_CC/V_CH*av,1./3) ; /* rc = Rc/R0 */
  double width = rc - rp ;
  double dn1dt = (rc > 0.) ? rp*rp/(1 + c*width*rp/rc) : 0 ;
  
  return(dn1dt) ;
}


double PermeabilityCoefficient(Element_t* el,double phi)
{
  double coeff_permeability ;
  
  {
    /* permeability Kozeny-Carman */
    double kozeny_carman  = pow(phi/phi0,3.)*pow(((1 - phi0)/(1 - phi)),2.) ;

    /* permeability Verma Pruess 1988 */
    /* frac  = fractionnal length of pore bodies (0.8) */
    /* phi_r = fraction of initial porosity (phi/phi0) at which permeability is 0 */

	  double S_s =  (phi0 - phi)/phi0    ; /* saturation en solide */
	  double w = 1 + (1/frac)/(1/phi_r - 1) ;
    double t = (1 - S_s - phi_r)/(1 - phi_r) ;
	  double verma_pruess = (t > 0) ? t*t*(1 - frac + (frac/(w*w)))/(1 - frac + frac*(pow(t/(t + w - 1),2.))) : 0 ;
	
    /* permeability coefficient */
    coeff_permeability = kozeny_carman ;
  }
  
  return(coeff_permeability) ;
}
