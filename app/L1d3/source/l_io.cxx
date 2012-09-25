/** \file l_io.cxx
 * \ingroup l1d3
 * \brief I/O functions for l1d.c.
 *
 * \version 30-Mar-95 : added entropy to the input and output routines
 * \version 17-Aug-95 : ANSI Compiler version
 * \version 05-Apr-98 : generalise input
 * \version 22-Apr-98 : echo_input parameter added to functions that read
 *             parameter file
 * \version 19-Jan-98 : Change state variable input to p & T (from rho & e)
 *             Keep tube diameters as well as areas.
 * \version 04-Jun-00 : Adaptivity added.
 * \version 24-Jul-06 : C++ port
 */

/*-----------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <valarray>
#include <iostream>
#include <sstream>
#include "../../../lib/util/source/useful.h"
#include "../../../lib/util/source/config_parser.hh"
#include "../../../lib/gas/models/gas-model.hh"
#include "../../../lib/gas/kinetics/reaction-update.hh"
#include "l_kernel.hh"
#include "l1d.hh"
#include "l_misc.hh"
using namespace std;

/*=================================================================*/

int print_simulation_status(FILE *strm, const char* efname, int step, simulation_data *SD,
			    vector<slug_data> &A, vector<diaphragm_data> &Diaph,
			    vector<piston_data> &Pist, double cfl_max, 
			    double cfl_tiny, double time_tiny) {
    /*
     * Print the simulation status to the specified stream.
     * If the specified stream is NULL, then send the data to the 
     * event log file.
     */

    char msg_string[256];
    int close_stream, js, jd, jp;
    double p_max, x_max, E_tot;

    UNUSED_VARIABLE(cfl_tiny);
    UNUSED_VARIABLE(time_tiny);
    
    close_stream = 0;

    if ( strm == NULL ) {
        /* Presume that we want to open the events file. */
        strm = fopen( efname, "a+" );
        if ( strm != NULL ) { close_stream = 1; }
    }

    if ( strm != NULL ) {
	sprintf( msg_string,
		 "Step= %7d, time= %e, dt= %10.3e, CFL= %10.3e\n",
                 step, SD->sim_time, SD->dt_global, cfl_max);
	fputs( msg_string, strm );
	for (js = 0; js < SD->nslug; ++js) {
	    maximum_p(&A[js], &p_max, &x_max);
	    total_energy(&A[js], &E_tot);
	    sprintf( msg_string,
		     "Slug %d: p_max=%10.3e, x=%10.3e, Etot=%10.3e, dt=%10.3e, nnx=%d\n",
		     js, p_max, x_max, E_tot, A[js].dt_allow, A[js].nnx);
	    fputs( msg_string, strm );
	}
	for (jd = 0; jd < SD->ndiaphragm; ++jd) {
	    sprintf( msg_string, "Diaph[%d].is_burst = %d, trigger_time = %e\n",
		     jd, Diaph[jd].is_burst, Diaph[jd].trigger_time);
	    fputs( msg_string, strm );
        }
	for (jp = 0; jp < SD->npiston; ++jp) {
	    sprintf( msg_string, "Piston %d: flags=%d%d%d",
		     jp, Pist[jp].is_restrain, Pist[jp].on_buffer,
		     Pist[jp].brakes_on);
	    fputs( msg_string, strm );
	    sprintf( msg_string, " x=%10.3e V=%10.3e a=%10.3e KE=%10.3e mass=%10.3e\n",
		     Pist[jp].x, Pist[jp].V, Pist[jp].DVDt[0],
		     0.5 * Pist[jp].mass * Pist[jp].V * Pist[jp].V, Pist[jp].mass);
	    fputs( msg_string, strm );
	    sprintf( msg_string, 
		     "          hit_count= %d, speed at last strike= %e\n",
		     Pist[jp].hit_buffer_count, Pist[jp].V_buffer);
	    fputs( msg_string, strm );
	} // end for
    }

    if ( close_stream ) {
        fclose( strm );
    }

    return SUCCESS;
}

int log_event(const char *efname, const char* event_message ) {
    /*
     * Write a message to the events log file.
     * This file is opened and closed each time so that
     * Windows users can see the data.
     */
    FILE *efp;
    efp = fopen(efname, "a+");
    if ( efp != NULL ) {
        fputs(event_message, efp);
        /* print_status( efp ); */
        fclose( efp );
    }
    return SUCCESS;
}

/*=================================================================*/

int L_set_case_parameters(simulation_data *SD, ConfigParser &dict, int echo_input)
{
    string reaction_scheme_file, gas_model_file;
    if (echo_input == 1) cout << endl << "Reading global_data..." << endl;

    dict.parse_int("global_data", "case_id", SD->test_case, 0);
    L_set_case_id( SD->test_case );
    dict.parse_string("global_data", "gas_model_file", gas_model_file, "gas-model.lua");
    Gas_model *gmodel = set_gas_model_ptr(create_gas_model(gas_model_file));
    dict.parse_string("global_data", "reaction_scheme_file", reaction_scheme_file, "None");
    dict.parse_int("global_data", "reacting_flag", SD->fr_chem, 0);
    if( SD->fr_chem ) set_reaction_update( reaction_scheme_file );
    if (echo_input == 1) {
	cout << "    test_case_id = " << SD->test_case << endl;
	cout << "    gas_model_file = " << gas_model_file << endl;
	cout << "    nsp = " << gmodel->get_number_of_species() << endl;
	cout << "    nmodes = " << gmodel->get_number_of_modes() << endl;
	cout << "    reacting_flag = " << SD->fr_chem << endl;
	cout << "    reaction_scheme_file = " << reaction_scheme_file << endl;
    }
    dict.parse_int("global_data", "nslug", SD->nslug, 0);
    dict.parse_int("global_data", "npiston", SD->npiston, 0);
    dict.parse_int("global_data", "ndiaphragm", SD->ndiaphragm, 0);
    if (echo_input == 1) {
	cout << "    nslug = " << SD->nslug << endl;
	cout << "    npiston = " << SD->npiston << endl;
	cout << "    ndiaphragm = " << SD->ndiaphragm << endl;
    }
    dict.parse_double("global_data", "max_time", SD->max_time, 1.0e-3);
    dict.parse_int("global_data", "max_step", SD->max_step, 100);
    dict.parse_double("global_data", "dt_init", SD->dt_init, 1.0e-9);
    dict.parse_double("global_data", "cfl", SD->CFL, 0.5);
    dict.parse_int("global_data", "x_order", SD->Xorder, 2);
    dict.parse_int("global_data", "t_order", SD->Torder, 2);
    dict.parse_double("global_data", "thermal_damping", SD->k, 0.0);
    if (echo_input == 1) {
	cout << "    max_time = " << SD->max_time << endl;
	cout << "    max_step = " << SD->max_step << endl;
	cout << "    dt_init = " << SD->dt_init << endl;
	cout << "    cfl = " << SD->CFL << endl;
	cout << "    x_order = " << SD->Xorder << endl;
	cout << "    t_order = " << SD->Torder << endl;
	cout << "    thermal_damping = " << SD->k << endl;
    }
    dict.parse_int("global_data", "n_dt_plot", SD->n_dt_plot, 0);
    std::vector<double> vdbl, vdbl_default;
    vdbl_default.resize(SD->n_dt_plot);
    for ( size_t i = 0; i < vdbl_default.size(); ++i ) vdbl_default[i] = 0.0;
    dict.parse_vector_of_doubles("global_data", "t_change", SD->t_change, vdbl_default);
    for ( size_t i = 0; i < vdbl_default.size(); ++i ) vdbl_default[i] = 1.0e-3;
    dict.parse_vector_of_doubles("global_data", "dt_plot", SD->dt_plot, vdbl_default);
    dict.parse_vector_of_doubles("global_data", "dt_his", SD->dt_his, vdbl_default);
    if (echo_input == 1) {
	cout << "    n_dt_plot = " << SD->n_dt_plot << endl;
	cout << "        t_change  dt_plot  dt_his" << endl;
	for ( int i = 0; i < SD->n_dt_plot; ++i ) {
	    cout << "        " << SD->t_change[i] 
		 << " " << SD->dt_plot[i] << " " << SD->dt_his[i] << endl;
	}
    }
    dict.parse_int("global_data", "hloc_n", SD->hnloc, 0);
    vdbl_default.resize(SD->hnloc);
    for ( size_t i = 0; i < vdbl_default.size(); ++i ) vdbl_default[i] = 0.0;
    dict.parse_vector_of_doubles("global_data", "hloc_x", SD->hxloc, vdbl_default);
    if (echo_input == 1) {
	cout << "    hloc_n = " << SD->hnloc << endl;
	cout << "    hloc_x =";
	for ( int i = 0; i < SD->hnloc; ++i ) cout << " " << SD->hxloc[i]; 
	cout << endl;
    }
    return SUCCESS;
} // end function L_set_case_parameters()

/*----------------------------------------------------------------*/

int set_piston_parameters(piston_data *B, int indx, ConfigParser &dict, 
			  double dt_init, int echo_input)
{
    std::stringstream tag;
    tag << indx;
    std::string section = "piston-" + tag.str();
    if (echo_input == 1) cout << "Reading piston " << indx << " parameters..." << endl;

    dict.parse_double(section, "mass", B->mass, 1.0);
    dict.parse_double(section, "diameter", B->diam, 1.0);
    dict.parse_double(section, "length", B->length, 1.0);
    const double myPI = 4.0*atan(1.0);
    B->area = myPI * 0.25 * B->diam * B->diam;
    dict.parse_double(section, "front_seal_f", B->front_seal_f, 0.0);
    dict.parse_double(section, "front_seal_area", B->front_seal_area, 0.0);
    dict.parse_double(section, "back_seal_f", B->back_seal_f, 0.0);
    dict.parse_double(section, "back_seal_area", B->back_seal_area, 0.0);
    if (echo_input == 1) {
	cout << "    mass = " << B->mass << endl;
	cout << "    diameter = " << B->diam << endl;
	cout << "    length = " << B->length << endl;
	cout << "    area = " << B->area << endl;
	cout << "    front_seal_f = " << B->front_seal_f << endl;
	cout << "    front_seal_area = " << B->front_seal_area << endl;
	cout << "    back_seal_f = " << B->back_seal_f << endl;
	cout << "    back_seal_area = " << B->back_seal_area << endl;
    }
    dict.parse_double(section, "p_restrain", B->p_restrain, 1.0);
    dict.parse_int(section, "is_restrain", B->is_restrain, 0);
    dict.parse_double(section, "x_buffer", B->x_buffer, 0.0);
    dict.parse_int(section, "hit_buffer", B->on_buffer, 0);
    B->hit_buffer_count = 0;  /* initialize counter for buffer strikes */
    B->V_buffer = 0.0;
    dict.parse_int(section, "with_brakes", B->with_brakes, 0);
    dict.parse_int(section, "brakes_on", B->brakes_on, 0);
    if (echo_input == 1) {
	cout << "    p_restrain = " << B->p_restrain << endl;
	cout << "    is_restrain = " << B->is_restrain << endl;
	cout << "    x_buffer = " << B->x_buffer << endl;
	cout << "    on_buffer = " << B->on_buffer << endl;
	cout << "    hit_buffer_count = " << B->hit_buffer_count << endl;
	cout << "    with_brakes = " << B->with_brakes << endl;
	cout << "    brakes_on = " << B->brakes_on << endl;
    }
    /* By default, the piston is free. */
    B->left_slug_id = -1;
    B->left_slug_end_id = -1;
    dict.parse_int(section, "left-slug-id", B->left_slug_id, -1);
    std::string label;
    dict.parse_string(section, "left-slug-end-id", label, "");
    if (label[0] == 'L' || label[0] == 'l' || label[0] == '0')
        B->left_slug_end_id = LEFT;
    if (label[0] == 'R' || label[0] == 'r' || label[0] == '1')
        B->left_slug_end_id = RIGHT;
    if (echo_input == 1) {
	cout << "    left-slug-id = " << B->left_slug_id << endl;
	cout << "    left-slug-end-id = " << B->left_slug_end_id << endl;
    }
    B->right_slug_id = -1;
    B->right_slug_end_id = -1;
    dict.parse_int(section, "right-slug-id", B->right_slug_id, -1);
    dict.parse_string(section, "right-slug-end-id", label, "");
    if (label[0] == 'L' || label[0] == 'l' || label[0] == '0')
        B->right_slug_end_id = LEFT;
    if (label[0] == 'R' || label[0] == 'r' || label[0] == '1')
        B->right_slug_end_id = RIGHT;
    if (echo_input == 1) {
	cout << "    right-slug-id = " << B->right_slug_id << endl;
	cout << "    right-slug-end-id = " << B->right_slug_end_id << endl;
    }
    // Initial position and velocity.
    B->dt = dt_init;
    dict.parse_double(section, "x0", B->x0, 0.0);
    dict.parse_double(section, "v0", B->V0, 0.0);
    B->x = B->x0;
    B->V = B->V0;
    if (echo_input == 1) {
	cout << "    x0 = " << B->x0 << endl;
	cout << "    v0 = " << B->V0 << endl;
    }
    // Mass decay
    dict.parse_double(section, "f_decay", B->f_decay, 0.0);
    dict.parse_double(section, "mass_limit", B->mass_limit, 0.0);
    if (echo_input == 1) {
	cout << "    f_decay = " << B->f_decay << endl;
	cout << "    mass_limit = " << B->mass_limit << endl;
    }
    if (B->f_decay != 0.0) 
	B->apply_decay = 1;
    else 
	B->apply_decay = 0;

    return SUCCESS;
} // end function set_piston_parameters()

/*----------------------------------------------------------------*/

int set_diaphragm_parameters(struct diaphragm_data *D, int indx,
                             ConfigParser &dict, int echo_input)
{
    std::stringstream tag;
    tag << indx;
    std::string section = "diaphragm-" + tag.str();
    if (echo_input == 1) cout << "Reading diaphragm " << indx << " parameters..." << endl;

    dict.parse_int(section, "is_burst", D->is_burst, 0);
    dict.parse_double(section, "p_burst", D->P_burst, 0.0);
    dict.parse_double(section, "dt_hold", D->hold_period, 0.0);
    dict.parse_double(section, "dt_blend", D->blend_delay, 0.0);
    dict.parse_double(section, "dx_blend", D->blend_dx, 0.0);
    if (echo_input == 1) {
	cout << "    is_burst = " << D->is_burst << endl;
	cout << "    p_burst = " << D->P_burst << endl;
	cout << "    dt_hold = " << D->hold_period << endl;
	cout << "    dt_blend = " << D->blend_delay << endl;
	cout << "    dx_blend = " << D->blend_dx << endl;
    }
    // Initially set the trigger_time to a negative number.
    D->trigger_time = -1.0;
    D->already_blended = 0;
    // By default, the diaphragm is unconnected.
    D->left_slug_id = -1;
    D->left_slug_end_id = -1;
    dict.parse_int(section, "left-slug-id", D->left_slug_id, -1);
    std::string label;
    dict.parse_string(section, "left-slug-end-id", label, "");
    if (label[0] == 'L' || label[0] == 'l' || label[0] == '0')
        D->left_slug_end_id = LEFT;
    if (label[0] == 'R' || label[0] == 'r' || label[0] == '1')
        D->left_slug_end_id = RIGHT;
    dict.parse_double(section, "dxL", D->left_slug_dx, 0.0);
    if (echo_input == 1) {
	cout << "    left-slug-id = " << D->left_slug_id << endl;
	cout << "    left-slug-end-id = " << D->left_slug_end_id << endl;
	cout << "    dxL = " << D->left_slug_dx << endl;
    }
    D->right_slug_id = -1;
    D->right_slug_end_id = -1;
    dict.parse_int(section, "right-slug-id", D->right_slug_id, -1);
    dict.parse_string(section, "right-slug-end-id", label, "");
    if (label[0] == 'L' || label[0] == 'l' || label[0] == '0')
        D->right_slug_end_id = LEFT;
    if (label[0] == 'R' || label[0] == 'r' || label[0] == '1')
        D->right_slug_end_id = RIGHT;
    dict.parse_double(section, "dxR", D->right_slug_dx, 0.0);
    if (echo_input == 1) {
	cout << "    right-slug-id = " << D->right_slug_id << endl;
	cout << "    right-slug-end-id = " << D->right_slug_end_id << endl;
	cout << "    dxR = " << D->right_slug_dx << endl;
    }

    return SUCCESS;
} // end function set_diaphragm_parameters()

//-------------------------------------------------------------------

int L_set_slug_parameters(slug_data* A, int indx, simulation_data* SD,
                          ConfigParser& dict, int echo_input)
{
    std::stringstream tag;
    tag << indx;
    std::string section = "slug-" + tag.str();
    if (echo_input == 1) cout << "Reading slug " << indx << " parameters..." << endl;
    Gas_model *gmodel = get_gas_model_ptr();

    dict.parse_int(section, "nn", A->nnx, 10);
    dict.parse_int(section, "cluster_to_end_L", A->cluster_to_end_1, 0);
    dict.parse_int(section, "cluster_to_end_R", A->cluster_to_end_2, 0);
    dict.parse_double(section, "cluster_strength", A->cluster_strength, 0.0);
    if (echo_input == 1) {
	cout << "    nn = " << A->nnx << endl;
	cout << "    cluster_to_end_L = " << A->cluster_to_end_1 << endl;
	cout << "    cluster_to_end_R = " << A->cluster_to_end_2 << endl;
	cout << "    cluster_strength = " << A->cluster_strength << endl;
    }
    // Adaptivity parameters for this gas slug. 
    dict.parse_int(section, "nnmax", A->nxmax, A->nnx);
    dict.parse_int(section, "adaptive", A->adaptive, 0);
    dict.parse_double(section, "dxmin", A->dxmin, 0.0);
    dict.parse_double(section, "dxmax", A->dxmax, 0.0);
    if (echo_input == 1) {
	cout << "    nnmax = " << A->nxmax << endl;
	cout << "    adaptive = " << A->adaptive << endl;
	cout << "    dxmin = " << A->dxmin << endl;
	cout << "    dxmax = " << A->dxmax << endl;
    }
    // Allow for ghost cells 
    A->nghost = 2;
    A->nxdim = A->nxmax + 2 * A->nghost;
    if ( A->nxdim > NDIM ) {
	cout << "**** Problem with Slug[" << indx << "]" << endl;
	cout << "     NDIM=" << NDIM 
	     << " is not large enough for A->nxdim=" << A->nxdim << endl;
        cout << "     A->nnx=" << A->nnx << " A->nxmax=" << A->nxmax << endl;
	cout << "Quitting." << endl;
	exit(1);
    }
    if ( L_alloc(A) != 0 ) {
        cout << "Memory allocation failed for slug." << endl; 
        exit(1);
    }
    // Set up min and max indices for convenience in later work.
    // Active cells should then be addressible as 
    // cell[ix], ixmin <= ix <= ixmax.
    L_set_index_range(A);
    // FIX-ME
    // Now that we know the size of the gas slug, 
    // we should resize vectors to max(nnx,nxmax)+4 elements.

    // Flag for viscous effects:
    // =1: include them
    // =0: do not include them
    // Flag for wall temperature
    // =0: use specified wall temperature
    // =1: use adiabatic wall temperature
    dict.parse_int(section, "viscous_effects", A->viscous_effects, 0);
    dict.parse_int(section, "adiabatic_flag", A->adiabatic, 0);
    if (echo_input == 1) {
	cout << "    viscous_effects = " << A->viscous_effects << endl;
	cout << "    adiabatic_flag = " << A->adiabatic << endl;
    }

    // Boundary conditions.
    // By default, the gas slug is unconnected.
    A->left_slug_id = -1;
    A->left_slug_end_id = -1;
    A->right_slug_id = -1;
    A->right_slug_end_id = -1;
    //
    A->left_piston_id = -1;
    A->right_piston_id = -1;
    //
    A->left_diaphragm_id = -1;
    A->right_diaphragm_id = -1;
    //
    A->set_left_end_ustar = 0;
    A->set_right_end_ustar = 0;
    A->left_ustar = 0.0;
    A->right_ustar = 0.0;

    // Process BC data for left boundary. 
    std::string line, control_string, label;
    dict.parse_string(section, "BC_L", line, "");
    stringstream ss_L(line);
    ss_L >> control_string;
    if (control_string == "S") {
        A->left_bc_type = SLUG;
	ss_L >> A->left_slug_id;
	ss_L >> label;
        if (label[0] == 'L' || label[0] == 'l' || label[0] == '0')
            A->left_slug_end_id = LEFT;
        if (label[0] == 'R' || label[0] == 'r' || label[0] == '1')
            A->left_slug_end_id = RIGHT;
        if (echo_input == 1) {
            cout << "   left_boundary: neighbour slug = " 
		 << A->left_slug_id << " end = " << A->left_slug_end_id << endl;
        }
    } else if (control_string == "SD") {
        A->left_bc_type = SLUG_DIAPHRAGM;
	ss_L >> A->left_slug_id;
	ss_L >> label;
        if (label[0] == 'L' || label[0] == 'l' || label[0] == '0')
            A->left_slug_end_id = LEFT;
        if (label[0] == 'R' || label[0] == 'r' || label[0] == '1')
            A->left_slug_end_id = RIGHT;
	ss_L >> A->left_diaphragm_id;
        if (echo_input == 1) {
            cout << "    left_boundary: neighbour slug = "
		 << A->left_slug_id << " end = " <<  A->left_slug_end_id
		 << " diaphragm = " << A->left_diaphragm_id << endl;
        }
    } else if (control_string == "P") {
        A->left_bc_type = PISTON;
        ss_L >> A->left_piston_id;
        if (echo_input == 1) {
            cout << "    left_boundary: neighbour piston = "
		 << A->left_piston_id << endl;
        }
        A->set_left_end_ustar = 1;
    } else if (control_string == "V") {
        A->left_bc_type = SOLID_BOUNDARY;
        ss_L >> A->left_ustar;
        if (echo_input == 1) {
            cout << "    left_boundary: imposed velocity = "
		 << A->left_ustar << endl;
        }
        A->set_left_end_ustar = 1;
    } else if (control_string == "F") {
        A->left_bc_type = FREE_END;
        if (echo_input == 1) {
            cout << "    left_boundary: free-end" << endl;
        }
    } else {
        cout << "    Invalid control string: " << control_string << endl;
        exit(-1);
    } // end if control_string...

    // Process BC data for right boundary. 
    dict.parse_string(section, "BC_R", line, "");
    stringstream ss_R(line);
    ss_R >> control_string;
    if (control_string == "S") {
        A->right_bc_type = SLUG;
	ss_R >> A->right_slug_id;
	ss_R >> label;
        if (label[0] == 'L' || label[0] == 'l' || label[0] == '0')
            A->right_slug_end_id = LEFT;
        if (label[0] == 'R' || label[0] == 'r' || label[0] == '1')
            A->right_slug_end_id = RIGHT;
        if (echo_input == 1) {
            cout << "   right_boundary: neighbour slug = " 
		 << A->right_slug_id << " end = " << A->right_slug_end_id << endl;
        }
    } else if (control_string == "SD") {
        A->right_bc_type = SLUG_DIAPHRAGM;
	ss_R >> A->right_slug_id;
	ss_R >> label;
        if (label[0] == 'L' || label[0] == 'l' || label[0] == '0')
            A->right_slug_end_id = LEFT;
        if (label[0] == 'R' || label[0] == 'r' || label[0] == '1')
            A->right_slug_end_id = RIGHT;
	ss_R >> A->right_diaphragm_id;
        if (echo_input == 1) {
            cout << "    right_boundary: neighbour slug = "
		 << A->right_slug_id << " end = " <<  A->right_slug_end_id
		 << " diaphragm = " << A->right_diaphragm_id << endl;
        }
    } else if (control_string == "P") {
        A->right_bc_type = PISTON;
        ss_R >> A->right_piston_id;
        if (echo_input == 1) {
            cout << "    right_boundary: neighbour piston = "
		 << A->right_piston_id << endl;
        }
        A->set_right_end_ustar = 1;
    } else if (control_string == "V") {
        A->right_bc_type = SOLID_BOUNDARY;
        ss_R >> A->right_ustar;
        if (echo_input == 1) {
            cout << "    right_boundary: imposed velocity = "
		 << A->right_ustar << endl;
        }
        A->set_right_end_ustar = 1;
    } else if (control_string == "F") {
        A->right_bc_type = FREE_END;
        if (echo_input == 1) {
            cout << "    right_boundary: free-end" << endl;
        }
    } else {
        cout << "    Invalid control string: " << control_string << endl;
        exit(-1);
    } // end if control_string...

    // Time stepping and order of reconstruction.
    A->dt = SD->dt_init;
    A->cfl_target = SD->CFL;
    A->dt0 = A->dt;
    A->Torder = SD->Torder;
    A->Xorder = SD->Xorder;
    // Plotting and history output events.
    dict.parse_int(section, "hncell", A->hncell, 0);
    std::vector<int> vint_default;
    vint_default.resize(A->hncell);
    for ( size_t i = 0; i < vint_default.size(); ++i ) vint_default[i] = 0;
    dict.parse_vector_of_ints(section, "hxcell", A->hxcell, vint_default);
    if (echo_input == 1) {
	cout << "    hncell = " << A->hncell << endl;
	cout << "    hxcell =";
	for ( int i = 0; i < A->hncell; ++i ) cout << " " << A->hxcell[i];
	cout << endl;
    }
    // Initial slug state. 
    int nsp = gmodel->get_number_of_species();
    A->init_str->gas = new Gas_data(gmodel);
    dict.parse_double(section, "initial_xL", A->xbegin, 0.0);
    dict.parse_double(section, "initial_xR", A->xend, 0.0);
    dict.parse_double(section, "initial_p", A->init_str->gas->p, 100.0e3);
    dict.parse_double(section, "initial_u", A->init_str->u, 0.0);
    dict.parse_double(section, "initial_T", A->init_str->gas->T[0], 300.0);
    std::vector<double> vdbl_default;
    vdbl_default.resize(nsp);
    for ( size_t i = 0; i < vdbl_default.size(); ++i ) vdbl_default[i] = 0.0;
    dict.parse_vector_of_doubles(section, "massf", A->init_str->gas->massf, vdbl_default);
    if (echo_input == 1) {
	cout << "    initial_xL = " << A->xbegin << endl;
	cout << "    initial_xR = " << A->xend << endl;
	cout << "    initial_p = " << A->init_str->gas->p << endl;
	cout << "    initial_u = " << A->init_str->u << endl;
	cout << "    initial_T = " << A->init_str->gas->T[0] << endl;
	cout << "    massf =";
	for ( int i = 0; i < nsp; ++i ) cout << " " << A->init_str->gas->massf[i];
	cout << endl;
    }
    double f_sum = A->init_str->gas->massf[0];
    for ( int isp = 1; isp < nsp; ++isp ) {
	f_sum += A->init_str->gas->massf[isp];
    }
    if ( fabs(f_sum - 1.0) > 1.0e-4 ) {
	printf( "Species mass fractions do not sum to 1.0: %e\n", f_sum );
	exit(-1);
    }
    // Density, Internal energy, Speed of Sound, and 
    // molecular transport coefficients. 
    gmodel->eval_thermo_state_pT(*(A->init_str->gas));
    gmodel->eval_transport_coefficients(*(A->init_str->gas));
    if (echo_input == 1) {
	cout << "    rho = " << A->init_str->gas->rho
	     << " e = " << A->init_str->gas->e[0]
	     << " a = " << A->init_str->gas->a << endl;
	cout << "    R = " << gmodel->R(*(A->init_str->gas))
	     << " Cv = " << gmodel->Cv(*(A->init_str->gas)) << endl;
	cout << "    mu = " << A->init_str->gas->mu
	     << " k = " << A->init_str->gas->k[0] << endl;
    }
    return SUCCESS;
} // end function L_set_slug_parameters()

//---------------------------------------------------------------------

int read_piston_solution(struct piston_data* B, FILE* infile)
// Read the piston solution (i.e. the present piston state)
{
#   define NCHAR 320
    char line[NCHAR];
    if (fgets(line, NCHAR, infile) == NULL) {
        printf("Empty solution file.\n");
        return(FAILURE);
    }
    sscanf(line, "%lf", &(B->sim_time));
    // Position, Velocity and acceleration.
    if (fgets(line, NCHAR, infile) == NULL) {
        printf("Empty solution file.\n");
        return(FAILURE);
    }
    sscanf(line, "%lf %lf %lf %lf", &(B->x), &(B->V), &(B->DVDt[0]), &(B->mass) );
    // Flags
    if (fgets(line, NCHAR, infile) == NULL) {
        printf("Empty solution file.\n");
        return(FAILURE);
    }
    sscanf(line, "%d %d %d", &(B->is_restrain),
           &(B->on_buffer), &(B->brakes_on));
#   undef NCHAR
    return SUCCESS;
} // end read_piston_solution()


int write_piston_solution(struct piston_data *B, FILE * outfile)
{
    fprintf(outfile, "%e  # begin piston data: sim_time\n",
	    B->sim_time);
    fprintf(outfile, "%e %e %e %e  # x, V, accel, mass\n",
	    B->x, B->V, B->DVDt[0], B->mass);
    fprintf(outfile, "%d %d %d  # is_restrain, on_buffer, brakes_on\n", 
	    B->is_restrain, B->on_buffer, B->brakes_on);
    fflush(outfile);
    return SUCCESS;
}

//---------------------------------------------------------------------------

int read_diaphragm_solution(struct diaphragm_data *D, FILE * infile)
// Read the diaphragm solution (i.e. the present diaphragm state)
{
#   define NCHAR 320
    char line[NCHAR];
    int nread;
    if (fgets(line, NCHAR, infile) == NULL) {
        printf("Empty solution file.\n");
        return(FAILURE);
    }
    nread = sscanf(line, "%lf", &(D->sim_time));
    if ( nread != 1 ) {
	printf( "read_diaphragm_solution(): didn't correctly read sim_time\n" );
        printf( "from line:\n%s\n", line );
	return(FAILURE);
    }
    if (fgets(line, NCHAR, infile) == NULL) {
        printf("Empty solution file.\n");
        return(FAILURE);
    }
    nread = sscanf(line, "%d %d %lf", &(D->is_burst), 
		   &(D->already_blended), &(D->trigger_time));
    if ( nread != 3 ) {
	printf( "read_diaphragm_solution(): " );
	printf( "didn't correctly read is_burst, already_blended, trigger_time\n" );
        printf( "from line:\n%s\n", line );
	return(FAILURE);
    }
    return SUCCESS;
#   undef NCHAR
}


int write_diaphragm_solution(struct diaphragm_data *D, FILE * outfile)
{
    fprintf(outfile, "%e  # begin diaphragm data: sim_time\n", 
	    D->sim_time);
    fprintf(outfile, "%d %d %e  # is_burst, already_blended, trigger_time\n", 
	    D->is_burst, D->already_blended, D->trigger_time);
    fflush(outfile);

    return 0;
}

//----------------------------------------------------------------------

std::string write_iface_values_to_string(struct L_cell& cell)
// Write the flow solution for a cell to a string.
{
    // The new format for L1d3 puts everything onto one line.
    ostringstream ost;
    ost.setf(ios_base::scientific);
    ost.precision(12);
    ost << cell.x << " " << cell.area; 
    // Don't put the newline char on the end.
    return ost.str();
} // end of write_iface_values_to_string()


int scan_iface_values_from_string(char *bufptr, struct L_cell& cell)
// Scan a string, extracting the data for an interface between cells.
// There isn't any checking of the file content.
// If anything gets out of place, the result is wrong data.
{
    // Look for a new-line character and truncate the string there.
    char *cptr = strchr(bufptr, '\n');
    if ( cptr != NULL ) cptr = '\0';
    // Now, we should have a string with only numbers separated by spaces.
    cell.x = atof(strtok( bufptr, " " )); // tokenize on space characters
    cell.area = atof(strtok( NULL, " " ));
    return SUCCESS;
} // end scan_iface_values_from_string()


std::string write_cell_values_to_string(struct L_cell& cell)
// Write the flow solution for a cell to a string.
{
    // The new format for L1d3 puts everything onto one line.
    ostringstream ost;
    ost.setf(ios_base::scientific);
    ost.precision(12);
    ost << cell.xmid << " " 
	<< cell.volume << " " 
	<< cell.u << " " 
	<< cell.L_bar << " " 
	<< cell.gas->rho << " " 
	<< cell.gas->p << " " 
	<< cell.gas->a << " " 
	<< cell.shear_stress << " " 
	<< cell.heat_flux << " " 
	<< cell.entropy;
    // Species mass fractions.
    size_t nsp = cell.gas->massf.size();
    for ( size_t isp = 0; isp < nsp; ++isp ) {
	ost << " " << cell.gas->massf[isp];
    }
    if ( nsp > 1 ) ost << " " << cell.dt_chem;
    // Individual energies (in e, T pairs)
    size_t nmodes = cell.gas->T.size();
    for ( size_t imode = 0; imode < nmodes; ++imode ) {
	ost << " " << cell.gas->e[imode] << " " << cell.gas->T[imode];
    }
    if ( nmodes > 1 ) ost << " " << cell.dt_therm;
    // Don't put the newline char on the end.
    return ost.str();
} // end of write_cell_values_to_string()


int scan_cell_values_from_string(char *bufptr, struct L_cell& cell)
// Scan a string, extracting the data for a cell.
// There isn't any checking of the file content.
// If anything gets out of place, the result is wrong data.
{
    // Look for a new-line character and truncate the string there.
    char *cptr = strchr(bufptr, '\n');
    if ( cptr != NULL ) cptr = '\0';
    // Now, we should have a string with only numbers separated by spaces.
    cell.xmid = atof(strtok( bufptr, " " )); // tokenize on space characters
    cell.volume = atof(strtok( NULL, " " ));
    cell.u = atof(strtok( NULL, " " ));
    cell.L_bar = atof(strtok( NULL, " " ));
    cell.gas->rho = atof(strtok( NULL, " " ));
    cell.gas->p = atof(strtok( NULL, " " ));
    cell.gas->a = atof(strtok( NULL, " " ));
    cell.shear_stress = atof(strtok( NULL, " " ));
    cell.heat_flux = atof(strtok( NULL, " " ));
    cell.entropy = atof(strtok( NULL, " " ));
    size_t nsp = cell.gas->massf.size();
    for ( size_t isp = 0; isp < nsp; ++isp ) {
	cell.gas->massf[isp] = atof(strtok( NULL, " " ));
    }
    if ( nsp > 1 ) cell.dt_chem = atof(strtok( NULL, " " ));
    size_t nmodes = cell.gas->T.size();
    for ( size_t imode = 0; imode < nmodes; ++imode ) {
	cell.gas->e[imode] = atof(strtok( NULL, " " ));
	cell.gas->T[imode] = atof(strtok( NULL, " " ));
    }
    if ( nmodes > 1 ) cell.dt_therm = atof(strtok( NULL, " " ));
    return SUCCESS;
} // end scan_cell_values_from_string()


int L_read_solution(struct slug_data *A, FILE * infile)
// Read the flow solution for all faces and cells in a slug. 
{
#   define NCHAR 3200
    char line[NCHAR];
    int ix, isp, imode;
    Gas_model *gmodel = get_gas_model_ptr();
    int nsp = gmodel->get_number_of_species();
    int nmodes = gmodel->get_number_of_modes();
    if (fgets(line, NCHAR, infile) == NULL) {
        printf("Empty flow field file.\n");
        return FAILURE;
    }
    sscanf(line, "%lf", &(A->sim_time));
    if (fgets(line, NCHAR, infile) == NULL) {
        printf("Empty flow field file.\n");
        return FAILURE;
    }
    sscanf(line, "%d %d %d", &ix, &isp, &imode);
    if (ix <= A->nxmax) {
        A->nnx = ix;
    } else {
        printf("Trying to read too many cells into this slug.\n");
        return FAILURE;
    }
    L_set_index_range(A);
    if ( isp != nsp ) {
        printf("Inconsistent number of species: expected %d, read %d\n", nsp, isp);
	return FAILURE;
    }
    if ( imode != nmodes ) {
        printf("Inconsistent number of energy modes: expected %d, read %d\n", nmodes, imode);
	return FAILURE;
    }
    // From here on, we hope that the solution file is ok.
    // Interfaces between cells.
    for ( ix = A->ixmin - 1; ix <= A->ixmax; ++ix ) {
	struct L_cell* c = &( A->Cell[ix] );
	if (fgets(line, NCHAR, infile) == NULL) {
	    printf("Problem reading flow field file.\n");
	    return FAILURE;
	}
	if ( scan_iface_values_from_string(line, *c) ) {
	    printf("IFace for Cell[%d] failed to read from line:\n", ix);
	    printf("%s\n", line);
	    return FAILURE;
	}
    } // end for ix
    // The actual cells.
    for ( ix = A->ixmin; ix <= A->ixmax; ++ix ) {
	struct L_cell* c = &( A->Cell[ix] );
	if (fgets(line, NCHAR, infile) == NULL) {
	    printf("Problem reading flow field file.\n");
	    return FAILURE;
	}
	if ( scan_cell_values_from_string(line, *c) ) {
	    printf("Cell[%d] failed to read from line:\n", ix);
	    printf("%s\n", line);
	    return FAILURE;
	}
	double f_sum = c->gas->massf[0];
	for ( isp = 1; isp < nsp; ++isp ) {
	    f_sum += c->gas->massf[isp];
	}
	if ( fabs(f_sum - 1.0) > 0.001 ) {
	    printf("Species don't sum correctly %g\n", f_sum );
	    for ( isp = 1; isp < nsp; ++isp ) {
		printf("    %d %e\n", isp, c->gas->massf[isp]);
	    }
	    return FAILURE;
	}
    } // end for ix
    return SUCCESS;
#   undef NCHAR
} // end function L_read_solution


int L_write_solution(struct slug_data* A, FILE* outfile)
// Write the flow solution (i.e. the interface positions and
// the primary variables at the cell centers) to a file.
{
    Gas_model *gmodel = get_gas_model_ptr();
    int nsp = gmodel->get_number_of_species();
    int nmodes = gmodel->get_number_of_modes();
    fprintf(outfile, "%e  # begin slug data: sim_time\n", A->sim_time);
    fprintf(outfile, "%d %d %d # nnx, nsp\n", A->nnx, nsp, nmodes);
    // Interfaces between cells.
    for ( int ix = A->ixmin - 1; ix <= A->ixmax; ++ix ) {
        fprintf(outfile, "%s\n", write_iface_values_to_string(A->Cell[ix]).c_str());
    }
    // The actual cells.
    for ( int ix = A->ixmin; ix <= A->ixmax; ++ix ) {
        fprintf(outfile, "%s\n", write_cell_values_to_string(A->Cell[ix]).c_str());
    }
    fflush(outfile);
    return SUCCESS;
} // end function L_write_solution()


int L_write_cell_history(struct slug_data* A, FILE* hisfile)
// Write out the flow solution in a (small) subset of cells
// at a different (often smaller) time interval to the full
// flow solution.
{
    // The output format for this function needs to be kept the 
    // same as that for L_write_x_history().
    for ( int i = 0; i < A->hncell; ++i) {
        int ix = A->hxcell[i] + A->ixmin - 1;
	fprintf(hisfile, "%s\n", write_cell_values_to_string(A->Cell[ix]).c_str());
    } // end for i...
    fflush(hisfile);
    return SUCCESS;
} // end function L_write_cell_history


int L_write_x_history(double xloc, std::vector<slug_data> &A, int nslug, FILE* hisfile)
// Write out the flow solution at a specified x-location
// at a different (often smaller) time interval to the full
// flow solution.
// To get values at the end of a slug, say at a reflecting
// wall, it may be necessary to specify an x-location which
// will always fall within the last cell and not at the very edge.
// Input...
// xloc    : x-location
// A       : pointer to the array of slug_data structures
// nslug   : number of gas slugs
// hisfile : file to which data is to be written
{
    // The output format for this function needs to be kept the 
    // same as that for L_write_cell_history().
    Gas_model *gmodel = get_gas_model_ptr();
    struct L_cell* icell = new(struct L_cell);
    icell->gas = new Gas_data(gmodel);
    // Find the gas slug containing the x-location
    int found=0;
    for ( int js = 0; js < nslug; ++js ) {
        found = L_interpolate_cell_data(&(A[js]), xloc, *icell);
        if (found == 1) break;
    } // for (js = ...
    fprintf(hisfile, "%s\n", write_cell_values_to_string(*icell).c_str() );
    fflush(hisfile);
    delete icell->gas;
    delete icell;
    return SUCCESS;
} // end function L_write_x_history
