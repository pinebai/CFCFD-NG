-- Auto-generated by gasfile on: 28-Dec-2010 17:21:16
model = 'composite gas'
equation_of_state = 'perfect gas'
thermal_behaviour = 'constant specific heats'
mixing_rule = 'Wilke'
sound_speed = 'equilibrium'
diffusion_coefficients = 'hard sphere'
ignore_mole_fraction = 1.0e-15
species = {'O2', 'N2', }

O2 = {}
O2.M = {
  value = 0.031999,
  reference = "from CEA2::thermo.inp",
  description = "molecular mass",
  units = "kg/mol",
}
O2.gamma = {
  value = 1.4,
  reference = "diatomic molecule at low temperatures, gamma = 7/5",
  description = "(ideal) ratio of specific heats at room temperature",
  units = "non-dimensional",
}
O2.d = {
  value = 3.433e-10,
  reference = "Bird, Stewart and Lightfoot (2001), p. 864",
  description = "equivalent hard sphere diameter, based on L-J parameters",
  units = "m",
}
O2.e_zero = {
  value = 0,
  description = "reference energy",
  units = "J/kg",
}
O2.q = {
  value = 0,
  description = "heat release",
  units = "J/kg",
}
O2.viscosity = {
  parameters = {
    T_ref = 273,
    ref = "Table 1-2, White (2006)",
    S = 139,
    mu_ref = 1.919e-05,
  },
  model = "Sutherland",
}
O2.thermal_conductivity = {
  parameters = {
    S = 240,
    ref = "Table 1-3, White (2006)",
    k_ref = 0.0244,
    T_ref = 273,
  },
  model = "Sutherland",
}
N2 = {}
N2.M = {
  value = 0.028013,
  reference = "from CEA2::thermo.inp",
  description = "molecular mass",
  units = "kg/mol",
}
N2.gamma = {
  value = 1.4,
  reference = "diatomic molecule at low temperatures, gamma = 7/5",
  description = "(ideal) ratio of specific heats at room temperature",
  units = "non-dimensional",
}
N2.d = {
  value = 3.667e-10,
  reference = "Bird, Stewart and Lightfoot (2001), p. 864",
  description = "equivalent hard sphere diameter, based on L-J parameters",
  units = "m",
}
N2.e_zero = {
  value = 0,
  description = "reference energy",
  units = "J/kg",
}
N2.q = {
  value = 0,
  description = "heat release",
  units = "J/kg",
}
N2.viscosity = {
  parameters = {
    T_ref = 273,
    ref = "Table 1-2, White (2006)",
    S = 107,
    mu_ref = 1.663e-05,
  },
  model = "Sutherland",
}
N2.thermal_conductivity = {
  parameters = {
    S = 150,
    ref = "Table 1-3, White (2006)",
    k_ref = 0.0242,
    T_ref = 273,
  },
  model = "Sutherland",
}
