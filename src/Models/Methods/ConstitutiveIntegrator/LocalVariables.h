#ifndef LOCALVARIABLES_H
#define LOCALVARIABLES_H


#include "Math_.h"
#include "Element.h"
#include "IntFct.h"
#include "CustomValues.h"
#include "FEM.h"
#include "FVM.h"


#define LocalVariables_MaxNbOfLocalValues   (Math_Max(Element_MaxNbOfNodes,IntFct_MaxNbOfIntPoints)+1)



template<typename T>
struct LocalVariables_t {
  private:
  double const* const* _u;
  double* _f;
  T  _localvalue[LocalVariables_MaxNbOfLocalValues];
  T  _localderivative[LocalVariables_MaxNbOfLocalValues];

  template<typename U>
  void _extract(int const& p,double const* f) {
    if(f) {
      U const* val1 = (U const*) f ;
      U* val2 = _localvalue + p ;
      
      val2[0] = val1[p];
    }
  }
  
  template<typename U>
  void _store(int const& p,double* f) {
    if(f) {
      U* val1 = (U*) f ;
      U* val2 = _localvalue + p ;
      
      val1[p] = val2[0];
    }
  }
  
  public:
  /* Constructors */
  LocalVariables_t(void) {}
  LocalVariables_t(double const* const* u,double* f): _u(u),_f(f) {}
  LocalVariables_t(LocalVariables_t const& a) {
    _u = a.GetPointerToNodalUnknowns();
    _f = a.GetImplicitTerm();
    {
      T* localvalue = a.GetLocalValue();
      T* localderivative = a.GetLocalDerivative();
      
      for(int i = 0 ; i < LocalVariables_MaxNbOfLocalValues ; i++) {
        _localvalue[i] = localvalue[i];
        _localderivative[i] = localderivative[i];
      }
    }
  }
  
  /* Destructor */
  ~LocalVariables_t(void) {}
  
  /* Function call operator */
  //virtual T& operator()(int const&) {}
  
  /* Assignement operator */
  inline LocalVariables_t& operator=(LocalVariables_t const& a) {
    if(this != &a) {
      _u = a.GetPointerToNodalUnknowns();
      _f = a.GetImplicitTerm();
      {
        T* localvalue = a.GetLocalValue();
        T* localderivative = a.GetLocalDerivative();
      
        for(int i = 0 ; i < LocalVariables_MaxNbOfLocalValues ; i++) {
          _localvalue[i] = localvalue[i];
          _localderivative[i] = localderivative[i];
        }
      }
    }
    return(*this);
  }

  T* Extract(Element_t const* el,int const& p) {
    if(p < LocalVariables_MaxNbOfLocalValues) {
      {
        double const* f = GetImplicitTerm();
        using U = CustomValues_TypeOfImplicitValues(T);
      
        _extract<U>(p,f);
      }
  
      {
        double const* f = Element_GetExplicitTerm(el);
        using U = CustomValues_TypeOfExplicitValues(T);
    
        _extract<U>(p,f);
      }
  
      {
        double const* f = Element_GetConstantTerm(el);
        using U = CustomValues_TypeOfConstantValues(T);
      
        _extract<U>(p,f);
      }
    
      return(_localvalue + p);
    }
    
    return(NULL);
  }
  
  void StoreImplicitTerms(int const& p) {
    if(p < LocalVariables_MaxNbOfLocalValues) {
      double* f = GetImplicitTerm();
      using U = CustomValues_TypeOfImplicitValues(T);
      
      _store<U>(p,f);
    }
  }
  
  void StoreExplicitTerms(Element_t* el,int const& p) {
    if(p < LocalVariables_MaxNbOfLocalValues) {
      double* f = Element_GetExplicitTerm(el);
      using U = CustomValues_TypeOfExplicitValues(T);
      
      _store<U>(p,f);
    }
  }
  
  void StoreConstantTerms(Element_t* el,int const& p) {
    if(p < LocalVariables_MaxNbOfLocalValues) {
      double* f = Element_GetConstantTerm(el);
      using U = CustomValues_TypeOfConstantValues(T);
      
      _store<U>(p,f);
    }
  }

  double* DisplacementVectorAndStrainFEM(Element_t const* el,int const& p,int const& u_mech,double* x) const {
    double const* const* u = _u;
    IntFct_t* intfct = Element_GetIntFct(el) ;
    //FEM_t*    fem    = FEM_GetInstance(el) ;
    int dim = Element_GetDimensionOfSpace(el) ;
    
    /* Displacements */
    {
      for(int i = 0 ; i < dim ; i++) {
        x[i] = Element_ComputeUnknown(el,u,intfct,p,u_mech + i) ;
      }
    
      for(int i = dim ; i < 3 ; i++) {
        x[i] = 0 ;
      }
    }
    
    /* Strain */
    {
      double* eps =  Element_ComputeLinearStrainTensor(el,u,intfct,p,u_mech) ;
    
      for(int i = 0 ; i < 9 ; i++) {
        x[3 + i] = eps[i] ;
      }
      
      Element_FreeBufferFrom(el,eps) ;
    }
    
    return(x);
  }

  double* ValueAndGradientFEM(Element_t const* el,int const& p,int const& u_mass,double* x) const {
    double const* const* u = _u;
    IntFct_t* intfct = Element_GetIntFct(el) ;
    //FEM_t*    fem    = FEM_GetInstance(el) ;
  
    /* Scalar */
    x[0] = Element_ComputeUnknown(el,u,intfct,p,u_mass) ;
    
    /* Scalar gradient */
    {
      double* grd = Element_ComputeUnknownGradient(el,u,intfct,p,u_mass) ;
    
      for(int i = 0 ; i < 3 ; i++) {
        x[1 + i] = grd[i] ;
      }
      
      Element_FreeBufferFrom(el,grd) ;
    }
    
    return(x);
  }

  #if 0
  double* ValueFVM(Element_t const* el,int const& n,int const& u_mass,double* x) const {
    double const* const* u = _u;
  
    /* Scalar */
    x[0] = Element_GetValueOfNodalUnknown(el,u,n,u_mass);
    
    return(x);
  }
  #endif
  
  T* GradientFVM(Element_t const* el,int const& i,int const& j,T& grdv) const {
    //FVM_t* fvm   = FVM_GetInstance(el) ;
    double* dist = Element_ComputeInternodeDistances(el) ;
    int nn = Element_GetNbOfNodes(el) ;
    double dij  = dist[nn*i + j] ;
    T const* val = _localvalue;
    
    if(i < LocalVariables_MaxNbOfLocalValues
    && j < LocalVariables_MaxNbOfLocalValues) {
      grdv = (val[j] - val[i])/dij ;
      
      return(&grdv);
    }
    
    return(NULL);
  }
  
  /* Accessors */
  double const* const* GetPointerToNodalUnknowns(void) const {return _u;}
  double*              GetImplicitTerm(void) const {return _f;}
  T*   GetLocalValue() {return _localvalue;}
  T*   GetLocalDerivative() {return _localderivative;}
  int  GetSizeOfLocalValues() const {return (sizeof(T)/sizeof(double));}
} ;


#endif
