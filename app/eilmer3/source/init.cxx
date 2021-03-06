/** \file init.cxx
 * \ingroup eilmer3
 * \brief Initialisation routines for Multiple-Block Navier-Stokes code.
 *
 * \version 02-Mar-08 Elmer3 port from mbcns2
 * \version 30-Jun-08 Conversion to use of C++ strings and Rowan's ConfigParser.
 */

//-----------------------------------------------------------------

#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include "../../../lib/util/source/useful.h"
#include "../../../lib/util/source/string_util.hh"
#include "../../../lib/util/source/config_parser.hh"
#include "../../../lib/gas/models/gas_data.hh"
#include "../../../lib/gas/models/gas-model.hh"
#include "../../wallcon/source/e3conn.hh"
#include "block.hh"
#include "kernel.hh"
#include "cell.hh"
#include "bc.hh"
#include "init.hh"
#include "diffusion.hh"
#include "visc.hh"
#include "flux_calc.hh"
#include "one_d_interp.hh"
#include "conj-ht-interface.hh"

using namespace std;

std::string & to_lower_case(std::string & mystring)
// Change to lower-case in place, for use in looking up name->object maps.
// PJ, 21-Nov-2013
{
    for ( size_t i = 0; i < mystring.length(); ++i )
	mystring[i] = tolower(mystring[i]);
    return mystring;
}

std::map<std::string,update_scheme_t> available_schemes;
int init_available_schemes_map()
{
    // Yes, this has quite a few entries and they're here because
    // I've already made a couple of errors in the input scripts.
    // And, yes, it would be tidier with an initialization list
    // but the Intel C++ compiler doesn't implement such a nicety.
    typedef std::pair<std::string,update_scheme_t> name_scheme_t;
    available_schemes.insert(name_scheme_t("euler",EULER_UPDATE));
    available_schemes.insert(name_scheme_t("pc",PC_UPDATE)); 
    available_schemes.insert(name_scheme_t("predictor-corrector",PC_UPDATE));
    available_schemes.insert(name_scheme_t("predictor_corrector",PC_UPDATE));
    available_schemes.insert(name_scheme_t("midpoint",MIDPOINT_UPDATE)); 
    available_schemes.insert(name_scheme_t("mid-point",MIDPOINT_UPDATE));
    available_schemes.insert(name_scheme_t("mid_point",MIDPOINT_UPDATE)); 
    available_schemes.insert(name_scheme_t("classic-rk3",CLASSIC_RK3_UPDATE));
    available_schemes.insert(name_scheme_t("classic_rk3",CLASSIC_RK3_UPDATE));
    available_schemes.insert(name_scheme_t("tvd-rk3",TVD_RK3_UPDATE));
    available_schemes.insert(name_scheme_t("tvd_rk3",TVD_RK3_UPDATE));
    available_schemes.insert(name_scheme_t("denman-rk3",DENMAN_RK3_UPDATE));
    available_schemes.insert(name_scheme_t("denman_rk3",DENMAN_RK3_UPDATE));
    return SUCCESS;
}

std::map<std::string,flux_calc_t> available_calculators;
int init_available_calculators_map()
{
    typedef std::pair<std::string,flux_calc_t> name_flux_t;
    available_calculators.insert(name_flux_t("0",FLUX_RIEMANN));
    available_calculators.insert(name_flux_t("riemann",FLUX_RIEMANN));
    available_calculators.insert(name_flux_t("1",FLUX_AUSM));
    available_calculators.insert(name_flux_t("ausm",FLUX_AUSM));
    available_calculators.insert(name_flux_t("2",FLUX_EFM));
    available_calculators.insert(name_flux_t("efm",FLUX_EFM));
    available_calculators.insert(name_flux_t("3",FLUX_AUSMDV));
    available_calculators.insert(name_flux_t("ausmdv",FLUX_AUSMDV));
    available_calculators.insert(name_flux_t("4",FLUX_ADAPTIVE));
    available_calculators.insert(name_flux_t("adaptive",FLUX_ADAPTIVE));
    available_calculators.insert(name_flux_t("5",FLUX_AUSM_PLUS_UP));
    available_calculators.insert(name_flux_t("ausm_plus_up",FLUX_AUSM_PLUS_UP));
    available_calculators.insert(name_flux_t("6",FLUX_HLLE));
    available_calculators.insert(name_flux_t("hlle",FLUX_HLLE));
    available_calculators.insert(name_flux_t("7",FLUX_HLLC));
    available_calculators.insert(name_flux_t("hllc",FLUX_HLLC));
    return SUCCESS;
}

std::map<std::string,thermo_interp_t> available_interpolators;
int init_available_interpolators_map()
{
    typedef std::pair<std::string,thermo_interp_t> name_interp_t;
    available_interpolators.insert(name_interp_t("pt",INTERP_PT));
    available_interpolators.insert(name_interp_t("rhoe",INTERP_RHOE));
    available_interpolators.insert(name_interp_t("rhop",INTERP_RHOP));
    available_interpolators.insert(name_interp_t("rhot",INTERP_RHOT));
    return SUCCESS;
}

std::map<std::string,turbulence_model_t> available_turbulence_models;
int init_available_turbulence_models_map()
{
    typedef std::pair<std::string,turbulence_model_t> name_turb_model_t;
    available_turbulence_models.insert(name_turb_model_t("none",TM_NONE));
    available_turbulence_models.insert(name_turb_model_t("baldwin_lomax",TM_BALDWIN_LOMAX));
    available_turbulence_models.insert(name_turb_model_t("baldwin-lomax",TM_BALDWIN_LOMAX));
    available_turbulence_models.insert(name_turb_model_t("k_omega",TM_K_OMEGA));
    available_turbulence_models.insert(name_turb_model_t("k-omega",TM_K_OMEGA));
    available_turbulence_models.insert(name_turb_model_t("spalart_allmaras",TM_SPALART_ALLMARAS));
    return SUCCESS;
}

std::map<std::string,bc_t> available_bcs;
int init_available_bcs_map()
{
    typedef std::pair<std::string,bc_t> name_bc_t;
    // We keep the integer values for backward compatibility.
    available_bcs.insert(name_bc_t("adjacent",ADJACENT));
    available_bcs.insert(name_bc_t("0",ADJACENT));
    available_bcs.insert(name_bc_t("sup_in",SUP_IN));
    available_bcs.insert(name_bc_t("1",SUP_IN));
    available_bcs.insert(name_bc_t("extrapolate_out",EXTRAPOLATE_OUT));
    available_bcs.insert(name_bc_t("2",EXTRAPOLATE_OUT));
    available_bcs.insert(name_bc_t("slip_wall",SLIP_WALL));
    available_bcs.insert(name_bc_t("3",SLIP_WALL));
    available_bcs.insert(name_bc_t("adiabatic",ADIABATIC));
    available_bcs.insert(name_bc_t("4",ADIABATIC));
    available_bcs.insert(name_bc_t("fixed_t",FIXED_T));
    available_bcs.insert(name_bc_t("5",FIXED_T));
    available_bcs.insert(name_bc_t("subsonic_in",SUBSONIC_IN));
    available_bcs.insert(name_bc_t("6",SUBSONIC_IN));
    available_bcs.insert(name_bc_t("subsonic_out",SUBSONIC_OUT));
    available_bcs.insert(name_bc_t("7",SUBSONIC_OUT));
    available_bcs.insert(name_bc_t("transient_uni",TRANSIENT_UNI));
    available_bcs.insert(name_bc_t("8",TRANSIENT_UNI));
    available_bcs.insert(name_bc_t("transient_prof",TRANSIENT_PROF));
    available_bcs.insert(name_bc_t("9",TRANSIENT_PROF));
    available_bcs.insert(name_bc_t("static_prof",STATIC_PROF));
    available_bcs.insert(name_bc_t("10",STATIC_PROF));
    available_bcs.insert(name_bc_t("fixed_p_out",FIXED_P_OUT));
    available_bcs.insert(name_bc_t("11",FIXED_P_OUT));
    available_bcs.insert(name_bc_t("transient_t_wall",TRANSIENT_T_WALL));
    available_bcs.insert(name_bc_t("13",TRANSIENT_T_WALL));
    available_bcs.insert(name_bc_t("surface_energy_balance",SEB));
    available_bcs.insert(name_bc_t("seb",SEB));
    available_bcs.insert(name_bc_t("15",SEB));
    available_bcs.insert(name_bc_t("user_defined",USER_DEFINED));
    available_bcs.insert(name_bc_t("16",USER_DEFINED));
    available_bcs.insert(name_bc_t("adjacent_plus_udf",ADJACENT_PLUS_UDF));
    available_bcs.insert(name_bc_t("17",ADJACENT_PLUS_UDF));
    available_bcs.insert(name_bc_t("ablating",ABLATING));
    available_bcs.insert(name_bc_t("18",ABLATING));
    available_bcs.insert(name_bc_t("sliding_t",SLIDING_T));
    available_bcs.insert(name_bc_t("19",SLIDING_T));
    available_bcs.insert(name_bc_t("fstc",FSTC));
    available_bcs.insert(name_bc_t("20",FSTC));
    available_bcs.insert(name_bc_t("shock_fitting_in",SHOCK_FITTING_IN));
    available_bcs.insert(name_bc_t("21",SHOCK_FITTING_IN));
    available_bcs.insert(name_bc_t("non_catalytic",NON_CATALYTIC));
    available_bcs.insert(name_bc_t("22",NON_CATALYTIC));
    available_bcs.insert(name_bc_t("equil_catalytic",EQUIL_CATALYTIC));
    available_bcs.insert(name_bc_t("23",EQUIL_CATALYTIC));
    available_bcs.insert(name_bc_t("super_catalytic",SUPER_CATALYTIC));
    available_bcs.insert(name_bc_t("24",SUPER_CATALYTIC));
    available_bcs.insert(name_bc_t("partially_catalytic",PARTIALLY_CATALYTIC));
    available_bcs.insert(name_bc_t("25",PARTIALLY_CATALYTIC));
    available_bcs.insert(name_bc_t("user_defined_mass_flux",USER_DEFINED_MASS_FLUX));
    available_bcs.insert(name_bc_t("user_defined_energy_flux",USER_DEFINED_ENERGY_FLUX));
    available_bcs.insert(name_bc_t("conjugate_ht",CONJUGATE_HT));
    available_bcs.insert(name_bc_t("moving_wall",MOVING_WALL));
    available_bcs.insert(name_bc_t("mass_flux_out",MASS_FLUX_OUT));
    available_bcs.insert(name_bc_t("mapped_cell",MAPPED_CELL));
    available_bcs.insert(name_bc_t("inlet_outlet",INLET_OUTLET));
    available_bcs.insert(name_bc_t("nonuniform_t",NONUNIFORM_T));
    available_bcs.insert(name_bc_t("jump_wall",JUMP_WALL));
    return SUCCESS;
}

std::map<std::string,cht_coupling_t> available_cht_coupling;
int init_available_cht_coupling_map()
{
    typedef std::pair<std::string,cht_coupling_t> name_cht_coupling_t;
    available_cht_coupling.insert(name_cht_coupling_t("TFS_TWS", TFS_TWS));
    available_cht_coupling.insert(name_cht_coupling_t("TFS_QWS", TFS_QWS));
    available_cht_coupling.insert(name_cht_coupling_t("QFS_TWS", QFS_TWS));
    available_cht_coupling.insert(name_cht_coupling_t("QFS_QWS", QFS_QWS));

    return SUCCESS;
}
 
/*-----------------------------------------------------------------*/

/// \brief Read simulation config parameters from the INI file.
///
/// These are grouped into global configuration parameters and
/// those which describe the blocks for holding the flow data.
///
/// \param filename : name of the INI parameter file
/// \param master: flag to indicate that this process is master
/// \param start_tindx : integer to indicate which time index to begin from
///
int read_config_parameters(const string filename, bool master, int start_tindx)
{
    global_data &G = *get_global_data_ptr();
    size_t jb;
    init_available_schemes_map();
    init_available_calculators_map();
    init_available_interpolators_map();
    init_available_turbulence_models_map();
    init_available_bcs_map();
    init_available_cht_coupling_map();

    // Default values for some configuration variables.
    G.dimensions = 2;
    G.Xorder = 2;
    G.radiation = false;
    G.adjust_invalid_cell_data = true;
    G.nghost = 2;

    // variables for Andrew's time averaging routine.
    G.nav = 0;
    G.tav_0 = 0.0;
    G.tav_f = 0.0;
    G.dtav = 0.0;
    G.tav = 0.0;
    if (G.do_tavg == 1) {
	G.tav = G.tav_0; /* time at which averaging is to commence */
	G.nav = 0;
    }
    
    // variables for profile recording.
    G.do_record = 0;
    G.block_record = 0;
    G.x_record = 0;
    G.n_record = 0;
    G.step_record = 0;
    G.step = 0;
    G.t_record = 0.0;
    G.t_start = 0.0;
    if ( G.do_record == 1 ) {
	if ( master ) printf( "\ndo_record = %d\n\n", G.do_record );
	G.n_record = 0;
       	G.t_start = 0;
    }

    // variables for time dependent profile input.
    G.do_vary_in = 0;
    G.t_old = 0.0;
    G.ta_old = 0.0;
    G.n_vary = 0;
    if ( G.do_vary_in == 1 ) {
	G.n_vary = 0;
	G.t_old = -100.0; /* something guaranteed to be smaller than the first time */
	G.ta_old = -101.0;
    }
    // error checking
    if ( G.do_vary_in == 1 && G.do_record == 1 ) {
	if ( master ) printf( "\n Cannot simultaneously write and read from profile file\n" );
	exit(BAD_INPUT_ERROR);
    }

    // these variables are all for special cases.
    G.dn_ruptured = 0;
    G.d2_ruptured = 0;
    G.secondary_diaphragm_ruptured = 0;
    /* defaults to no diaphragm (in block -5) */
    G.diaphragm_block = -1;
    G.diaphragm_rupture_time = 0.0;
    G.diaphragm_rupture_diameter = 0.0;
    G.drummond_progressive = 0;

    G.fixed_time_step = false; // Set to false as a default
    G.cfl_count = 10;
    G.print_count = 20;
    G.control_count = 10;

    G.separate_update_for_viscous_terms = false;
    G.viscous = false;
    G.viscous_factor = 1.0;
    G.viscous_factor_increment = 0.01;
    G.diffusion = false;
    G.diffusion_factor = 1.0;
    G.diffusion_factor_increment = 0.01;
    G.diffusion_lewis = 1.0;
    G.diffusion_schmidt = 0.7;

    G.heat_factor = 1.0;
    G.heat_factor_increment = 0.01;

    G.ignition_zone_active = false;

    G.electric_field_work = false;

    // At the start of a fresh simulation,
    // we need to set a few items that will be updated later.
    G.sim_time = 0.0;   // Global simulation time.
    G.cfl_max = 1.0;    // Dummy value
    G.cfl_min = 1.0;    // Dummy value
    G.cfl_tiny = 1.0;   // Smallest CFL so far, dummy value
    G.time_tiny = 1.0e6;

    G.turbulence_model = TM_NONE;
    G.turbulence_prandtl = 0.89;
    G.turbulence_schmidt = 0.75;
    G.separate_update_for_k_omega_source = false;

    // Daryl's MHD and BGK additions are, by default, off.
    G.MHD = false;
    G.BGK = 0;

    // MHD divergence cleaning
    G.div_clean = true;
    G.c_h = 1.0;
    G.divB_damping_length = 0.0; // default to zero, calc using domain bounds (main.cxx)
    G.bounding_box_min = 0.0;
    G.bounding_box_max = 1.0;
    

    // Most configuration comes from the previously-generated INI file.
    ConfigParser dict = ConfigParser(filename);
    int i_value;
    bool b_value;
    string s_value, s_value2;
    double d_value;

    dict.parse_string("global_data", "title", G.title, "unknown");
    dict.parse_size_t("global_data", "dimensions", G.dimensions, 2);
    dict.parse_size_t("global_data", "control_count", G.control_count, 10);
    if ( G.verbosity_level >= 2 ) {
	cout << "title = " << G.title << endl;
	cout << "dimensions = " << G.dimensions << endl;
	cout << "control_count = " << G.control_count << endl;
    }

    dict.parse_string("global_data", "udf_file", G.udf_file, "");
    dict.parse_int("global_data", "udf_source_vector_flag", G.udf_source_vector_flag, 0);
    dict.parse_int("global_data", "udf_vtx_velocity_flag", G.udf_vtx_velocity_flag, 0);

    dict.parse_string("global_data", "gas_model_file", s_value, "gas-model.lua");
    Gas_model *gmodel = set_gas_model_ptr(create_gas_model(s_value));
    if ( G.verbosity_level >= 2 ) {
	cout << "gas_model_file = " << s_value << endl;
	cout << "nsp = " << gmodel->get_number_of_species() << endl;
	cout << "nmodes = " << gmodel->get_number_of_modes() << endl;
    }

    dict.parse_boolean("global_data", "reacting_flag", G.reacting, false);
    if ( G.reacting and !gmodel->good_for_reactions() ) {
	throw runtime_error("The selected gas model is NOT compatible with finite-rate chemistry.");
    }
    dict.parse_double("global_data", "reaction_time_start", G.reaction_time_start, 0.0);
    dict.parse_double("global_data", "T_frozen", G.T_frozen, 300.0);
    dict.parse_string("global_data", "reaction_update", s_value, "dummy_scheme");
    if( G.reacting ) set_reaction_update( s_value );
    if ( G.verbosity_level >= 2 ) {
	cout << "reacting_flag = " << G.reacting << endl;
	cout << "reaction_time_start = " << G.reaction_time_start << endl;
	cout << "reaction_update = " << s_value << endl;
	cout << "T_frozen = " << G.T_frozen << endl;
    }

    dict.parse_boolean("global_data", "energy_exchange_flag", G.thermal_energy_exchange, false);
    dict.parse_string("global_data", "energy_exchange_update", s_value, "dummy_scheme");
    if( G.thermal_energy_exchange ) set_energy_exchange_update(s_value);
    dict.parse_double("global_data", "T_frozen_energy", G.T_frozen_energy, 300.0);
    dict.parse_boolean("global_data", "electric_field_work_flag", G.electric_field_work, false);
    if( G.verbosity_level >= 2 ) {
	cout << "energy_exchange_flag = " << G.thermal_energy_exchange << endl;
	cout << "energy_exchange_update = " << s_value << endl;
	cout << "T_frozen_energy = " << G.T_frozen_energy << endl;
	cout << "electric_field_work_flag = " << G.electric_field_work << endl;
    }

    dict.parse_boolean("global_data", "mhd_flag", G.MHD, false);
    dict.parse_boolean("global_data", "div_clean_flag", G.div_clean, true);
    dict.parse_double("global_data", "divB_damping_length", G.divB_damping_length, 0.0);

    dict.parse_int("global_data", "BGK_flag", G.BGK, 0);
    if ( G.BGK > 0) {
	dict.parse_int("global_data", "velocity_buckets", i_value, 0);
	set_velocity_buckets( i_value );
	if (get_velocity_buckets() > 0) {
	    std::vector<Vector3> *vct = get_vcoords_ptr();
	    std::vector<double> tmp;
	    dict.parse_vector_of_doubles("global_data", "vcoords_x", tmp, tmp);
	    for (size_t tid = 0; tid < get_velocity_buckets(); ++tid) {
		(*vct)[tid].x = tmp[tid];
	    }
	    tmp.resize(0);
	    dict.parse_vector_of_doubles("global_data", "vcoords_y", tmp, tmp);
	    for (size_t tid = 0; tid < get_velocity_buckets(); ++tid) {
		(*vct)[tid].y = tmp[tid];
	    }
	    tmp.resize(0);
	    dict.parse_vector_of_doubles("global_data", "vcoords_z", tmp, tmp);
	    for (size_t tid = 0; tid < get_velocity_buckets(); ++tid) {
		(*vct)[tid].z = tmp[tid];
	    }
	    std::vector<double> *vwt = get_vweights_ptr();
	    dict.parse_vector_of_doubles("global_data", "vweights", tmp, tmp);
	    for (size_t tid = 0; tid < get_velocity_buckets(); ++tid) {
		(*vwt)[tid] = tmp[tid];
	    }
	} else {
	    cout << "Failure setting BGK velocities." << endl;
	    return FAILURE;
	}
    } // end if ( G.BGK )

    dict.parse_boolean("global_data", "radiation_flag", G.radiation, false);
    dict.parse_string("global_data", "radiation_input_file", s_value, "no_file");
    // radiation_update_frequency in .control file.
    if( G.radiation ) {
    	set_radiation_transport_model( s_value );
    }
    if ( G.verbosity_level >= 2 ) {
	cout << "radiation_flag = " << G.radiation << endl;
	cout << "radiation_input_file = " << s_value << endl;
    }

    dict.parse_boolean("global_data", "axisymmetric_flag", G.axisymmetric, false);
    if ( G.verbosity_level >= 2 ) {
	cout << "axisymmetric_flag = " << G.axisymmetric << endl;
    }

    dict.parse_boolean("global_data", "viscous_flag", G.viscous, false);
    dict.parse_double("global_data", "viscous_delay", G.viscous_time_delay, 0.0);
    dict.parse_double("global_data", "viscous_factor_increment", G.viscous_factor_increment, 0.01);
    // FIX-ME 2013-04-23 should probably merge diffusion_model and diffusion_flag
    // as we have done for turbulence_model, below.
    dict.parse_boolean("global_data", "diffusion_flag", G.diffusion, false);
    dict.parse_double("global_data", "diffusion_delay", G.diffusion_time_delay, 0.0);
    dict.parse_double("global_data", "diffusion_factor_increment", G.diffusion_factor_increment, 0.01);
    dict.parse_double("global_data", "diffusion_lewis_number", G.diffusion_lewis, 1.0);
    dict.parse_double("global_data", "diffusion_schmidt_number", G.diffusion_schmidt, 0.7);
    if ( G.verbosity_level >= 2 ) {
	cout << "viscous_flag = " << G.viscous << endl;
	cout << "viscous_delay = " << G.viscous_time_delay << endl;
	cout << "viscous_factor_increment = " << G.viscous_factor_increment << endl;
	cout << "diffusion_flag = " << G.diffusion << endl;
	cout << "diffusion_delay = " << G.diffusion_time_delay << endl;
	cout << "diffusion_factor_increment = " << G.diffusion_factor_increment << endl;
	cout << "diffusion_lewis = " << G.diffusion_lewis << endl;
    }
    if( G.diffusion && (gmodel->get_number_of_species() > 1) ) { 
 	dict.parse_string("global_data", "diffusion_model", s_value, "Stefan-Maxwell");
	set_diffusion_model(s_value);
	if( G.verbosity_level >= 2 ) {
	    cout << "diffusion_model = " << s_value << endl;
	}
    }

    dict.parse_boolean("global_data", "shock_fitting_flag", G.shock_fitting, false);
    dict.parse_boolean("global_data", "shock_fitting_decay_flag", G.shock_fitting_decay, false);
    dict.parse_double("global_data", "shock_fitting_speed_factor", G.shock_fitting_speed_factor, 1.0);
    dict.parse_boolean("global_data", "moving_grid_flag", G.moving_grid, false);
    dict.parse_boolean("global_data", "write_vertex_velocities_flag", G.write_vertex_velocities, false);
    dict.parse_boolean("global_data", "flow_induced_moving_flag", G.flow_induced_moving, false);    
    dict.parse_boolean("global_data", "wall_function_flag", G.wall_function, false);
    dict.parse_boolean("global_data", "artificial_diffusion_flag", G.artificial_diffusion, false);        
    dict.parse_double("global_data", "artificial_kappa_2", G.artificial_kappa_2, 0.0);
    dict.parse_double("global_data", "artificial_kappa_4", G.artificial_kappa_4, 0.0);            
    if ( G.verbosity_level >= 2 ) {
	cout << "shock_fitting_flag = " << G.shock_fitting << endl;
	cout << "shock_fitting_decay_flag = " << G.shock_fitting_decay << endl;
	cout << "shock_fitting_speed_factor = " << G.shock_fitting_speed_factor << endl;
	cout << "moving_grid_flag = " << G.moving_grid << endl;
	cout << "write_vertex_velocities_flag = " << G.write_vertex_velocities << endl;
	cout << "flow_induced_moving_flag = " << G.flow_induced_moving << endl;	
	cout << "wall_function_flag = " << G.wall_function << endl;
	cout << "artificial_diffusion_flag = " << G.artificial_diffusion << endl;			
	cout << "artificial_kappa_2 = " << G.artificial_kappa_2 << endl;
	cout << "artificial_kappa_4 = " << G.artificial_kappa_4 << endl;	
    }

    // 2013-apr-23 New specification scheme for turbulence models.
    dict.parse_string("global_data", "turbulence_model", s_value, "none");
    s_value = to_lower_case(s_value);
    if ( available_turbulence_models.find(s_value) == available_turbulence_models.end() ) {
	throw std::runtime_error(std::string("Requested turbulence model not available: ") + s_value);
    }
    G.turbulence_model = available_turbulence_models[s_value];
    dict.parse_double("global_data", "turbulence_prandtl_number", d_value, 0.89);
    G.turbulence_prandtl = d_value;
    dict.parse_double("global_data", "turbulence_schmidt_number", d_value, 0.75);
    G.turbulence_schmidt = d_value;
    dict.parse_double("global_data", "max_mu_t_factor", G.max_mu_t_factor, 300.0);
    dict.parse_double("global_data", "transient_mu_t_factor", G.transient_mu_t_factor, 1.0);
    dict.parse_boolean("global_data", "separate_update_for_k_omega_source",
		       G.separate_update_for_k_omega_source, false);
    if ( G.turbulence_model == TM_SPALART_ALLMARAS )
	throw std::runtime_error("Spalart-Allmaras turbulence model not available.");
    if ( G.verbosity_level >= 2 ) {
	cout << "turbulence_model = " << get_name_of_turbulence_model(G.turbulence_model) << endl;
	cout << "turbulence_prandtl_number = " << G.turbulence_prandtl << endl;
	cout << "turbulence_schmidt_number = " << G.turbulence_schmidt << endl;
	cout << "max_mu_t_factor = " << G.max_mu_t_factor << endl;
	cout << "transient_mu_t_factor = " << G.transient_mu_t_factor << endl;
	cout << "separate_update_for_k_omega_source = " << G.separate_update_for_k_omega_source << endl;
    }

    dict.parse_size_t("global_data", "max_invalid_cells", G.max_invalid_cells, 10);
    dict.parse_string("global_data", "flux_calc", s_value, "adaptive");
    s_value = to_lower_case(s_value);
    if ( available_calculators.find(s_value) == available_calculators.end() ) {
	throw std::runtime_error(std::string("Requested flux calculator not available: ") + s_value);
    }
    set_flux_calculator(available_calculators[s_value]);
    dict.parse_double("global_data", "compression_tolerance", G.compression_tolerance, -0.30);
    dict.parse_double("global_data", "shear_tolerance", G.shear_tolerance, 0.20);
    dict.parse_double("global_data", "M_inf", d_value, 0.01);
    set_M_inf(d_value);
    dict.parse_string("global_data", "interpolation_type", s_value, "rhoe");
    s_value = to_lower_case(s_value);
    if ( available_interpolators.find(s_value) == available_interpolators.end() ) {
	throw std::runtime_error(std::string("Requested field interpolator not available: ") + s_value);
    }
    set_thermo_interpolator(available_interpolators[s_value]);
    dict.parse_boolean("global_data", "interpolate_in_local_frame", b_value, true);
    set_interpolate_in_local_frame_flag(b_value);
    dict.parse_boolean("global_data", "apply_limiter_flag", b_value, true);
    set_apply_limiter_flag(b_value);
    dict.parse_boolean("global_data", "extrema_clipping_flag", b_value, true);
    set_extrema_clipping_flag(b_value);
    dict.parse_double("global_data", "overshoot_factor", d_value, 1.0);
    set_overshoot_factor(d_value);
    if ( G.verbosity_level >= 2 ) {
	cout << "max_invalid_cells = " << G.max_invalid_cells << endl;
	cout << "flux_calc = " << get_flux_calculator_name(get_flux_calculator()) << endl;
	cout << "compression_tolerance = " << G.compression_tolerance << endl;
	cout << "shear_tolerance = " << G.shear_tolerance << endl;
	cout << "M_inf = " << get_M_inf() << endl;
	cout << "interpolation_type = " << get_thermo_interpolator_name(get_thermo_interpolator()) << endl;
	cout << "interpolate_in_local_frame = " << get_interpolate_in_local_frame_flag() << endl;
	cout << "apply_limiter_flag = " << get_apply_limiter_flag() << endl;
	cout << "extrema_clipping_flag = " << get_extrema_clipping_flag() << endl;
	cout << "overshoot_factor = " << get_overshoot_factor() << endl;
    }

    dict.parse_boolean("global_data", "filter_flag", G.filter_flag, false);
    dict.parse_double("global_data", "filter_tstart", G.filter_tstart, 0.0);
    dict.parse_double("global_data", "filter_tend", G.filter_tend, 0.0);
    dict.parse_double("global_data", "filter_dt", G.filter_dt, 0.0);
    dict.parse_double("global_data", "filter_mu", G.filter_mu, 0.0);
    dict.parse_size_t("global_data", "filter_npass", G.filter_npass, 0);

    dict.parse_int("global_data", "sequence_blocks", i_value, 0);
    G.sequence_blocks = (i_value == 1);
    if ( G.verbosity_level >= 2 ) {
	cout << "sequence_blocks = " << G.sequence_blocks << endl;
    }

    // Read a number of gas-states.
    dict.parse_size_t("global_data", "nflow", G.n_gas_state, 0);
    if ( G.verbosity_level >= 2 ) {
	cout << "nflow = " << G.n_gas_state << endl;
    }
    for ( size_t ig = 0; ig < G.n_gas_state; ++ig ) {
	G.gas_state.push_back(read_flow_condition_from_ini_dict(dict, ig, master));
	if ( G.verbosity_level >= 2 ) {
	    cout << "flow condition[" << ig << "]: " << *(G.gas_state[ig]) << endl;
	}
    }

    // Read the parameters for a number of blocks.
    dict.parse_size_t("global_data", "nblock", G.nblock, 0);
    if ( G.verbosity_level >= 2 ) {
	printf( "nblock = %d\n", static_cast<int>(G.nblock));
    }
    // We keep a record of all of the configuration data for all blocks
    // but, eventually, we may allocate the flow-field data for only a 
    // subset of these blocks. 
    G.bd.resize(G.nblock);

    // Number of pistons
    // FIX-ME code needs to be reworked...
    dict.parse_size_t("global_data", "npiston", G.npiston, 0);
#   ifdef _MPI
    if ( G.npiston > 0 ) {
	G.npiston = 0;
	cout << "Pistons cannot be used with MPI code, yet." << endl;
    }
#   endif
    G.pistons.resize(G.npiston);
    if ( G.verbosity_level >= 2 ) {
	printf( "npiston = %d\n", static_cast<int>(G.npiston));
    }
    for ( size_t jp = 0; jp < G.npiston; ++jp ) {
	string piston_label;
	bool piston_cvf, piston_pvf;
	double piston_D, piston_L, piston_m, piston_x0, piston_v0, piston_f;
	string section = "piston/" + tostring(jp);
	dict.parse_string(section, "label", piston_label, "");
	dict.parse_double(section, "D", piston_D, 0.0);
	dict.parse_double(section, "L", piston_L, 0.0);
	dict.parse_double(section, "m", piston_m, 0.0);
	dict.parse_double(section, "x0", piston_x0, 0.0);
	dict.parse_double(section, "v0", piston_v0, 0.0);
	dict.parse_double(section, "f", piston_f, 0.0);
	dict.parse_boolean(section, "const_v_flag", piston_cvf, false);
	dict.parse_boolean(section, "postv_v_flag", piston_pvf, false);
	if ( G.verbosity_level >= 2 ) {
	    cout << "piston/" << jp << ": label= " << piston_label << endl;
	    cout << "    L=" << piston_L << ", m=" << piston_m
		 << ", D=" << piston_D << ", x0=" << piston_x0
		 << ", v0=" << piston_v0
		 << ", f=" << piston_f << endl;
	}
	
	G.pistons[jp] = new Piston();
	
	// This variable is used as a default to set the
	// vanishing distance.  We hope nobody ever simulates
	// something of this dimension.
	static const double VERY_LARGE_X = 1.0e6; // m

	// one way of making defaults
	vector<double> bore_resistance_f, bore_resistance_x;
	bore_resistance_x.push_back(0.0);
	bore_resistance_f.push_back(piston_f);
	double rifling_twist = 0.0;
	double rog = 0.0;
	double vanish_at_x = VERY_LARGE_X;
	cout << "Attempting to set piston values from config file..." << endl;
	int status = G.pistons[jp]->set_values(jp, piston_D, piston_L, piston_m, 
					       piston_x0, piston_v0, 
					       rifling_twist, rog, vanish_at_x, 
					       bore_resistance_f, bore_resistance_x);
	G.pistons[jp]->set_const_v_flag(piston_cvf);
	G.pistons[jp]->set_postv_v_flag(piston_pvf);
	if (status != SUCCESS) {
	    cout << "Failure setting piston parameters." << endl;
	    return FAILURE;
	} else {
	    cout << "Success setting piston parameters." << endl;
	}
    }

    dict.parse_size_t("global_data", "nheatzone", G.n_heat_zone, 0);
    dict.parse_double("global_data", "heat_time_start", G.heat_time_start, 0.0);
    dict.parse_double("global_data", "heat_time_stop", G.heat_time_stop, 0.0);
    dict.parse_double("global_data", "heat_factor_increment", G.heat_factor_increment, 0.01);
    if ( G.verbosity_level >= 2 ) {
	printf("nheatzone = %d\n", static_cast<int>(G.n_heat_zone));
	printf("heat_time_start = %e\n", G.heat_time_start);
	printf("heat_time_stop = %e\n", G.heat_time_stop);
	printf("heat_factor_increment = %e\n", G.heat_factor_increment);
    }
    G.heat_zone.resize(G.n_heat_zone);
    for ( size_t indx = 0; indx < G.n_heat_zone; ++indx ) {
	struct CHeatZone* hzp = &(G.heat_zone[indx]);
	string section = "heat_zone/" + tostring(indx);
	dict.parse_double(section, "qdot", hzp->qdot, 0.0);
	dict.parse_double(section, "x0", hzp->x0, 0.0);
	dict.parse_double(section, "y0", hzp->y0, 0.0);
	dict.parse_double(section, "z0", hzp->z0, 0.0);
	dict.parse_double(section, "x1", hzp->x1, 0.0);
	dict.parse_double(section, "y1", hzp->y1, 0.0);
	dict.parse_double(section, "z1", hzp->z1, 0.0);
	if ( G.verbosity_level >= 2 ) {
	    cout << "heat_zone/" << indx << " qdot= " << hzp->qdot << endl;
	    cout << "    point0= " << hzp->x0 << " " << hzp->y0 << " " << hzp->z0 << endl;
	    cout << "    point1= " << hzp->x1 << " " << hzp->y1 << " " << hzp->z1 << endl;
	}
    }

    dict.parse_size_t("global_data", "nignitionzone", G.n_ignition_zone, 0);
    dict.parse_double("global_data", "ignition_time_start", G.ignition_time_start, 0.0);
    dict.parse_double("global_data", "ignition_time_stop", G.ignition_time_stop, 0.0);
    if ( G.verbosity_level >= 2 ) {
	printf("nignitionzone = %d\n", static_cast<int>(G.n_ignition_zone));
	printf("ignition_time_start = %e\n", G.ignition_time_start);
	printf("ignition_time_stop = %e\n", G.ignition_time_stop);
    }
    G.ignition_zone.resize(G.n_ignition_zone);
    for ( size_t indx = 0; indx < G.n_ignition_zone; ++indx ) {
	struct CIgnitionZone* izp = &(G.ignition_zone[indx]);
	string section = "ignition_zone/" + tostring(indx);
	dict.parse_double(section, "Tig", izp->Tig, 0.0);
	dict.parse_double(section, "x0", izp->x0, 0.0);
	dict.parse_double(section, "y0", izp->y0, 0.0);
	dict.parse_double(section, "z0", izp->z0, 0.0);
	dict.parse_double(section, "x1", izp->x1, 0.0);
	dict.parse_double(section, "y1", izp->y1, 0.0);
	dict.parse_double(section, "z1", izp->z1, 0.0);
	if ( G.verbosity_level >= 2 ) {
	    cout << "ignition_zone/" << indx << " Tig= " << izp->Tig << endl;
	    cout << "    point0= " << izp->x0 << " " << izp->y0 << " " << izp->z0 << endl;
	    cout << "    point1= " << izp->x1 << " " << izp->y1 << " " << izp->z1 << endl;
	}
    }

    dict.parse_size_t("global_data", "nreactionzone", G.n_reaction_zone, 0);
    if ( G.verbosity_level >= 2 ) {
	printf("nreactionzone = %d\n", static_cast<int>(G.n_reaction_zone));
    }
    G.reaction_zone.resize(G.n_reaction_zone);
    for ( size_t indx = 0; indx < G.n_reaction_zone; ++indx ) {
	struct CReactionZone* rzp = &(G.reaction_zone[indx]);
	string section = "reaction_zone/" + tostring(indx);
	dict.parse_double(section, "x0", rzp->x0, 0.0);
	dict.parse_double(section, "y0", rzp->y0, 0.0);
	dict.parse_double(section, "z0", rzp->z0, 0.0);
	dict.parse_double(section, "x1", rzp->x1, 0.0);
	dict.parse_double(section, "y1", rzp->y1, 0.0);
	dict.parse_double(section, "z1", rzp->z1, 0.0);
	if ( G.verbosity_level >= 2 ) {
	    cout << "reaction_zone/" << indx << endl;
	    cout << "    point0= " << rzp->x0 << " " << rzp->y0 << " " << rzp->z0 << endl;
	    cout << "    point1= " << rzp->x1 << " " << rzp->y1 << " " << rzp->z1 << endl;
	}
    }

    dict.parse_size_t("global_data", "nturbulencezone", G.n_turbulent_zone, 0);
    if ( G.verbosity_level >= 2 ) {
	printf("nturbulencezone = %d\n", static_cast<int>(G.n_turbulent_zone));
    }
    G.turbulent_zone.resize(G.n_turbulent_zone);
    for ( size_t indx = 0; indx < G.n_turbulent_zone; ++indx ) {
	struct CTurbulentZone* tzp = &(G.turbulent_zone[indx]);
	string section = "turbulence_zone/" + tostring(indx);
	dict.parse_double(section, "x0", tzp->x0, 0.0);
	dict.parse_double(section, "y0", tzp->y0, 0.0);
	dict.parse_double(section, "z0", tzp->z0, 0.0);
	dict.parse_double(section, "x1", tzp->x1, 0.0);
	dict.parse_double(section, "y1", tzp->y1, 0.0);
	dict.parse_double(section, "z1", tzp->z1, 0.0);
	if ( G.verbosity_level >= 2 ) {
	    cout << "turbulence_zone/" << indx << endl;
	    cout << "    point0= " << tzp->x0 << " " << tzp->y0 << " " << tzp->z0 << endl;
	    cout << "    point1= " << tzp->x1 << " " << tzp->y1 << " " << tzp->z1 << endl;
	}
    }

    dict.parse_int("global_data", "conjugate_ht_flag", i_value, 0);
    G.conjugate_ht_active = i_value;
    dict.parse_string("global_data", "conjugate_ht_coupling", s_value, "QFS_QWS");
    if ( available_cht_coupling.find(s_value) == available_cht_coupling.end() ) {
	throw std::runtime_error(std::string("Requested cht_coupling not available: ") + s_value);
    }
    G.cht_coupling = available_cht_coupling[s_value];
    dict.parse_string("global_data", "conjugate_ht_file", s_value, "dummy_ht_file");
    if ( G.conjugate_ht_active ) {
	if ( !G.viscous ) {
	    cout << "WARNING: Conjugate heat transfer is active\n";
	    cout << "WARNING: but the viscous flag is not set.\n";
	    cout << "WARNING: No heat fluxes will be computed at wall.\n";
	}
    	G.wm = initialise_wall_model(s_value, G.cht_coupling, start_tindx);
    }
    if ( G.verbosity_level >= 2 ) {
	cout << "conjugate_ht_flag = " << G.conjugate_ht_active << endl;
	cout << "conjugate_ht_coupling = " << G.cht_coupling << endl;
	cout << "conjugate_ht_file = " << s_value << endl;
    }
    // Now, for the individual block configuration.
    for ( jb = 0; jb < G.nblock; ++jb ) {
        set_block_parameters( jb, dict, master );
    }
    check_connectivity();

    return SUCCESS;
} // end read_config_parameters()


int read_control_parameters( const string filename, bool master, bool first_time )
// These are read at the start of every time-step and may be used 
// to alter the simulation behaviour during a run.
{
    int i_value;
    std::string s_value;
    global_data &G = *get_global_data_ptr();
    // Parse the previously-generated INI file.
    ConfigParser dict = ConfigParser(filename);

    dict.parse_int("control_data", "x_order", G.Xorder, 2); // default high-order
    // 2013-03-31 change to use an explicitly-named update scheme.
    dict.parse_string("control_data", "gasdynamic_update_scheme", s_value,
		      "predictor-corrector");
    s_value = to_lower_case(s_value);
    if ( available_schemes.find(s_value) == available_schemes.end() ) {
	throw std::runtime_error(std::string("Requested update-scheme not available: ") + s_value);
    }
    set_gasdynamic_update_scheme(available_schemes[s_value]);
    if ( G.conjugate_ht_active ) {
	// Translate our enumerated schemes to Justin's integers
	switch ( available_schemes[s_value] ) {
	case EULER_UPDATE: set_wallcon_time_update_scheme(*(G.wm), 0); break;
	case PC_UPDATE: set_wallcon_time_update_scheme(*(G.wm), 1); break;
	    // For all higher-order timestepping, just ask wallcon
	    // to use predictor-corrector
	default: set_wallcon_time_update_scheme(*(G.wm), 1);
	} 
    }
    // To keep backward compatibility with old simulation files,
    // read Torder if it exists and set the equivalent update scheme.
    dict.parse_int("control_data", "t_order", i_value, 0);
    switch ( i_value ) {
    case 1: set_gasdynamic_update_scheme(available_schemes["euler"]); break;
    case 2: set_gasdynamic_update_scheme(available_schemes["predictor-corrector"]); break;
    case 3: set_gasdynamic_update_scheme(available_schemes["denman-rk3"]); break;
    default: /* do nothing */;
    }
    dict.parse_boolean("control_data", "separate_update_for_viscous_flag", 
		       G.separate_update_for_viscous_terms, false);
    dict.parse_double("control_data", "dt", G.dt_init, 1.0e-6);
    if ( first_time ) G.dt_global = G.dt_init;
    dict.parse_double("control_data", "dt_max", G.dt_max, 1.0e-3);
    dict.parse_boolean("control_data", "fixed_time_step", G.fixed_time_step, 0);
    dict.parse_double("control_data", "dt_reduction_factor",
		      G.dt_reduction_factor, 0.2);
    dict.parse_double("control_data", "cfl", G.cfl_target, 0.5);
    dict.parse_double("control_data", "viscous_signal_factor", G.viscous_signal_factor, 1.0);
    dict.parse_boolean("control_data", "stringent_cfl", G.stringent_cfl, false);
    dict.parse_size_t("control_data", "print_count", G.print_count, 20);
    dict.parse_size_t("control_data", "cfl_count", G.cfl_count, 10);
    dict.parse_double("control_data", "dt_moving", G.dt_moving, 1.0e-3);
    dict.parse_double("control_data", "dt_plot", G.dt_plot, 1.0e-3);
    dict.parse_size_t("control_data", "write_at_step", G.write_at_step, 0);
    dict.parse_double("control_data", "dt_history", G.dt_his, 1.0e-3);
    dict.parse_double("control_data", "dt_fstc", G.dt_fstc, 1.0e-3);
    dict.parse_double("control_data", "max_time", G.max_time, 1.0e-3);
    dict.parse_size_t("control_data", "max_step", G.max_step, 10);
    dict.parse_int("control_data", "halt_now", G.halt_now, 0);
    dict.parse_int("control_data", "implicit_flag", G.implicit_mode, 0);
    dict.parse_double("control_data", "cfl_moving", G.cfl_moving_target, 20.0);    
    G.implicit_mode = i_value; // FIX-ME PJ We'll replace this with a type map soon.
    dict.parse_size_t("control_data", "wall_update_count", G.wall_update_count, 1);
    dict.parse_int("control_data", "radiation_update_frequency", G.radiation_update_frequency, 1);
    if ( G.radiation_update_frequency < 0 ) {
	throw runtime_error("ERROR: radiation_update_frequency needs to be larger than or equal to 0.");
    }
    dict.parse_double("control_data", "tolerance_in_T", G.tolerance_in_T, 100.0);
    dict.parse_boolean("control_data", "halt_on_large_flow_change", G.halt_on_large_flow_change, false);   
    if ( first_time && G.verbosity_level >= 2 ) {
	cout << "Time-step control parameters:" << endl;
	cout << "    x_order = " << G.Xorder << endl;
	cout << "    gasdynamic_update_scheme = " 
	     << get_name_of_gasdynamic_update_scheme(get_gasdynamic_update_scheme())
	     << endl;
	cout << "separate_update_for_viscous_terms = " 
	     << G.separate_update_for_viscous_terms << endl;
	cout << "    dt = " << G.dt_init << endl;
	cout << "    dt_max = " << G.dt_max << endl;
	cout << "    fixed_time_step = " << G.fixed_time_step << endl;
	cout << "    dt_reduction_factor = " 
	     << G.dt_reduction_factor << endl;
	cout << "    cfl = " << G.cfl_target << endl;
	cout << "    stringent_cfl = " << G.stringent_cfl << endl;
	cout << "    print_count = " << G.print_count << endl;
	cout << "    cfl_count = " << G.cfl_count << endl;
	cout << "    dt_plot = " << G.dt_plot << endl;
	cout << "    write_at_step = " << G.write_at_step << endl;
	cout << "    dt_moving = " << G.dt_moving << endl;
	cout << "    dt_history = " << G.dt_his << endl;
	cout << "    dt_fstc = " << G.dt_fstc << endl;
	cout << "    max_time = " << G.max_time << endl;
	cout << "    max_step = " << G.max_step << endl;
	cout << "    halt_now = " << G.halt_now << endl;
	cout << "    halt_now = " << G.halt_now << endl;
	cout << "    radiation_update_frequency = " << G.radiation_update_frequency << endl;
	cout << "    wall_update_count = " << G.wall_update_count << endl;
	cout << "    halt_on_large_flow_change = " << G.halt_on_large_flow_change << endl;
	cout << "    tolerance_in_T = " << G.tolerance_in_T << endl;
	cout << "    implicit flag value: " << G.implicit_mode;
	switch ( G.implicit_mode ) {
	case 0: cout << " (Explicit viscous advancements)" << endl; break;
	case 1: cout << " (Point implicit viscous advancements)" << endl; break;
	case 2: cout << " (Fully implicit viscous advancements)" << endl; break;
	default: 
	    throw runtime_error("ERROR: invalid implicit flag was specified.");
	}
    }
    return SUCCESS;
} // end read_control_parameters()


/// \brief Read simulation config parameters from the INI file.
///
/// At this point, we know the number of blocks in the calculation.
/// Depending on whether we are running all blocks in the one process
/// or we are running a subset of blocks in this process, talking to
/// the other processes via MPI, we need to decide what blocks belong
/// to the current process.
///
/// \param filename : name of the INI file containing the mapping
/// \param master: flag to indicate that this process is master
///
int assign_blocks_to_mpi_rank(const string filename, bool master)
{
    global_data &G = *get_global_data_ptr();
    if ( G.verbosity_level >= 2 && master ) printf("Assign blocks to processes:\n");
    if ( G.mpi_parallel ) {
	if ( filename.size() > 0 ) {
	    if ( G.verbosity_level >= 2 && master ) {
		printf("    MPI parallel, mpimap filename=%s\n", filename.c_str());
		printf("    Assigning specific blocks to specific MPI processes.\n");
	    }
	    G.mpi_rank_for_block.resize(G.nblock);
	    // The mapping comes from the previously-generated INI file.
	    ConfigParser dict = ConfigParser(filename);
	    size_t nrank = 0;
	    size_t nblock;
	    size_t nblock_total = 0;
	    std::vector<int> block_ids, dummy_block_ids;
	    dict.parse_size_t("global", "nrank", nrank, 0);
	    if ( G.num_mpi_proc != static_cast<int>(nrank) ) {
		if ( master ) {
		    printf("    Error in specifying mpirun -np\n");
		    printf("    It needs to match number of nrank; present values are:\n");
		    printf("    num_mpi_proc= %d nrank= %d\n", G.num_mpi_proc,
			   static_cast<int>(nrank));
		}
		return FAILURE;
	    }
	    for ( size_t rank=0; rank < nrank; ++rank ) {
		string section = "rank/" + tostring(rank);
		dict.parse_size_t(section, "nblock", nblock, 0);
		block_ids.resize(0);
		dummy_block_ids.resize(nblock);
		for ( size_t i = 0; i < nblock; ++i ) dummy_block_ids[i] = -1;
		dict.parse_vector_of_ints(section, "blocks", block_ids, dummy_block_ids);
		if ( nblock != block_ids.size() ) {
		    if ( master ) {
			printf("    Did not pick up correct number of block_ids:\n");
			printf("        rank=%d, nblock=%d, block_ids.size()=%d\n",
			       static_cast<int>(rank), static_cast<int>(nblock),
			       static_cast<int>(block_ids.size()));
		    }
		    return FAILURE;
		}
		for ( size_t i = 0; i < nblock; ++i ) {
		    int this_block_id = block_ids[i];
		    if ( this_block_id < 0 ) {
			if ( master ) printf("    Error, invalid block id: %d\n", this_block_id);
			return FAILURE;
		    }
		    if ( G.my_mpi_rank == static_cast<int>(rank) )
			G.my_blocks.push_back(&(G.bd[this_block_id]));
		    G.mpi_rank_for_block[this_block_id] = static_cast<int>(rank);
		    nblock_total += 1;
		} // end for i
	    } // end for rank
	    if ( G.verbosity_level >= 2 ) {
		printf("    my_rank=%d, block_ids=", static_cast<int>(G.my_mpi_rank));
		for ( size_t i=0; i < G.my_blocks.size(); ++i ) {
		    printf(" %d", static_cast<int>(G.my_blocks[i]->id));
		}
		printf("\n");
	    }
	    if ( master ) {
		if ( nblock_total != G.nblock ) {
		    printf("    Error, total number of blocks incorrect: total=%d G.nblock=%d\n",
			   static_cast<int>(nblock_total), static_cast<int>(G.nblock));
		    return FAILURE;
		}
	    }
	} else {
	    if ( G.verbosity_level >= 2 && master ) {
		printf("    MPI parallel, No MPI map file specified.\n");
		printf("    Identify each block with the corresponding MPI rank.\n");
	    }
	    if ( G.num_mpi_proc != static_cast<int>(G.nblock) ) {
		if ( master ) {
		    printf("    Error in specifying mpirun -np\n");
		    printf("    It needs to match number of blocks; present values are:\n");
		    printf("    num_mpi_proc= %d nblock= %d\n", 
			   static_cast<int>(G.num_mpi_proc), static_cast<int>(G.nblock));
		}
		return FAILURE;
	    }
	    G.my_blocks.push_back(&(G.bd[G.my_mpi_rank]));
	    for ( size_t jb=0; jb < G.nblock; ++jb ) {
		G.mpi_rank_for_block.push_back(jb);
	    }
	}
    } else {
	if ( G.verbosity_level >= 2 ) {
	    printf("    Since we are not doing MPI, all blocks in same process.\n");
	}
	for ( size_t jb=0; jb < G.nblock; ++jb ) {
	    G.my_blocks.push_back(&(G.bd[jb]));
	    G.mpi_rank_for_block.push_back(G.my_mpi_rank);
	} 
    } // endif
    return SUCCESS;
} // end assign_blocks_to_mpi_rank()


/** \brief Use config_parser functions to read the flow condition data. 
 */
CFlowCondition *read_flow_condition_from_ini_dict(ConfigParser &dict, size_t indx, bool master)
{
    double p, u, v, w, Bx, By, Bz, mu_t, k_t, tke, omega, sigma_T, sigma_c;
    string value_string, flow_label;
    std::vector<double> massf, T, vnf;
    CFlowCondition *cfcp;
    Gas_model *gmodel = get_gas_model_ptr();

    string section = "flow/" + tostring(indx);
    dict.parse_string(section, "label", flow_label, "");
    dict.parse_double(section, "p", p, 100.0e3);
    dict.parse_double(section, "u", u, 0.0);
    dict.parse_double(section, "v", v, 0.0);
    dict.parse_double(section, "w", w, 0.0);
    dict.parse_double(section, "Bx", Bx, 0.0);
    dict.parse_double(section, "By", By, 0.0);
    dict.parse_double(section, "Bz", Bz, 0.0);
    dict.parse_double(section, "mu_t", mu_t, 0.0);
    dict.parse_double(section, "k_t", k_t, 0.0);
    dict.parse_double(section, "tke", tke, 0.0);
    dict.parse_double(section, "omega", omega, 1.0);
    dict.parse_double(section, "sigma_T", sigma_T, 0.0);
    dict.parse_double(section, "sigma_c", sigma_c, 1.0);
    size_t nsp = gmodel->get_number_of_species();
    vnf.resize(nsp);
    for ( size_t isp = 0; isp < nsp; ++isp ) vnf[isp] = 0.0;
    dict.parse_vector_of_doubles(section, "massf", massf, vnf);
    size_t nmodes = gmodel->get_number_of_modes();
    vnf.resize(nmodes);
    for ( size_t imode = 0; imode < nmodes; ++imode ) vnf[imode] = 300.0; 
    dict.parse_vector_of_doubles(section, "T", T, vnf);
    int S = 0;  // shock indicator
    cfcp = new CFlowCondition( gmodel, p, u, v, w, T, massf, flow_label, 
			       tke, omega, mu_t, k_t, S, Bx, By, Bz);
    vnf.clear();
    massf.clear();
    T.clear();
    flow_label.clear();
    return cfcp;
} // end  read_flow_condition_from_ini_dict()


/** \brief Set the parameters for the current block.
 *
 * Copy some of the global parameter values into the block and
 * then read a few parameters for the current block from parameter file.
 * Finally, and set a few more useful parameters from these.
 *
 * Also, Label the block with an integer (usually block number)
 * so that log-file messages can be labelled and special code
 * can be implemented for particular blocks.
 *
 * \param id   : (integer) block identity
 * \param dict : the dictionary associated with the INI-file
 * \param master : int flag to indicate whether this block is
 *                 associated with the master (0) process.
 *
 * \version 07-Jul-97 : Added iturb flag (in the place of nghost)
 * \version 22-Sep-97 : Updated the input format.
 * \version 10-May-2006 : Read from INI dictionary.
 * \version 20-Aug-2006 : Read in wall catalytic b.c.
 * \version 08-Sep-2006 : absorbed impose_global_parameters()
 */
int set_block_parameters(size_t id, ConfigParser &dict, bool master)
{
    global_data &G = *get_global_data_ptr();
    Block &bd = *get_block_data_ptr(id);
    int indx, iface;
    bc_t bc_type_code;
    std::string value_string, block_label;

    bd.id = id;
  
    // Read parameters from INI-file dictionary.
    indx = id;
    std::string section = "block/" + tostring(indx);

    dict.parse_string(section, "label", block_label, "");
    // Assume all blocks are active. 
    // The active flag will be used to skip over inactive
    // or unused blocks in later sections of code.
    dict.parse_boolean(section, "active", bd.active, true);
    if ( G.verbosity_level >= 2 ) {
	cout << section << ":label = " << block_label << endl;
	cout << "    active = " << bd.active << endl;
    }

    // Number of active cells in block.
    dict.parse_size_t(section, "nni", bd.nni, 2);
    dict.parse_size_t(section, "nnj", bd.nnj, 2);
    // Allow for ghost cells
    bd.nidim = bd.nni + 2 * G.nghost;
    bd.njdim = bd.nnj + 2 * G.nghost;
    // Set up min and max indices for convenience in later work.
    // Active cells should then be addressible as
    // get_cell(i,j), imin <= i <= imax, jmin <= j <= jmax.
    bd.imin = G.nghost;
    bd.imax = bd.imin + bd.nni - 1;
    bd.jmin = G.nghost;
    bd.jmax = bd.jmin + bd.nnj - 1;
    if ( G.dimensions == 3 ) {
	dict.parse_size_t(section, "nnk", bd.nnk, 2);
	bd.nkdim = bd.nnk + 2 * G.nghost;
	bd.kmin = G.nghost;
	bd.kmax = bd.kmin + bd.nnk - 1;
    } else {
	// For purely 2D flow geometry, we keep only one layer of cells.
	bd.nnk = 1;
	bd.nkdim = 1;
	bd.kmin = 0;
	bd.kmax = 0;
    }
    if ( G.verbosity_level >= 2 ) {
	printf( "    nni = %d, nnj = %d, nnk = %d\n", 
		static_cast<int>(bd.nni), static_cast<int>(bd.nnj),
		static_cast<int>(bd.nnk) );
	printf( "    nidim = %d, njdim = %d, nkdim = %d\n",
		static_cast<int>(bd.nidim), static_cast<int>(bd.njdim),
		static_cast<int>(bd.nkdim) );
    }

    // Rotating frame of reference.
    dict.parse_double(section, "omegaz", bd.omegaz, 0.0);

    // Boundary condition flags, 
    for ( iface = NORTH; iface <= ((G.dimensions == 3)? BOTTOM : WEST); ++iface ) {
	section = "block/" + tostring(indx) + "/face/" + get_face_name(iface);
	dict.parse_string(section, "bc", value_string, "slip_wall");
	value_string = to_lower_case(value_string);
	if ( available_bcs.find(value_string) == available_bcs.end() ) {
	    throw std::runtime_error(std::string("Requested boundary condition not available: ") + 
				     value_string);
	}
	bc_type_code = available_bcs[value_string];
	bd.bcp[iface] = create_BC(&bd, iface, bc_type_code, dict, section);
	if ( G.verbosity_level >= 2 ) {
	    cout << "    " << get_face_name(iface) << " face:" << endl;
	    bd.bcp[iface]->print_info("        ");
	}
	// Special work if the conjugate heat transfer model is active.
	if ( G.conjugate_ht_active && (iface == NORTH) ) {
	    // We always add an entry corresponding to every rank.
	    // If our boundary is a conjugate ht boundary, we set aside
	    // enough space in the global vectors for the number of cells
	    // on the north boundary, otherwise we don't need to set
	    // aside any space (ie. nentries = 0)
	    int nentries = 0;
	    if (bd.bcp[iface]->type_code == CONJUGATE_HT ) {
		nentries = bd.nni;
	    }
	    add_entries_to_wall_vectors(G, id, nentries);
	    // Later, after computing block geometry, we'll be able
	    // to gather up the interface locations to pass to the
	    // wall model. SEE: main.cxx
	}
    } // end for iface

    // History Cells.
    section = "block/" + tostring(indx);
    dict.parse_size_t(section, "nhcell", bd.hncell, 0);
    for ( size_t ih = 0; ih < bd.hncell; ++ih ) {
	section = "block/" + tostring(indx);
	string key = "history-cell-" + tostring(ih);
	dict.parse_string(section, key, value_string, "0 0 0");
	if ( G.dimensions == 3 ) {
	    unsigned int i, j, k;
	    sscanf( value_string.c_str(), "%u %u %u", &i, &j, &k );
	    bd.hicell.push_back(i);
	    bd.hjcell.push_back(j);
	    bd.hkcell.push_back(k);
	} else {
	    unsigned int i, j;
	    sscanf( value_string.c_str(), "%u %u", &i, &j );
	    bd.hicell.push_back(i);
	    bd.hjcell.push_back(j);
	    bd.hkcell.push_back(0);
	}
	if ( G.verbosity_level >= 2 ) {
	    printf( "    History cell[%d] located at indices [%d][%d][%d]\n",
		    static_cast<int>(ih), static_cast<int>(bd.hicell[ih]),
		    static_cast<int>(bd.hjcell[ih]), static_cast<int>(bd.hkcell[ih]) );
	}
    }

    // Flow-monitor cells.
    section = "block/" + tostring(indx);
    dict.parse_size_t(section, "nmcell", bd.mncell, 0);
    for ( size_t im = 0; im < bd.mncell; ++im ) {
	section = "block/" + tostring(indx);
	string key = "monitor-cell-" + tostring(im);
	dict.parse_string(section, key, value_string, "0 0 0");
	if ( G.dimensions == 3 ) {
	    unsigned int i, j, k;
	    sscanf( value_string.c_str(), "%u %u %u", &i, &j, &k );
	    bd.micell.push_back(i);
	    bd.mjcell.push_back(j);
	    bd.mkcell.push_back(k);
	} else {
	    unsigned int i, j;
	    sscanf( value_string.c_str(), "%u %u", &i, &j );
	    bd.micell.push_back(i);
	    bd.mjcell.push_back(j);
	    bd.mkcell.push_back(0);
	}
	bd.initial_T_value.push_back(0.0); // place holder value
	if ( G.verbosity_level >= 2 ) {
	    printf( "    Monitor cell[%d] located at indices [%d][%d][%d]\n",
		    static_cast<int>(im), static_cast<int>(bd.micell[im]),
		    static_cast<int>(bd.mjcell[im]), static_cast<int>(bd.mkcell[im]) );
	}
    }

    // Writing of transient profiles for David Gildfind's X-tube simulations.
    // Peter J, 2014-11-16
    section = "block/" + tostring(indx);
    dict.parse_string(section, "transient_profile_faces", value_string, "");
    // Expect space separated entries that are either face indices or face names.
    std::istringstream itemstream(value_string);
    string item;
    while ( itemstream >> item ) {
	int i = get_face_index(item);
	if ( G.dimensions == 2 && (i == TOP || i == BOTTOM) ) continue;
	bd.transient_profile_faces.push_back(i);
    }
    if ( G.verbosity_level >= 2 ) {
	cout << "    transient_profile_faces=";
	for ( int i: bd.transient_profile_faces ) cout << " " << get_face_name(i);
	cout << endl;
    }

    return SUCCESS;
} // end set_block_parameters()

//------------------------------------------------------------------
