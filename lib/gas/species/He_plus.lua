-- Author: Daniel F. Potter
-- Date: 23-Feb-2010

He_plus = {}
He_plus.M = {
   value = 4.0020534e-3,
   units = 'kg/mol',
   description = 'molecular mass',
   reference = 'cea2::thermo.inp'
}
He_plus.gamma = {
   value = 5/3,
   units = 'non-dimensional',
   description = '(ideal) ratio of specific heats at room temperature',
   reference = 'monatomic gas'
}
He_plus.CEA_coeffs = {
   { T_low  = 298.150,
     T_high = 1000.0,
     coeffs = { 0.000000000e+00,  0.000000000e+00,  2.500000000e+00,
     	        0.000000000e+00,  0.000000000e+00,  0.000000000e+00,
     	        0.000000000e+00,  2.853233739e+05,  1.621665557e+00
	      }
   },
   { T_low  = 1000.0,
     T_high = 6000.0,
     coeffs = { 0.000000000e+00,  0.000000000e+00,  2.500000000e+00,
     	        0.000000000e+00,  0.000000000e+00,  0.000000000e+00,
     	        0.000000000e+00,  2.853233739e+05,  1.621665557e+00
	      }
   },
   { T_low  = 6000.0,
     T_high = 20000.0,
     coeffs = { 0.000000000e+00,  0.000000000e+00,  2.500000000e+00,
     	        0.000000000e+00,  0.000000000e+00,  0.000000000e+00,
     	        0.000000000e+00,  2.853233739e+05,  1.621665557e+00
	      }
   },
   ref='cea2::thermo.inp'
}

-- Thermal nonequilibrium data

He_plus.species_type = "monatomic"
He_plus.s_0 = {
   value = 32960.58,
   units = 'J/kg-K',
   description = 'Standard state entropy at 1 bar',
   reference = 'NIST Chemistry WebBook: http://webbook.nist.gov/chemistry/'
}
He_plus.h_f = {
   value = 594325271.37,
   units = 'J/kg',
   description = 'Heat of formation',
   reference = 'from CEA2::thermo.inp'
}
He_plus.I = {
   value = 0.0,
   units = 'J/kg',
   description = 'Dummy ground state ionization energy',
   reference = 'NA'
}
He_plus.Z = {
   value = 1,
   units = 'ND',
   description = 'Charge number',
   reference = 'NA'
}
He_plus.electronic_levels = {
   -- n_levels = 153,
   n_levels = 2,
   ref = 'NIST ASD: http://physics.nist.gov/PhysRefData/ASD/index.html',
   -- ===========================================================
   --   No.    n       E(cm-1)     g     l     L     S     parity 
   -- ===========================================================
   ilev_0   =  { 1,        0.00,     2,    0,    0,    1,    2 },
   ilev_1   =  { 2,   329179.77,     2,    0,    0,    1,    2 },
   ilev_2   =  { 2,   329183.20,     6,    1,    1,    1,    1 },
   ilev_3   =  { 3,   390140.97,     2,    0,    0,    1,    2 },
   ilev_4   =  { 3,   390141.99,     6,    1,    1,    1,    1 },
   ilev_5   =  { 3,   390142.91,    10,    2,    2,    1,    2 },
   ilev_6   =  { 4,   411477.19,     2,    0,    0,    1,    2 },
   ilev_7   =  { 4,   411477.62,     6,    1,    1,    1,    1 },
   ilev_8   =  { 4,   411478.01,    10,    2,    2,    1,    2 },
   ilev_9   =  { 4,   411478.17,    14,    3,    3,    1,    1 },
   ilev_10  =  { 5,   421352.72,     2,    0,    0,    1,    2 },
   ilev_11  =  { 5,   421352.94,     6,    1,    1,    1,    1 },
   ilev_12  =  { 5,   421353.14,    10,    2,    2,    1,    2 },
   ilev_13  =  { 5,   421353.22,    14,    3,    3,    1,    1 },
   ilev_14  =  { 5,   421353.27,    18,   -1,   -1,    1,    2 },
   ilev_15  =  { 6,   426717.16,     2,    0,    0,    1,    2 },
   ilev_16  =  { 6,   426717.29,     6,    1,    1,    1,    1 },
   ilev_17  =  { 6,   426717.40,    10,    2,    2,    1,    2 },
   ilev_18  =  { 6,   426717.45,    14,    3,    3,    1,    1 },
   ilev_19  =  { 6,   426717.48,    18,   -1,   -1,    1,    2 },
   ilev_20  =  { 6,   426717.50,    22,   -1,   -1,    1,    1 },
   ilev_21  =  { 7,   429951.73,     2,    0,    0,    1,    2 },
   ilev_22  =  { 7,   429951.81,     6,    1,    1,    1,    1 },
   ilev_23  =  { 7,   429951.89,    10,    2,    2,    1,    2 },
   ilev_24  =  { 7,   429951.92,    14,    3,    3,    1,    1 },
   ilev_25  =  { 7,   429951.93,    18,   -1,   -1,    1,    2 },
   ilev_26  =  { 7,   429951.95,    22,   -1,   -1,    1,    1 },
   ilev_27  =  { 7,   429951.95,    26,   -1,   -1,    1,    2 },
   ilev_28  =  { 8,   432051.09,     2,    0,    0,    1,    2 },
   ilev_29  =  { 8,   432051.14,     6,    1,    1,    1,    1 },
   ilev_30  =  { 8,   432051.19,    10,    2,    2,    1,    2 },
   ilev_31  =  { 8,   432051.21,    14,    3,    3,    1,    1 },
   ilev_32  =  { 8,   432051.22,    18,   -1,   -1,    1,    2 },
   ilev_33  =  { 8,   432051.23,    22,   -1,   -1,    1,    1 },
   ilev_34  =  { 8,   432051.23,    26,   -1,   -1,    1,    2 },
   ilev_35  =  { 8,   432051.24,    30,   -1,   -1,    1,    1 },
   ilev_36  =  { 9,   433490.39,     2,    0,    0,    1,    2 },
   ilev_37  =  { 9,   433490.43,     6,    1,    1,    1,    1 },
   ilev_38  =  { 9,   433490.46,    10,    2,    2,    1,    2 },
   ilev_39  =  { 9,   433490.48,    14,    3,    3,    1,    1 },
   ilev_40  =  { 9,   433490.48,    18,   -1,   -1,    1,    2 },
   ilev_41  =  { 9,   433490.49,    22,   -1,   -1,    1,    1 },
   ilev_42  =  { 9,   433490.49,    26,   -1,   -1,    1,    2 },
   ilev_43  =  { 9,   433490.50,    30,   -1,   -1,    1,    1 },
   ilev_44  =  { 9,   433490.50,    34,   -1,   -1,    1,    2 },
   ilev_45  =  {10,   434519.91,     2,    0,    0,    1,    2 },
   ilev_46  =  {10,   434519.94,     6,    1,    1,    1,    1 },
   ilev_47  =  {10,   434519.96,    10,    2,    2,    1,    2 },
   ilev_48  =  {10,   434519.98,    14,    3,    3,    1,    1 },
   ilev_49  =  {10,   434519.98,    18,   -1,   -1,    1,    2 },
   ilev_50  =  {10,   434519.99,    22,   -1,   -1,    1,    1 },
   ilev_51  =  {10,   434519.99,    26,   -1,   -1,    1,    2 },
   ilev_52  =  {10,   434519.99,    30,   -1,   -1,    1,    1 },
   ilev_53  =  {10,   434519.99,    34,   -1,   -1,    1,    2 },
   ilev_54  =  {10,   434519.99,    38,   -1,   -1,    1,    1 },
   ilev_55  =  {11,   435281.64,     2,    0,    0,    1,    2 },
   ilev_56  =  {11,   435281.66,     6,    1,    1,    1,    1 },
   ilev_57  =  {11,   435281.68,    10,    2,    2,    1,    2 },
   ilev_58  =  {11,   435281.69,    14,    3,    3,    1,    1 },
   ilev_59  =  {11,   435281.69,    18,   -1,   -1,    1,    2 },
   ilev_60  =  {11,   435281.70,    22,   -1,   -1,    1,    1 },
   ilev_61  =  {11,   435281.70,    26,   -1,   -1,    1,    2 },
   ilev_62  =  {11,   435281.70,    30,   -1,   -1,    1,    1 },
   ilev_63  =  {11,   435281.70,    34,   -1,   -1,    1,    2 },
   ilev_64  =  {11,   435281.70,    18,   -1,   -1,    1,    1 },
   ilev_65  =  {11,   435281.70,     2,   -1,   -1,    1,    2 },
   ilev_66  =  {12,   435861.00,     2,    0,    0,    1,    2 },
   ilev_67  =  {12,   435861.03,     6,    1,    1,    1,    1 },
   ilev_68  =  {12,   435861.04,     2,   -1,   -1,    1,    1 },
   ilev_69  =  {13,   436311.87,     2,    1,    1,    1,    1 },
   ilev_70  =  {13,   436311.87,     2,    0,    0,    1,    2 },
   ilev_71  =  {13,   436311.91,     2,   -1,   -1,    1,    2 },
   ilev_72  =  {14,   436669.62,     2,    1,    1,    1,    1 },
   ilev_73  =  {14,   436669.63,     2,    0,    0,    1,    2 },
   ilev_74  =  {14,   436669.66,     2,   -1,   -1,    1,    1 },
   ilev_75  =  {15,   436958.24,     2,    1,    1,    1,    1 },
   ilev_76  =  {15,   436958.24,     2,    0,    0,    1,    2 },
   ilev_77  =  {15,   436958.27,     2,   -1,   -1,    1,    2 },
   ilev_78  =  {16,   437194.45,     2,    1,    1,    1,    1 },
   ilev_79  =  {16,   437194.46,     2,    0,    0,    1,    2 },
   ilev_80  =  {16,   437194.48,     2,   -1,   -1,    1,    1 },
   ilev_81  =  {17,   437390.22,     2,    1,    1,    1,    1 },
   ilev_82  =  {17,   437390.22,     2,    0,    0,    1,    2 },
   ilev_83  =  {17,   437390.24,     2,   -1,   -1,    1,    2 },
   ilev_84  =  {18,   437554.28,     2,    1,    1,    1,    1 },
   ilev_85  =  {18,   437554.28,     2,    0,    0,    1,    2 },
   ilev_86  =  {18,   437554.29,     2,   -1,   -1,    1,    1 },
   ilev_87  =  {19,   437693.11,     2,    1,    1,    1,    1 },
   ilev_88  =  {19,   437693.11,     2,    0,    0,    1,    2 },
   ilev_89  =  {19,   437693.13,     2,   -1,   -1,    1,    2 },
   ilev_90  =  {20,   437811.65,     2,    1,    1,    1,    1 },
   ilev_91  =  {20,   437811.65,     2,    0,    0,    1,    2 },
   ilev_92  =  {20,   437811.66,     2,   -1,   -1,    1,    1 },
   ilev_93  =  {21,   437913.63,     2,    0,    0,    1,    2 },
   ilev_94  =  {21,   437913.64,     6,    1,    1,    1,    1 },
   ilev_95  =  {22,   438002.05,     2,    0,    0,    1,    2 },
   ilev_96  =  {22,   438002.06,     6,    1,    1,    1,    1 },
   ilev_97  =  {23,   438079.19,     2,    0,    0,    1,    2 },
   ilev_98  =  {23,   438079.19,     6,    1,    1,    1,    1 },
   ilev_99  =  {24,   438146.89,     2,    0,    0,    1,    2 },
   ilev_100  =  {24,   438146.89,     6,    1,    1,    1,    1 },
   ilev_101  =  {25,   438206.63,     2,    0,    0,    1,    2 },
   ilev_102  =  {25,   438206.63,     6,    1,    1,    1,    1 },
   ilev_103  =  {26,   438259.60,     2,    0,    0,    1,    2 },
   ilev_104  =  {26,   438259.61,     6,    1,    1,    1,    1 },
   ilev_105  =  {27,   438306.81,     2,    0,    0,    1,    2 },
   ilev_106  =  {27,   438306.81,     6,    1,    1,    1,    1 },
   ilev_107  =  {28,   438349.04,     2,    0,    0,    1,    2 },
   ilev_108  =  {28,   438349.04,     6,    1,    1,    1,    1 },
   ilev_109  =  {29,   438386.98,     2,    0,    0,    1,    2 },
   ilev_110  =  {29,   438386.99,     6,    1,    1,    1,    1 },
   ilev_111  =  {30,   438421.20,     2,    0,    0,    1,    2 },
   ilev_112  =  {30,   438421.20,     6,    1,    1,    1,    1 },
   ilev_113  =  {31,   438452.15,     2,    0,    0,    1,    2 },
   ilev_114  =  {31,   438452.15,     6,    1,    1,    1,    1 },
   ilev_115  =  {32,   438480.25,     2,    0,    0,    1,    2 },
   ilev_116  =  {32,   438480.25,     6,    1,    1,    1,    1 },
   ilev_117  =  {33,   438505.83,     2,    0,    0,    1,    2 },
   ilev_118  =  {33,   438505.83,     6,    1,    1,    1,    1 },
   ilev_119  =  {34,   438529.19,     2,    0,    0,    1,    2 },
   ilev_120  =  {34,   438529.19,     6,    1,    1,    1,    1 },
   ilev_121  =  {35,   438550.57,     2,    0,    0,    1,    2 },
   ilev_122  =  {35,   438550.58,     6,    1,    1,    1,    1 },
   ilev_123  =  {36,   438570.20,     2,    0,    0,    1,    2 },
   ilev_124  =  {36,   438570.20,     6,    1,    1,    1,    1 },
   ilev_125  =  {37,   438588.26,     2,    0,    0,    1,    2 },
   ilev_126  =  {37,   438588.26,     6,    1,    1,    1,    1 },
   ilev_127  =  {38,   438604.91,     2,    0,    0,    1,    2 },
   ilev_128  =  {38,   438604.91,     6,    1,    1,    1,    1 },
   ilev_129  =  {39,   438620.30,     2,    0,    0,    1,    2 },
   ilev_130  =  {39,   438620.30,     6,    1,    1,    1,    1 },
   ilev_131  =  {40,   438634.55,     2,    0,    0,    1,    2 },
   ilev_132  =  {40,   438634.55,     2,    1,    1,    1,    1 },
   ilev_133  =  {41,   438647.76,     2,    0,    0,    1,    2 },
   ilev_134  =  {41,   438647.76,     2,    1,    1,    1,    1 },
   ilev_135  =  {42,   438660.05,     2,    1,    1,    1,    1 },
   ilev_136  =  {42,   438660.05,     2,    0,    0,    1,    2 },
   ilev_137  =  {43,   438671.49,     2,    0,    0,    1,    2 },
   ilev_138  =  {43,   438671.49,     6,    1,    1,    1,    1 },
   ilev_139  =  {44,   438682.15,     2,    0,    0,    1,    2 },
   ilev_140  =  {44,   438682.15,     6,    1,    1,    1,    1 },
   ilev_141  =  {45,   438692.12,     2,    0,    0,    1,    2 },
   ilev_142  =  {45,   438692.12,     6,    1,    1,    1,    1 },
   ilev_143  =  {46,   438701.44,     2,    0,    0,    1,    2 },
   ilev_144  =  {46,   438701.44,     6,    1,    1,    1,    1 },
   ilev_145  =  {47,   438710.17,     2,    0,    0,    1,    2 },
   ilev_146  =  {47,   438710.17,     6,    1,    1,    1,    1 },
   ilev_147  =  {48,   438718.36,     2,    0,    0,    1,    2 },
   ilev_148  =  {48,   438718.36,     6,    1,    1,    1,    1 },
   ilev_149  =  {49,   438726.06,     2,    0,    0,    1,    2 },
   ilev_150  =  {49,   438726.06,     6,    1,    1,    1,    1 },
   ilev_151  =  { 5,   438733.30,     2,    0,    0,    1,    2 },
   ilev_152  =  { 5,   438733.30,     6,    1,    1,    1,    1 },
   -- ===========================================================
}
