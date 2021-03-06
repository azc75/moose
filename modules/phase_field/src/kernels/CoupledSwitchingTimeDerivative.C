/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/
#include "CoupledSwitchingTimeDerivative.h"

template<>
InputParameters validParams<CoupledSwitchingTimeDerivative>()
{
  InputParameters params = validParams<CoupledTimeDerivative>();
  params.addClassDescription("Coupled time derivative Kernel that multiplies time derivative by (dh_a/deta_i * Fa + dh_b/deta_i * Fb + ..)");
  params.addRequiredParam<std::vector<MaterialPropertyName> >("Fj_names", "List of functions for each phase. Place in same order as hj_names!");
  params.addRequiredParam<std::vector<MaterialPropertyName> >("hj_names", "Switching Function Materials that provide h. Place in same order as Fj_names!");
  params.addCoupledVar("args", "Vector of arguments of Fj and hj");
  return params;
}

CoupledSwitchingTimeDerivative::CoupledSwitchingTimeDerivative(const InputParameters & parameters) :
    DerivativeMaterialInterface<JvarMapKernelInterface<CoupledTimeDerivative> >(parameters),
    _nvar(_coupled_moose_vars.size()), // number of coupled variables
    _v_name(getVar("v", 0)->name()),
    _Fj_names(getParam<std::vector<MaterialPropertyName> >("Fj_names")),
    _num_j(_Fj_names.size()),
    _prop_Fj(_num_j),
    _prop_dFjdv(_num_j),
    _prop_dFjdarg(_num_j),
    _hj_names(getParam<std::vector<MaterialPropertyName> >("hj_names")),
    _prop_dhjdetai(_num_j),
    _prop_d2hjdetai2(_num_j),
    _prop_d2hjdetaidarg(_num_j)
{
  // check passed in parameter vectors
  if (_num_j != _hj_names.size())
    mooseError("Need to pass in as many hj_names as Fj_names in CoupledSwitchingTimeDerivative ", name());

  // reserve space and set phase material properties
  for (unsigned int n = 0; n < _num_j; ++n)
  {
    // get phase free energy and derivatives
    _prop_Fj[n] = &getMaterialPropertyByName<Real>(_Fj_names[n]);
    _prop_dFjdv[n] = &getMaterialPropertyDerivative<Real>(_Fj_names[n], _var.name());
    _prop_dFjdarg[n].resize(_nvar);

    // get switching function and derivatives wrt eta_i, the nonlinear variable
    _prop_dhjdetai[n] = &getMaterialPropertyDerivative<Real>(_hj_names[n], _v_name);
    _prop_d2hjdetai2[n] = &getMaterialPropertyDerivative<Real>(_hj_names[n], _v_name, _v_name);
    _prop_d2hjdetaidarg[n].resize(_nvar);

    for (unsigned int i = 0; i < _nvar; ++i)
    {
      MooseVariable *cvar = _coupled_moose_vars[i];
      // Get derivatives of all Fj wrt all coupled variables
      _prop_dFjdarg[n][i] = &getMaterialPropertyDerivative<Real>(_Fj_names[n], cvar->name());

      // Get second derivatives of all hj wrt eta_i and all coupled variables
      _prop_d2hjdetaidarg[n][i] = &getMaterialPropertyDerivative<Real>(_hj_names[n], _v_name, cvar->name());
    }
  }
}

void
CoupledSwitchingTimeDerivative::initialSetup()
{
  for (unsigned int n = 0; n < _num_j; ++n)
  {
    validateNonlinearCoupling<Real>(_Fj_names[n]);
    validateNonlinearCoupling<Real>(_hj_names[n]);
  }
}

Real
CoupledSwitchingTimeDerivative::computeQpResidual()
{
  Real sum = 0.0;
  for (unsigned int n = 0; n < _num_j; ++n)
    sum += (*_prop_dhjdetai[n])[_qp] * (*_prop_Fj[n])[_qp];

  return CoupledTimeDerivative::computeQpResidual() * sum;
}

Real
CoupledSwitchingTimeDerivative::computeQpJacobian()
{
  Real sum = 0.0;
  for (unsigned int n = 0; n < _num_j; ++n)
    sum += (*_prop_dhjdetai[n])[_qp] * (*_prop_dFjdv[n])[_qp];

  return CoupledTimeDerivative::computeQpResidual() * sum * _phi[_j][_qp];
}

Real
CoupledSwitchingTimeDerivative::computeQpOffDiagJacobian(unsigned int jvar)
{
  // get the coupled variable jvar is referring to
  const unsigned int cvar = mapJvarToCvar(jvar);

  if (jvar == _v_var)
  {
    Real sum = 0.0;

    for (unsigned int n = 0; n < _num_j; ++n)
      sum += ((*_prop_d2hjdetai2[n])[_qp] * _v_dot[_qp]
              + (*_prop_dhjdetai[n])[_qp] * _dv_dot[_qp]) * (*_prop_Fj[n])[_qp];

    return _phi[_j][_qp] * sum * _test[_i][_qp];
  }

  Real sum = 0.0;
  for (unsigned int n = 0; n < _num_j; ++n)
    sum += (*_prop_d2hjdetaidarg[n][cvar])[_qp] * (*_prop_Fj[n])[_qp]
           + (*_prop_dhjdetai[n])[_qp] * (*_prop_dFjdarg[n][cvar])[_qp];

  return CoupledTimeDerivative::computeQpResidual() * sum * _phi[_j][_qp];
}
