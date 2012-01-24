-- Collater: Brendan T. O'Flaherty
-- Date: 07 Aug 2009

CO = {}
CO.M = {
   value = 28.010100e-3,
   units = 'kg/mol',
   description = 'molecular mass',
   reference = 'Periodic table'
}
CO.gamma = {
   value = 1.3992e+00,
   units = 'non-dimensional',
   description = 'ratio of specific heats at 300.0K',
   reference = 'evaluated using Cp/R from Chemkin-II coefficients'
}
-- CO.CEA_coeffs = {
--    { T_low  = 200.0,
--      T_high = 1000.0,
--      coeffs = {0.000000000e+00, 0.000000000e+00, 3.579533470e+00, -6.103536800e-04, 1.016814330e-06, 9.070058840e-10, -9.044244990e-13, -1.434408600e+04, 3.508409280e+00, }
--    },
--    { T_low  = 1000.0,
--      T_high = 3500.0,
--      coeffs = {0.000000000e+00, 0.000000000e+00, 2.715185610e+00, 2.062527430e-03, -9.988257710e-07, 2.300530080e-10, -2.036477160e-14, -1.415187240e+04, 7.818687720e+00, }
--    },
--    ref='The Chemkin Thermodynamic Data Base, Kee R.J. et al (1993)'
-- }
CO.CEA_coeffs = {
   { T_low  = 200.0,
     T_high = 1000.0,
     coeffs = { 1.489045326e+04, -2.922285939e+02,  5.724527170e+00,
     	       -8.176235030e-03,  1.456903469e-05, -1.087746302e-08,
                3.027941827e-12, -1.303131878e+04, -7.859241350e+00 }
   },
   { T_low  = 1000.0,
     T_high = 6000.0,
     coeffs = { 4.619197250e+05, -1.944704863e+03,  5.916714180e+00,
     	       -5.664282830e-04,  1.398814540e-07, -1.787680361e-11,
                9.620935570e-16, -2.466261084e+03, -1.387413108e+01 }
   },
   { T_low  = 6000.0,
     T_high = 20000.0,
     coeffs = { 8.868662960e+08, -7.500377840e+05,  2.495474979e+02,
     	       -3.956351100e-02,  3.297772080e-06, -1.318409933e-10,
                1.998937948e-15,  5.701421130e+06, -2.060704786e+03 }
   },
  ref="Gurvich (1979) from CEA2::thermo.inp"
}
CO.T_c = {
   value = 132.85,
   units = 'K',
   description = 'critical temperature',
   reference = 'Poling, B.E. et al. (2001). The Properties of Gases and Liquids. Section A, p.A.5'
}
CO.p_c = {
   value = 34.94e+05,
   units = 'Pa',
   description = 'critical pressure',
   reference = 'Poling, B.E. et al. (2001). The Properties of Gases and Liquids. Section A, p.A.5'
}

-- Nonequilibrium data

CO.species_type = "polar diatomic"
CO.eps0 = {
   value = 1.266063386e-21,
   units = 'J',
   description = 'Depth of the intermolecular potential minimum',
   reference = 'Svehla (1962) NASA Technical Report R-132'
}
CO.sigma = {
   value = 3.690e-10,
   units = 'm',
   description = 'Hard sphere collision diameter',
   reference = 'Svehla (1962) NASA Technical Report R-132'
}
CO.s_0 = {
   value = 7056.74,
   units = 'J/kg-K',
   description = 'Standard state entropy at 1 bar',
   reference = 'NIST Chemistry WebBook: http://webbook.nist.gov/chemistry/'
}
CO.h_f = {
   value = -3946262.10,
   units = 'J/kg',
   description = 'Heat of formation',
   reference = 'from CEA2::thermo.inp'
}
CO.I = {
   value = 48274173.39,
   units = 'J/kg',
   description = 'Ground state ionization energy',
   reference = 'NIST Chemistry WebBook: http://webbook.nist.gov/chemistry/'
}
CO.Z = {
   value = 0,
   units = 'ND',
   description = 'Charge number',
   reference = 'NA'
}
CO.electronic_levels = {
   -- n_levels = 14,
   n_levels = 5,
   ref = 'Spradian07::diatom.dat',
   -- ===========================================================================================================================================================
   --   n       Te         re       g   dzero      we         wexe      weye        weze        be        alphae      de          betae       spn-orb     l   s  
   -- ===========================================================================================================================================================
   ilev_0  = {      0.00,  1.1283,  1,  89490.00,  2169.814,  13.2883,  1.051E-02,  0.000E+00,  1.93128,  1.750E-02,  6.121E-06, -1.153E-09,  0.000E+00,  0,  1 },
   ilev_1  = {  48686.70,  1.2057,  6,  41020.00,  1743.410,  14.3600, -4.500E-02,  0.000E+00,  1.69124,  1.904E-02,  6.360E-06,  4.000E-08,  4.153E+01,  1,  3 },
   ilev_2  = {  55825.49,  1.3523,  3,  34140.00,  1228.600,  10.4680,  9.100E-03,  0.000E+00,  1.34460,  1.892E-02,  6.410E-06,  0.000E+00,  0.000E+00,  0,  3 },
   ilev_3  = {  61120.10,  1.3696,  6,  28870.00,  1171.940,  10.6350,  7.850E-02,  0.000E+00,  1.31080,  1.782E-02,  6.590E-06,  0.000E+00, -1.600E+01,  1,  3 },
   ilev_4  = {  64230.24,  1.3840,  3,  25790.00,  1117.720,  10.6860,  1.174E-01,  0.000E+00,  1.28360,  1.753E-02,  6.770E-06,  0.000E+00,  0.000E+00,  0,  3 },
   ilev_5  = {  65075.77,  1.2353,  2,  24740.00,  1518.240,  19.4000,  7.660E-01,  0.000E+00,  1.61150,  2.325E-02,  7.330E-06,  1.000E-07,  0.000E+00,  1,  1 },
   ilev_6  = {  65084.40,  1.3911,  1,  24940.00,  1092.220,  10.7040,  5.540E-02,  0.000E+00,  1.27050,  1.848E-02,  9.000E-06,  0.000E+00,  0.000E+00,  0,  1 },
   ilev_7  = {  65928.00,  1.3990,  2,  24100.00,  1094.000,  10.2000,  0.000E+00,  0.000E+00,  1.25700,  1.700E-02,  0.000E+00,  0.000E+00,  0.000E+00,  2,  1 },
   ilev_8  = {  83814.00,  1.1130,  3,  25000.00,  2199.300,   0.0000,  0.000E+00,  0.000E+00,  1.98600,  4.200E-02,  0.000E+00,  0.000E+00,  0.000E+00,  0,  3 },
   ilev_9  = {  86945.20,  1.1197,  1,  50000.00,  2112.700,  15.2200,  0.000E+00,  0.000E+00,  1.96120,  2.610E-02,  7.100E-06,  0.000E+00,  0.000E+00,  0,  1 },
   ilev_10 = {  91970.00,  1.1070,  1,  50000.00,  2175.900,  14.7600,  0.000E+00,  0.000E+00,  1.95330,  1.960E-02,  5.700E-06,  0.000E+00,  0.000E+00,  0,  1 },
   ilev_11 = {  92930.00,  1.1188,  2,  50000.00,  2134.900,  14.7600,  0.000E+00,  0.000E+00,  1.96440,  0.000E+00,  6.500E-06,  0.000E+00,  0.000E+00,  1,  1 },
   ilev_12 = {  99730.00,  1.1500,  2,  35000.00,  2112.000, 198.0000,  1.000E-01,  0.000E+00,  1.87150,  2.300E-02,  6.500E-06,  0.000E+00,  0.000E+00,  0,  1 },
   ilev_13 = { 105750.00,  1.1190,  2,  35000.00,  1097.000,  11.0000,  0.000E+00,  0.000E+00,  1.96300,  0.000E+00,  7.000E-06,  0.000E+00,  0.000E+00,  1,  1 },
   -- ===========================================================================================================================================================
}
