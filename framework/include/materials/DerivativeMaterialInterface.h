/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/
#ifndef DERIVATIVEMATERIALINTERFACE_H
#define DERIVATIVEMATERIALINTERFACE_H

#include "Material.h"
#include "MaterialProperty.h"
#include "KernelBase.h"
#include "FEProblem.h"
#include "BlockRestrictable.h"
#include "BoundaryRestrictable.h"
#include "DerivativeMaterialPropertyNameInterface.h"

/**
 * Helper function templates to set a variable to zero.
 * Specializations may have to be implemented (for examples see
 * RankTwoTensor, RankFourTensor, ElasticityTensorR4).
 */
template<typename T>
void mooseSetToZero(T & v)
{
  /**
   * The default for non-pointer types is to assign zero.
   * This should either do something sensible, or throw a compiler error.
   * Otherwise the T type is designed badly.
   */
  v = 0;
}
template<typename T>
void mooseSetToZero(T* &)
{
  mooseError("Cannot use pointer types for MaterialProperty derivatives.");
}

/**
 * Interface class ("Veneer") to provide generator methods for derivative
 * material property names
 */
template<class T>
class DerivativeMaterialInterface :
  public T,
  public DerivativeMaterialPropertyNameInterface
{
public:
  DerivativeMaterialInterface(const InputParameters & parameters);

  /**
   * Fetch a material property if it exists, otherwise return getZeroMaterialProperty.
   * @param name The input parameter key of type MaterialPropertyName
   */
  template<typename U>
  const MaterialProperty<U> & getDefaultMaterialProperty(const std::string & name);

  /// Fetch a material property by name if it exists, otherwise return getZeroMaterialProperty
  template<typename U>
  const MaterialProperty<U> & getDefaultMaterialPropertyByName(const std::string & name);

  ///@{
  /**
   * Methods for declaring derivative material properties
   * @tparam U The material property type
   * @param base The name of the property to take the derivative of
   * @param c The variable(s) to take the derivatives with respect to
   */
  template<typename U>
  MaterialProperty<U> & declarePropertyDerivative(const std::string & base, const std::vector<VariableName> & c);
  template<typename U>
  MaterialProperty<U> & declarePropertyDerivative(const std::string & base, const VariableName & c1, const VariableName & c2 = "", const VariableName & c3 = "");
  ///@}

  ///@{
  /**
   * Methods for retreiving derivative material properties
   * @tparam U The material property type
   * @param base The name of the property to take the derivative of
   * @param c The variable(s) to take the derivatives with respect to
   */
  template<typename U>
  const MaterialProperty<U> & getMaterialPropertyDerivative(const std::string & base, const std::vector<VariableName> & c);
  template<typename U>
  const MaterialProperty<U> & getMaterialPropertyDerivative(const std::string & base, const VariableName & c1, const VariableName & c2 = "", const VariableName & c3 = "");
  ///@}

  ///@{
  /**
   * Methods for retreiving derivative material properties
   * @tparam U The material property type
   * @param base The name of the property to take the derivative of
   * @param c The variable(s) to take the derivatives with respect to
   */
  template<typename U>
  const MaterialProperty<U> & getMaterialPropertyDerivativeByName(const MaterialPropertyName & base, const std::vector<VariableName> & c);
  template<typename U>
  const MaterialProperty<U> & getMaterialPropertyDerivativeByName(const MaterialPropertyName & base, const VariableName & c1, const VariableName & c2 = "", const VariableName & c3 = "");
  ///@}

  ///@{
  /**
   * check if derivatives of the passed in material property exist w.r.t a variable
   * that is _not_ coupled in to the current object
   */
  template<typename U>
  void validateCoupling(const MaterialPropertyName & base, const std::vector<VariableName> & c, bool validate_aux = true);
  template<typename U>
  void validateCoupling(const MaterialPropertyName & base, const VariableName & c1 = "", const VariableName & c2 = "", const VariableName & c3 = "");
  template<typename U>
  void validateNonlinearCoupling(const MaterialPropertyName & base, const VariableName & c1 = "", const VariableName & c2 = "", const VariableName & c3 = "");
  ///@}

private:
  /// Return a constant zero property
  template<typename U>
  const MaterialProperty<U> & getZeroMaterialProperty(const std::string & prop_name);

  /// Check if a material property is present with the applicable restrictions
  template<typename U>
  bool haveMaterialProperty(const std::string & prop_name);

  /// helper method to combine multiple VariableNames into a vector (if they are != "")
  std::vector<VariableName> buildVariableVector(const VariableName & c1, const VariableName & c2, const VariableName & c3);

  /// helper method to compile list of missing coupled variables for a given system
  template<typename U>
  void validateCouplingHelper(const MaterialPropertyName & base, const std::vector<VariableName> & c, const System & system, std::vector<VariableName> & missing);

  // check if the speciified variable name is not the variable this kernel is acting on (always true for any other type of object)
  bool isNotKernelVariable(const VariableName & name);

  /// Reference to FEProblem
  FEProblem & _dmi_fe_problem;

  /// Reference to this objects MaterialData object
  MaterialData & _dmi_material_data;
};


template<class T>
DerivativeMaterialInterface<T>::DerivativeMaterialInterface(const InputParameters & parameters) :
    T(parameters),
    _dmi_fe_problem(*parameters.getCheckedPointerParam<FEProblem *>("_fe_problem")),
    _dmi_material_data(*parameters.getCheckedPointerParam<MaterialData *>("_material_data"))
{
}

template<>
template<typename U>
const MaterialProperty<U> &
DerivativeMaterialInterface<Material>::getZeroMaterialProperty(const std::string & prop_name)
{
  // declare this material property
  MaterialProperty<U> & preload_with_zero = this->template declareProperty<U>(prop_name);

  // resize to accomodate maximum number of qpoints
  unsigned int nqp = _dmi_fe_problem.getMaxQps();
  preload_with_zero.resize(nqp);

  // set values for all qpoints to zero
  for (unsigned int qp = 0; qp < nqp; ++qp)
    mooseSetToZero<U>(preload_with_zero[qp]);

  return preload_with_zero;
}

template<class T>
template<typename U>
const MaterialProperty<U> &
DerivativeMaterialInterface<T>::getZeroMaterialProperty(const std::string & /*prop_name*/)
{
  static MaterialProperty<U> _zero;

  // make sure _zero is in a sane state
  unsigned int nqp = _dmi_fe_problem.getMaxQps();
  if (nqp > _zero.size())
  {
    // resize to accomodate maximum number of qpoints
    _zero.resize(nqp);

    // set values for all qpoints to zero
    for (unsigned int qp = 0; qp < nqp; ++qp)
      mooseSetToZero<U>(_zero[qp]);
  }

  // return a reference to a static zero property
  return _zero;
}

template<>
template<typename U>
bool
DerivativeMaterialInterface<Material>::haveMaterialProperty(const std::string & prop_name)
{
  return ((this->boundaryRestricted() && this->template hasBoundaryMaterialProperty<U>(prop_name)) ||
         (this->template hasBlockMaterialProperty<U>(prop_name)));
}

template<class T>
template<typename U>
bool
DerivativeMaterialInterface<T>::haveMaterialProperty(const std::string & prop_name)
{
  // Call the correct method to test for material property declarations
  BlockRestrictable * blk = dynamic_cast<BlockRestrictable *>(this);
  BoundaryRestrictable * bnd = dynamic_cast<BoundaryRestrictable *>(this);
  return ((bnd && bnd->boundaryRestricted() && bnd->template hasBoundaryMaterialProperty<U>(prop_name)) ||
         (blk && blk->template hasBlockMaterialProperty<U>(prop_name)) ||
         (this->template hasMaterialProperty<U>(prop_name)));
}

template<class T>
template<typename U>
const MaterialProperty<U> &
DerivativeMaterialInterface<T>::getDefaultMaterialProperty(const std::string & name)
{
  // get the base property name
  std::string prop_name = this->deducePropertyName(name);

  // Check if it's just a constant
  const MaterialProperty<U> * default_property = this->template defaultMaterialProperty<U>(prop_name);
  if (default_property)
    return *default_property;

  // if found return the requested property
  return getDefaultMaterialPropertyByName<U>(prop_name);
}

template<class T>
template<typename U>
const MaterialProperty<U> &
DerivativeMaterialInterface<T>::getDefaultMaterialPropertyByName(const std::string & prop_name)
{
  // if found return the requested property
  if (haveMaterialProperty<U>(prop_name))
    return this->template getMaterialPropertyByName<U>(prop_name);

  return getZeroMaterialProperty<U>(prop_name);
}


template<class T>
template<typename U>
MaterialProperty<U> &
DerivativeMaterialInterface<T>::declarePropertyDerivative(const std::string & base, const std::vector<VariableName> & c)
{
  return this->template declareProperty<U>(propertyName(base, c));
}

template<class T>
template<typename U>
MaterialProperty<U> &
DerivativeMaterialInterface<T>::declarePropertyDerivative(const std::string & base, const VariableName & c1, const VariableName & c2, const VariableName & c3)
{
  if (c3 != "")
    return this->template declareProperty<U>(propertyNameThird(base, c1, c2, c3));
  if (c2 != "")
    return this->template declareProperty<U>(propertyNameSecond(base, c1, c2));
  return this->template declareProperty<U>(propertyNameFirst(base, c1));
}


template<class T>
template<typename U>
const MaterialProperty<U> &
DerivativeMaterialInterface<T>::getMaterialPropertyDerivative(const std::string & base, const std::vector<VariableName> & c)
{
  // get the base property name
  std::string prop_name = this->deducePropertyName(base);

  /**
   * Check if base is a default property and shortcut to returning zero, as
   * derivatives of constants are zero.
   */
  if (this->template defaultMaterialProperty<U>(prop_name))
    return getZeroMaterialProperty<U>(prop_name + "_zeroderivative");

  return getDefaultMaterialPropertyByName<U>(propertyName(prop_name, c));
}

template<class T>
template<typename U>
const MaterialProperty<U> &
DerivativeMaterialInterface<T>::getMaterialPropertyDerivative(const std::string & base, const VariableName & c1, const VariableName & c2, const VariableName & c3)
{
  // get the base property name
  std::string prop_name = this->deducePropertyName(base);

  /**
   * Check if base is a default property and shortcut to returning zero, as
   * derivatives of constants are zero.
   */
  if (this->template defaultMaterialProperty<U>(prop_name))
    return getZeroMaterialProperty<U>(prop_name + "_zeroderivative");

  if (c3 != "")
    return getDefaultMaterialPropertyByName<U>(propertyNameThird(prop_name, c1, c2, c3));
  if (c2 != "")
    return getDefaultMaterialPropertyByName<U>(propertyNameSecond(prop_name, c1, c2));
  return getDefaultMaterialPropertyByName<U>(propertyNameFirst(prop_name, c1));
}


template<class T>
template<typename U>
const MaterialProperty<U> &
DerivativeMaterialInterface<T>::getMaterialPropertyDerivativeByName(const MaterialPropertyName & base, const std::vector<VariableName> & c)
{
  return getDefaultMaterialPropertyByName<U>(propertyName(base, c));
}

template<class T>
template<typename U>
const MaterialProperty<U> &
DerivativeMaterialInterface<T>::getMaterialPropertyDerivativeByName(const MaterialPropertyName & base, const VariableName & c1, const VariableName & c2, const VariableName & c3)
{
  if (c3 != "")
    return getDefaultMaterialPropertyByName<U>(propertyNameThird(base, c1, c2, c3));
  if (c2 != "")
    return getDefaultMaterialPropertyByName<U>(propertyNameSecond(base, c1, c2));
  return getDefaultMaterialPropertyByName<U>(propertyNameFirst(base, c1));
}

template<class T>
template<typename U>
void
DerivativeMaterialInterface<T>::validateCouplingHelper(const MaterialPropertyName & base, const std::vector<VariableName> & c, const System & system, std::vector<VariableName> & missing)
{
  unsigned int ncoupled = this->_coupled_moose_vars.size();

  // iterate over all variables in the current system (in groups)
  for (unsigned int i = 0; i < system.n_variable_groups(); ++i)
  {
    const VariableGroup & vg = system.variable_group(i);
    for (unsigned int j = 0; j < vg.n_variables(); ++j)
    {
      std::vector<VariableName> cj(c);
      VariableName jname = vg.name(j);
      cj.push_back(jname);

      // if the derivative exists make sure the variable is coupled
      if (haveMaterialProperty<U>(propertyName(base, cj)))
      {
        // kernels to not have the variable they are acting on in coupled_moose_vars
        bool is_missing = isNotKernelVariable(jname);

        for (unsigned int k = 0; k < ncoupled; ++k)
          if (this->_coupled_moose_vars[k]->name() == jname)
          {
            is_missing = false;
            break;
          }

        if (is_missing)
          missing.push_back(jname);
      }
    }
  }
}

template<class T>
template<typename U>
void
DerivativeMaterialInterface<T>::validateCoupling(const MaterialPropertyName & base, const std::vector<VariableName> & c, bool validate_aux)
{
  // get the base property name
  std::string prop_name = this->deducePropertyName(base);
  // list of potentially missing coupled variables
  std::vector<VariableName> missing;

  // iterate over all variables in the both the non-linear and auxiliary system (optional)
  validateCouplingHelper<U>(prop_name, c, _dmi_fe_problem.getNonlinearSystem().system(), missing);
  if (validate_aux)
    validateCouplingHelper<U>(prop_name, c, _dmi_fe_problem.getAuxiliarySystem().system(), missing);

  if (missing.size() > 0)
  {
    // join list of missing variable names
    std::string list = missing[0];
    for (unsigned int i = 1; i < missing.size(); ++i)
      list += ", " + missing[i];

    mooseWarning("Missing coupled variables {" << list << "} (add them to args parameter of " << this->name() << ")");
  }
}

template<class T>
std::vector<VariableName>
DerivativeMaterialInterface<T>::buildVariableVector(const VariableName & c1, const VariableName & c2, const VariableName & c3)
{
  std::vector<VariableName> c;
  if (c1 != "")
  {
    c.push_back(c1);
    if (c2 != "")
    {
      c.push_back(c2);
      if (c3 != "")
        c.push_back(c3);
    }
  }
  return c;
}

template<class T>
template<typename U>
void
DerivativeMaterialInterface<T>::validateCoupling(const MaterialPropertyName & base, const VariableName & c1, const VariableName & c2, const VariableName & c3)
{
  validateCoupling<U>(base, buildVariableVector(c1, c2, c3), true);
}

template<class T>
template<typename U>
void
DerivativeMaterialInterface<T>::validateNonlinearCoupling(const MaterialPropertyName & base, const VariableName & c1, const VariableName & c2, const VariableName & c3)
{
  validateCoupling<U>(base, buildVariableVector(c1, c2, c3), false);
}

template<class T>
inline bool
DerivativeMaterialInterface<T>::isNotKernelVariable(const VariableName & name)
{
  // try to cast this to a Kernel pointer
  KernelBase * k = dynamic_cast<KernelBase *>(this);

  // This interface is not templated on a class derived from Kernel
  if (k == NULL)
    return true;

  // We are templated on a kernel class, so we check if the kernel variable
  return k->variable().name() != name;
}

#endif //DERIVATIVEMATERIALINTERFACE_H
