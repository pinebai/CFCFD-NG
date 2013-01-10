-- Auto-generated by gasfile on: 25-Sep-2012 22:35:30
model = 'composite gas'
equation_of_state = 'perfect gas'
thermal_behaviour = 'thermal nonequilibrium'
mixing_rule = 'GuptaYos'
sound_speed = 'equilibrium'
diffusion_coefficients = 'GuptaYos'
min_massf = 1.000000e-15

thermal_modes = { 'transrotational', 'V_N2' }

transrotational = {}
transrotational.type = 'constant Cv'
transrotational.iT = 0
transrotational.components = { 'all-translation', 'all-rotation' }

V_N2 = {}
V_N2.type = 'variable Cv'
V_N2.iT = 1
V_N2.components = { 'all-vibration', 'all-electronic' }
V_N2.T_min = 20.000000
V_N2.T_max = 100000.000000
V_N2.iterative_method = 'NewtonRaphson'
V_N2.convergence_tolerance = 1.000000e-06
V_N2.max_iterations = 100

species = {'N2', }

N2 = {}
N2.species_type = "nonpolar diatomic"
N2.oscillator_type = "truncated harmonic"
N2.M = {
  value = 0.0280134,
  reference = "from CEA2::thermo.inp",
  description = "molecular mass",
  units = "kg/mol",
}
N2.s_0 = {
  value = 6839.91,
  reference = "NIST Chemistry WebBook: http://webbook.nist.gov/chemistry/",
  description = "Standard state entropy at 1 bar",
  units = "J/kg-K",
}
N2.h_f = {
  value = 0,
  reference = "from CEA2::thermo.inp",
  description = "Heat of formation",
  units = "J/kg",
}
N2.I = {
  value = 53661441,
  reference = "NIST Chemistry WebBook: http://webbook.nist.gov/chemistry/",
  description = "Ground state ionization energy",
  units = "J/kg",
}
N2.Z = {
  value = 0,
  reference = "NA",
  description = "Charge number",
  units = "ND",
}
N2.eps0 = {
  value = 9.85789812e-22,
  reference = "Svehla (1962) NASA Technical Report R-132",
  description = "Depth of the intermolecular potential minimum",
  units = "J",
}
N2.sigma = {
  value = 3.798e-10,
  reference = "Svehla (1962) NASA Technical Report R-132",
  description = "Hard sphere collision diameter",
  units = "m",
}
N2.r0 = {
  value = 4.2e-10,
  reference = "Thivet et al (1991) Phys. Fluids A 3 (11)",
  description = "Zero of the intermolecular potential",
  units = "m",
}
N2.r_eq = {
  value = 1.1e-10,
  reference = "See ilev_0 data below",
  description = "Equilibrium intermolecular distance",
  units = "m",
}
N2.f_m = {
  value = 1,
  reference = "Thivet et al (1991) Phys. Fluids A 3 (11)",
  description = "Mass factor = ( M ( Ma^2 + Mb^2 ) / ( 2 Ma Mb ( Ma + Mb ) )",
  units = "ND",
}
N2.mu = {
  value = 2.32587e-26,
  reference = "See molecular weight for N",
  description = "Reduced mass of constituent atoms",
  units = "kg/particle",
}
N2.alpha = {
  value = 1.09,
  reference = "Hirschfelder, Curtiss, and Bird (1954). Molecular theory of gases and liquids.",
  description = "Polarizability",
  units = "ND",
}
N2.mu_B = {
  value = 1.561317e-18,
  reference = "FIXME: Rowans mTg_input.dat",
  description = "Dipole moment",
  units = "Debye",
}
N2.electronic_levels = {
  n_levels = 5,
  ref = "Spradian07::diatom.dat",
  ilev_0 = {
    0,
    1.0977,
    1,
    78740,
    2358.569,
    14.3244,
    -0.002258,
    -0.00024,
    1.99824,
    0.01732,
    5.76e-06,
    0,
    0,
    0,
    1,
  },
  ilev_1 = {
    50203.66,
    1.2864,
    3,
    28980,
    1460.941,
    13.98,
    0.024,
    -0.00256,
    1.4539,
    0.0175,
    5.78e-06,
    0,
    0,
    0,
    3,
  },
  ilev_2 = {
    59619.09,
    1.2126,
    6,
    38660,
    1734.025,
    14.412,
    -0.0033,
    -0.00079,
    1.63772,
    0.01793,
    5.9e-06,
    0,
    42.24,
    1,
    3,
  },
  ilev_3 = {
    59808,
    1.27,
    6,
    38590,
    1501.4,
    11.6,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    2,
    3,
  },
  ilev_4 = {
    66272.5,
    1.2784,
    3,
    51340,
    1516.88,
    12.181,
    0.04186,
    0,
    1.4733,
    0.01666,
    5.56e-06,
    0,
    0,
    0,
    3,
  },
}
N2.CEA_coeffs = {
  {
    T_high = 1000,
    T_low = 200,
    coeffs = {
      22103.71497,
      -381.846182,
      6.08273836,
      -0.00853091441,
      1.384646189e-05,
      -9.62579362e-09,
      2.519705809e-12,
      710.846086,
      -10.76003744,
    },
  },
  {
    T_high = 6000,
    T_low = 1000,
    coeffs = {
      587712.406,
      -2239.249073,
      6.06694922,
      -0.00061396855,
      1.491806679e-07,
      -1.923105485e-11,
      1.061954386e-15,
      12832.10415,
      -15.86640027,
    },
  },
  {
    T_high = 20000,
    T_low = 6000,
    coeffs = {
      831013916,
      -642073.354,
      202.0264635,
      -0.03065092046,
      2.486903333e-06,
      -9.70595411e-11,
      1.437538881e-15,
      4938707.04,
      -1672.09974,
    },
  },
  ref = "from CEA2::thermo.inp",
}
N2.viscosity = {
  model = 'collision integrals'
}
N2.thermal_conductivity = {
  model = 'collision integrals'
}

collision_integrals = {
  {
    i = "N2",
    j = "N2",
    reference = "Wright et al, AIAA Journal Vol. 43 No. 12 December 2005",
    model = "GuptaYos curve fits",
    parameters = {
      {
        Pi_Omega_11 = {
          -0.0066,
          0.1392,
          -1.1559,
          6.9352,
        },
        T_high = 10000,
        T_low = 300,
        D = {
          0.0066,
          -0.1392,
          2.6559,
          -9.9442,
        },
        Pi_Omega_22 = {
          -0.0087,
          0.1948,
          -1.6023,
          8.1845,
        },
      },
    },
  },

}
