#! /usr/bin/env python
"""
pitot_input_utils.py: pitot input utilities

This file collects all of the functions that the larger pitot program uses to
check and build its input dictionary from a user specified config file.

Chris James (c.james4@uq.edu.au) - 07/05/13 

"""

from cfpylib.gasdyn.cea2_gas import Gas, make_gas_from_name
from cfpylib.gasdyn.gas_flow import *
from cfpylib.gasdyn.ideal_gas_flow import p0_p, pitot_p

import cfpylib.gasdyn.ideal_gas as pg

PRINT_STATUS = 1 #if print status is 1, some basic printouts are done

def config_loader(config_file):
    """Function that loads the pitot input dictionary from a config file,
    and returns the loaded dictionary to the main program."""
    
    cfg = {}
    
    try: #from Rowan's onedval program
        execfile(config_file, globals(), cfg)
    except IOError:
        print "There was a problem reading the config file: '{0}'".format(config_file)
        print "Check that it conforms to Python syntax."
        print "Bailing out!"
        sys.exit(1)
        
    return cfg

def is_valid(command,valid_commands):
    """Prompts the user for a valid command, and only returns a valid command.

    is_valid_command(float,tuple<floats>) -> float

    Preconditions: input must be a single number."""
    
    check = False
    
    while check == False:
        for i in valid_commands:
            if i == command:
                check = True
    
        if check == False:
             print 'That is an invalid command. Valid commands are {0}'.format(valid_commands)
             command = str(raw_input('Try again? '))             
    return command
    
def make_test_gas(gasName, outputUnits='moles'):
    """
    Manufacture a Gas object for the test gas from a small library of options.
    
    Also has workable gamma's and R's for the test gases at high temperature stored
    in a dictionary of the form {'gam' : gam, 'R' : R}.
    
    Gases using CO2 (that will not give valid CEA solutions below 800 K) also have
    a specified gamma and Mmass in their dictionaries (as the third and fourth values)
    this is so the gases can be used for perfect gas calculations even if they won't solve
    in CEA at atmospheric pressure. These gas values were taken from Cengel and Boles'
    Thermodynamics text book.
    
    I stole this from the make_gas_from_name function in cea2_gas.py and added
    my own gases - Chris James

    :param gasName: one of the names for the special cases set out below
    """
    if gasName.lower() == 'air':
        return Gas({'Air':1.0,}, outputUnits=outputUnits, trace=1.0e-4), {'gam':1.35,'R':571.49}
    elif gasName.lower() == 'air5species':
        return Gas(reactants={'N2':0.79, 'O2':0.21, 'N':0.0, 'O':0.0, 'NO':0.0}, 
                   inputUnits='moles', onlyList=['N2','O2','N','O','NO'],with_ions=False,
                   outputUnits=outputUnits), {'gam':1.35,'R':571.49}
    elif gasName.lower() == 'n2':
        return Gas(reactants={'N2':1.0, 'N':0.0}, onlyList=['N2', 'N'], with_ions=True,
                   outputUnits=outputUnits), {'gam': 1.36,'R':593.56}
    elif gasName.lower() == 'titan': #composition used by Hadas Porat
        return Gas(reactants={'N2':0.95, 'CH4':0.05}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits), {'gam':1.35,'R':652.03}      
    elif gasName.lower() == 'mars': #this is the composition used in Troy Eichmann's Mars testing
        return Gas(reactants={'CO2':0.96, 'N2':0.04}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits), None, 1.2934, 43.37 #Gas, ideal gas guess, gam, Mmass    
    elif gasName.lower() == 'venus': #this is what Guerric was after, from the Park and Ahn paper on Heat Transfer for Venus Probes
        return Gas(reactants={'CO2':0.97, 'N2':0.03}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits), None, 1.2923, 43.53 #Gas, ideal gas guess, gam, Mmass  
    elif gasName.lower() == 'co2':
        return Gas(reactants={'CO2':1.0}, outputUnits=outputUnits), None, 1.289, 44.01 #Gas, ideal gas guess, gam, Mmass
    elif gasName.lower() == 'gasgiant_h215ne': #composition used in Chris James' equivalent condition tests
        return Gas(reactants={'H2':0.85, 'Ne':0.15}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits),{'gam':1.15,'R':3245.1}
    elif gasName.lower() == 'gasgiant_h240ne':
        return Gas(reactants={'H2':0.6, 'Ne':0.4}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits),{'gam':1.5,'R':1443.2}
    elif gasName.lower() == 'gasgiant_h285ne': #composition used by Charlotte Higgins
        return Gas(reactants={'H2':0.15, 'Ne':0.85}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits),{'gam':1.5,'R':547.8}
    elif gasName.lower() == 'gasgiant_h215he': #composition used in Chris James' equivalent condition tests
        return Gas(reactants={'H2':0.85, 'He':0.15}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits), {'gam':1.2,'R':6303.2}
    elif gasName.lower() == 'gasgiant_h210he': #composition used in Chris James' undergrad thesis
        return Gas(reactants={'H2':0.9, 'He':0.10}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits), None    
    elif gasName.lower() == 'gasgiant_h210ne': #composition used in Chris James' undergrad thesis
        return Gas(reactants={'H2':0.9, 'Ne':0.10}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits), None
    elif gasName.lower() == 'h2':
        return Gas(reactants={'H2':1.0}, inputUnits='moles', with_ions=True,
                   outputUnits=outputUnits), None                   
    else:
        raise Exception, 'make_test_gas(): unknown gasName: %s' % gasName 
   
def input_checker(cfg):
    """Takes the input file and checks it works. Duh.
    
    Returns the checked input file and will tell the bigger program to 
    bail out if it finds an issue.
    
    """
    
    print '-'*60
    print "Performing input check before test is ran."
    
    cfg['bad_input'] = False
    
    if cfg['test'] not in ['fulltheory-shock','fulltheory-pressure','experiment']:
        print "No 'test' specified. You need to specify a test type. Bailing out."
        print "Available tests are 'fulltheory-shock', 'fulltheory-pressure', 'experiment'."
        cfg['bad_input'] = True    
    
    if 'solver' not in cfg:
        print "No 'solver' specified. You need to specify a solver. Bailing out."
        print "Available solvers are 'eq','pg', and 'pg-eq'."
        cfg['bad_input'] = True
    if cfg['solver'] not in ['eq','pg','pg-eq']:
        print "'solver' is not a valid solver. You need to specify a solver. Bailing out."
        print "Valid solvers are 'eq','pg', and 'pg-eq'."
        cfg['bad_input'] = True
        
    if 'facility' not in cfg:
        print "No 'facility' specified. You need to specify a facility. Bailing out."
        print "Available facilities are 'x2','x3', and 'custom'."
        cfg['bad_input'] = True
    if cfg['facility'] not in ['x2','x3','custom']:
        print "'facility' is not a valid facility. You need to specify a facility. Bailing out."
        print "Valid facilities are 'x2','x3', and 'custom'."
        cfg['bad_input'] = True
        
    # add some more checks here for the custom facility mode
    # custom facility must have initial driver state specified, driver composition,
    # driver burst state (either from compression ratio or burst pressure),
    # and Mach number terminating steady expansion
    
    if cfg['facility'] == 'custom':
        if 'driver_p' not in cfg:
            print "You have specified a custom facility but not set the 'driver_p' variable."
            print "Bailing out."
            cfg['bad_input'] = True
        if 'driver_T' not in cfg:
            print "You have specified a custom facility but not set the 'driver_T' variable."
            print "Bailing out."
            cfg['bad_input'] = True
        if 'driver_composition' not in cfg:
            print "You have specified a custom facility but have not set 'driver_composition' variable."
            print "Bailing out."
            cfg['bad_input'] = True
        if not isinstance(cfg['driver_composition'], dict):
            print "'driver_composition' variable is not a valid Python dictionary."
            print "Bailing out."
            cfg['bad_input'] = True
        if 'driver_inputUnits' not in cfg:
            print "'driver_inputUnits' not set. Setting it to 'moles'."
            cfg['driver_inputUnits'] = 'moles'
        if 'p4' not in cfg and 'compression_ratio' not in cfg:
            print "Both 'compression_ratio' and 'p4' variables not set."
            print "Not enough information to make the custom driver work."
            print "Bailing out."
        if 'M_throat' not in cfg:
            print "Throat Mach number that terminates unsteady expansion is not set."
            print "Variable is 'M_throat'."
            print "Cannot finish problem without this. Bailing out."
            
    if cfg['test_gas'] == 'custom':
        if 'test_gas_composition' not in cfg:
            print "You have specified a custom test gas but have not set 'test_gas_composition' variable."
            print "Bailing out."
            cfg['bad_input'] = True
        if not isinstance(cfg['test_gas_composition'], dict):
            print "'test_gas_composition' variable is not a valid Python dictionary."
            print "Bailing out."
            cfg['bad_input'] = True
        if 'test_gas_inputUnits' not in cfg:
            print "'test_gas_inputUnits' not set. Setting it to 'moles'."
            cfg['test_gas_inputUnits'] = 'moles'            
            
    if 'mode' not in cfg:
        print "Program mode not specified. Will use default printout mode"
        cfg['mode'] = 'printout'
    if 'filename' not in cfg and cfg['mode'] == 'printout' or \
        'filename' not in cfg and cfg['mode'] == 'cea-printout':
        print "No filename selected for printouts."
        print "Default filename will be 'pitot_run'"
        cfg['filename'] = 'pitot_run'

    if cfg['nozzle'] and not 'area_ratio' in cfg: 
        #if they haven't specified their own area ratio, give them one
        print 'No area ratio was set. So a default will be used.'
        if cfg['facility'] == 'x2':
            print 'Effective area ratio for x2 facility nozzle chosen (2.5)'
            cfg['area_ratio'] = 2.5 #effective area ratio x2's nozzle normally has
        elif cfg['facility'] == 'x3':
            print 'Geometric area ratio for x3 facility nozzle chosen (5.8)'
            cfg['area_ratio'] = 5.8 #geometric area ratio of x3's nozzle
            
    if 'tunnel_mode' not in cfg and 'p5' in cfg or 'tunnel_mode' not in cfg and 'Vs2' in cfg:
        #assert expansion tube mode if the values are there
        print "tunnel_mode variable not set but your inputs imply expansion tube mode."
        print "tunnel_mode set to expansion-tube mode"
        cfg['tunnel_mode'] = 'expansion-tube'
    elif 'tunnel_mode' not in cfg and 'p5' not in cfg or 'tunnel_mode' not in cfg and 'Vs2' not in cfg:
        #assert expansion tube mode if the values are there
        print "tunnel_mode variable not set but your inputs imply non-reflected shock tube mode."
        print "tunnel_mode set to nr-shock-tunnel"
        cfg['tunnel_mode'] = 'nr-shock-tunnel'
        
    if 'area_ratio_check' not in cfg:
        #If they haven't told pitot to do the area ratio check, don't do it.
        print "Area ratio check variable not set. Setting it to false."
        cfg['area_ratio_check'] = False
        
    if cfg['area_ratio_check'] and 'area_ratio_check_list' not in cfg:
        print "Need to specify an 'area_ratio_check_list' to use area ratio check."
        print "Bailing out."
        cfg['bad_input'] = True
            
    if 'cleanup' not in cfg:
        #if they haven't specified whether to cleanup or not, just cleanup
        print 'Not told whether to cleanup temp files or not. Will not clean them up.'
        cfg['cleanup'] = False
        
    if cfg['facility'] == 'x2' and 'piston' not in cfg:
        #if they're using x2 and they haven't set a Piston, use the lwp
        print 'X2 facility in use and no piston selected. Using lightweight piston (lwp).'
        cfg['piston'] = 'lwp'
        
    if 'expand_to' not in cfg:
        print "expand_to variable not set. Setting program to expand to the flow behind the shock."
        cfg['expand_to'] = 'flow-behind-shock'
        
    if 'expansion_factor' not in cfg:
        print "expansion_factor variable is not set. Setting it to 1.0."
        cfg['expansion_factor'] = 1.0
        
    if 'shock_switch' not in cfg:
        print "Shock switch not selected, so we'll keep it turned off."
        cfg['shock_switch'] = False
        
    if 'conehead' not in cfg:
        print "conehead switch not set, so we'll leave it turned off."
        cfg['conehead'] = False
        
    if 'conehead' in cfg and 'conehead_angle' not in cfg:
        print "conehead angle not specified. 15 degree conehead angle selected."
        cfg['conehead_angle'] = 15.0
        
    if 'shock_over_model' not in cfg:
        print "shock_over_model switch not set, so we'll leave it turned off."
        cfg['shock_over_model'] = False
        
    if cfg['secondary'] and 'secondary_driver_expansion_steps' not in cfg:
        #if they don't specify amount of steps for the unsteady expansion, give them one
        cfg['secondary_driver_expansion_steps'] = 300
        print "Number of steps for secondary driver unsteady expansion not selected. {0} steps chosen.".format(cfg['secondary_driver_expansion_steps'])       
    elif cfg['secondary'] and 'secondary_driver_expansion_steps' in cfg: 
        # check that the chosen value is an integer and fix it if not
        if not isinstance(cfg['secondary_driver_expansion_steps'], int):
            cfg['secondary_driver_expansion_steps'] = int(cfg['secondary_driver_expansion_steps'])
            print "Number of steps for secondary driver unsteady expansion not an integer."
            print "Making it one. New value is {0}".format(cfg['secondary_driver_expansion_steps'])
    
    if 'shock_tube_expansion_steps' not in cfg:
        #if they don't specify amount of steps for the unsteady expansion, give them one
        cfg['shock_tube_expansion_steps'] = 300
        print "Number of steps for shock tube unsteady expansion not selected. {0} steps chosen.".format(cfg['shock_tube_expansion_steps'])
    else: # check that the chosen value is an integer and fix it if not
        if not isinstance(cfg['shock_tube_expansion_steps'], int):
            cfg['shock_tube_expansion_steps'] = int(cfg['shock_tube_expansion_steps'])
            print "Number of steps for shock tube unsteady expansion not an integer."
            print "Making it one. New value is {0}".format(cfg['shock_tube_expansion_steps'])
    
    if cfg['tunnel_mode'] == 'expansion-tube' and 'acc_tube_expansion_steps' not in cfg:
        #if they don't specify amount of steps for the unsteady expansion, give them one
        cfg['acc_tube_expansion_steps'] = 1000
        print "Number of steps for acceleration tube unsteady expansion not selected. {0} steps chosen.".format(cfg['acc_tube_expansion_steps'])
    elif cfg['tunnel_mode'] == 'expansion-tube' and 'acc_tube_expansion_steps' in cfg:
        # check that the chosen value is an integer and fix it if not
        if not isinstance(cfg['acc_tube_expansion_steps'], int):
            cfg['acc_tube_expansion_steps'] = int(cfg['acc_tube_expansion_steps'])  
            print "Number of steps for acceleration unsteady expansion not an integer."
            print "Making it one. New value is {0}".format(cfg['acc_tube_expansion_steps'])
            
    if cfg['test'] == 'fulltheory-shock' and 'p1' in cfg: #if they specify both pressures and shock speeds, bail out
        print "You need to choose only shock speeds to solve this test case. Bailing out here."
        cfg['bad_input'] = True
        
    if cfg['test'] == 'fulltheory-shock' and cfg['secondary'] and 'Vsd' not in cfg \
        or cfg['test'] == 'experiment' and cfg['secondary'] and 'Vsd' not in cfg:
        print "Need to supply a float value for Vsd."
        cfg['bad_input'] = True
    
    if cfg['test'] == 'fulltheory-shock' and 'Vs1' not in cfg \
        or cfg['test'] == 'experiment' and 'Vs1' not in cfg:
        print "Need to supply a float value for Vs1."
        cfg['bad_input'] = True
        
    if cfg['test'] == 'fulltheory-shock' and cfg['tunnel_mode'] == 'expansion-tube' and 'Vs2' not in cfg \
        or cfg['test'] == 'experiment' and cfg['tunnel_mode'] == 'expansion-tube' and 'Vs2' not in cfg:
        print "Need to supply a float value for Vs2."
        cfg['bad_input'] = True
          
    if cfg['test'] == 'fulltheory-pressure' and 'Vs1' in cfg: #if they specify both pressures and shock speeds, bail out
        print "You need to choose either only fill pressures to solve this test case. Bailing out here."
        cfg['bad_input'] = True       
    
    if cfg['test'] == 'fulltheory-pressure' and cfg['secondary'] and 'psd1' not in cfg \
        or cfg['test'] == 'experiment' and cfg['secondary'] and 'psd1' not in cfg:
        print "Need to supply a float value for psd1."
        cfg['bad_input'] = True        
        
    if cfg['test'] == 'fulltheory-pressure' and 'p1' not in cfg \
        or cfg['test'] == 'experiment' and 'p1' not in cfg:
        print "Need to supply a float value for p1."
        cfg['bad_input'] = True

    if cfg['test'] == 'fulltheory-pressure' and cfg['tunnel_mode'] == 'expansion-tube' and 'p5' not in cfg \
        or cfg['test'] == 'experiment' and cfg['tunnel_mode'] == 'expansion-tube' and 'p5' not in cfg:
        print "Need to supply a float value for p5."
        cfg['bad_input'] = True
        
    if cfg['bad_input']: #bail out here if you end up having issues with your input
        print "Config failed check. Bailing out now."
        print '-'*60
        sys.exit(1)
        
    if not cfg['bad_input']:
        print "Input check completed. Test will now run."
        print '-'*60
           
    return cfg
    
def start_message(cfg):
    """
    Takes the config file and prints a short introduction to the 
    test to the screen before the run starts.
    """
    
    if PRINT_STATUS: print "Let's get started, shall we:"
    if PRINT_STATUS and not cfg['facility'] == 'custom': 
        print "Facility is {0}. Driver gas is {1}.".format(cfg['facility'], cfg['driver_gas'])
    if PRINT_STATUS and cfg['facility'] == 'custom': 
        print "Facility is {0}. Driver gas is {1}.".format(cfg['facility'], cfg['driver_composition'])    
    if PRINT_STATUS: 
        if 'Vsd' in cfg and cfg['secondary']:
            print 'Selected Vsd = {0} m/s'.format(cfg['Vsd'])
        if 'Vs1' in cfg:
            print 'Selected Vs1 = {0} m/s'.format(cfg['Vs1'])
        if 'Vs2' in cfg:
            print 'Selected Vs2 = {0} m/s'.format(cfg['Vs2'])
        if 'psd1' in cfg and cfg['secondary']:
            print 'Selected secondary driver fill pressure (psd1) = {0} Pa.'.format(cfg['psd1'])
        if 'p1' in cfg:
            print 'Selected shock tube fill pressure (p1) = {0} Pa.'.format(cfg['p1'])
        if 'p5' in cfg:            
            print 'Selected acceleration tube fill pressure (p5) = {0} Pa.'.format(cfg['p5'])
        print '-'*60       
    return
    
def state_builder(cfg):
    """Function to build the various states required by the program."""

    #--------------------------- property dictionaries -------------------------

    #primary driver conditions, sorted into a dictionary with a gas object at the
    #right conditions, and then mach number at the change over from steady to
    #unsteady expansion, this was based on calcs done by RGM

    primary_driver_x2 = {'He:1.0':[Gas({'He':1.0},inputUnits='moles'),2.15],
                       'He:0.80,Ar:0.20':[Gas({'He':0.8,'Ar':0.2},inputUnits='moles'),1],
                        'He:0.90,Ar:0.10':[Gas({'He':0.9,'Ar':0.1},inputUnits='moles'),1.59],
                        'He:0.85,Ar:0.15':[Gas({'He':0.85,'Ar':0.15},inputUnits='moles'),1.385],
                        'He:0.825,Ar:0.175':[Gas({'He':0.825,'Ar':0.175},inputUnits='moles'),1.256]                   }
                        
    primary_driver_x3 = dict([('He:0.60,Ar:0.40',[Gas({'He':0.6,'Ar':0.4},inputUnits='moles'),2.23])])
    
    #here the states are made as CEA2 gas states, and then the ideal gam and MW are pulled out if required   
    #and the gas objects are then redefined to be perfect gas
       
    states = {} #states dictionary that we'll fill up later
    V = {} #same for velocity
    M = {} #same for Mach number
    
    if PRINT_STATUS: print "Building initial gas states."
        
    #state 4 is diaphragm burst state (taken to basically be a total condition)
    if cfg['facility'] == 'x2':
        if cfg['piston'] == 'lwp': 
            #This is the tuned driver condition designed by David Gildfind in his PhD.
            states['s4']=primary_driver_x2[cfg['driver_gas']][0].clone()
            p4 = 2.79e7; T4 = 2700.0 #Pa, K
            states['s4'].set_pT(p4,T4)
            V['s4']=0.0
            M['s4']=0.0
            M['s3s']=primary_driver_x2[cfg['driver_gas']][1]
        elif cfg['piston'] == 'ostp':
            #this is the first attempt at designing a single stage piston driver for X2.
            #Completed by Michael Scott as part of his PhD.
            states['s4']=primary_driver_x2['He:1.0'][0].clone()
            p4 = 15.5e6; T4 = 2500.0 #Pa, K
            states['s4'].set_pT(p4,T4)
            V['s4']=0.0
            M['s4']=0.0
            M['s3s'] = 1.0
                  
    elif cfg['facility'] == 'x3':
        states['s4']=primary_driver_x3[cfg['driver_gas']][0].clone()
        p4 = 2.79e7; T4 = 2700.0 #Pa, K
        states['s4'].set_pT(p4,T4)
        V['s4']=0.0
        M['s4']=0.0
        M['s3s']=primary_driver_x3[cfg['driver_gas']][1]
        
    elif cfg['facility'] == 'custom':
        # set driver fill condition
        print "Custom driver gas is {0}.".format(cfg['driver_composition'])
        print "Custom driver fill condition is {0} Pa, {1} K.".format(cfg['driver_p'], cfg['driver_T'])
        states['primary_driver_fill']=Gas(cfg['driver_composition'],inputUnits=cfg['driver_inputUnits'])
        states['primary_driver_fill'].set_pT(cfg['driver_p'],cfg['driver_T'])
        # now do the compression to state 4
        # If both p4 and compression ratio are set, code will use p4
        if 'p4' in cfg:
            print "Performing isentropic compression from driver fill condition to {0} Pa.".format(cfg['p4'])
            p4 = cfg['p4'] #Pa
            T4 = states['primary_driver_fill'].T*\
            (cfg['p4']/states['primary_driver_fill'].p)**(1.0-(1.0/states['primary_driver_fill'].gam)) #K
        else:
            print "Performing isentropic compression from driver fill condition over compression ratio of {0}.".format(cfg['compression_ratio'])
            cfg['pressure_ratio'] = cfg['compression_ratio']**states['primary_driver_fill'].gam #pressure ratio is compression ratio to the power of gamma
            p4 = states['primary_driver_fill'].p*cfg['pressure_ratio'] #Pa
            T4 = states['primary_driver_fill'].T*\
            (cfg['pressure_ratio'])**(1.0-(1.0/states['primary_driver_fill'].gam)) #K
        states['s4'] =  states['primary_driver_fill'].clone()
        states['s4'].set_pT(p4,T4)
        V['s4']=0.0
        M['s4']=0.0
        M['s3s'] = cfg['M_throat']    
            
    if cfg['solver'] == 'pg': #make perfect gas object, and then re-set the state
        states['s4']=pg.Gas(Mmass=states['s4'].Mmass,
                                    gamma=states['s4'].gam, name='s4')
        states['s4'].set_pT(p4,T4)
    
    #state3s is driver gas after steady expansion at the throat between 
    #the primary driver and the next section
    
    (states['s3s'], V['s3s']) = expand_from_stagnation(1.0/(p0_p(M['s3s'],states['s4'].gam)),states['s4'])

    #build the gas objects for all the other sections based on knowledge of what is what

    cfg['T0'] = 300.0 #atmospheric temperature (K), used for any section starting at ambient
    cfg['p0'] = 101300.0 #atmospheric pressure (Pa)

    #start with shock tube and acc tube, start with atmospheric p and T

    if cfg['secondary']: #state sd1 is pure He secondary driver (if used)
        states['sd1'] =  Gas({'He':1.0,},outputUnits='moles')
        if 'psd1' not in cfg: #set atmospheric state if a pressure was not specified
            cfg['psd1'] = cfg['p0']
        states['sd1'].set_pT(cfg['psd1'],cfg['T0'])
        if cfg['solver'] == 'pg': #make perfect gas object if asked to do so, then re-set the gas state
            states['sd1']=pg.Gas(Mmass=states['sd1'].Mmass,
                             gamma=states['sd1'].gam, name='sd1')
            states['sd1'].set_pT(cfg['psd1'],cfg['T0'])
        V['sd1']=0.0
        M['sd1']=0.0

    #state 1 is shock tube
    if cfg['test_gas'] == 'custom':
        if 'test_gas_with_ions' not in cfg:
            print "'test_gas_with_ions' variable not set. Setting to boolean True."
            cfg['test_gas_with_ions'] = True
        states['s1'] = Gas(cfg['test_gas_composition'],inputUnits=cfg['test_gas_inputUnits'],
                        with_ions=cfg['test_gas_with_ions'])
        states['s1'].set_pT(cfg['p1'],cfg['T0'])
        cfg['gas_guess'] = None
    else:
        if cfg['test_gas'] == 'mars' or cfg['test_gas'] == 'co2' or cfg['test_gas'] == 'venus':
            states['s1'], cfg['gas_guess'], test_gas_gam, test_gas_Mmass = make_test_gas(cfg['test_gas'])   
        else: #need to split this up as the function returns 4 values if CO2 is in the test gas
              # and trying to set the state of the co2 gas object at room temp will break it
            states['s1'], cfg['gas_guess'] = make_test_gas(cfg['test_gas'])
            if 'p1' not in cfg: #set atmospheric state if a pressure was not specified
                cfg['p1'] = cfg['p0']
            states['s1'].set_pT(cfg['p1'],cfg['T0'])
        if cfg['solver'] == 'pg' or cfg['solver'] == 'pg-eq': #make perfect gas object if asked to do so, then re-set the gas state
            if cfg['solver'] == 'pg-eq': #store the eq gas object as we'll want to come back to it later...      
                states['s1-eq'] = states['s1'].clone()        
            if cfg['test_gas'] == 'co2' or cfg['test_gas'] == 'mars' or cfg['test_gas'] == 'venus': #need to force our own gam and Mmass onto the gas object if CO2 is in the gas
                states['s1'].gam =  test_gas_gam; states['s1'].Mmass =  test_gas_Mmass
            states['s1']=pg.Gas(Mmass=states['s1'].Mmass,
                                gamma=states['s1'].gam, name='s1')
            states['s1'].set_pT(cfg['p1'],cfg['T0'])
    V['s1']=0.0
    M['s1']=0.0
        
    if cfg['tunnel_mode'] == 'expansion-tube':
        #state 5 is acceleration tube
        states['s5'] = Gas({'Air':1.0,},outputUnits='moles')
        if 'p5' not in cfg: #set atmospheric state if a pressure was not specified
            cfg['p5'] = cfg['p0']
        states['s5'].set_pT(cfg['p5'],cfg['T0'])
        if cfg['solver'] == 'pg': #make perfect gas object if asked to do so, then re-set the gas state
            states['s5']=pg.Gas(Mmass=states['s5'].Mmass,
                                gamma=states['s5'].gam, name='s5')
            states['s5'].set_pT(cfg['p5'],cfg['T0'])
        V['s5']=0.0
        M['s5']=0.0
        #now let's clone the states we just defined to get the states derved from these
       
    if cfg['secondary']:
        states['sd2'] = states['sd1'].clone() #sd2 is sd1 shock heated
        states['sd3'] = states['s3s'].clone() #sd3 is s3s after unsteady expansion
        states['s3'] = states['sd2'].clone() #s3 will be sd2 after unsteady expansion
    else:
        states['s3'] = states['s3s'].clone() #s3 will be s3s after unsteady expansion
            
    states['s2'] = states['s1'].clone() #s2 is s1 shock heated
    
    if cfg['tunnel_mode'] == 'expansion-tube':
        states['s6'] = states['s5'].clone() #s6 is s5 shock heated
        states['s7'] = states['s2'].clone() #7s is s2 after unsteady expansion
          
        # This code turns off the ions for state 7 if the user finds out it is 
        # making the unsteady expansion of the test gas into the acceleration tube fail
    
        if 'state7_no_ions' not in cfg:
            cfg['state7_no_ions'] = False

        if cfg['state7_no_ions']:
            # rebuild state 7 with no ions if we need
            #states['s7'] = Gas(reactants=states['s2'].reactants, 
            #                inputUnits = states['s2'].inputUnits,
            #               with_ions = False)
            #states['s7'].set_pT(states['s2'].p, states['s2'].T)
            states['s7'].with_ions = False

    #need this set to something for later in the piece
    if cfg['secondary'] and 'Vsd' not in cfg: cfg['Vsd'] = None
    if 'Vs1' not in cfg: cfg['Vs1'] = None
    if 'Vs2' not in cfg: cfg['Vs2'] = None

    if PRINT_STATUS: print '-'*60               
    
    return cfg, states, V, M