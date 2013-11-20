#ifndef EILMER3_CONN_HH
#define EILMER3_CONN_HH

#include "init.hh"

struct Wall_model {
	list_of_inputs inputs;
	list_of_vars vars;
};

//Functions

Wall_model* initialise_wall_model(std::string fname);
int grab_config_from_file(string fname, Wall_model &wm);
int sv(Wall_model &wm);
int initialise_wall_node_positions(Wall_model &wm, const vector<double> &wall_xs, const vector<double> &wall_ys);
int update_temperatures_from_fluxes(Wall_model &wm, double dt, const vector<double> q_wall, vector<double> &T_wall);
int write_soln(Wall_model &wm, double time_elapsed, int print_number);

#endif

