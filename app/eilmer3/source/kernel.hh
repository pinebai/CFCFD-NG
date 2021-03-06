/// \file kernel.hh
/// \ingroup eilmer3
/// \brief Header file for the kernel module.
///
/// Contains some configuration elements, also.

#ifndef KERNEL_HH
#define KERNEL_HH

#include <string>
#include "../../../lib/util/source/useful.h"
#include "../../../lib/gas/models/gas_data.hh"
#include "../../../lib/gas/models/gas-model.hh"
#include "../../../lib/gas/kinetics/reaction-update.hh"
#include "../../../lib/gas/kinetics/energy-exchange-update.hh"
#include "../../wallcon/source/e3conn.hh"
#include "c-flow-condition.hh"
#include "flux_calc.hh"
#include "cell.hh"
#include "block.hh"
#include "piston.hh"
#include "radiation_transport.hh"
#include "conj-ht-interface.hh"
#include "cht_coupling_enum.hh"

//-------------------------------------------------------------------

enum turbulence_model_t {TM_NONE, TM_BALDWIN_LOMAX, TM_K_OMEGA, TM_SPALART_ALLMARAS};

struct CHeatZone {
    double qdot;  // rate of heat addition in W/m**3
    double x0, y0, z0;
    double x1, y1, z1;
};

struct CIgnitionZone {
    double Tig;  // temperature to use with reaction_update, to ensure ignition
    double x0, y0, z0;
    double x1, y1, z1;
};

struct CReactionZone {
    double x0, y0, z0;
    double x1, y1, z1;
};

struct CTurbulentZone {
    double x0, y0, z0;
    double x1, y1, z1;
};

/** \brief Global data structure for control of the overall time-stepping. */
struct global_data
{
    size_t dimensions;      // 2 or 3 dimensions
    size_t nghost; // Number of ghost cells surrounding the active cells.
    bool axisymmetric;
    int verbosity_level; 
    // Messages have a hierarchy:
    // 0 : only error messages will be omitted
    // 1 : emit messages that are useful for a long-running job (default)
    // 2 : plus verbose init messages
    // 3 : plus verbose boundary condition messages
    // 4 : temporary messages for debugging
    FILE *logfile;          // log file handle
    FILE *timestampfile;
    FILE *fstctimesfile;
    std::string base_file_name;
    std::string title;

    size_t nblock;             // number of blocks in overall simulation
    // Aug-2012 rework of the block-handling code for MPI.
    // We eventually want to have each task/process look after 
    // a "bag" of blocks that may not be sequentially numbered.
    std::vector<Block> bd;  // The array of vectors of blocks, holding arrays of cells.
    std::vector<Block *> my_blocks; // Collection that we can iterate over.
    
    bool mpi_parallel;      // ==1 if we are using MPI parallel
    int num_mpi_proc;       // count of MPI tasks participating in the simulation
    int my_mpi_rank;        // identification for MPI process
    std::vector<int> mpi_rank_for_block; // process in which each block resides

    size_t npiston;            // number of pistons
    std::vector<Piston *> pistons;

    size_t step;                    // global iteration count
    size_t max_step;                // global iteration limit
    size_t t_level;                 // global time level within update
    int halt_now;                   // flag for premature halt
    bool halt_on_large_flow_change; // Set to true to halt simulation when any
                                    // monitor point sees a large flow change.
    double tolerance_in_T;          // Temperature change for the flow change.
    size_t print_count;   // Number of steps between writing status message to console.
    size_t control_count; // Number of steps between rereading .control file.

    /// When decoding the array of conserved quantities, 
    /// the temperature or the density may try to go negative.  
    /// If it does and adjust_invalid_cell_data == true, the cell data
    /// is adjusted to make it reasonable.
    bool adjust_invalid_cell_data;

    double sim_time;        /* present simulation time    */
    double max_time;        /* final solution time, s     */
    double dt_init;         /* initial time step          */
    double dt_global;       /* simulation time step       */
    double dt_allow;        /* allowable global time step */
    double CFL;             /* target CFL (worst case)    */
    double viscous_signal_factor; 
    // 2015-03-14: scale factor for the viscous stability condition
    // A value of 1.0 applies the usual viscous component.
    // The viscous contribution to the signal speed might
    // be usefully scaled down so that time steps do not go down
    // to crazily-small values for rarefied flows on fine grids. 
    bool stringent_cfl;     // If true, assume the worst with respect to cell geometry and wave speed.
    double dt_max;          // Maximum allowable time-step, after all other considerations.
    bool fixed_time_step;   /* flag for fixed time-stepping */
    int Xorder; // Low order reconstruction (1) uses just the cell-centre data as left- and right-
                // flow properties in the flux calculation.
                // High-order reconstruction (2) adds a correction term to the cell-centre values
                // to approach something like a piecewise-quadratic interpolation between the
                // cell centres.

    bool sequence_blocks;   // if true, iterate blocks sequentially (like space-marching)
    size_t max_invalid_cells;  // the maximum number of bad cells (per block) 
                            // which will be tolerated without too much complaint.
    double dt_reduction_factor; 
    /*
     * If an attempt at a time step fails because of invalid cells,
     * the time step is re-attempted with a smaller time step.
     * This reduction factor is somewhat arbitrary and can now be set
     * by the user's imput script.
     * A factor of 0.5 would seem to be not enough but a factor of
     * 0.1 would seem too expensive.  We have settled on a default of 0.2.
     */

    // The implicit mode encodes a number of options:
    //   0 normal explicit viscous updates, 
    //   1 point implicit viscous treatment,
    //   2 fully implicit viscous treatment.
    int implicit_mode;

    /// We might update some properties in with the main convective-terms
    /// time-stepping function or we might choose to update them separately, 
    /// like the chemistry update.
    bool separate_update_for_viscous_terms;
    bool separate_update_for_k_omega_source;

    bool viscous; // if true, viscous effects are included in the gas-dynamic update.
    // A factor to scale the viscosity in order to achieve a soft start. 
    // The soft-start for viscous effects may be handy for impulsively-started flows.
    // A value of 1.0 means that the viscous effects are fully applied.
    double viscous_factor;
    // The amount by which to increment the viscous factor during soft-start.
    double viscous_factor_increment;
    double viscous_time_delay;

    // When the diffusion is calculated is treated as part of the viscous calculation:
    //   false for neglecting multicomponent diffusion, 
    //   true when considering the diffusion 
    bool diffusion; 
    // A factor to scale the diffusion in order to achieve a soft start, separate to viscous effects.
    // The soft-start for diffusion effects may be handy for impulsively-started flows.
    double diffusion_factor;
    // The amount by which to increment the diffusion factor during soft-start.
    double diffusion_factor_increment;
    double diffusion_time_delay;
    // The Lewis number when using the constant Lewis number diffusion model
    double diffusion_lewis;
    // The Schmidt number when using the constant Schmidt number diffusion model
    double diffusion_schmidt;

    turbulence_model_t turbulence_model;
    double turbulence_prandtl;
    double turbulence_schmidt;
    double max_mu_t_factor;
    double transient_mu_t_factor;

    bool shock_fitting;
    bool shock_fitting_decay;
    double shock_fitting_speed_factor;
    
    bool moving_grid;              /* moving grid flag */
    bool write_vertex_velocities;  /* write vertex velocities flag */
    bool flow_induced_moving;      /* flow induced moving flag */
    double dt_moving_allow;        /* allowable global time step for moving grid */
    double t_moving;               /* time to next adapt new vertex velocity    */
    double dt_moving;              /* interval for running setting vertex velocity for moving grid  */
    double cfl_moving_target;      /* target CFL value for moving grid    */       

    bool wall_function;              /* wall function flag */ 
    bool artificial_diffusion;       /* artificial diffusion flag */
    double artificial_kappa_2;       /* coefficient for the second order artificial diffussion flux */
    double artificial_kappa_4;       /* coefficient for the fourth order artificial diffussion flux */        

    /// Set the tolerance in relative velocity change for the shock detector.
    /// This value is expected to be a negative number (for compression)
    /// and not too large in magnitude.
    /// We have been using a value of -0.05 for years, based on some
    /// early experiments with the sod and cone20 test cases, however,
    /// the values may need to be tuned for other cases, especially where
    /// viscous effects are important.
    double compression_tolerance;

    /// Set the tolerance to shear when applying the adaptive flux calculator.
    /// We don't want EFM to be applied in situations of significant shear.
    /// The shear value is computed as the tangential-velocity difference across an interface
    /// normalised by the local sound speed.
    double shear_tolerance;

    double t_plot;          /* time to write next soln    */
    size_t write_at_step;   /* update step at which to write a solution, 0=don't do it */
    double t_his;           /* time to write next sample  */
    double t_fstc;          /* time to write next fluid-structure exchange data*/  
    double dt_plot;         /* interval for writing soln  */
    double dt_his;          /* interval for writing sample */
    double dt_fstc;         /* interval for writing next f-s exchange data*/

    double cfl_target;      /* target CFL (worst case)    */
    size_t cfl_count;          /* check CFL occasionally     */
    double cfl_min;         /* current CFL minimum        */
    double cfl_max;         /* current CFL maximum        */
    double cfl_tiny;        /* smallest cfl so far        */
    double time_tiny;       /* time at which it occurred  */

    double L_min;           /* minimum cell size */

    double energy_residual; /* to be monitored for steady state */
    double mass_residual;
    Vector3 energy_residual_loc, mass_residual_loc; /* location of largest value */

    std::vector<CFlowCondition*> gas_state; /* gas,flow properties */
    size_t n_gas_state;

    // Filter application parameters.
    bool   filter_flag;
    double filter_tstart;
    double filter_tend;
    double filter_dt;
    double filter_next_time;
    double filter_mu;
    size_t filter_npass;

    // variables for Andrew's time averaging routine.
    size_t nav;
    double tav_0, tav_f, dtav;
    double tav;
    int do_tavg;

    // variables for profile recording.
    int do_record;
    int block_record, x_record, n_record, step_record;
    double t_record, t_start;

    // variables for time dependent profile input.
    double t_old, ta_old;
    int n_vary;
    int do_vary_in;

    // these variables are all for special cases
    int secondary_diaphragm_ruptured, d2_ruptured, dn_ruptured;
    double diaphragm_rupture_time, diaphragm_rupture_diameter;
    int diaphragm_block, drummond_progressive;

    // Turning on the reactions activates the chemical update function calls.
    // Chemical equilibrium simulations (via Look-Up Table) does not use this
    // chemical update function call.
    bool reacting;

    // With this flag on, finite-rate evolution of the vibrational energies 
    // (and in turn the total energy) is computed.
    bool thermal_energy_exchange;

    bool radiation;
    int radiation_update_frequency; // = 1 for every time-step
    bool radiation_scaling;

    bool electric_field_work;

    // For Daryl Bond and Vince Wheatley's MHD additions.
    bool MHD;      // flag indicating MHD effects are to be calculated
    bool div_clean;  // flag indicating whether divergence cleaning should be run
    double c_h;    // advection velocity for divergence cleaning
    double divB_damping_length;  // measure of the domain size
    Vector3 bounding_box_min; // minimum point of bounding box
    Vector3 bounding_box_max; // maximum point of bounding box

    // A flag for turning on the BGK non-equilibrium gas solver:
    //   BGK == 0: OFF
    //   BGK == 1: ON, do not try to import velocity distribution values
    //   BGK == 2: ON, read in velocity distribution values from "flow" file
    int BGK;

    size_t n_heat_zone;
    double heat_time_start;
    double heat_time_stop;
    std::vector<struct CHeatZone> heat_zone;
    double heat_factor; // A factor to scale the heat-addition in order to achieve a soft start.
    double heat_factor_increment;

    size_t n_ignition_zone;
    double ignition_time_start;
    double ignition_time_stop;
    std::vector<struct CIgnitionZone> ignition_zone;
    bool ignition_zone_active;

    size_t n_reaction_zone;
    double reaction_time_start;
    double T_frozen; // temperature (in K) below which reactions are frozen
    double T_frozen_energy; // temperature (in K) below which energy exchanges are skipped
    std::vector<struct CReactionZone> reaction_zone;

    size_t n_turbulent_zone;
    std::vector<struct CTurbulentZone> turbulent_zone;

    std::string udf_file; // This file will contain user-defined procedures.
    int udf_source_vector_flag; // set to 1 to use (expensive) user-defined source terms
    int udf_vtx_velocity_flag; // set to 1 to use (expensive) user-defined vextex velocity    

    // variables related to a wall model for conjugate heat transfer
    bool conjugate_ht_active; // if true, enables the conjugate heat transfer computation at a wall
    cht_coupling_t cht_coupling; // coupling mode between flow solver and wall solver
    size_t wall_update_count; // no. steps to take before updating wall values (for loosely-coupled approach)
    double dt_acc; // Timestep for wall-update when accumulating many steps of flow solver
    Wall_model *wm;
    Conjugate_HT_Interface *conj_ht_iface;
    std::vector<double> T_gas_near_wall;
    std::vector<double> k_gas_near_wall;
    std::vector<double> T_solid_near_wall;
    std::vector<double> k_solid_near_wall;
    std::vector<double> q_wall;
    std::vector<double> T_wall;
    std::vector<int> recvcounts;
    std::vector<int> displs;
};

//---------------------------------------------------------------
// Function declarations for things that don't fit neatly into
// the global_data structure.

std::string get_revision_string();
global_data * get_global_data_ptr(void);
Gas_model *set_gas_model_ptr(Gas_model *gmptr);
Gas_model *get_gas_model_ptr();
int set_reaction_update(std::string file_name);
Reaction_update *get_reaction_update_ptr();
int set_energy_exchange_update( std::string file_name );
Energy_exchange_update *get_energy_exchange_update_ptr();
int set_radiation_transport_model(std::string file_name);
RadiationTransportModel *get_radiation_transport_model_ptr();
Block * get_block_data_ptr(size_t i);
void eilmer_finalize( void );

double incr_viscous_factor( double value );
double incr_diffusion_factor( double value );
double incr_heat_factor( double value );

size_t set_velocity_buckets(size_t i);
size_t get_velocity_buckets( void );
Vector3 get_vcoord(int i);
std::vector<Vector3> *get_vcoords_ptr(void);
double get_vweight(int i);
std::vector<double> *get_vweights_ptr(void);

void update_MHD_c_h(void);

std::string get_name_of_turbulence_model(turbulence_model_t my_model);
#endif
