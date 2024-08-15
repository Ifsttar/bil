#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "CommonModel.h"

#define MODELINDEX  69
#define TITLE   "Nano Double Porosity model (2011)"
#define AUTHORS "Nikoosokhan"

#include "OldMethods.h"

/* Macros */
#define NEQ     (1+dim)
#define NVI     (21)
#define NVE     (1)

#define E_co2   (dim)
#define E_mec   (0)

#define I_p_co2 (dim)
#define I_u     (0)

#define N_CO2   (f[0])
#define W_CO2   (f + 1)
#define SIG     (f + 4)
#define F_MASS  (f + 13)
#define PHI1    (f[16])
#define N_CO2_M (f[17])
#define N_CO2_m (f[18])
#define TRE     (f[19])
#define PHI_eul (f[20])

#define N_CO2n  (f_n[0])

#define K_CO2   (va[0])

/* constantes physiques */
#define RT        (2436.)   /* produit de R=8.3143 et T=293 (Pa.m3/mole) */
/* masse molaire (kg/mole) */
#define M_CO2     (44.e-3)

/* Fonctions */
static int    pm(const char*) ;
static int    c69(double**,double**,double**,double*,double*,double*,elem_t,int,geom_t,double,double*) ;
static int    k69(double**,double**,double**,double*,double*,double*,elem_t,int,geom_t,double,double*) ;
static double kozeny_carman(double,double) ;
/* Parametres */
static double gravite,young,poisson ;
static double K_m ;
static double k_int,mu_co2 ;
static double phi0_1,rho_s ;
static double p0_co2,sig0_11,sig0_22,sig0_33 ;
/*static double c_simu ; */   

#define CONCENTRATION_ADS(p)     courbe(p,el.mat->cb[0])
#define MOLAR_DENSITY(p)         courbe(p,el.mat->cb[1])
#define PRESSURE_ADS(p)        courbe(p,el.mat->cb[2])
#define COEFF(p)                 courbe(p,el.mat->cb[3])
#define DCONCENTRATION_ADS(p)    dcourbe(p,el.mat->cb[0])
#define DMOLAR_DENSITY(p)        dcourbe(p,el.mat->cb[1])
#define DPRESSURE_ADS(p)       dcourbe(p,el.mat->cb[2])
#define DCOEFF(p)                dcourbe(p,el.mat->cb[3])
#define KOZENY_CARMAN(p)         kozeny_carman(phi0_1,p)

int pm(const char *s)
{
  if(strcmp(s,"gravite") == 0) return (0) ;
  else if(strcmp(s,"poisson") == 0) return (1) ;
  else if(strcmp(s,"K_m") == 0) return (2) ;
  else if(strcmp(s,"phi0_1") == 0) return (3) ;
  else if(strcmp(s,"p0_co2") == 0) return (4) ;
  else if(strcmp(s,"k_int") == 0) return (5) ;
  else if(strcmp(s,"mu_co2") == 0) return (6) ;
  else if(strcmp(s,"rho_s") == 0) return (7) ;
  else if(strcmp(s,"sig0_11") == 0) return (8) ;
  else if(strcmp(s,"sig0_22") == 0) return (9) ;
  else if(strcmp(s,"sig0_33") == 0) return (10) ;
  else if(strcmp(s,"young") == 0) return (11) ;
  else return(-1) ;
}

int dm69(int dim,mate_t *mat,FILE *ficd)
/* Lecture des donnees materiaux dans le fichier ficd */
{
  int n_donnees = 12 ,n_courbes = 4 ;
  int i ;

  mat->neq = NEQ ;

  for(i=0;i<dim;i++) {
    sprintf(mat->eqn[E_mec + i],"meca_%d",i+1) ;
    sprintf(mat->inc[I_u + i],"u_%d",i+1) ;
  }
  strcpy(mat->eqn[E_co2],"co2") ;
  strcpy(mat->inc[I_p_co2],"p_co2") ;

  /* Par defaut tout a 0 */
  for(i=0;i<n_donnees;i++) mat->pr[i] = 0. ;
  { /* initialisation automatique (a completer) SAEID */
    mat->pr[pm("K_m")]       = 2.e8 ;
    mat->pr[pm("mu_co2")]  = 1.e-3 ;
    mat->pr[pm("gravite")] = 0;
    mat->pr[pm("rho_s")] = 0;
    mat->pr[pm("phi0_1")] = 0.3;
    mat->pr[pm("p0_co2")] = 101325;
    mat->pr[pm("k_int")] = 1.e-16 ;
    mat->pr[pm("sig0_11")] = 0 ;
    mat->pr[pm("sig0_22")] = 0 ;
    mat->pr[pm("sig0_33")] = 0 ;
    mat->pr[pm("young")] = 0.8e9;
    mat->pr[pm("poisson")] = 0.25;

  }

  lit_mate(mat,ficd,pm,n_donnees,n_courbes) ;

  return(mat->n) ;
}

int qm69(int dim,FILE *ficd)
/* Saisie des donnees materiaux */
{
  
  printf(TITLE) ;
  
  if(!ficd) return(NEQ) ;
  
  printf("\n\n\
The system consists in 1+dim equations:\n\
\t 1. Mass Balance Equation (p_co2)\n\
\t 2. Mechanical Equilibrium (u_1,u_2,u_3)\n") ;

  printf("\n\
Example of input data\n\n") ;

  fprintf(ficd,"gravite = 0       # gravity\n") ;
  fprintf(ficd,"rho_s = 0         # masse density of the dry material\n") ;
  fprintf(ficd,"K_m = 2e8    # Compression modulus of the non fractured material(K_m > young/(3-6*poisson))\n") ;
  fprintf(ficd,"phi0_1 = 0.1         # Macroporosity\n") ;
  fprintf(ficd,"p0_co2 = 1e5        # initial pressure of CO2\n") ;
  fprintf(ficd,"k_int = 1.e-12    # intrinsic permeability\n") ;
  fprintf(ficd,"mu_co2 = 1.84e-5    # viscosity of CO2\n") ;
  fprintf(ficd,"sig0_11 = 0       # initial stress 11\n") ;
  fprintf(ficd,"sig0_22 = 0       # initial stress 22\n") ;
  fprintf(ficd,"sig0_33 = 0       # initial stress 33\n") ;
  fprintf(ficd,"young = 2713e6     # Young's modulus of coal\n") ;
  fprintf(ficd,"poisson = 0.339    # Poisson's ratio of coal\n") ;
  fprintf(ficd,"Courbes = myfile    # courbes : p,gamma,Vp,sigma_s\n") ;
  
  return(NEQ) ;
}

void tb69(elem_t *el,inte_t *fi,unsigned int *n_fi,int dim)
{
  el->n_vi = NVI*el->fi->np ;
  el->n_ve = NVE*el->fi->np ;
}

void ch69(double **x,double **u,double **u_n,double *f,double *f_n,double *va,double *r,elem_t el,int dim,geom_t geom,double dt,double t,char_t cg)
/* Residu du aux chargements (r) */
{
  int i ;

  chsurf(x,r,dim,geom,dt,t,cg,el,el.fi) ;
  /* hydraulique */
  if(Element_FindEquationPositionIndex(&el,cg.eqn) == E_co2) {
    for(i=0;i<NEQ*el.nn;i++) r[i] = -r[i] ;
  }
}

void in69(double **x,double **u,double *f,double *va,elem_t el,int dim,geom_t geom)
/* Initialise les variables du systeme (f,va) */ 
{
  double K,dmu,lame ;
  double b, N ;
  int    p ;

  if(el.dim < dim) return ;

  /*
    Donnees
  */
  gravite = el.mat->pr[pm("gravite")] ;
  young   = el.mat->pr[pm("young")] ;
  K_m   = el.mat->pr[pm("K_m")] ;
  poisson = el.mat->pr[pm("poisson")] ;
  phi0_1  = el.mat->pr[pm("phi0_1")] ;
  k_int   = el.mat->pr[pm("k_int")] ;
  mu_co2  = el.mat->pr[pm("mu_co2")] ;
  rho_s   = el.mat->pr[pm("rho_s")] ;
  p0_co2  = el.mat->pr[pm("p0_co2")] ;
  sig0_11 = el.mat->pr[pm("sig0_11")] ;
  sig0_22 = el.mat->pr[pm("sig0_22")] ;
  sig0_33 = el.mat->pr[pm("sig0_33")] ;

  dmu     = young/(1 + poisson) ;
  lame    = dmu*poisson/(1 - 2*poisson) ;
  K  = young/(3-6*poisson) ;
  b = 1 - K/K_m ;
  N = (b - phi0_1)/K_m ;
 
  /*Validation of initial parameters*/
  if (K > K_m) {
      printf("\nK_m should be greater than K = %e", K) ;
      arret ("in69") ;
  }
  else if (phi0_1 > b) {
      printf("\nphi0_1 should be less than b = %f", b) ;  
      arret ("in69") ;
  }
    
  /* boucle sur les points d'integration */
  for(p=0;p<el.fi->np;p++,f += NVI,va += NVE) {
    /* fonctions d'interpolation */
    double *h  = el.fi->h  + p*el.nn ;
    double *dh = el.fi->dh + p*dim*el.nn ;
    /* pression */
    double p_co2  = param(u,h,el.nn,I_p_co2) ;
    double dp_co2 = p_co2 - p0_co2 ;
    /* contrainte superficielle */
    double p_ads   = PRESSURE_ADS(p_co2) ;
    double p_ads0  = PRESSURE_ADS(p0_co2) ;
    double dp_ads  = p_ads - p_ads0 ;
    /* pression effective */
    /*double dp_a = c_simu*dp_ads ;*/
    double dp_eff   = dp_co2 - dp_ads ;
    
    /* concentration adsorbee */
    double c_co2   = CONCENTRATION_ADS(p_co2) ;
    /* densite molaire en co2 */
    double rho_co2 = MOLAR_DENSITY(p_co2) ;
    /* coefficient  */
    double c_simu = COEFF(p_co2) ;
    /* deformations */
    double eps[9] ;
    def(x,u,h,dh,eps,el.nn,dim,geom,I_u) ;
    {
      double tre  = eps[0] + eps[4] + eps[8] ;
      /* Macroporosite */
      double dphi1 = b*tre + N*dp_eff ;
      double phi1  = phi0_1 + dphi1 ;
      /* Eulerian porosity */
      double phi_eul = phi1/(1 + tre) ;
      /* Microporosite */
    /*double dphi2 = b2*tre + N2*dp2 + G*(dp2 - dp1) ;
      double phi2  = phi0_2 + dphi2 ;*/
      /* contraintes */
      double sig[9] ;
      int    i ;
      for(i=0;i<9;i++) sig[i] = dmu*eps[i] ;
      sig[0] += sig0_11 + lame*tre - b*dp_eff  - dp_ads ;
      sig[4] += sig0_22 + lame*tre - b*dp_eff  - dp_ads ;
      sig[8] += sig0_33 + lame*tre - b*dp_eff  - dp_ads ;
      {
        /* surface solide interne */
        /*double omega  = omega0 + c_eps*tre + c_phi1*dphi1 + c_phi2*dphi2 ;*/
        /* contenu molaire en co2 */
        double n_co2_m = c_co2*(1 - phi0_1 + c_simu*((1 - b)*tre - N*dp_eff))  ; /* nombre de molecules de fluid dans Micropore par unite volume*/
        double n_co2_M = rho_co2*phi1 ; /* nombre de molecules de fluid dans le volume de Macropore par unite volume*/
        double n_co2   = rho_co2*phi1 + n_co2_m ;
        /* coefficient de transfert -kozeny-carman*/
        double k_co2 = rho_co2*k_int/mu_co2*KOZENY_CARMAN(dphi1) ;
        /* flux */
        double w_co2[3],grd_co2[3] ;
        grad(x,u,dh,grd_co2,el.nn,dim,I_p_co2) ;
        for(i=0;i<3;i++) w_co2[i] = - k_co2*grd_co2[i] ;
        w_co2[dim-1] += k_co2*rho_co2*gravite ;
        /* rangement dans f */
        N_CO2 = n_co2 ;
        N_CO2_M = n_co2_M ;
        N_CO2_m = n_co2_m ;
        for(i=0;i<3;i++) W_CO2[i]  = w_co2[i] ;
        for(i=0;i<9;i++) SIG[i]    = sig[i] ;
        for(i=0;i<3;i++) F_MASS[i] = 0. ;
        F_MASS[dim-1] = (rho_s + M_CO2*n_co2)*gravite ;
        PHI1 = phi1 ;
        PHI_eul = phi_eul ;
        TRE  = tre ;
        /* rangement dans va */
        K_CO2 = k_co2 ;
      }
    }
  }
}

int ex69(double **x,double **u,double *f,double *va,elem_t el,int dim,geom_t geom,double t) 
/* Termes explicites (va)  */
{
  int    p ;
  double b, N, K ;
  if(el.dim < dim) return(0) ;

  /*
    Donnees
  */
  gravite = el.mat->pr[pm("gravite")] ;
  young   = el.mat->pr[pm("young")] ;
  K_m   = el.mat->pr[pm("K_m")] ;
  poisson = el.mat->pr[pm("poisson")] ;
  phi0_1  = el.mat->pr[pm("phi0_1")] ;
  k_int   = el.mat->pr[pm("k_int")] ;
  mu_co2  = el.mat->pr[pm("mu_co2")] ;
  rho_s   = el.mat->pr[pm("rho_s")] ;
  p0_co2  = el.mat->pr[pm("p0_co2")] ;
  sig0_11 = el.mat->pr[pm("sig0_11")] ;
  sig0_22 = el.mat->pr[pm("sig0_22")] ;
  sig0_33 = el.mat->pr[pm("sig0_33")] ;
  
  K = young/(3-6*poisson) ;
  b  = 1 - K/K_m ;
  N = (b - phi0_1)/K_m ;
   
  /* boucle sur les points d'integration */
  for(p=0;p<el.fi->np;p++,va += NVE) {
    /* fonctions d'interpolation */
    double *h  = el.fi->h  + p*el.nn ;
    double *dh = el.fi->dh + p*dim*el.nn ;
    /* pression */
    double p_co2  = param(u,h,el.nn,I_p_co2) ;
    double dp_co2 = p_co2 - p0_co2 ;
    /* contrainte superficielle */
    double p_ads   = PRESSURE_ADS(p_co2) ;
    double p_ads0  = PRESSURE_ADS(p0_co2) ;
    double dp_ads  = p_ads - p_ads0 ;
    /* pression effective */
    /*double dp_a = c_simu*dp_ads ;*/
    double dp_eff   = dp_co2 - dp_ads ;
    /* densite molaire en co2 */
    double rho_co2 = MOLAR_DENSITY(p_co2) ;
    /* coefficient  */
    /*double c_simu = COEFF(p_co2) ;*/
    /* deformations */
    double eps[9] ;
    def(x,u,h,dh,eps,el.nn,dim,geom,I_u) ;
    {
      double tre  = eps[0] + eps[4] + eps[8] ;
      /* Macroporosite */
      double dphi1 = b*tre + N*dp_eff ;
      /* Microporosite */
      /*double dphi2 = b2*tre + N2*dp2 + G*dpp_a ;*/
      /*double phi2  = phi0_2 + dphi2 ;*/
      /* coefficient de transfert -kozeny-carman*/
      double k_co2 = rho_co2*k_int/mu_co2*KOZENY_CARMAN(dphi1) ;

      /* rangement dans va */
      K_CO2 = k_co2 ;
    }
  }
  return(0) ;
}


int ct69(double **x,double **u,double **u_n,double *f,double *f_n,double *va,elem_t el,int dim,geom_t geom,double dt,double t)
/* Les variables donnees par la loi de comportement (f_1) */
{
  double K,dmu,lame ;
  int    p ;
  double b, N ;
  /*double *f_1 = f ;*/
  
  if(el.dim < dim) return(0) ;

  /*
    Donnees
  */
  gravite = el.mat->pr[pm("gravite")] ;
  young   = el.mat->pr[pm("young")] ;
  K_m   = el.mat->pr[pm("K_m")] ;
  poisson = el.mat->pr[pm("poisson")] ;
  phi0_1  = el.mat->pr[pm("phi0_1")] ;
  k_int   = el.mat->pr[pm("k_int")] ;
  mu_co2  = el.mat->pr[pm("mu_co2")] ;
  rho_s   = el.mat->pr[pm("rho_s")] ;
  p0_co2  = el.mat->pr[pm("p0_co2")] ;
  sig0_11 = el.mat->pr[pm("sig0_11")] ;
  sig0_22 = el.mat->pr[pm("sig0_22")] ;
  sig0_33 = el.mat->pr[pm("sig0_33")] ;

  dmu     = young/(1 + poisson) ;
  lame    = dmu*poisson/(1 - 2*poisson) ;
  K = young/(3-6*poisson) ;
  b  = 1 - K/K_m ;
  N = (b - phi0_1)/K_m ;
    
  /* boucle sur les points d'integration */
  for(p=0;p<el.fi->np;p++,f += NVI,va += NVE) {
    /* fonctions d'interpolation */
    double *h  = el.fi->h  + p*el.nn ;
    double *dh = el.fi->dh + p*dim*el.nn ;
    /* pression */
    double p_co2  = param(u,h,el.nn,I_p_co2) ;
    double dp_co2 = p_co2 - p0_co2 ;
    /* contrainte superficielle */
    double p_ads   = PRESSURE_ADS(p_co2) ;
    double p_ads0  = PRESSURE_ADS(p0_co2) ;
    double dp_ads  = p_ads - p_ads0 ;
   /*double dp_a = c_simu*dp_ads ;*/
    double dp_eff   = dp_co2 - dp_ads ;
    /* concentration adsorbee */
    double c_co2   = CONCENTRATION_ADS(p_co2) ;
    /* densite molaire en co2 */
    double rho_co2 = MOLAR_DENSITY(p_co2) ;
    /* coefficient  */
    double c_simu = COEFF(p_co2) ;
    /* deformations */
    double eps[9] ;
    
    if(p_co2 < 0) {
      printf("p_co2 = %e\n",p_co2) ;
      return(-1) ;
    }
    def(x,u,h,dh,eps,el.nn,dim,geom,I_u) ;
    {
      double tre  = eps[0] + eps[4] + eps[8] ;
      /* Macroporosite */
      double dphi1 = b*tre + N*dp_eff ;
      double phi1  = phi0_1 + dphi1 ;
      /* Eulerian porosity */
      double phi_eul = phi1/(1 + tre) ;
      /* Microporosite */
      /*double dphi2 = b2*tre + N2*dp2 + G*(dp2 - dp1) ;
      double phi2  = phi0_2 + dphi2 ;*/
      /* contraintes */
      double sig[9] ;
      int    i ;
      for(i=0;i<9;i++) sig[i] = dmu*eps[i] ;
      sig[0] += sig0_11 + lame*tre - b*dp_eff  - dp_ads ;
      sig[4] += sig0_22 + lame*tre - b*dp_eff  - dp_ads ;
      sig[8] += sig0_33 + lame*tre - b*dp_eff  - dp_ads ;
      {
        /* surface solide interne */
       /* double omega   = omega0 + c_eps*tre + c_phi1*dphi1 + c_phi2*dphi2 ;*/
       /* contenu molaire en co2 */
        double n_co2_m = c_co2*(1 - phi0_1 + c_simu*((1 - b)*tre - N*dp_eff))  ; /* nombre de molecules de fluid dans Micropore par unite volume*/
        double n_co2_M = rho_co2*phi1 ; /* nombre de molecules de fluid dans le volume de Macropore par unite volume*/
        double n_co2   = rho_co2*phi1 + n_co2_m ;
        /* coefficient de transfert */
        double k_co2   = K_CO2 ;
        /* flux */
        double w_co2[3],grd_co2[3] ;
        grad(x,u,dh,grd_co2,el.nn,dim,I_p_co2) ;
        for(i=0;i<3;i++) w_co2[i] = - k_co2*grd_co2[i] ;
        w_co2[dim-1] += k_co2*rho_co2*gravite ;
        /* rangement dans f */
        N_CO2 = n_co2 ;
        N_CO2_M = n_co2_M ;
        N_CO2_m = n_co2_m ;
        for(i=0;i<3;i++) W_CO2[i]  = w_co2[i] ;
        for(i=0;i<9;i++) SIG[i]    = sig[i] ;
        for(i=0;i<3;i++) F_MASS[i] = 0. ;
        F_MASS[dim-1] = (rho_s + M_CO2*n_co2)*gravite ;
        PHI1 = phi1 ;
        PHI_eul = phi_eul ; 
        TRE  = tre ;
      }
    }
  }

  return(0) ;
}

int mx69(double **x,double **u,double **u_n,double *f,double *f_n,double *va,double *k,elem_t el,int dim,geom_t geom,double dt,double t)
/* Matrice (k) */
{
#define K(i,j)    (k[(i)*el.nn*NEQ+(j)])
  int    i,n,m,dec ;
  double c[MAX_PGAUSS*100] ;
  double kb[MAX_NOEUDS*MAX_NOEUDS] ;
  
  /* initialisation */
  for(i=0;i<el.nn*el.nn*NEQ*NEQ;i++) k[i] = 0. ;

  if(el.dim < dim) return(0) ;

  /*
  ** Matrice de comportement
  */
  dec = c69(x,u,u_n,f,f_n,va,el,dim,geom,dt,c) ;
  mxbiot(k,x,*el.fi,c,dim,dec,geom,NEQ,E_mec,I_u,E_co2,I_p_co2) ;
  /*
  ** Matrice de conduction
  */
  dec = k69(x,u,u_n,f,f_n,va,el,dim,geom,dt,c) ;
  mxcond(kb,x,*el.fi,c,dim,dec,geom) ;
  for(n=0;n<el.nn;n++) for(m=0;m<el.nn;m++) {
    K(E_co2+n*NEQ,I_p_co2+m*NEQ) += dt*kb[n*el.nn+m] ;
  }

  return(0) ;
#undef K
}

void rs69(double **x,double **u,double **u_n,double *f,double *f_n,double *va,double *r,elem_t el,int dim,geom_t geom,double dt,double t)
/* Residu (r) */
{
#define R(n,i)    (r[(n)*NEQ+(i)])
  int    i,n ;
  double rb[3*MAX_NOEUDS],g1[MAX_PGAUSS] ;
  double *f_1 = f ;

  /* initialisation */
  for(i=0;i<el.nn*NEQ;i++) r[i] = 0. ;

  if(el.dim < dim) return ;

  /* 1. Mecanique */
  /* 1.1 Contraintes */
  rscont(rb,x,*el.fi,SIG,dim,NVI,geom) ;
  for(n=0;n<el.nn;n++) for(i=0;i<dim;i++) R(n,E_mec+i) -= rb[i+n*dim] ;
  /* 1.2 Forces de masse */
  rsmass(rb,x,*el.fi,F_MASS+dim-1,dim,NVI,geom) ;
  for(n=0;n<el.nn;n++) R(n,E_mec+dim-1) -= -rb[n] ;

  /* 2. Hydraulique */
  /* 2.1 Termes d'accumulation */
  for(i=0;i<el.fi->np;i++,f += NVI,f_n += NVI) g1[i] = N_CO2 - N_CO2n ;
  rsmass(rb,x,*el.fi,g1,dim,1,geom) ;
  for(i=0;i<el.nn;i++) R(i,E_co2) -= rb[i] ;
  /* 2.2 Termes de transport */
  f = f_1 ;
  rsflux(rb,x,*el.fi,W_CO2,dim,NVI,geom) ;
  for(i=0;i<el.nn;i++) R(i,E_co2) -= -dt*rb[i] ;

#undef R
}

int so69(double **x,double **u,double *f,double *va,double *s,resu_t *r,elem_t el,int dim,geom_t geom,double t)
/* Les valeurs exploitees (s) */
{
#define NSO   (15)

  if(el.dim < dim) return(0) ;

  /*
    Donnees
  */
  gravite = el.mat->pr[pm("gravite")] ;
  young   = el.mat->pr[pm("young")] ;
  K_m   = el.mat->pr[pm("K_m")] ;
  poisson = el.mat->pr[pm("poisson")] ;
  phi0_1  = el.mat->pr[pm("phi0_1")] ;
  k_int   = el.mat->pr[pm("k_int")] ;
  mu_co2  = el.mat->pr[pm("mu_co2")] ;
  rho_s   = el.mat->pr[pm("rho_s")] ;
  p0_co2  = el.mat->pr[pm("p0_co2")] ;
  sig0_11 = el.mat->pr[pm("sig0_11")] ;
  sig0_22 = el.mat->pr[pm("sig0_22")] ;
  sig0_33 = el.mat->pr[pm("sig0_33")] ;

  /* initialisation */
  {
    int    i,j ;
    for(i=0;i<NSO;i++) for(j=0;j<9;j++) r[i].v[j] = 0. ;
  }

  {
    int    nso = 0 ;
    double h_s[MAX_NOEUDS],dh_s[3*MAX_NOEUDS] ;
    /* quantites exploitees en s */
    fint_abs(dim,el.nn,x,s,el.dim,h_s,dh_s) ;
    {
      int    i,p ;
      /* pressions */
      double p_co2   = param(u,h_s,el.nn,I_p_co2) ;
      /* concentration adsorbe */
      double c_co2   = CONCENTRATION_ADS(p_co2) ;
      /* densite molaire en co2 */
      double rho_co2 = MOLAR_DENSITY(p_co2) ;
      /* contrainte superficielle */
      double p_ads   = PRESSURE_ADS(p_co2) ;
	
	  /* coefficient  */
      double c_simu = COEFF(p_co2) ;
    
    
      strcpy(r[nso].text,"pression-co2") ; r[nso].n = 1 ;
      r[nso++].v[0] =  p_co2 ;
      
      strcpy(r[nso].text,"deplacements") ; r[nso].n = 3 ;
      for(i=0;i<dim;i++) r[nso].v[i] = param(u,h_s,el.nn,I_u+i) ;
      nso++ ;
      
      strcpy(r[nso].text,"concentration_adsorbee") ; r[nso].n = 1 ;
      r[nso++].v[0] =  c_co2 ;
      
      strcpy(r[nso].text,"adsorption-induced pressure") ; r[nso].n = 1 ;
      r[nso++].v[0] =  p_ads ;
      
      strcpy(r[nso].text,"densite_molaire_co2") ; r[nso].n = 1 ;
      r[nso++].v[0] =  rho_co2 ;
      
      strcpy(r[nso].text,"cxoeff") ; r[nso].n = 1 ;
      r[nso++].v[0] =  c_simu ;
      
      /* boucle sur les points d'integration */
      for(p=0;p<el.fi->np;p++,f += NVI) {
        strcpy(r[nso].text,"Macroporosite") ; r[nso].n = 1 ;
        r[nso].v[0] += PHI1/el.fi->np ;
		
        strcpy(r[nso+1].text,"contraintes") ; r[nso+1].n = 9 ;
        for(i=0;i<9;i++) r[nso+1].v[i] += SIG[i]/el.fi->np ;
	
        strcpy(r[nso+2].text,"flux") ; r[nso+2].n = 3 ;
        for(i=0;i<3;i++) r[nso+2].v[i] += W_CO2[i]/el.fi->np ;
	
        strcpy(r[nso+3].text,"permeabilite") ; r[nso+3].n = 1 ;
        r[nso+3].v[0] += K_CO2/rho_co2/k_int*mu_co2/el.fi->np ;

        strcpy(r[nso+4].text,"moles_totales") ; r[nso+4].n = 1 ;
        r[nso+4].v[0] += N_CO2/el.fi->np ;
	
        strcpy(r[nso+5].text,"moles_Macropores") ; r[nso+5].n = 1 ;
        r[nso+5].v[0] += N_CO2_M/el.fi->np ;
	
        strcpy(r[nso+6].text,"moles_Micropores") ; r[nso+6].n = 1 ;
        r[nso+6].v[0] += N_CO2_m/el.fi->np ;
        
        strcpy(r[nso+7].text,"Volumetric strain") ; r[nso+7].n = 1 ;
        r[nso+7].v[0] += TRE/el.fi->np ;
        
        strcpy(r[nso+8].text,"Eulerian Porosity") ; r[nso+8].n = 1 ;
        r[nso+8].v[0] += PHI_eul/el.fi->np ;
        
           
       }
    }
    nso += 9 ;
    if(nso != NSO) arret("so69") ;
  }

  return (NSO) ;
}

int c69(double **x,double **u,double **u_n,double *f,double *f_n,double *va,elem_t el,int dim,geom_t geom,double dt,double *c)
/*
**  Matrice de comportement (c) et decalage (dec)
*/
{
#define C1(i,j,k,l)  (c1[(((i)*3+(j))*3+(k))*3+(l)])
#define B1(i,j)      (c1[(i)*3+(j)])
  double K,dmu,lame,mu ;
  double b, N ;
  int    dec = 100 ;
  int    p ;
  
  /*
    Donnees
  */
  gravite = el.mat->pr[pm("gravite")] ;
  young   = el.mat->pr[pm("young")] ;
  K_m   = el.mat->pr[pm("K_m")] ;
  poisson = el.mat->pr[pm("poisson")] ;
  phi0_1  = el.mat->pr[pm("phi0_1")] ;
  k_int   = el.mat->pr[pm("k_int")] ;
  mu_co2  = el.mat->pr[pm("mu_co2")] ;
  rho_s   = el.mat->pr[pm("rho_s")] ;
  p0_co2  = el.mat->pr[pm("p0_co2")] ;
  sig0_11 = el.mat->pr[pm("sig0_11")] ;
  sig0_22 = el.mat->pr[pm("sig0_22")] ;
  sig0_33 = el.mat->pr[pm("sig0_33")] ;
  dmu     = young/(1 + poisson) ;
  mu      = dmu/2 ;
  lame    = dmu*poisson/(1 - 2*poisson) ;
  K  = young/(3-6*poisson) ;
  b = 1 - K/K_m ;
  N = (b - phi0_1)/K_m ;
   
  for(p=0;p<el.fi->np;p++) {
    int    i,j ;
    double *c1 = c + p*dec ;
    /* fonctions d'interpolation */
    double *h  = el.fi->h  + p*el.nn ;
    double *dh = el.fi->dh + p*dim*el.nn ;
    /* pressions */
    double p_co2  = param(u,h,el.nn,I_p_co2) ;
    double dp_co2 = p_co2 - p0_co2 ;
    /* contrainte superficielle */
    double p_ads   = PRESSURE_ADS(p_co2) ;
    double p_ads0  = PRESSURE_ADS(p0_co2) ;
    double dp_ads  = p_ads - p_ads0 ;
     /* pression effective */
    /*double dp_a = c_simu*dp_ads ;*/
    double dp_eff   = dp_co2 - dp_ads ;
    /* densite molaire en co2 */
    double rho_co2 = MOLAR_DENSITY(p_co2) ;
    /* concentration adsorbe */
    double c_co2   = CONCENTRATION_ADS(p_co2) ;
    /* coefficient  */
    double c_simu = COEFF(p_co2) ;
    /* deformations */
    double eps[9] ;
    def(x,u,h,dh,eps,el.nn,dim,geom,I_u) ;
    {
      double tre  = eps[0] + eps[4] + eps[8] ;
     /* Macroporosite */
      double dphi1 = b*tre + N*dp_eff ;
      double phi1  = phi0_1 + dphi1 ;
     
     /* derivees */
      /* ... par rapport a p_co2 */
      double dp_adssdp_co2   = DPRESSURE_ADS(p_co2) ;
      double dpsdp_co2  = 1. - dp_adssdp_co2 ;
      double drho_co2sdp_co2 = DMOLAR_DENSITY(p_co2) ;
      double dphi1sdp_co2   = N*dpsdp_co2  ;
      double dc_co2sdp_co2  = DCONCENTRATION_ADS(p_co2) ;
      double dcoeffsdp_co2 = DCOEFF(p_co2) ;
      /* initialisation */
      for(i=0;i<dec;i++) c1[i] = 0. ;
      
      /* mecanique */
      for(i=0;i<3;i++) for(j=0;j<3;j++) {
        C1(i,i,j,j) += lame ;
        C1(i,j,i,j) += mu ;
        C1(i,j,j,i) += mu ;
      }
      c1 += 81 ;
      for(i=0;i<3;i++) B1(i,i) = -b*dpsdp_co2 - dp_adssdp_co2 ;  /*dsigma/dp*/
      
      c1 += 9 ;
      /* hydraulique */
      for(i=0;i<3;i++) B1(i,i) = rho_co2*b + c_co2*c_simu*(1-b) ;/*dn_co2/depsilon*/
      c1 += 9 ;
      c1[0] = rho_co2*dphi1sdp_co2 + drho_co2sdp_co2*phi1 + dc_co2sdp_co2*(1 - phi0_1 + c_simu*((1 - b)*tre - N*dp_eff)) + c_co2*(dcoeffsdp_co2*((1 - b)*tre - N*dp_eff) - c_simu*N*dpsdp_co2) ; /*dn_co2/dp*/
    }
  }
  return(dec) ;
  
#undef C1
#undef B1
}

int k69(double **x,double **u,double **u_n,double *f,double *f_n,double *va,elem_t el,int dim,geom_t geom,double dt,double *c)
/*
**  Matrice de conduction (c) et decalage (dec)
*/
{
  int    dec = 9 ;
  int    p ;
  
  for(p=0;p<el.fi->np;p++,va += NVE) {
    double *c1 = c + p*dec ;
    int    i ;
    /* initialisation */
    for(i=0;i<dec;i++) c1[i] = 0. ;
    /* permeabilite */
    c1[0] = K_CO2 ;
    c1[4] = K_CO2 ;
    c1[8] = K_CO2 ;
  }
  return(dec) ;
}

double kozeny_carman(double phi0,double dphi)
{
  /* linearization of (phi/phi0)^3*((1-phi0)/(1-phi))^2 */
  return(1 + (3/phi0 + 2/(1 - phi0))*dphi) ;
}
