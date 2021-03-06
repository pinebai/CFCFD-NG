// bc_fixed_t.cxx

#include "../../../lib/util/source/useful.h"
#include "../../../lib/gas/models/gas_data.hh"
#include "../../../lib/gas/models/gas-model.hh"
#include "../../../lib/gas/models/physical_constants.hh"
#include "block.hh"
#include "kernel.hh"
#include "bc.hh"
#include "bc_fixed_t.hh"
#include "bc_catalytic.hh"
#include "bc_menter_correction.hh"

//------------------------------------------------------------------------

FixedTBC::FixedTBC(Block *bdp, int which_boundary, double Twall, double _emissivity)
    : BoundaryCondition(bdp, which_boundary, FIXED_T),
      Twall(Twall)
{
    is_wall_flag = true;
    emissivity = _emissivity;
}

FixedTBC::FixedTBC(const FixedTBC &bc)
    : BoundaryCondition(bc.bdp, bc.which_boundary, bc.type_code),
      Twall(bc.Twall) 
{
    is_wall_flag = bc.is_wall_flag;
    emissivity = bc.emissivity;
}

FixedTBC::FixedTBC()
    : BoundaryCondition(0, 0, FIXED_T),
      Twall(300.0)
{
    is_wall_flag = true;
    emissivity = 1.0;
}

FixedTBC & FixedTBC::operator=(const FixedTBC &bc)
{
    BoundaryCondition::operator=(bc);
    Twall = bc.Twall; // Ok for self-assignment.
    return *this;
}

FixedTBC::~FixedTBC() {}

void FixedTBC::print_info(std::string lead_in)
{
    BoundaryCondition::print_info(lead_in);
    cout << lead_in << "Twall= " << Twall << endl;
    return;
}

int FixedTBC::apply_viscous(double t)
{
    size_t i, j, k;
    FV_Cell *cell;
    FV_Interface *IFace;
    size_t nmodes = get_gas_model_ptr()->get_number_of_modes();
    Block & bd = *bdp;
    global_data &G = *get_global_data_ptr();    

    switch ( which_boundary ) {
    case NORTH:
	j = bd.jmax;
	for (k = bd.kmin; k <= bd.kmax; ++k) {
	    for (i = bd.imin; i <= bd.imax; ++i) {
		cell = bd.get_cell(i,j,k);
		IFace = cell->iface[NORTH];
		FlowState &fs = *(IFace->fs);
		fs.copy_values_from(*(cell->fs));
		fs.vel.x = 0.0; fs.vel.y = 0.0; fs.vel.z = 0.0;
		if ( G.moving_grid ) {
		    IFace->ivel.transform_to_local(IFace->n, IFace->t1, IFace->t2);
		    fs.vel.transform_to_local(IFace->n, IFace->t1, IFace->t2);		    
		    fs.vel.y = IFace->ivel.y;
		    fs.vel.z = IFace->ivel.z;
                    fs.vel.transform_to_global(IFace->n, IFace->t1, IFace->t2);		    
                    IFace->ivel.transform_to_global(IFace->n, IFace->t1, IFace->t2);
                }
		for ( size_t imode=0; imode < nmodes; ++imode ) fs.gas->T[imode] = Twall;
		// [TODO] should we re-evaluate the thermo and transport coeffs?
		fs.tke = 0.0;
		fs.omega = ideal_omega_at_wall(cell);
		if (bd.bcp[NORTH]->wc_bc != NON_CATALYTIC) {
		    cw->apply(*(cell->fs->gas), fs.gas->massf);
		}
	    } // end i loop
	} // for k
	break;
    case EAST:
	i = bd.imax;
	for (k = bd.kmin; k <= bd.kmax; ++k) {
	    for (j = bd.jmin; j <= bd.jmax; ++j) {
		cell = bd.get_cell(i,j,k);
		IFace = cell->iface[EAST];
		FlowState &fs = *(IFace->fs);
		fs.copy_values_from(*(cell->fs));
		fs.vel.x = 0.0; fs.vel.y = 0.0; fs.vel.z = 0.0;
		if ( G.moving_grid ) {
		    IFace->ivel.transform_to_local(IFace->n, IFace->t1, IFace->t2);
		    fs.vel.transform_to_local(IFace->n, IFace->t1, IFace->t2);		    
		    fs.vel.y = IFace->ivel.y;
		    fs.vel.z = IFace->ivel.z;
                    fs.vel.transform_to_global(IFace->n, IFace->t1, IFace->t2);		    
                    IFace->ivel.transform_to_global(IFace->n, IFace->t1, IFace->t2);
                }		
		for ( size_t imode=0; imode < nmodes; ++imode ) fs.gas->T[imode] = Twall;
		fs.tke = 0.0;
		fs.omega = ideal_omega_at_wall(cell);
		if (bd.bcp[EAST]->wc_bc != NON_CATALYTIC) {
		    cw->apply(*(cell->fs->gas), fs.gas->massf);
		}
	    } // end j loop
	} // for k
	break;
    case SOUTH:
	j = bd.jmin;
	for (k = bd.kmin; k <= bd.kmax; ++k) {
	    for (i = bd.imin; i <= bd.imax; ++i) {
		cell = bd.get_cell(i,j,k);
		IFace = cell->iface[SOUTH];
		FlowState &fs = *(IFace->fs);
		fs.copy_values_from(*(cell->fs));
		fs.vel.x = 0.0; fs.vel.y = 0.0; fs.vel.z = 0.0;
		if ( G.moving_grid ) {
		    IFace->ivel.transform_to_local(IFace->n, IFace->t1, IFace->t2);
		    fs.vel.transform_to_local(IFace->n, IFace->t1, IFace->t2);		    
		    fs.vel.y = IFace->ivel.y;
		    fs.vel.z = IFace->ivel.z;
                    fs.vel.transform_to_global(IFace->n, IFace->t1, IFace->t2);		    
                    IFace->ivel.transform_to_global(IFace->n, IFace->t1, IFace->t2);
                }		
		for ( size_t imode=0; imode < nmodes; ++imode ) fs.gas->T[imode] = Twall;
		fs.tke = 0.0;
		fs.omega = ideal_omega_at_wall(cell);
		if (bd.bcp[SOUTH]->wc_bc != NON_CATALYTIC) {
		    cw->apply(*(cell->fs->gas), fs.gas->massf);
		}
	    } // end i loop
	} // for k
	break;
    case WEST:
	i = bd.imin;
	for (k = bd.kmin; k <= bd.kmax; ++k) {
	    for (j = bd.jmin; j <= bd.jmax; ++j) {
		cell = bd.get_cell(i,j,k);
		IFace = cell->iface[WEST];
		FlowState &fs = *(IFace->fs);
		fs.copy_values_from(*(cell->fs));
		fs.vel.x = 0.0; fs.vel.y = 0.0; fs.vel.z = 0.0;
		if ( G.moving_grid ) {
		    IFace->ivel.transform_to_local(IFace->n, IFace->t1, IFace->t2);
		    fs.vel.transform_to_local(IFace->n, IFace->t1, IFace->t2);		    
		    fs.vel.y = IFace->ivel.y;
		    fs.vel.z = IFace->ivel.z;
                    fs.vel.transform_to_global(IFace->n, IFace->t1, IFace->t2);		    
                    IFace->ivel.transform_to_global(IFace->n, IFace->t1, IFace->t2);
                }		
		for ( size_t imode=0; imode < nmodes; ++imode ) fs.gas->T[imode] = Twall;
		fs.tke = 0.0;
		fs.omega = ideal_omega_at_wall(cell);
		if (bd.bcp[WEST]->wc_bc != NON_CATALYTIC) {
		    cw->apply(*(cell->fs->gas), fs.gas->massf);
		}
	    } // end j loop
	} // for k
 	break;
    case TOP:
	k = bd.kmax;
	for (i = bd.imin; i <= bd.imax; ++i) {
	    for (j = bd.jmin; j <= bd.jmax; ++j) {
		cell = bd.get_cell(i,j,k);
		IFace = cell->iface[TOP];
		FlowState &fs = *(IFace->fs);
		fs.copy_values_from(*(cell->fs));
		fs.vel.x = 0.0; fs.vel.y = 0.0; fs.vel.z = 0.0;
		if ( G.moving_grid ) {
		    IFace->ivel.transform_to_local(IFace->n, IFace->t1, IFace->t2);
		    fs.vel.transform_to_local(IFace->n, IFace->t1, IFace->t2);		    
		    fs.vel.y = IFace->ivel.y;
		    fs.vel.z = IFace->ivel.z;
                    fs.vel.transform_to_global(IFace->n, IFace->t1, IFace->t2);		    
                    IFace->ivel.transform_to_global(IFace->n, IFace->t1, IFace->t2);
                }		
		for ( size_t imode=0; imode < nmodes; ++imode ) fs.gas->T[imode] = Twall;
		fs.tke = 0.0;
		fs.omega = ideal_omega_at_wall(cell);
		if (bd.bcp[TOP]->wc_bc != NON_CATALYTIC) {
		    cw->apply(*(cell->fs->gas), fs.gas->massf);
		}
	    } // end j loop
	} // for i
	break;
    case BOTTOM:
	k = bd.kmin;
	for (i = bd.imin; i <= bd.imax; ++i) {
	    for (j = bd.jmin; j <= bd.jmax; ++j) {
		cell = bd.get_cell(i,j,k);
		IFace = cell->iface[BOTTOM];
		FlowState &fs = *(IFace->fs);
		fs.copy_values_from(*(cell->fs));
		fs.vel.x = 0.0; fs.vel.y = 0.0; fs.vel.z = 0.0;
		if ( G.moving_grid ) {
		    IFace->ivel.transform_to_local(IFace->n, IFace->t1, IFace->t2);
		    fs.vel.transform_to_local(IFace->n, IFace->t1, IFace->t2);		    
		    fs.vel.y = IFace->ivel.y;
		    fs.vel.z = IFace->ivel.z;
                    fs.vel.transform_to_global(IFace->n, IFace->t1, IFace->t2);		    
                    IFace->ivel.transform_to_global(IFace->n, IFace->t1, IFace->t2);
                }		
		for ( size_t imode=0; imode < nmodes; ++imode ) fs.gas->T[imode] = Twall;
		fs.tke = 0.0;
		fs.omega = ideal_omega_at_wall(cell);
		if (bd.bcp[BOTTOM]->wc_bc != NON_CATALYTIC) {
		    cw->apply(*(cell->fs->gas), fs.gas->massf);
		}
	    } // end j loop
	} // for i
 	break;
    default:
	printf( "Error: apply_viscous not implemented for boundary %d\n", 
		which_boundary );
	return NOT_IMPLEMENTED_ERROR;
    }
    return SUCCESS;
} // end FixedTBC::apply_viscous()
