-- Collater: Rowan J. Gollan
-- Date: 30-Mar-2009

NO = {}
NO.M = {
   value = 30.0061e-3,
   units = 'kg/mol',
   description = 'molecular mass',
   reference = 'CEA2::thermo.inp'
}
NO.atomic_constituents = {N=1,O=1}
NO.charge = 0
NO.gamma = {
   value = 1.386,
   units = 'non-dimensional',
   description = 'ratio of specific heats at room temperature (= Cp/(Cp - R))',
   reference = 'using Cp evaluated from CEA2 coefficients at T=300.0 K'
}
NO.viscosity = {
   model = "CEA",
   parameters = {
      {T_low=200.0, T_high=1000.0, A=0.60262029e+00, B=-0.62017783e+02, C=-0.13954524e+03, D=0.20268332e+01},
      {T_low=1000.0, T_high=5000.0, A=0.78009050e+00, B=0.30486891e+03, C=-0.94847722e+05, D=0.52873381e+00},
      {T_low=5000.0, T_high=15000.0, A=0.80580582e+00, B=0.62427878e+03, C=-0.57879210e+06, D=0.26516450e+00},
      ref = 'from CEA2::trans.inp which cites Bousheri et al. (1987), Svehla (1994)'
   }
}
NO.thermal_conductivity = {
   model = "CEA",
   parameters = {
      {T_low=200.0, T_high=1000.0, A=0.95028758e+00, B=0.76667058e+02, C=-0.99894764e+04, D=-0.62776717e-02},
      {T_low=1000.0, T_high=5000.0, A=0.86215238e+00, B=0.44568223e+03, C=-0.23856466e+06, D=0.46209876e+00},
      {T_low=5000.0, T_high=15000.0, A=-0.10377865e+01, B=-0.34486864e+05, C=0.67451187e+08, D=0.20928749e+02},
      ref = 'from CEA2::trans.inp which cites Bousheri et al (1987), Svehla (1994)'
   }
}
NO.CEA_coeffs = {
  { T_low  = 200.0,
    T_high = 1000.0,
    coeffs = { -1.143916503e+04,  1.536467592e+02,  3.431468730e+00,
	       -2.668592368e-03,  8.481399120e-06, -7.685111050e-09,
                2.386797655e-12,  9.098214410e+03,  6.728725490e+00
    }
  },
  { T_low  = 1000.0,
    T_high = 6000.0,
    coeffs = {  2.239018716e+05, -1.289651623e+03,  5.433936030e+00,
               -3.656034900e-04,  9.880966450e-08, -1.416076856e-11,
                9.380184620e-16,  1.750317656e+04, -8.501669090e+00
    }
  },
  { T_low  = 6000.0,
    T_high = 20000.0,
    coeffs = { -9.575303540e+08,  5.912434480e+05, -1.384566826e+02,
	        1.694339403e-02, -1.007351096e-06,  2.912584076e-11,
               -3.295109350e-16, -4.677501240e+06,  1.242081216e+03
    }
  },
  ref="from CEA2::thermo.inp"
}
NO.T_c = {
   value = 180.00,
   units = 'K',
   description = 'critical temperature',
   reference = 'Poling, B.E. et al. (2001). The Properties of Gases and Liquids. Section A, p.A.5'
}
NO.p_c = {
   value = 64.80e+05,
   units = 'Pa',
   description = 'critical pressure',
   reference = 'Poling, B.E. et al. (2001). The Properties of Gases and Liquids. Section A, p.A.5'
}

-- Nonequilibrium data

NO.species_type = "polar diatomic"
NO.eps0 = {
   value = 1.611227886e-21,
   units = 'J',
   description = 'Depth of the intermolecular potential minimum',
   reference = 'Svehla (1962) NASA Technical Report R-132'
}
NO.sigma = {
   value = 3.492e-10,
   units = 'm',
   description = 'Hard sphere collision diameter',
   reference = 'Svehla (1962) NASA Technical Report R-132'
}
NO.r0 = {
   value = 2.020e-10,
   units = 'm',
   description = 'Zero of the intermolecular potential',
   reference = 'FIXME: Rowans mTg_input.dat.'
}
NO.r_eq = {
   value = 1.15e-10,
   units = 'm',
   description = 'Equilibrium intermolecular distance',
   reference = 'See ilev_0 data below'
}
NO.f_m = {
   value = 0.99115,
   units = 'ND',
   description = 'Mass factor = ( M ( Ma^2 + Mb^2 ) / ( 2 Ma Mb ( Ma + Mb ) )',
   reference = 'Thivet et al (1991) Phys. Fluids A 3 (11)'
}
NO.mu = {
   value = 2.480328e-26,
   units = 'kg/particle',
   description = 'Reduced mass of constituent atoms',
   reference = 'See molecular weight for O and N'
}
NO.alpha = {
   value = 1.09,
   units = 'ND',
   description = 'Polarizability',
   reference = 'FIXME: Rowans mTg_input.dat'
}
NO.mu_B = {
   value = 1.041131e-18,
   units = 'Debye',
   description = 'Dipole moment',
   reference = 'FIXME: Rowans mTg_input.dat'
}
NO.s_0 = {
   value = 7023.91,
   units = 'J/kg-K',
   description = 'Standard state entropy at 1 bar',
   reference = 'NIST Chemistry WebBook: http://webbook.nist.gov/chemistry/'
}
NO.h_f = {
   value = 3.041758509e+6,
   units = 'J/kg',
   description = 'Heat of formation',
   reference = 'from CEA2::thermo.inp'
}
NO.I = {
   value = 2.9789815e+7,
   units = 'J/kg',
   description = 'Ground state ionization energy',
   reference = 'NIST Chemistry WebBook: http://webbook.nist.gov/chemistry/'
}
NO.Z = {
   value = 0,
   units = 'ND',
   description = 'Charge number',
   reference = 'NA'
}
NO.NIST_electronic_levels = {
   n_levels = 22,
   ref = 'NIST Chemistry WebBook: http://webbook.nist.gov/chemistry/ (with spradian07::diatom.dat data for some parameters)',
   -- ===========================================================================================================================================================
   --    n      Te         re       g   dzero      we         wexe      weye        weze        be        alphae      de          betae       spn-orb     l   s  
   -- ===========================================================================================================================================================
   ilev_0  = {    60.55,  1.1508,  4,  52335.00,  1904.135,  14.0884,  1.005E-02, -1.533E-04,  1.69500,  1.750E-02,  5.385E-06,  0.000E+00,  1.233E+02,  1,  2, },
   ilev_1  = { 38440.00,  1.3200,  8,  14420.00,  1017.000,  11.0000,  0.000E+00,  0.000E+00,  1.00000,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  1,  4, },
   ilev_2  = { 43965.70,  1.0634,  2,  27597.00,  2374.310,  16.1060, -0.465E-01,  0.000E+00,  1.99650,  0.192E-01,  0.540E-05,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_3  = { 45932.30,  1.4167,  4,  27408.00,  1042.400,   7.7726,  1.160E-01, -3.958E-03,  1.12440,  1.343E-02,  4.900E-06,  0.000E+00,  3.132E+01,  1,  2, },
   ilev_4  = { 48680.00,  1.3000,  4,  24660.00,  1206.000,  15.0000,  0.000E+00,  0.000E+00,  1.00000,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  0,  4, },
   ilev_5  = { 52175.70,  1.0584,  4,  25060.00,  2381.300,  15.7020,  0.000E+00,  0.000E+00,  2.01550,  3.244E-02,  0.000E+00,  0.000E+00,  3.000E+00,  1,  2, },
   ilev_6  = { 53084.70,  1.0618,  2,  16616.00,  2323.900,  22.8850,  7.500E-01, -2.200E-01,  2.00260,  2.175E-02,  5.800E-06,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_7  = { 60364.20,  1.3020,  4,  11604.00,  1217.400,  15.6100,  0.000E+00,  0.000E+00,  1.33200,  2.100E-02,  0.000E+00,  0.000E+00, -2.200E+00,  2,  2, },
   ilev_8  = { 60628.80,  1.0662,  2,  10760.60,  2375.300,  16.4300,  0.000E+00,  0.000E+00,  1.98630,  1.820E-02,  5.600E-06,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_9  = { 61800.00,  1.0670,  4,   9600.00,  2394.000,  20.0000,  0.000E+00,  0.000E+00,  1.98200,  2.300E-02,  0.000E+00,  0.000E+00,  0.000E+00,  2,  2, },
   ilev_10 = { 62473.40,  1.0617,  2,  24830.00,  2339.400,   0.0000,  0.000E+00,  0.000E+00,  2.00300,  1.800E-02,  0.000E+00,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_11 = { 62485.40,  1.0585,  4,   8930.00,  2371.300,  16.1700,  0.000E+00,  0.000E+00,  2.01500,  2.100E-02,  0.000E+00,  0.000E+00,  9.600E-01,  1,  2, },
   ilev_12 = { 62913.00,  1.3427,  2,  10000.00,  1085.540,  11.0830, -1.439E-01,  0.000E+00,  1.25230,  2.040E-02,  0.000E+00,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_13 = { 64437.00,  0.0000,  2,  20000.00,  2352.000,  19.5000,  0.000E+00,  0.000E+00,  2.02200,  1.800E-02,  0.000E+00,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_14 = { 66900.00,  0.0000,  2,  20000.00,  2378.000,  16.5000,  0.000E+00,  0.000E+00,  1.98000,  2.000E-02,  0.000E+00,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_15 = { 67374.00,  0.0000,  4,  20000.00,  2375.000,  15.0000,  0.000E+00,  0.000E+00,  1.96900,  2.600E-02,  0.000E+00,  0.000E+00,  0.000E+00,  2,  2, },
   ilev_16 = { 67757.00,  0.0000,  2,  20000.00,  2371.000,  16.0000,  0.000E+00,  0.000E+00,  1.99000,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_17 = { 69728.00,  0.0000,  2,  20000.00,  2372.000,  15.7000,  0.000E+00,  0.000E+00,  1.00000,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_18 = { 69977.00,  0.0000,  4,  20000.00,  2371.000,  16.0000,  0.000E+00,  0.000E+00,  1.00000,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  2,  2, },
   ilev_19 = { 70512.00,  0.0000,  4,  20000.00,  2375.000,  15.0000,  0.000E+00,  0.000E+00,  1.00000,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  1,  2, },
   ilev_20 = { 70614.00,  0.0000,  2,  20000.00,  2370.000,  15.0000,  0.000E+00,  0.000E+00,  2.11000,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_21 = { 71224.00,  0.0000,  2,  20000.00,  2377.000,  16.4000,  0.000E+00,  0.000E+00,  1.93800,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  0,  2, }
   -- ===========================================================================================================================================================
}
NO.electronic_levels = {
   -- n_levels = 12,
   n_levels = 5,
   ref = 'Spradian07::diatom.dat',
   -- ===========================================================================================================================================================
   --    n      Te         re       g   dzero      we         wexe      weye        weze        be        alphae      de          betae       spn-orb     l   s  
   -- ===========================================================================================================================================================
   ilev_0  = {    60.55,  1.1508,  4,  52335.00,  1904.135,  14.0884,  1.005E-02, -1.533E-04,  1.69500,  1.750E-02,  5.385E-06,  0.000E+00,  1.233E+02,  1,  2, },
   ilev_1  = { 38440.00,  1.3200,  8,  14420.00,  1017.000,  11.0000,  0.000E+00,  0.000E+00,  1.00000,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  1,  4, },
   ilev_2  = { 43965.70,  1.0634,  2,  27597.00,  2374.310,  16.1060, -0.465E-01,  0.000E+00,  1.99650,  0.192E-01,  0.540E-05,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_3  = { 45932.30,  1.4167,  4,  27408.00,  1042.400,   7.7726,  1.160E-01, -3.958E-03,  1.12440,  1.343E-02,  4.900E-06,  0.000E+00,  3.132E+01,  1,  2, },
   ilev_4  = { 48680.00,  1.3000,  4,  24660.00,  1206.000,  15.0000,  0.000E+00,  0.000E+00,  1.00000,  0.000E+00,  0.000E+00,  0.000E+00,  0.000E+00,  0,  4, },
   ilev_5  = { 52175.70,  1.0584,  4,  25060.00,  2381.300,  15.7020,  0.000E+00,  0.000E+00,  2.01550,  3.244E-02,  0.000E+00,  0.000E+00,  3.000E+00,  1,  2, },
   ilev_6  = { 53084.70,  1.0618,  2,  16616.00,  2323.900,  22.8850,  7.500E-01, -2.200E-01,  2.00260,  2.175E-02,  5.800E-06,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_7  = { 60364.20,  1.3020,  4,  11604.00,  1217.400,  15.6100,  0.000E+00,  0.000E+00,  1.33200,  2.100E-02,  0.000E+00,  0.000E+00, -2.200E+00,  2,  2, },
   ilev_8  = { 60628.80,  1.0662,  2,  10760.60,  2375.300,  16.4300,  0.000E+00,  0.000E+00,  1.98630,  1.820E-02,  5.600E-06,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_9  = { 61800.00,  1.0670,  4,   9600.00,  2394.000,  20.0000,  0.000E+00,  0.000E+00,  1.98200,  2.300E-02,  0.000E+00,  0.000E+00,  0.000E+00,  2,  2, },
   ilev_10 = { 62473.40,  1.0617,  2,  24830.00,  2339.400,   0.0000,  0.000E+00,  0.000E+00,  2.00300,  1.800E-02,  0.000E+00,  0.000E+00,  0.000E+00,  0,  2, },
   ilev_11 = { 62485.40,  1.0585,  4,   8930.00,  2371.300,  16.1700,  0.000E+00,  0.000E+00,  2.01500,  2.100E-02,  0.000E+00,  0.000E+00,  9.600E-01,  1,  2, },
   -- ===========================================================================================================================================================
}
  
