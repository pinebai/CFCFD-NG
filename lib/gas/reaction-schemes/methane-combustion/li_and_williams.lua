-- this file has been automatically generated by chemkin2libgas.py

-- scaling factor, 1/R
S = 1.0/8.314472e-3

reaction{'CH4 + O2 <=> HO2 + CH3',
   fr={'Arrhenius', A=3.98000e+13, n=0.00000e+00, T_a=2.38000e+02*S},
   label='r0'
}
reaction{'CH4 + HO2 <=> H2O2 + CH3',
   fr={'Arrhenius', A=9.04000e+12, n=0.00000e+00, T_a=1.03100e+02*S},
   label='r1'
}
reaction{'CH4 + OH <=> H2O + CH3',
   fr={'Arrhenius', A=1.60000e+07, n=1.83000e+00, T_a=1.16000e+01*S},
   label='r2'
}
reaction{'CH3 + O2 <=> CH2O + OH',
   fr={'Arrhenius', A=3.30000e+11, n=0.00000e+00, T_a=3.74000e+01*S},
   label='r3'
}
reaction{'CH2O + OH <=> CHO + H2O',
   fr={'Arrhenius', A=3.90000e+10, n=8.90000e-01, T_a=1.70000e+00*S},
   label='r4'
}
reaction{'CHO + O2 <=> CO + HO2',
   fr={'Arrhenius', A=3.00000e+12, n=0.00000e+00, T_a=0.00000e+00*S},
   label='r5'
}
reaction{'CHO + M <=> CO + H + M',
   fr={'Arrhenius', A=1.86000e+17, n=-1.00000e+00, T_a=7.11000e+01*S},
   label='r6'
}
reaction{'H + O2 + M <=> HO2 + M',
   fr={'Arrhenius', A=6.76000e+19, n=-1.40000e+00, T_a=0.00000e+00*S},
   label='r7'
}
reaction{'H2O2 + M <=> 2OH + M',
   fr={'Arrhenius', A=1.20000e+17, n=0.00000e+00, T_a=1.90400e+02*S},
   label='r8'
}
reaction{'CH4 ( + M ) <=> CH3 + H ( + M )',
   fr={'pressure dependent',
       k_inf={A=2.22000e+16, n=0.00000e+00, T_a=4.39000e+02*S},
       k_0={A=6.59000e+25, n=-1.80000e+00, T_a=4.39000e+02*S},
   },
   label='r9'
}
reaction{'CH4 + H <=> H2 + CH3',
   fr={'Arrhenius', A=1.30000e+04, n=3.00000e+00, T_a=3.36000e+01*S},
   label='r10'
}
reaction{'CH4 + O <=> CH3 + OH',
   fr={'Arrhenius', A=1.90000e+09, n=1.44000e+00, T_a=3.63000e+01*S},
   label='r11'
}
reaction{'CH3 + O2 <=> CH3O + O',
   fr={'Arrhenius', A=1.33000e+14, n=0.00000e+00, T_a=1.31400e+02*S},
   label='r12'
}
reaction{'CH3 + O <=> CH2O + H',
   fr={'Arrhenius', A=8.43000e+13, n=0.00000e+00, T_a=0.00000e+00*S},
   label='r13'
}
reaction{'CH3 + HO2 <=> CH3O + OH',
   fr={'Arrhenius', A=2.00000e+13, n=0.00000e+00, T_a=0.00000e+00*S},
   label='r14'
}
reaction{'2CH3 ( + M ) <=> C2H6 ( + M )',
   fr={'pressure dependent',
       k_inf={A=1.81000e+13, n=0.00000e+00, T_a=0.00000e+00*S},
       k_0={A=1.27000e+41, n=-7.00000e+00, T_a=1.16000e+01*S},
   },
   label='r15'
}
reaction{'CH3O + O2 <=> CH2O + HO2',
   fr={'Arrhenius', A=4.28000e-13, n=7.60000e+00, T_a=-1.48000e+01*S},
   label='r16'
}
reaction{'CH3O + M <=> CH2O + H + M',
   fr={'Arrhenius', A=1.00000e+13, n=0.00000e+00, T_a=5.65000e+01*S},
   label='r17'
}
reaction{'CH2O + H <=> CHO + H2',
   fr={'Arrhenius', A=1.26000e+08, n=1.62000e+00, T_a=9.10000e+00*S},
   label='r18'
}
reaction{'CH2O + O <=> CHO + OH',
   fr={'Arrhenius', A=3.50000e+13, n=0.00000e+00, T_a=1.47000e+01*S},
   label='r19'
}
reaction{'H + O2 <=> OH + O',
   fr={'Arrhenius', A=3.52000e+16, n=-7.00000e-01, T_a=7.14000e+01*S},
   label='r20'
}
reaction{'H2 + O <=> OH + H',
   fr={'Arrhenius', A=5.06000e+04, n=2.67000e+00, T_a=2.63000e+01*S},
   label='r21'
}
reaction{'H2 + OH <=> H2O + H',
   fr={'Arrhenius', A=1.17000e+09, n=1.30000e+00, T_a=1.52000e+01*S},
   label='r22'
}
reaction{'H2O + O <=> 2OH',
   fr={'Arrhenius', A=7.60000e+00, n=3.84000e+00, T_a=5.35000e+01*S},
   label='r23'
}

