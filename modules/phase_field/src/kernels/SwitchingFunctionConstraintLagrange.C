#include "SwitchingFunctionConstraintLagrange.h"

template<>
InputParameters validParams<SwitchingFunctionConstraintLagrange>()
{
  InputParameters params = validParams<Kernel>();
  params.addClassDescription("Lagrange multiplier kernel to constrain the sum of all switching functions in a multiphase system. This kernel acts on the lagrange multiplier variable.");
  params.addParam<std::vector<std::string> >("h_names", "Switching Function Materials that provide h(eta_i)");
  params.addRequiredCoupledVar("etas", "eta_i order parameters, one for each h");
  return params;
}

SwitchingFunctionConstraintLagrange::SwitchingFunctionConstraintLagrange(const std::string & name, InputParameters parameters) :
    DerivativeMaterialInterface<Kernel>(name, parameters),
    _h_names(getParam<std::vector<std::string> >("h_names")),
    _num_h(_h_names.size()),
    _h(_num_h),
    _dh(_num_h)
{
  // parameter check. We need exactly one eta per h
  if (_num_h != coupledComponents("etas"))
    mooseError("Need to pass in as many h_names as etas in SwitchingFunctionConstraintLagrange kernel " << name);

  // fetch switching functions (for the residual) and h derivatives (for the Jacobian)
  for (unsigned int i = 0; i < _num_h; ++i)
  {
    _h[i] = &getMaterialProperty<Real>(_h_names[i]);
    _dh[i] = &getMaterialPropertyDerivative<Real>(_h_names[i], getVar("etas", i)->name());

    // generate the lookup table from j_var -> eta index
    _j_eta[coupled("etas", i)] = i;
  }
}

Real
SwitchingFunctionConstraintLagrange::computeQpResidual()
{
  Real g = -1.0;
  for (unsigned int i = 0; i < _num_h; ++i)
    g += (*_h[i])[_qp];

  return _test[_i][_qp] * g;
}

Real
SwitchingFunctionConstraintLagrange::computeQpJacobian()
{
  return 0.0;
}

Real
SwitchingFunctionConstraintLagrange::computeQpOffDiagJacobian(unsigned int j_var)
{
  const LIBMESH_BEST_UNORDERED_MAP<unsigned int, unsigned int>::iterator eta = _j_eta.find(j_var);

  if (eta != _j_eta.end())
    return (*_dh[eta->second])[_qp] * _phi[_j][_qp] * _test[_i][_qp];
  else
    return 0.0;
}