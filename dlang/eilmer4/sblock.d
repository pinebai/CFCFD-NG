// sblock.d
// Class for structured blocks of cells, for use within Eilmer4.
// This is the "classic" block within the mbcns/Eilmer series 
// of flow simulation codes.

// Peter J. 2014-07-20 first cut.

module sblock;

import std.conv;
import std.file;
import std.json;
import std.stdio;
import std.format;
import std.string;
import std.array;
import std.math;
import gzip;
import geom;
import gasmodel;
import globalconfig;
import fvcore;
import fvvertex;
import fvinterface;
import fvcell;
import block;
import bc;
import bc_slip_wall;
import bc_full_face_exchange;
import bc_mapped_cell_exchange;

class SBlock: Block {
public:
    size_t nicell;
    size_t njcell;
    size_t nkcell;
    size_t imin, imax;
    size_t jmin, jmax;
    size_t kmin, kmax;
    size_t[] hicell, hjcell, hkcell; // locations of sample cells for history record
    size_t[] micell, mjcell, mkcell; // locations of monitor cells

private:
    // Total number of cells in each direction for this block.
    // these will be used in the array allocation routines.
    size_t _nidim;
    size_t _njdim;
    size_t _nkdim;
    // Most of the data is stored in the following arrays.
    // ctr = cell center values
    // ifi = east-facing face properties and fluxes (unit normal in the i-index direction)
    // ifj = north-facing face properties and fluxes (normal in the j-index direction)
    // ifk = top-facing 
    // vtx = cell vertex values (used for the viscous terms, mostly)
    // sifi, sifj and sifk are secondary-cell faces (whose corner nodes are the
    //                     the primary-cell centres.
    FVCell[] _ctr;
    FVInterface[] _ifi;
    FVInterface[] _ifj;
    FVInterface[] _ifk;
    FVVertex[] _vtx;
    FVInterface[] _sifi;
    FVInterface[] _sifj;
    FVInterface[] _sifk;

public:
    this(int id, in char[] file_name)
    // Need to create blocks in the context of the GlobalConfig.
    {
	this.id = id;
	if ( file_name.length == 0 ) {
	    throw new Error("We need a file from which to read the block parameters.");
	}
	auto text = cast(string) read(file_name);
	auto items = parseJSON(text);
	nicell = to!size_t(items["nicell"].integer);
	njcell = to!size_t(items["njcell"].integer);
	nkcell = to!size_t(items["nkcell"].integer);
	_nidim = nicell + 2 * nghost;
	_njdim = njcell + 2 * nghost;
	// Indices, in each grid direction for the active cells.
	// These limits are inclusive. The mincell and max cell
	// are both within the active set of cells.
	imin = nghost; imax = imin + nicell - 1;
	jmin = nghost; jmax = jmin + njcell - 1;
	if ( GlobalConfig.dimensions == 2 ) {
	    // In 2D simulations, the k range is from 0 to 0 for the
	    // storage arrays of cells and relevant faces.
	    if ( nkcell != 1 ) {
		writeln("Warning: inconsistent dimensions nkcell set to 1 for 2D");
		nkcell = 1;
	    }
	    _nkdim = 1;
	    kmin = 0; kmax = 0;
	} else {
	    // In 3D simulations the k index is just like the i and j indices.
	    _nkdim = nkcell + 2 * nghost;
	    kmin = nghost; kmax = kmin + nkcell - 1;
	}
    } // end constructor

    override string toString() const
    {
	char[] repr;
	repr ~= "SBlock(";
	repr ~= "id=" ~ to!string(id);
	repr ~= ", nicell=" ~ to!string(nicell);
	repr ~= ", njcell=" ~ to!string(njcell);
	repr ~= ", nkcell=" ~ to!string(nkcell);
	repr ~= ")";
	return to!string(repr);
    }

    size_t to_global_index(size_t i, size_t j, size_t k) const
    {
	if ( k < 0 || k >= _nkdim || j < 0 || j >= _njdim || i < 0 || i >= _nidim ) {
	    throw new Error(text("SBlock:to_global_index: index out of bounds for block[", id,
				 "] i=", i, " j=", j, " k=", k, " nidim=", _nidim, 
				 " njdim=", _njdim, " nkdim=", _nkdim));
	}
	return k * (_njdim * _nidim) + j * _nidim + i; 
    }

    size_t[] to_ijk_indices(size_t gid) const
    {
	size_t k = gid / (_njdim * _nidim);
	size_t j = (gid - k * (_njdim * _nidim)) / _nidim;
	size_t i = gid - k * (_njdim * _nidim) - j * _nidim;
	return [i, j, k];
    }

    ref FVCell get_cell(size_t i, size_t j, size_t k=0) { return _ctr[to_global_index(i,j,k)]; }
    ref FVInterface get_ifi(size_t i, size_t j, size_t k=0) { return _ifi[to_global_index(i,j,k)]; }
    ref FVInterface get_ifj(size_t i, size_t j, size_t k=0) { return _ifj[to_global_index(i,j,k)]; }
    ref FVInterface get_ifk(size_t i, size_t j, size_t k=0) { return _ifk[to_global_index(i,j,k)]; }
    ref FVVertex get_vtx(size_t i, size_t j, size_t k=0) { return _vtx[to_global_index(i,j,k)]; }
    ref FVInterface get_sifi(size_t i, size_t j, size_t k=0) { return _sifi[to_global_index(i,j,k)]; }
    ref FVInterface get_sifj(size_t i, size_t j, size_t k=0) { return _sifj[to_global_index(i,j,k)]; }
    ref FVInterface get_sifk(size_t i, size_t j, size_t k=0) { return _sifk[to_global_index(i,j,k)]; }

    override void assemble_arrays()
    // We shouldn't be calling this until the essential bits of the GlobalConfig
    // have been set up.
    {
	if ( GlobalConfig.verbosity_level >= 2 ) 
	    writefln("assemble_arrays(): Begin for block %d", id);
	auto gm = GlobalConfig.gmodel;
	// Check for obvious errors.
	if ( _nidim <= 0 || _njdim <= 0 || _nkdim <= 0 ) {
	    throw new Error(text("resize_arrays(): invalid dimensions nidim=",
				 _nidim, " njdim=", _njdim, " nkdim=", _nkdim));
	}
	size_t ntot = _nidim * _njdim * _nkdim;
	try {
	    // Create the cell and interface objects for the entire block.
	    foreach (gid; 0 .. ntot) {
		_ctr ~= new FVCell(gm); _ctr[gid].id = to!int(gid);
		auto ijk = to_ijk_indices(gid);
		if ( ijk[0] >= imin && ijk[0] <= imax && 
		     ijk[1] >= jmin && ijk[1] <= jmax && 
		     ijk[2] >= kmin && ijk[2] <= kmax ) {
		    active_cells ~= _ctr[gid];
		}
		_ifi ~= new FVInterface(gm); _ifi[gid].id = gid;
		_ifj ~= new FVInterface(gm); _ifj[gid].id = gid;
		if ( GlobalConfig.dimensions == 3 ) {
		    _ifk ~= new FVInterface(gm); _ifk[gid].id = gid;
		}
		_vtx ~= new FVVertex(gm); _vtx[gid].id = gid;
		_sifi ~= new FVInterface(gm); _sifi[gid].id = gid;
		_sifj ~= new FVInterface(gm); _sifj[gid].id = gid;
		if ( GlobalConfig.dimensions == 3 ) {
		    _sifk ~= new FVInterface(gm); _sifk[gid].id = gid;
		}
	    } // gid loop
	} catch (Error err) {
	    writeln("Crapped out while assembling block arrays.");
	    writefln("nicell=%d njcell=%d nkcell=%d", nicell, njcell, nkcell);
	    writefln("nidim=%d njdim=%d nkdim=%d", _nidim, _njdim, _nkdim);
	    writeln("Probably ran out of memory.");
	    writeln("Be a little less ambitious and try a smaller grid next time.");
	    writefln("System message: %s", err.msg);
	    throw new Error("Block.assemble_arrays() failed.");
	}
	if ( GlobalConfig.verbosity_level >= 2 ) {
	    writefln("Done assembling arrays for %d cells.", ntot);
	}
    } // end of assemble_arrays()

    override void bind_faces_and_vertices_to_cells()
    // There is a fixed order of faces and vertices for each cell.
    // Refer to fvcore.d
    {
	size_t kstart, kend;
	if ( GlobalConfig.dimensions == 3 ) {
	    kstart = kmin - 1;
	    kend = kmax + 1;
	} else {
	    kstart = 0;
	    kend = 0;
	}
	// With these ranges, we also do the first layer of ghost cells.
	for ( size_t k = kstart; k <= kend; ++k ) {
	    for ( size_t j = jmin-1; j <= jmax+1; ++j ) {
		for ( size_t i = imin-1; i <= imax+1; ++i ) {
		    FVCell cell = get_cell(i,j,k);
		    cell.iface ~= get_ifj(i,j+1,k); // north
		    cell.iface ~= get_ifi(i+1,j,k); // east
		    cell.iface ~= get_ifj(i,j,k); // south
		    cell.iface ~= get_ifi(i,j,k); // west
		    cell.vtx ~= get_vtx(i,j,k);
		    cell.vtx ~= get_vtx(i+1,j,k);
		    cell.vtx ~= get_vtx(i+1,j+1,k);
		    cell.vtx ~= get_vtx(i,j+1,k);
		    if ( GlobalConfig.dimensions == 3 ) {
			cell.iface ~= get_ifk(i,j,k+1); // top
			cell.iface ~= get_ifk(i,j,k); // bottom
			cell.vtx ~= get_vtx(i,j,k+1);
			cell.vtx ~= get_vtx(i+1,j,k+1);
			cell.vtx ~= get_vtx(i+1,j+1,k+1);
			cell.vtx ~= get_vtx(i,j+1,k+1);
		    } // end if
		} // for i
	    } // for j
	} // for k
    } // end bind_faces_and_vertices_to_cells()

    override void identify_reaction_zones(int gtl)
    // Set the reactions-allowed flag for cells in this block.
    {
	size_t total_cells_in_reaction_zones = 0;
	size_t total_cells = 0;
	foreach(cell; active_cells) {
	    if ( GlobalConfig.reaction_zones.length > 0 ) {
		cell.fr_reactions_allowed = false;
		foreach(rz; GlobalConfig.reaction_zones) {
		    if ( rz.is_inside(cell.pos[gtl], GlobalConfig.dimensions) ) {
			cell.fr_reactions_allowed = true;
		    }
		} // foreach rz
	    } else {
		cell.fr_reactions_allowed = true;
	    }
	    total_cells_in_reaction_zones += (cell.fr_reactions_allowed ? 1: 0);
	    total_cells += 1;
	} // foreach cell
	if ( GlobalConfig.reacting && GlobalConfig.verbosity_level >= 2 ) {
	    writeln("identify_reaction_zones(): block ", id,
		    " cells inside zones = ", total_cells_in_reaction_zones, 
		    " out of ", total_cells);
	    if ( GlobalConfig.reaction_zones.length == 0 ) {
		writeln("Note that for no user-specified zones,",
			" the whole domain is allowed to be reacting.");
	    }
	}
    } // end identify_reaction_zones()

    override void identify_turbulent_zones(int gtl)
    // Set the in-turbulent-zone flag for cells in this block.
    {
	size_t total_cells_in_turbulent_zones = 0;
	size_t total_cells = 0;
	foreach(cell; active_cells) {
	    if ( GlobalConfig.turbulent_zones.length > 0 ) {
		cell.in_turbulent_zone = false;
		foreach(tz; GlobalConfig.turbulent_zones) {
		    if ( tz.is_inside(cell.pos[gtl], GlobalConfig.dimensions) ) {
			cell.in_turbulent_zone = true;
		    }
		} // foreach tz
	    } else {
		cell.in_turbulent_zone = true;
	    }
	    total_cells_in_turbulent_zones += (cell.in_turbulent_zone ? 1: 0);
	    total_cells += 1;
	} // foreach cell
	if ( GlobalConfig.turbulence_model != tm_none && 
	     GlobalConfig.verbosity_level >= 2 ) {
	    writeln("identify_turbulent_zones(): block ", id,
		    " cells inside zones = ", total_cells_in_turbulent_zones, 
		    " out of ", total_cells);
	    if ( GlobalConfig.turbulent_zones.length == 0 ) {
		writeln("Note that for no user-specified zones,",
			" the whole domain is allowed to be turbulent.");
	    }
	}
    } // end identify_turbulent_zones()

    override void clear_fluxes_of_conserved_quantities()
    {
	for ( size_t k = kmin; k <= kmax; ++k ) {
	    for (size_t j = jmin; j <= jmax; ++j) {
		for (size_t i = imin; i <= imax+1; ++i) {
		    get_ifi(i,j,k).F.clear_values();
		} // for i
	    } // for j
	} // for k
	for ( size_t k = kmin; k <= kmax; ++k ) {
	    for (size_t j = jmin; j <= jmax+1; ++j) {
		for (size_t i = imin; i <= imax; ++i) {
		    get_ifj(i,j,k).F.clear_values();
		} // for i
	    } // for j
	} // for k
	if ( GlobalConfig.dimensions == 3 ) {
	    for ( size_t k = kmin; k <= kmax+1; ++k ) {
		for (size_t j = jmin; j <= jmax; ++j) {
		    for (size_t i = imin; i <= imax; ++i) {
			get_ifk(i,j,k).F.clear_values();
		    } // for i
		} // for j
	    } // for k
	} // end if G.dimensions == 3
    } // end clear_fluxes_of_conserved_quantities()

    override int count_invalid_cells(int gtl)
    // Returns the number of cells that contain invalid data.
    //
    // This data can be identified by the density of internal energy 
    // being on the minimum limit or the velocity being very large.
    {
	int number_of_invalid_cells = 0;
	foreach(FVCell cell; active_cells) {
	    if ( cell.check_flow_data() == false ) {
		++number_of_invalid_cells;
		auto ijk = to_ijk_indices(cell.id);
		size_t i = ijk[0]; size_t j = ijk[1]; size_t k = ijk[2];
		writefln("count_invalid_cells: block_id = %d, cell[%d,%d,%d]\n", id, i, j, k);
		writeln(cell);
		if ( GlobalConfig.adjust_invalid_cell_data ) {
		    // We shall set the cell data to something that
		    // is valid (and self consistent).
		    FVCell other_cell;
		    FVCell[] neighbours;
		    printf( "Adjusting cell data to a local average.\n" );
		    other_cell = get_cell(i-1,j,k);
		    if ( other_cell.check_flow_data() ) neighbours ~= other_cell;
		    other_cell = get_cell(i+1,j,k);
		    if ( other_cell.check_flow_data() ) neighbours ~= other_cell;
		    other_cell = get_cell(i,j-1,k);
		    if ( other_cell.check_flow_data() ) neighbours ~= other_cell;
		    other_cell = get_cell(i,j+1,k);
		    if ( other_cell.check_flow_data() ) neighbours ~= other_cell;
		    if ( GlobalConfig.dimensions == 3 ) {
			other_cell = get_cell(i,j,k-1);
			if ( other_cell.check_flow_data() ) neighbours ~= other_cell;
			other_cell = get_cell(i,j,k+1);
			if ( other_cell.check_flow_data() ) neighbours ~= other_cell;
		    }
		    if ( neighbours.length == 0 ) {
			throw new Error(text("Block::count_invalid_cells(): "
					     "There were no valid neighbours to replace cell data."));
		    }
		    cell.replace_flow_data_with_average(neighbours);
		    cell.encode_conserved(gtl, 0, omegaz);
		    cell.decode_conserved(gtl, 0, omegaz);
		    writefln("after flow-data replacement: block_id = %d, cell[%d,%d,%d]\n",
			     id, i, j, k);
		    writeln(cell);
		} // end adjust_invalid_cell_data 
	    } // end of if invalid data...
	} // foreach cell
	return number_of_invalid_cells;
    } // end count_invalid_cells()

    override void init_residuals()
    // Initialization of data for later computing residuals.
    {
	mass_residual = 0.0;
	mass_residual_loc = Vector3(0.0, 0.0, 0.0);
	energy_residual = 0.0;
	energy_residual_loc = Vector3(0.0, 0.0, 0.0);
	foreach(FVCell cell; active_cells) {
	    cell.rho_at_start_of_step = cell.fs.gas.rho;
	    cell.rE_at_start_of_step = cell.U[0].total_energy;
	}
    } // end init_residuals()

    override void compute_residuals(int gtl)
    // Compute the residuals using previously stored data.
    //
    // The largest residual of density for all cells was the traditional way
    // mbcns/Elmer estimated the approach to steady state.
    // However, with the splitting up of the increments for different physical
    // processes, this residual calculation code needed a bit of an update.
    // Noting that the viscous-stress, chemical and radiation increments
    // do not affect the mass within a cell, we now compute the residuals 
    // for both mass and (total) energy for all cells, the record the largest
    // with their location. 
    {
	mass_residual = 0.0;
	mass_residual_loc = Vector3(0.0, 0.0, 0.0);
	energy_residual = 0.0;
	energy_residual_loc = Vector3(0.0, 0.0, 0.0);
	foreach(FVCell cell; active_cells) {
	    double local_residual = (cell.fs.gas.rho - cell.rho_at_start_of_step) 
		/ cell.fs.gas.rho;
	    local_residual = fabs(local_residual);
	    if ( local_residual > mass_residual ) {
		mass_residual = local_residual;
		mass_residual_loc.refx = cell.pos[gtl].x;
		mass_residual_loc.refy = cell.pos[gtl].y;
		mass_residual_loc.refz = cell.pos[gtl].z;
	    }
	    // In the following line, the zero index is used because,
	    // at the end of the gas-dynamic update, that index holds
	    // the updated data.
	    local_residual = (cell.U[0].total_energy - cell.rE_at_start_of_step) 
		/ cell.U[0].total_energy;
	    local_residual = fabs(local_residual);
	    if ( local_residual > energy_residual ) {
		energy_residual = local_residual;
		energy_residual_loc.refx = cell.pos[gtl].x;
		energy_residual_loc.refy = cell.pos[gtl].y;
		energy_residual_loc.refz = cell.pos[gtl].z;
	    }
	} // for cell
    } // end compute_residuals()

    override void determine_time_step_size()
    // Compute the local time step limit for all cells in the block.
    // The overall time step is limited by the worst-case cell.
    {
	double dt_local;
	double cfl_local;
	double signal;
	double cfl_allow; // allowable CFL number, t_order dependent

	// The following limits allow the simulation of the sod shock tube
	// to get just a little wobbly around the shock.
	// Lower values of cfl should be used for a smooth solution.
	switch ( number_of_stages_for_update_scheme(gasdynamic_update_scheme) ) {
	case 1: cfl_allow = 0.9; break;
	case 2: cfl_allow = 1.2; break;
	case 3: cfl_allow = 1.6; break;
	default: cfl_allow = 0.9;
	}
	bool first = true;
	foreach(FVCell cell; active_cells) {
	    signal = cell.signal_frequency();
	    cfl_local = GlobalConfig.dt_global * signal; // Current (Local) CFL number
	    dt_local = GlobalConfig.cfl_target / signal; // Recommend a time step size.
	    if ( first ) {
		cfl_min = cfl_local;
		cfl_max = cfl_local;
		dt_allow = dt_local;
		first = false;
	    } else {
		cfl_min = fmin(cfl_min, cfl_local);
		cfl_max = fmax(cfl_max, cfl_local);
		dt_allow = fmin(dt_allow, dt_local);
	    }
	} // foreach cell
	if ( cfl_max < 0.0 || cfl_max > cfl_allow ) {
	    writeln( "determine_time_step_size(): bad CFL number was encountered");
	    writeln( "    cfl_max=", cfl_max, " for Block ", id);
	    writeln( "    If this cfl_max value is not much larger than 1.0,");
	    writeln( "    your simulation could probably be restarted successfully");
	    writeln( "    with some minor tweaking.");
	    writeln( "    That tweaking should probably include a reduction");
	    writeln( "    in the size of the initial time-step, dt");
	    writeln( "    If this job is a restart/continuation of an old job, look in");
	    writeln( "    the old-job.finish file for the value of dt at termination.");
	    throw new Error(text("Bad cfl number encountered cfl_max=", cfl_max));
	}
    } // end determine_time_step_size()

    override void detect_shock_points()
    // Detects shocks by looking for compression between adjacent cells.
    //
    // The velocity component normal to the cell interfaces
    // is used as the indicating variable.
    {
	double uL, uR, aL, aR, a_min;

	// Change in normalised velocity to indicate a shock.
	// A value of -0.05 has been found suitable to detect the levels of
	// shock compression observed in the "sod" and "cone20" test cases.
	// It may need to be tuned for other situations, especially when
	// viscous effects are important.
	double tol = GlobalConfig.compression_tolerance;

	// First, work across North interfaces and
	// locate shocks using the (local) normal velocity.
	for ( size_t k = kmin; k <= kmax; ++k ) {
	    for ( size_t i = imin; i <= imax; ++i ) {
		for ( size_t j = jmin-1; j <= jmax; ++j ) {
		    auto cL = get_cell(i,j,k);
		    auto cR = get_cell(i,j+1,k);
		    auto IFace = cL.iface[north];
		    uL = cL.fs.vel.x * IFace.n.x + cL.fs.vel.y * IFace.n.y + cL.fs.vel.z * IFace.n.z;
		    uR = cR.fs.vel.x * IFace.n.x + cR.fs.vel.y * IFace.n.y + cR.fs.vel.z * IFace.n.z;
		    aL = cL.fs.gas.a;
		    aR = cR.fs.gas.a;
		    if (aL < aR)
			a_min = aL;
		    else
			a_min = aR;
		    IFace.fs.S = ((uR - uL) / a_min < tol);
		} // j loop
	    } // i loop
	} // for k
    
	// Second, work across East interfaces and
	// locate shocks using the (local) normal velocity.
	for ( size_t k = kmin; k <= kmax; ++k ) {
	    for ( size_t i = imin-1; i <= imax; ++i ) {
		for ( size_t j = jmin; j <= jmax; ++j ) {
		    auto cL = get_cell(i,j,k);
		    auto cR = get_cell(i+1,j,k);
		    auto IFace = cL.iface[east];
		    uL = cL.fs.vel.x * IFace.n.x + cL.fs.vel.y * IFace.n.y + cL.fs.vel.z * IFace.n.z;
		    uR = cR.fs.vel.x * IFace.n.x + cR.fs.vel.y * IFace.n.y + cR.fs.vel.z * IFace.n.z;
		    aL = cL.fs.gas.a;
		    aR = cR.fs.gas.a;
		    if (aL < aR)
			a_min = aL;
		    else
			a_min = aR;
		    IFace.fs.S = ((uR - uL) / a_min < tol);
		} // j loop
	    } // i loop
	} // for k
    
	if ( GlobalConfig.dimensions == 3 ) {
	    // Third, work across Top interfaces.
	    for ( size_t i = imin; i <= imax; ++i ) {
		for ( size_t j = jmin; j <= jmax; ++j ) {
		    for ( size_t k = kmin-1; k <= kmax; ++k ) {
			auto cL = get_cell(i,j,k);
			auto cR = get_cell(i,j,k+1);
			auto IFace = cL.iface[top];
			uL = cL.fs.vel.x * IFace.n.x + cL.fs.vel.y * IFace.n.y + cL.fs.vel.z * IFace.n.z;
			uR = cR.fs.vel.x * IFace.n.x + cR.fs.vel.y * IFace.n.y + cR.fs.vel.z * IFace.n.z;
			aL = cL.fs.gas.a;
			aR = cR.fs.gas.a;
			if (aL < aR)
			    a_min = aL;
			else
			    a_min = aR;
			IFace.fs.S = ((uR - uL) / a_min < tol);
		    } // for k
		} // j loop
	    } // i loop
	} // if ( dimensions == 3 )
    
	// Finally, mark cells as shock points if any of their
	// interfaces are shock points.
	for ( size_t k = kmin; k <= kmax; ++k ) {
	    for ( size_t i = imin; i <= imax; ++i ) {
		for ( size_t j = jmin; j <= jmax; ++j ) {
		    auto cell = get_cell(i,j,k);
		    cell.fs.S = cell.iface[east].fs.S || cell.iface[west].fs.S ||
			cell.iface[north].fs.S || cell.iface[south].fs.S ||
			( GlobalConfig.dimensions == 3 && 
			  (cell.iface[bottom].fs.S || cell.iface[top].fs.S) );
		} // j loop
	    } // i loop
	} // for k
    } // end detect_shock_points()

    override void compute_primary_cell_geometric_data(int gtl)
    // Compute cell and interface geometric properties.
    {
	size_t i, j, k;
	Vector3 dummy;
	Vector3 ds;
	if ( GlobalConfig.dimensions == 2 ) {
	    calc_volumes_2D(gtl);
	    calc_faces_2D(gtl);
	    calc_ghost_cell_geom_2D(gtl);
	    return;
	}
	// Cell properties of volume and position.
	// Estimates of cross-cell distances for use in high-order reconstruction.
	for ( i = imin; i <= imax; ++i ) {
	    for ( j = jmin; j <= jmax; ++j ) {
		for ( k = kmin; k <= kmax; ++k ) {
		    auto cell = get_cell(i,j,k);
		    auto p0 = get_vtx(i,j,k).pos[gtl];
		    auto p1 = get_vtx(i+1,j,k).pos[gtl];
		    auto p2 = get_vtx(i+1,j+1,k).pos[gtl];
		    auto p3 = get_vtx(i,j+1,k).pos[gtl];
		    auto p4 = get_vtx(i,j,k+1).pos[gtl];
		    auto p5 = get_vtx(i+1,j,k+1).pos[gtl];
		    auto p6 = get_vtx(i+1,j+1,k+1).pos[gtl];
		    auto p7 = get_vtx(i,j+1,k+1).pos[gtl];
		    hex_cell_properties(p0, p1, p2, p3, p4, p5, p6, p7, 
					cell.pos[gtl], cell.volume[gtl], cell.iLength,
					cell.jLength, cell.kLength);
		    cell.L_min = cell.iLength;
		    if ( cell.jLength < cell.L_min ) cell.L_min = cell.jLength;
		    if ( cell.kLength < cell.L_min ) cell.L_min = cell.kLength;
		}
	    }
	}
	// work on ifi face as a WEST face
	// t1 in the j-ordinate direction
	// t2 in the k-ordinate direction
	for ( i = imin; i <= imax + 1; ++i ) {
	    for ( j = jmin; j <= jmax; ++j ) {
		for ( k = kmin; k <= kmax; ++k ) {
		    auto iface = get_ifi(i,j,k);
		    auto p0 = get_vtx(i,j,k).pos[gtl];
		    auto p3 = get_vtx(i,j+1,k).pos[gtl];
		    auto p7 = get_vtx(i,j+1,k+1).pos[gtl];
		    auto p4 = get_vtx(i,j,k+1).pos[gtl];
		    quad_properties(p0, p3, p7, p4,
				    iface.pos, iface.n, iface.t1, iface.t2,
				    iface.area[gtl]);
		}
	    }
	}
	// work on ifj face as a SOUTH face
	// t1 in the k-ordinate direction
	// t2 in the i-ordinate direction
	for ( i = imin; i <= imax; ++i ) {
	    for ( j = jmin; j <= jmax + 1; ++j ) {
		for ( k = kmin; k <= kmax; ++k ) {
		    auto iface = get_ifj(i,j,k);
		    auto p0 = get_vtx(i,j,k).pos[gtl];
		    auto p4 = get_vtx(i,j,k+1).pos[gtl];
		    auto p5 = get_vtx(i+1,j,k+1).pos[gtl];
		    auto p1 = get_vtx(i+1,j,k).pos[gtl];
		    quad_properties(p0, p4, p5, p1,
				    iface.pos, iface.n, iface.t1, iface.t2,
				    iface.area[gtl]);
		}
	    }
	}
	// work on ifk face as a BOTTOM face
	// t1 in the i-ordinate direction
	// t2 in the j-ordinate direction
	for ( i = imin; i <= imax; ++i ) {
	    for ( j = jmin; j <= jmax; ++j ) {
		for ( k = kmin; k <= kmax + 1; ++k ) {
		    auto iface = get_ifk(i,j,k);
		    auto p0 = get_vtx(i,j,k).pos[gtl];
		    auto p1 = get_vtx(i+1,j,k).pos[gtl];
		    auto p2 = get_vtx(i+1,j+1,k).pos[gtl];
		    auto p3 = get_vtx(i,j+1,k).pos[gtl];
		    quad_properties(p0, p1, p2, p3,
				    iface.pos, iface.n, iface.t1, iface.t2,
				    iface.area[gtl]);
		}
	    }
	}
	// Propagate cross-cell lengths into the ghost cells.
	// 25-Feb-2014
	// Jason Qin and Paul Petrie-Repar have identified the lack of exact symmetry in
	// the reconstruction process at the wall as being a cause of the leaky wall
	// boundary conditions.  Note that the symmetry is not consistent with the 
	// linear extrapolation used for the positions and volumes in the next section.
	// [TODO] -- think about this carefully.
	auto option = CopyDataOption.cell_lengths_only;
	for ( j = jmin; j <= jmax; ++j ) {
	    for ( k = kmin; k <= kmax; ++k ) {
		i = imin;
		get_cell(i-1,j,k).copy_values_from(get_cell(i,j,k), option);
		get_cell(i-2,j,k).copy_values_from(get_cell(i+1,j,k), option);
		i = imax;
		get_cell(i+1,j,k).copy_values_from(get_cell(i,j,k), option);
		get_cell(i+2,j,k).copy_values_from(get_cell(i-1,j,k), option);
	    }
	}
	for ( i = imin; i <= imax; ++i ) {
	    for ( k = kmin; k <= kmax; ++k ) {
		j = jmin;
		get_cell(i,j-1,k).copy_values_from(get_cell(i,j,k), option);
		get_cell(i,j-2,k).copy_values_from(get_cell(i,j+1,k), option);
		j = jmax;
		get_cell(i,j+1,k).copy_values_from(get_cell(i,j,k), option);
		get_cell(i,j+2,k).copy_values_from(get_cell(i,j-1,k), option);
	    }
	}
	for ( i = imin; i <= imax; ++i ) {
	    for ( j = jmin; j <= jmax; ++j ) {
		k = kmin;
		get_cell(i,j,k-1).copy_values_from(get_cell(i,j,k), option);
		get_cell(i,j,k-2).copy_values_from(get_cell(i,j,k+1), option);
		k = kmax;
		get_cell(i,j,k+1).copy_values_from(get_cell(i,j,k), option);
		get_cell(i,j,k+2).copy_values_from(get_cell(i,j,k-1), option);
	    }
	}
	/* Extrapolate (with first-order) cell positions and volumes to ghost cells. */
	// TODO -- think about how to make these things consistent.
	for ( j = jmin; j <= jmax; ++j ) {
	    for ( k = kmin; k <= kmax; ++k ) {
		i = imin;
		auto cell_1 = get_cell(i,j,k);
		auto cell_2 = get_cell(i+1,j,k);
		auto ghost_cell = get_cell(i-1,j,k);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
		cell_2 = cell_1;
		cell_1 = ghost_cell;
		ghost_cell = get_cell(i-2,j,k);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_2.volume[gtl] - cell_2.volume[gtl];
		i = imax;
		cell_1 = get_cell(i,j,k);
		cell_2 = get_cell(i-1,j,k);
		ghost_cell = get_cell(i+1,j,k);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
		cell_2 = cell_1;
		cell_1 = ghost_cell;
		ghost_cell = get_cell(i+2,j,k);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	    }
	}
	for ( i = imin; i <= imax; ++i ) {
	    for ( k = kmin; k <= kmax; ++k ) {
		j = jmin;
		auto cell_1 = get_cell(i,j,k);
		auto cell_2 = get_cell(i,j+1,k);
		auto ghost_cell = get_cell(i,j-1,k);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
		cell_2 = cell_1;
		cell_1 = ghost_cell;
		ghost_cell = get_cell(i,j-2,k);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
		j = jmax;
		cell_1 = get_cell(i,j,k);
		cell_2 = get_cell(i,j-1,k);
		ghost_cell = get_cell(i,j+1,k);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
		cell_2 = cell_1;
		cell_1 = ghost_cell;
		ghost_cell = get_cell(i,j+2,k);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	    }
	}
	for ( i = imin; i <= imax; ++i ) {
	    for ( j = jmin; j <= jmax; ++j ) {
		k = kmin;
		auto cell_1 = get_cell(i,j,k);
		auto cell_2 = get_cell(i,j,k+1);
		auto ghost_cell = get_cell(i,j,k-1);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
		cell_2 = cell_1;
		cell_1 = ghost_cell;
		ghost_cell = get_cell(i,j,k-2);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
		k = kmax;
		cell_1 = get_cell(i,j,k);
		cell_2 = get_cell(i,j,k-1);
		ghost_cell = get_cell(i,j,k+1);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
		cell_2 = cell_1;
		cell_1 = ghost_cell;
		ghost_cell = get_cell(i,j,k+2);
		ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
		ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	    }
	}
    } // end compute_primary_cell_geometric_data()

    override void compute_distance_to_nearest_wall_for_all_cells(int gtl)
    // Used for the turbulence modelling.
    {
	FVCell[6] cell_at_wall;
	double[6] dist, half_width;
	FVInterface face_at_wall;

	foreach(ref FVCell cell; active_cells) {
	    auto ijk = to_ijk_indices(cell.id);
	    size_t i = ijk[0]; size_t j = ijk[1]; size_t k = ijk[2];
	    // Step 1: get distances to all boundaries along all index directions.
	    // If the block is not too distorted, these directions should take us
	    // straight to the bounding walls.
	    // North
	    face_at_wall = get_ifj(i,jmax+1,k);
	    dist[north] = abs(cell.pos[gtl] - face_at_wall.pos);
	    cell_at_wall[north] = get_cell(i,jmax,k);
	    half_width[north] = abs(cell_at_wall[north].pos[gtl] - face_at_wall.pos);
	    // East
	    face_at_wall = get_ifi(imax+1,j,k);
	    dist[east] = abs(cell.pos[gtl] - face_at_wall.pos);
	    cell_at_wall[east] = get_cell(imax,j,k);
	    half_width[east] = abs(cell_at_wall[east].pos[gtl] - face_at_wall.pos);
	    // South
	    face_at_wall = get_ifj(i,jmin,k);
	    dist[south] = abs(cell.pos[gtl] - face_at_wall.pos);
	    cell_at_wall[south] = get_cell(i,jmin,k);
	    half_width[south] = abs(cell_at_wall[south].pos[gtl] - face_at_wall.pos);
	    // West
	    face_at_wall = get_ifi(imin,j,k);
	    dist[west] = abs(cell.pos[gtl] - face_at_wall.pos);
	    cell_at_wall[west] = get_cell(imin,j,k);
	    half_width[west] = abs(cell_at_wall[west].pos[gtl] - face_at_wall.pos);
	    if ( GlobalConfig.dimensions == 3 ) {
		// Top
		face_at_wall = get_ifk(i,j,kmax+1);
		dist[top] = abs(cell.pos[gtl] - face_at_wall.pos);
		cell_at_wall[top] = get_cell(i,j,kmax);
		half_width[top] = abs(cell_at_wall[top].pos[gtl] - face_at_wall.pos);
		// Bottom
		face_at_wall = get_ifk(i,j,kmin);
		dist[bottom] = abs(cell.pos[gtl] - face_at_wall.pos);
		cell_at_wall[bottom] = get_cell(i,j,kmin);
		half_width[bottom] = abs(cell_at_wall[bottom].pos[gtl] - face_at_wall.pos);
	    }

	    // Step 2: Just in case there are no real walls for this block...
	    //
	    // We'll start by storing the largest distance and 
	    // corresponding wall-cell half-width so that we have 
	    // a relatively large distance in case there are no walls
	    // on the boundary of the block.
	    size_t num_faces = (GlobalConfig.dimensions == 3) ? 6 : 4;
	    cell.distance_to_nearest_wall = dist[0];
	    cell.half_cell_width_at_wall = half_width[0];
	    for ( size_t iface = 1; iface < num_faces; ++iface ) {
		if (dist[iface] > cell.distance_to_nearest_wall ) {
		    cell.distance_to_nearest_wall = dist[iface];
		    cell.half_cell_width_at_wall = half_width[iface];
		    cell.cell_at_nearest_wall = cell_at_wall[iface];
		}
	    }
		
	    // Step 3: find the closest real wall.
	    for ( size_t iface = 0; iface < num_faces; ++iface ) {
		if ( bc[iface].is_wall && 
		     bc[iface].type_code == BCCode.slip_wall &&
		     dist[iface] < cell.distance_to_nearest_wall ) {
		    cell.distance_to_nearest_wall = dist[iface];
		    cell.half_cell_width_at_wall = half_width[iface];
		    cell.cell_at_nearest_wall = cell_at_wall[iface];
		}
	    }
	} // end foreach cell
    } // end compute_distance_to_nearest_wall_for_all_cells()

    override void compute_secondary_cell_geometric_data(int gtl)
    // Compute secondary-cell and interface geometric properties.
    // Will be used for computing gradients for viscous terms.
    //
    // [TODO]: The code for the 3D cells has been ported from Eilmer3, 
    // which was ported from eilmer2 (as part of a 25-year project)
    // without taking advantage of the eilmer3 structure where ghost-cells
    // have been filled in with useful geometric information.
    // We should make use of this information.
    {
	size_t i, j, k;
	Vector3 dummy;
	double iLen, jLen, kLen;
	Vector3 ds;
	if ( GlobalConfig.dimensions == 2 ) {
	    secondary_areas_2D(gtl);
	    return;
	}
	/*
	 * Internal secondary cell geometry information
	 */
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( j = jmin; j <= jmax-1; ++j ) {
		for ( k = kmin; k <= kmax-1; ++k ) {
		    auto vertex = get_vtx(i+1,j+1,k+1);
		    auto p0 = get_cell(i,j,k).pos[gtl];
		    auto p1 = get_cell(i+1,j,k).pos[gtl];
		    auto p2 = get_cell(i+1,j+1,k).pos[gtl];
		    auto p3 = get_cell(i,j+1,k).pos[gtl];
		    auto p4 = get_cell(i,j,k+1).pos[gtl];
		    auto p5 = get_cell(i+1,j,k+1).pos[gtl];
		    auto p6 = get_cell(i+1,j+1,k+1).pos[gtl];
		    auto p7 = get_cell(i,j+1,k+1).pos[gtl];
		    hex_cell_properties(p0, p1, p2, p3, p4, p5, p6, p7, 
					dummy, vertex.volume, iLen, jLen, kLen);
		}
	    }
	}
	for ( i = imin; i <= imax; ++i ) {
	    for ( j = jmin; j <= jmax-1; ++j ) {
		for ( k = kmin; k <= kmax-1; ++k ) {
		    auto iface = get_sifi(i,j,k);
		    auto p0 = get_cell(i,j,k).pos[gtl];
		    auto p3 = get_cell(i,j+1,k).pos[gtl];
		    auto p7 = get_cell(i,j+1,k+1).pos[gtl];
		    auto p4 = get_cell(i,j,k+1).pos[gtl];
		    quad_properties(p0, p3, p7, p4,
				    iface.pos, iface.n, iface.t1, iface.t2,
				    iface.area[gtl]);
		}
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( j = jmin; j <= jmax; ++j ) {
		for ( k = kmin; k <= kmax-1; ++k ) {
		    auto iface = get_sifj(i,j,k);
		    auto p0 = get_cell(i,j,k).pos[gtl];
		    auto p4 = get_cell(i,j,k+1).pos[gtl];
		    auto p5 = get_cell(i+1,j,k+1).pos[gtl];
		    auto p1 = get_cell(i+1,j,k).pos[gtl];
		    quad_properties(p0, p4, p5, p1,
				    iface.pos, iface.n, iface.t1, iface.t2,
				    iface.area[gtl]);
		}
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( j = jmin; j <= jmax-1; ++j ) {
		for ( k = kmin; k <= kmax; ++k ) {
		    auto iface = get_sifk(i,j,k);
		    auto p0 = get_cell(i,j,k).pos[gtl];
		    auto p1 = get_cell(i+1,j,k).pos[gtl];
		    auto p2 = get_cell(i+1,j+1,k).pos[gtl];
		    auto p3 = get_cell(i,j+1,k).pos[gtl];
		    quad_properties(p0, p1, p2, p3,
				    iface.pos, iface.n, iface.t1, iface.t2,
				    iface.area[gtl]);
		}
	    }
	}
	/*
	 * East boundary secondary cell geometry information
	 */
	i = imax;
	for ( j = jmin; j <= jmax-1; ++j ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto vertex = get_vtx(i+1,j+1,k+1);
		auto p0 = get_cell(i,j,k).pos[gtl];
		auto p1 = get_ifi(i+1,j,k).pos;
		auto p2 = get_ifi(i+1,j+1,k).pos;
		auto p3 = get_cell(i,j+1,k).pos[gtl];
		auto p4 = get_cell(i,j,k+1).pos[gtl];
		auto p5 = get_ifi(i+1,j,k+1).pos;
		auto p6 = get_ifi(i+1,j+1,k+1).pos;
		auto p7 = get_cell(i,j+1,k+1).pos[gtl];
		hex_cell_properties(p0, p1, p2, p3, p4, p5, p6, p7, 
				    dummy, vertex.volume, iLen, jLen, kLen );
	    }
	}
	for ( j = jmin; j <= jmax-1; ++j ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto iface = get_sifi(i+1,j,k);
		auto p1 = get_ifi(i+1,j,k).pos;
		auto p2 = get_ifi(i+1,j+1,k).pos;
		auto p6 = get_ifi(i+1,j+1,k+1).pos;
		auto p5 = get_ifi(i+1,j,k+1).pos;
		quad_properties(p1, p2, p6, p5,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( j = jmin; j <= jmax; ++j ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto iface = get_sifj(i,j,k);
		auto p0 = get_cell(i,j,k).pos[gtl];
		auto p4 = get_cell(i,j,k+1).pos[gtl];
		auto p5 = get_ifi(i+1,j,k+1).pos;
		auto p1 = get_ifi(i+1,j,k).pos;
		quad_properties(p0, p4, p5, p1,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( j = jmin; j <= jmax-1; ++j ) {
	    for ( k = kmin; k <= kmax; ++k ) {
		auto iface = get_sifk(i,j,k);
		auto p0 = get_cell(i,j,k).pos[gtl];
		auto p1 = get_ifi(i+1,j,k).pos;
		auto p2 = get_ifi(i+1,j+1,k).pos;
		auto p3 = get_cell(i,j+1,k).pos[gtl];
		quad_properties(p0, p1, p2, p3,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	/*
	 * West boundary secondary cell geometry information
	 */
	i = imin - 1;
	for ( j = jmin; j <= jmax-1; ++j ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto vertex = get_vtx(i+1,j+1,k+1);
		auto p0 = get_ifi(i+1,j,k).pos;
		auto p1 = get_cell(i+1,j,k).pos[gtl];
		auto p2 = get_cell(i+1,j+1,k).pos[gtl];
		auto p3 = get_ifi(i+1,j+1,k).pos;
		auto p4 = get_ifi(i+1,j,k+1).pos;
		auto p5 = get_cell(i+1,j,k+1).pos[gtl];
		auto p6 = get_cell(i+1,j+1,k+1).pos[gtl];
		auto p7 = get_ifi(i+1,j+1,k+1).pos;
		hex_cell_properties(p0, p1, p2, p3, p4, p5, p6, p7, 
				    dummy, vertex.volume, iLen, jLen, kLen);
	    }
	}
	for ( j = jmin; j <= jmax-1; ++j ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto iface = get_sifi(i,j,k);
		auto p0 = get_ifi(i+1,j,k).pos;
		auto p3 = get_ifi(i+1,j+1,k).pos;
		auto p7 = get_ifi(i+1,j+1,k+1).pos;
		auto p4 = get_ifi(i+1,j,k+1).pos;
		quad_properties(p0, p3, p7, p4,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( j = jmin; j <= jmax; ++j ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto iface = get_sifj(i,j,k);
		auto p0 = get_ifi(i+1,j,k).pos;
		auto p4 = get_ifi(i+1,j,k+1).pos;
		auto p5 = get_cell(i+1,j,k+1).pos[gtl];
		auto p1 = get_cell(i+1,j,k).pos[gtl];
		quad_properties(p0, p4, p5, p1,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( j = jmin; j <= jmax-1; ++j ) {
	    for ( k = kmin; k <= kmax; ++k ) {
		auto iface = get_sifk(i,j,k);
		auto p0 = get_ifi(i+1,j,k).pos;
		auto p1 = get_cell(i+1,j,k).pos[gtl];
		auto p2 = get_cell(i+1,j+1,k).pos[gtl];
		auto p3 = get_ifi(i+1,j+1,k).pos;
		quad_properties(p0, p1, p2, p3,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	/*
	 * North boundary secondary cell geometry information
	 */
	j = jmax;
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto vertex = get_vtx(i+1,j+1,k+1);
		auto p0 = get_cell(i,j,k).pos[gtl];
		auto p1 = get_cell(i+1,j,k).pos[gtl];
		auto p2 = get_ifj(i+1,j+1,k).pos;
		auto p3 = get_ifj(i,j+1,k).pos;
		auto p4 = get_cell(i,j,k+1).pos[gtl];
		auto p5 = get_cell(i+1,j,k+1).pos[gtl];
		auto p6 = get_ifj(i+1,j+1,k+1).pos;
		auto p7 = get_ifj(i,j+1,k+1).pos;
		hex_cell_properties(p0, p1, p2, p3, p4, p5, p6, p7, 
				    dummy, vertex.volume, iLen, jLen, kLen);
	    }
	}
	for ( i = imin; i <= imax; ++i ) {
	    for (k = kmin; k <= kmax-1; ++k) {
		auto iface = get_sifi(i,j,k);
		auto p0 = get_cell(i,j,k).pos[gtl];
		auto p3 = get_ifj(i,j+1,k).pos;
		auto p7 = get_ifj(i,j+1,k+1).pos;
		auto p4 = get_cell(i,j,k+1).pos[gtl];
		quad_properties(p0, p3, p7, p4,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for (k = kmin; k <= kmax-1; ++k) {
		auto iface = get_sifj(i,j+1,k);
		auto p3 = get_ifj(i,j+1,k).pos;
		auto p7 = get_ifj(i,j+1,k+1).pos;
		auto p6 = get_ifj(i+1,j+1,k+1).pos;
		auto p2 = get_ifj(i+1,j+1,k).pos;
		quad_properties(p3, p7, p6, p2,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( k = kmin; k <= kmax; ++k ) {
		auto iface = get_sifk(i,j,k);
		auto p0 = get_cell(i,j,k).pos[gtl];
		auto p1 = get_cell(i+1,j,k).pos[gtl];
		auto p2 = get_ifj(i+1,j+1,k).pos;
		auto p3 = get_ifj(i,j+1,k).pos;
		quad_properties(p0, p1, p2, p3,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	/*
	 * South boundary secondary cell geometry information
	 */
	j = jmin - 1;
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto vertex = get_vtx(i+1,j+1,k+1);
		auto p0 = get_ifj(i,j+1,k).pos;
		auto p1 = get_ifj(i+1,j+1,k).pos;
		auto p2 = get_cell(i+1,j+1,k).pos[gtl];
		auto p3 = get_cell(i,j+1,k).pos[gtl];
		auto p4 = get_ifj(i,j+1,k+1).pos;
		auto p5 = get_ifj(i+1,j+1,k+1).pos;
		auto p6 = get_cell(i+1,j+1,k+1).pos[gtl];
		auto p7 = get_cell(i,j+1,k+1).pos[gtl];
		hex_cell_properties(p0, p1, p2, p3, p4, p5, p6, p7, 
				    dummy, vertex.volume, iLen, jLen, kLen);
	    }
	}
	for ( i = imin; i <= imax; ++i ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto iface = get_sifi(i,j,k);
		auto p0 = get_ifj(i,j+1,k).pos;
		auto p3 = get_cell(i,j+1,k).pos[gtl];
		auto p7 = get_cell(i,j+1,k+1).pos[gtl];
		auto p4 = get_ifj(i,j+1,k+1).pos;
		quad_properties(p0, p3, p7, p4,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( k = kmin; k <= kmax-1; ++k ) {
		auto iface = get_sifj(i,j,k);
		auto p0 = get_ifj(i,j+1,k).pos;
		auto p4 = get_ifj(i,j+1,k+1).pos;
		auto p5 = get_ifj(i+1,j+1,k+1).pos;
		auto p1 = get_ifj(i+1,j+1,k).pos;
		quad_properties(p0, p4, p5, p1,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( k = kmin; k <= kmax; ++k ) {
		auto iface = get_sifk(i,j,k);
		auto p0 = get_ifj(i,j+1,k).pos;
		auto p1 = get_ifj(i+1,j+1,k).pos;
		auto p2 = get_cell(i+1,j+1,k).pos[gtl];
		auto p3 = get_cell(i,j+1,k).pos[gtl];
		quad_properties(p0, p1, p2, p3,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	/*
	 * Top boundary secondary cell geometry information
	 */
	k = kmax;
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( j = jmin; j <= jmax-1; ++j ) {
		auto vertex = get_vtx(i+1,j+1,k+1);
		auto p0 = get_cell(i,j,k).pos[gtl];
		auto p1 = get_cell(i+1,j,k).pos[gtl];
		auto p2 = get_cell(i+1,j+1,k).pos[gtl];
		auto p3 = get_cell(i,j+1,k).pos[gtl];
		auto p4 = get_ifk(i,j,k+1).pos;
		auto p5 = get_ifk(i+1,j,k+1).pos;
		auto p6 = get_ifk(i+1,j+1,k+1).pos;
		auto p7 = get_ifk(i,j+1,k+1).pos;
		hex_cell_properties(p0, p1, p2, p3, p4, p5, p6, p7, 
				    dummy, vertex.volume, iLen, jLen, kLen);
	    }
	}
	for ( i = imin; i <= imax; ++i ) {
	    for ( j = jmin; j <= jmax-1; ++j ) {
		auto iface = get_sifi(i,j,k);
		auto p0 = get_cell(i,j,k).pos[gtl];
		auto p3 = get_cell(i,j+1,k).pos[gtl];
		auto p7 = get_ifk(i,j+1,k+1).pos;
		auto p4 = get_ifk(i,j,k+1).pos;
		quad_properties(p0, p3, p7, p4,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( j = jmin; j <= jmax; ++j ) {
		auto iface = get_sifj(i,j,k);
		auto p0 = get_cell(i,j,k).pos[gtl];
		auto p4 = get_ifk(i,j,k+1).pos;
		auto p5 = get_ifk(i+1,j,k+1).pos;
		auto p1 = get_cell(i+1,j,k).pos[gtl];
		quad_properties(p0, p4, p5, p1,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( j = jmin; j <= jmax-1; ++j ) {
		auto iface = get_sifk(i,j,k+1);
		auto p4 = get_ifk(i,j,k+1).pos;
		auto p5 = get_ifk(i+1,j,k+1).pos;
		auto p6 = get_ifk(i+1,j+1,k+1).pos;
		auto p7 = get_ifk(i,j+1,k+1).pos;
		quad_properties(p4, p5, p6, p7,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	/*
	 * Bottom boundary secondary cell geometry information
	 */
	k = kmin - 1;
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( j = jmin; j <= jmax-1; ++j ) {
		auto vertex = get_vtx(i+1,j+1,k+1);
		auto p0 = get_ifk(i,j,k+1).pos;
		auto p1 = get_ifk(i+1,j,k+1).pos;
		auto p2 = get_ifk(i+1,j+1,k+1).pos;
		auto p3 = get_ifk(i,j+1,k+1).pos;
		auto p4 = get_cell(i,j,k+1).pos[gtl];
		auto p5 = get_cell(i+1,j,k+1).pos[gtl];
		auto p6 = get_cell(i+1,j+1,k+1).pos[gtl];
		auto p7 = get_cell(i,j+1,k+1).pos[gtl];
		hex_cell_properties(p0, p1, p2, p3, p4, p5, p6, p7, 
				    dummy, vertex.volume, iLen, jLen, kLen);
	    }
	}
	for ( i = imin; i <= imax; ++i) {
	    for ( j = jmin; j <= jmax-1; ++j ) {
		auto iface = get_sifi(i,j,k);
		auto p0 = get_ifk(i,j,k+1).pos;
		auto p3 = get_ifk(i,j+1,k+1).pos;
		auto p7 = get_cell(i,j+1,k+1).pos[gtl];
		auto p4 = get_cell(i,j,k+1).pos[gtl];
		quad_properties(p0, p3, p7, p4,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( j = jmin; j <= jmax; ++j ) {
		auto iface = get_sifj(i,j,k);
		auto p0 = get_ifk(i,j,k+1).pos;
		auto p4 = get_cell(i,j,k+1).pos[gtl];
		auto p5 = get_cell(i+1,j,k+1).pos[gtl];
		auto p1 = get_ifk(i+1,j,k+1).pos;
		quad_properties(p0, p4, p5, p1,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
	for ( i = imin; i <= imax-1; ++i ) {
	    for ( j = jmin; j <= jmax-1; ++j ) {
		auto iface = get_sifk(i,j,k);
		auto p0 = get_ifk(i,j,k+1).pos;
		auto p1 = get_ifk(i+1,j,k+1).pos;
		auto p2 = get_ifk(i+1,j+1,k+1).pos;
		auto p3 = get_ifk(i,j+1,k+1).pos;
		quad_properties(p0, p1, p2, p3,
				iface.pos, iface.n, iface.t1, iface.t2,
				iface.area[gtl]);
	    }
	}
    } // end compute_secondary_cell_geometric_data()

    void calc_volumes_2D(int gtl)
    // Compute the PRIMARY cell volumes, areas, and centers 
    //  from the vertex positions.
    //
    // For 2D planar, assume unit length in the Z-direction.
    // For axisymmetry, compute a volume per radian.
    //
    // Determine minimum length and aspect ratio, also.
    {
	size_t i, j;
	double xA, yA, xB, yB, xC, yC, xD, yD;
	double xN, yN, xS, yS, xE, yE, xW, yW;
	double vol, max_vol, min_vol, xyarea;
	double dx, dy, dxN, dyN, dxE, dyE;
	double lengthN, lengthE, length_max, length_min, length_cross;
	double max_aspect, aspect_ratio;
	FVCell cell, source_cell, target_cell;

	// Cell layout
	// C-----B     3-----2
	// |     |     |     |
	// |  c  |     |  c  |
	// |     |     |     |
	// D-----A     0-----1
    
	max_vol = 0.0;
	min_vol = 1.0e30;    /* arbitrarily large */
	max_aspect = 0.0;
	for ( i = imin; i <= imax; ++i ) {
	    for ( j = jmin; j <= jmax; ++j ) {
		cell = get_cell(i,j);
		// These are the corners.
		xA = cell.vtx[1].pos[gtl].x;
		yA = cell.vtx[1].pos[gtl].y;
		xB = cell.vtx[2].pos[gtl].x;
		yB = cell.vtx[2].pos[gtl].y;
		xC = cell.vtx[3].pos[gtl].x;
		yC = cell.vtx[3].pos[gtl].y;
		xD = cell.vtx[0].pos[gtl].x;
		yD = cell.vtx[0].pos[gtl].y;
		// Cell area in the (x,y)-plane.
		xyarea = 0.5 * ((xB + xA) * (yB - yA) + (xC + xB) * (yC - yB) +
				(xD + xC) * (yD - yC) + (xA + xD) * (yA - yD));
		// Cell Centroid.
		cell.pos[gtl].refx = 1.0 / (xyarea * 6.0) * 
		    ((yB - yA) * (xA * xA + xA * xB + xB * xB) + 
		     (yC - yB) * (xB * xB + xB * xC + xC * xC) +
		     (yD - yC) * (xC * xC + xC * xD + xD * xD) + 
		     (yA - yD) * (xD * xD + xD * xA + xA * xA));
		cell.pos[gtl].refy = -1.0 / (xyarea * 6.0) * 
		    ((xB - xA) * (yA * yA + yA * yB + yB * yB) + 
		     (xC - xB) * (yB * yB + yB * yC + yC * yC) +
		     (xD - xC) * (yC * yC + yC * yD + yD * yD) + 
		     (xA - xD) * (yD * yD + yD * yA + yA * yA));
		cell.pos[gtl].refz = 0.0;
		// Cell Volume.
		if ( GlobalConfig.axisymmetric ) {
		    // Volume per radian = centroid y-ordinate * cell area
		    vol = xyarea * cell.pos[gtl].y;
		} else {
		    // Assume unit depth in the z-direction.
		    vol = xyarea;
		}
		if (vol < 0.0) {
		    throw new Error(text("Negative cell volume: Block ", id,
					 " vol[", i, " ,", j, "]= ", vol));
		}
		if (vol > max_vol) max_vol = vol;
		if (vol < min_vol) min_vol = vol;
		cell.volume[gtl] = vol;
		cell.areaxy[gtl] = xyarea;
		// Check cell length scale using North and East boundaries.
		// Also, save the minimum length for later use in the CFL
		// checking routine.  Record max aspect ratio over all cells.
		dxN = xC - xB;
		dyN = yC - yB;
		dxE = xA - xB;
		dyE = yA - yB;
		lengthN = sqrt(dxN * dxN + dyN * dyN);
		lengthE = sqrt(dxE * dxE + dyE * dyE);

		length_max = lengthN;
		if (lengthE > length_max) length_max = lengthE;
		length_cross = xyarea / length_max; 
		// estimate of minimum width of cell
		length_min = lengthN;
		if (lengthE < length_min) length_min = lengthE;
		if (length_cross < length_min) length_min = length_cross;
		cell.L_min = length_min;
		aspect_ratio = length_max / length_min;
		if (aspect_ratio > max_aspect) max_aspect = aspect_ratio;

		// Record the cell widths in the i- and j-index directions.
		// The widths are measured between corresponding midpoints of
		// the bounding interfaces.
		// This data is later used by the high-order reconstruction.
		xN = 0.5 * (xC + xB);
		yN = 0.5 * (yC + yB);
		xS = 0.5 * (xD + xA);
		yS = 0.5 * (yD + yA);
		xE = 0.5 * (xA + xB);
		yE = 0.5 * (yA + yB);
		xW = 0.5 * (xD + xC);
		yW = 0.5 * (yD + yC);
		dx = xN - xS;
		dy = yN - yS;
		cell.jLength = sqrt(dx * dx + dy * dy);
		dx = xE - xW;
		dy = yE - yW;
		cell.iLength = sqrt(dx * dx + dy * dy);
		cell.kLength = 0.0;
	    } // j loop
	} // i loop

	// We now need to mirror the cell iLength and jLength
	// around the boundaries.
	// Those boundaries that are adjacent to another block
	// will be updated later with the other-block's cell lengths.
	for ( i = imin; i <= imax; ++i ) {
	    // North boundary
	    j = jmax;
	    source_cell = get_cell(i,j);
	    target_cell = get_cell(i,j+1);
	    target_cell.iLength = source_cell.iLength;
	    target_cell.jLength = source_cell.jLength;
	    target_cell.kLength = 0.0;
	    source_cell = get_cell(i,j-1);
	    target_cell = get_cell(i,j+2);
	    target_cell.iLength = source_cell.iLength;
	    target_cell.jLength = source_cell.jLength;
	    target_cell.kLength = 0.0;
	    // South boundary
	    j = jmin;
	    source_cell = get_cell(i,j);
	    target_cell = get_cell(i,j-1);
	    target_cell.iLength = source_cell.iLength;
	    target_cell.jLength = source_cell.jLength;
	    target_cell.kLength = 0.0;
	    source_cell = get_cell(i,j+1);
	    target_cell = get_cell(i,j-2);
	    target_cell.iLength = source_cell.iLength;
	    target_cell.jLength = source_cell.jLength;
	    target_cell.kLength = 0.0;
	} // end for i

	for ( j = jmin; j <= jmax; ++j ) {
	    // East boundary
	    i = imax;
	    source_cell = get_cell(i,j);
	    target_cell = get_cell(i+1,j);
	    target_cell.iLength = source_cell.iLength;
	    target_cell.jLength = source_cell.jLength;
	    target_cell.kLength = 0.0;
	    source_cell = get_cell(i-1,j);
	    target_cell = get_cell(i+2,j);
	    target_cell.iLength = source_cell.iLength;
	    target_cell.jLength = source_cell.jLength;
	    target_cell.kLength = 0.0;
	    // West boundary
	    i = imin;
	    source_cell = get_cell(i,j);
	    target_cell = get_cell(i-1,j);
	    target_cell.iLength = source_cell.iLength;
	    target_cell.jLength = source_cell.jLength;
	    target_cell.kLength = 0.0;
	    source_cell = get_cell(i+1,j);
	    target_cell = get_cell(i-2,j);
	    target_cell.iLength = source_cell.iLength;
	    target_cell.jLength = source_cell.jLength;
	    target_cell.kLength = 0.0;
	} // end for j
    } // end calc_volumes_2D()

    void secondary_areas_2D(int gtl)
    // Compute the secondary cell cell areas in the (x,y)-plane.
    //
    // The secondary cells are centred on the vertices of the 
    // primary cells and have primary cell centres as their corners.
    // For this particular secondary cell, centred on a vertex v (i,j),
    // the near-by primary cell i,j is centred on B'.
    //
    //          +-----+
    //          |     |
    //       C'-+--B' |
    //       |  |  |  |
    //       |  v--+--+
    //       |     |
    //       D'----A'
    //
    {
	size_t i, j;
	double xA, yA, xB, yB, xC, yC, xD, yD;
	double xyarea, max_area, min_area;

	max_area = 0.0;
	min_area = 1.0e6;   // arbitrarily large
	// First, do all of the internal secondary cells.
	// i.e. The ones centred on primary vertices which 
	// are not on a boundary.
	for (i = imin+1; i <= imax; ++i) {
	    for (j = jmin+1; j <= jmax; ++j) {
		// These are the corners.
		xA = get_cell(i,j-1).pos[gtl].x;
		yA = get_cell(i,j-1).pos[gtl].y;
		xB = get_cell(i,j).pos[gtl].x;
		yB = get_cell(i,j).pos[gtl].y;
		xC = get_cell(i-1,j).pos[gtl].x;
		yC = get_cell(i-1,j).pos[gtl].y;
		xD = get_cell(i-1,j-1).pos[gtl].x;
		yD = get_cell(i-1,j-1).pos[gtl].y;
		// Cell area in the (x,y)-plane.
		xyarea = 0.5 * ((xB + xA) * (yB - yA) + (xC + xB) * (yC - yB) +
				(xD + xC) * (yD - yC) + (xA + xD) * (yA - yD));
		if (xyarea < 0.0) {
		    throw new Error(text("Negative secondary-cell area: Block ", id,
					 " vtx[", i, " ,", j, "]= ", xyarea));
		}
		if (xyarea > max_area) max_area = xyarea;
		if (xyarea < min_area) min_area = xyarea;
		get_vtx(i,j).areaxy = xyarea;
	    } // j loop
	} // i loop

	// Note that the secondary cells along block boundaries are HALF cells.
	//
	// East boundary.
	i = imax+1;
	for (j = jmin+1; j <= jmax; ++j) {
	    xA = get_ifi(i,j-1).pos.x;
	    yA = get_ifi(i,j-1).pos.y;
	    xB = get_ifi(i,j).pos.x;
	    yB = get_ifi(i,j).pos.y;
	    xC = get_cell(i-1,j).pos[gtl].x;
	    yC = get_cell(i-1,j).pos[gtl].y;
	    xD = get_cell(i-1,j-1).pos[gtl].x;
	    yD = get_cell(i-1,j-1).pos[gtl].y;
	    // Cell area in the (x,y)-plane.
	    xyarea = 0.5 * ((xB + xA) * (yB - yA) + (xC + xB) * (yC - yB) +
			    (xD + xC) * (yD - yC) + (xA + xD) * (yA - yD));
	    if (xyarea < 0.0) {
		throw new Error(text("Negative secondary-cell area: Block ", id,
				     " vtx[", i, " ,", j, "]= ", xyarea));
	    }
	    if (xyarea > max_area) max_area = xyarea;
	    if (xyarea < min_area) min_area = xyarea;
	    get_vtx(i,j).areaxy = xyarea;
	} // j loop 

	// Fudge corners -- not expecting to use this data.
	get_vtx(i,jmin).areaxy = 0.5 * get_vtx(i,jmin+1).areaxy;
	get_vtx(i,jmax+1).areaxy = 0.5 * get_vtx(i,jmax).areaxy;
    
	// West boundary.
	i = imin;
	for (j = jmin+1; j <= jmax; ++j) {
	    xA = get_cell(i,j-1).pos[gtl].x;
	    yA = get_cell(i,j-1).pos[gtl].y;
	    xB = get_cell(i,j).pos[gtl].x;
	    yB = get_cell(i,j).pos[gtl].y;
	    xC = get_ifi(i,j).pos.x;
	    yC = get_ifi(i,j).pos.y;
	    xD = get_ifi(i,j-1).pos.x;
	    yD = get_ifi(i,j-1).pos.y;
	    // Cell area in the (x,y)-plane.
	    xyarea = 0.5 * ((xB + xA) * (yB - yA) + (xC + xB) * (yC - yB) +
			    (xD + xC) * (yD - yC) + (xA + xD) * (yA - yD));
	    if (xyarea < 0.0) {
		throw new Error(text("Negative secondary-cell area: Block ", id,
				     " vtx[", i, " ,", j, "]= ", xyarea));
	    }
	    if (xyarea > max_area) max_area = xyarea;
	    if (xyarea < min_area) min_area = xyarea;
	    get_vtx(i,j).areaxy = xyarea;
	} // j loop 

	// Fudge corners.
	get_vtx(i,jmin).areaxy = 0.5 * get_vtx(i,jmin+1).areaxy;
	get_vtx(i,jmax+1).areaxy = 0.5 * get_vtx(i,jmax).areaxy;

	// North boundary.
	j = jmax+1;
	for (i = imin+1; i <= imax; ++i) {
	    // These are the corners.
	    xA = get_cell(i,j-1).pos[gtl].x;
	    yA = get_cell(i,j-1).pos[gtl].y;
	    xB = get_ifj(i,j).pos.x;
	    yB = get_ifj(i,j).pos.y;
	    xC = get_ifj(i-1,j).pos.x;
	    yC = get_ifj(i-1,j).pos.y;
	    xD = get_cell(i-1,j-1).pos[gtl].x;
	    yD = get_cell(i-1,j-1).pos[gtl].y;
	    // Cell area in the (x,y)-plane.
	    xyarea = 0.5 * ((xB + xA) * (yB - yA) + (xC + xB) * (yC - yB) +
			    (xD + xC) * (yD - yC) + (xA + xD) * (yA - yD));
	    if (xyarea < 0.0) {
		throw new Error(text("Negative secondary-cell area: Block ", id,
				     " vtx[", i, " ,", j, "]= ", xyarea));
	    }
	    if (xyarea > max_area) max_area = xyarea;
	    if (xyarea < min_area) min_area = xyarea;
	    get_vtx(i,j).areaxy = xyarea;
	} // i loop 

	// Fudge corners.
	get_vtx(imin,j).areaxy = 0.5 * get_vtx(imin+1,j).areaxy;
	get_vtx(imax+1,j).areaxy = 0.5 * get_vtx(imax,j).areaxy;

	// South boundary.
	j = jmin;
	for (i = imin+1; i <= imax; ++i) {
	    xA = get_ifj(i,j).pos.x;
	    yA = get_ifj(i,j).pos.y;
	    xB = get_cell(i,j).pos[gtl].x;
	    yB = get_cell(i,j).pos[gtl].y;
	    xC = get_cell(i-1,j).pos[gtl].x;
	    yC = get_cell(i-1,j).pos[gtl].y;
	    xD = get_ifj(i-1,j).pos.x;
	    yD = get_ifj(i-1,j).pos.y;
	    // Cell area in the (x,y)-plane.
	    xyarea = 0.5 * ((xB + xA) * (yB - yA) + (xC + xB) * (yC - yB) +
			    (xD + xC) * (yD - yC) + (xA + xD) * (yA - yD));
	    if (xyarea < 0.0) {
		throw new Error(text("Negative secondary-cell area: Block ", id,
				     " vtx[", i, " ,", j, "]= ", xyarea));
	    }
	    if (xyarea > max_area) max_area = xyarea;
	    if (xyarea < min_area) min_area = xyarea;
	    get_vtx(i,j).areaxy = xyarea;
	} // i loop

	// Fudge corners.
	get_vtx(imin,j).areaxy = 0.5 * get_vtx(imin+1,j).areaxy;
	get_vtx(imax+1,j).areaxy = 0.5 * get_vtx(imax,j).areaxy;

	// writefln("Max area = %e, Min area = %e", max_area, min_area);
    } // end secondary_areas_2D()

    void calc_faces_2D(int gtl)
    {
	FVInterface iface;
	size_t i, j;
	double xA, xB, yA, yB, xC, yC;
	double LAB, LBC;

	// East-facing interfaces.
	for (i = imin; i <= imax+1; ++i) {
	    for (j = jmin; j <= jmax; ++j) {
		iface = get_ifi(i,j);
		// These are the corners.
		xA = get_vtx(i,j).pos[gtl].x; 
		yA = get_vtx(i,j).pos[gtl].y;
		xB = get_vtx(i,j+1).pos[gtl].x; 
		yB = get_vtx(i,j+1).pos[gtl].y;
		LAB = sqrt((xB - xA) * (xB - xA) + (yB - yA) * (yB - yA));
		if (LAB < 1.0e-9) {
		    writefln("Zero length ifi[%d,%d]: %e", i, j, LAB);
		}
		// Direction cosines for the unit normal.
		iface.n.refx = (yB - yA) / LAB;
		iface.n.refy = -(xB - xA) / LAB;
		iface.n.refz = 0.0;  // 2D plane
		iface.t2 = Vector3(0.0, 0.0, 1.0);
		iface.t1 = cross(iface.n, iface.t2);
		// Length in the XY-plane.
		iface.length = LAB;
		// Mid-point and area.
		iface.Ybar = 0.5 * (yA + yB);
		if ( GlobalConfig.axisymmetric ) {
		    // Interface area per radian.
		    iface.area[gtl] = LAB * iface.Ybar;
		} else {
		    // Assume unit depth in the Z-direction.
		    iface.area[gtl] = LAB;
		}
		iface.pos = (get_vtx(i,j).pos[gtl] + get_vtx(i,j+1).pos[gtl])/2.0;
	    
	    } // j loop
	} // i loop
    
	// North-facing interfaces.
	for (i = imin; i <= imax; ++i) {
	    for (j = jmin; j <= jmax+1; ++j) {
		iface = get_ifj(i,j);
		// These are the corners.
		xB = get_vtx(i+1,j).pos[gtl].x;
		yB = get_vtx(i+1,j).pos[gtl].y;
		xC = get_vtx(i,j).pos[gtl].x;
		yC = get_vtx(i,j).pos[gtl].y;
		LBC = sqrt((xC - xB) * (xC - xB) + (yC - yB) * (yC - yB));
		if (LBC < 1.0e-9) {
		    writefln("Zero length ifj[%d,%d]: %e", i, j, LBC);
		}
		// Direction cosines for the unit normal.
		iface.n.refx = (yC - yB) / LBC;
		iface.n.refy = -(xC - xB) / LBC;
		iface.n.refz = 0.0;  // 2D plane
		iface.t2 = Vector3(0.0, 0.0, 1.0);
		iface.t1 = cross(iface.n, iface.t2);
		// Length in the XY-plane.
		iface.length = LBC;
		// Mid-point and area.
		iface.Ybar = 0.5 * (yC + yB);
		if ( GlobalConfig.axisymmetric ) {
		    // Interface area per radian.
		    iface.area[gtl] = LBC * iface.Ybar;
		} else {
		    // Assume unit depth in the Z-direction.
		    iface.area[gtl] = LBC;
		}
		iface.pos = (get_vtx(i+1,j).pos[gtl] + get_vtx(i,j).pos[gtl])/2.0;
	    } // j loop
	} // i loop
    } // end calc_faces_2D()

    void calc_ghost_cell_geom_2D(int gtl)
    // Compute the ghost cell positions and volumes.
    //
    // 'Compute' is a bit too strong to describe what we do here.
    //  Rather this is a first-order extrapolation
    // from interior cells to estimate the position
    // and volume of the ghost cells.
    {
	size_t i, j;
	FVCell cell_1, cell_2, ghost_cell;
	// East boundary
	i = imax;
	for ( j = jmin; j <= jmax; ++j ) {
	    cell_1 = get_cell(i,j);
	    cell_2 = get_cell(i-1,j);
	    ghost_cell = get_cell(i+1,j);
	    ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
	    ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	    cell_2 = cell_1;
	    cell_1 = ghost_cell;
	    ghost_cell = get_cell(i+2,j);
	    ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
	    ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	}
	// West boundary
	i = imin;
	for ( j = jmin; j <= jmax; ++j ) {
	    cell_1 = get_cell(i,j);
	    cell_2 = get_cell(i+1,j);
	    ghost_cell = get_cell(i-1,j);
	    ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
	    ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	    cell_2 = cell_1;
	    cell_1 = ghost_cell;
	    ghost_cell = get_cell(i-2,j);
	    ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
	    ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	}
	// North boundary
	j = jmax;
	for ( i = imin; i <= imax; ++i ) {
	    cell_1 = get_cell(i,j);
	    cell_2 = get_cell(i,j-1);
	    ghost_cell = get_cell(i,j+1);
	    ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
	    ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	    cell_2 = cell_1;
	    cell_1 = ghost_cell;
	    ghost_cell = get_cell(i,j+2);
	    ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
	    ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	}
	// South boundary
	j = jmin;
	for ( i = imin; i <= imax; ++i ) {
	    cell_1 = get_cell(i,j);
	    cell_2 = get_cell(i,j+1);
	    ghost_cell = get_cell(i,j-1);
	    ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
	    ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	    cell_2 = cell_1;
	    cell_1 = ghost_cell;
	    ghost_cell = get_cell(i,j-2);
	    ghost_cell.pos[gtl] = 2.0*cell_1.pos[gtl] - cell_2.pos[gtl];
	    ghost_cell.volume[gtl] = 2.0*cell_1.volume[gtl] - cell_2.volume[gtl];
	}
    } // end calc_ghost_cell_geom_2D()


    override void read_grid(string filename, size_t gtl=0)
    // Read the grid vertices from a gzip file.
    {
	size_t nivtx, njvtx, nkvtx;
	double x, y, z;
	if ( GlobalConfig.verbosity_level >= 1 && id == 0 ) {
	    writeln("read_grid(): Start block ", id);
	}
	auto byLine = new GzipByLine(filename);
	auto line = byLine.front; byLine.popFront();
	formattedRead(line, "%d %d %d", &nivtx, &njvtx, &nkvtx);
	if ( GlobalConfig.dimensions == 3 ) {
	    if ( nivtx-1 != nicell || njvtx-1 != njcell || nkvtx-1 != nkcell ) {
		throw new Error(text("For block[", id, "] we have a mismatch in 3D grid size.",
                                     " Have read nivtx=", nivtx, " njvtx=", njvtx,
				     " nkvtx=", nkvtx));
	    }
	    for ( size_t k = kmin; k <= kmax+1; ++k ) {
		for ( size_t j = jmin; j <= jmax+1; ++j ) {
		    for ( size_t i = imin; i <= imax+1; ++i ) {
			line = byLine.front; byLine.popFront();
			// Note that the line starts with whitespace.
			formattedRead(line, " %g %g %g", &x, &y, &z);
			auto vtx = get_vtx(i,j,k);
			vtx.pos[gtl].refx = x;
			vtx.pos[gtl].refy = y;
			vtx.pos[gtl].refz = z;
		    } // for i
		} // for j
	    } // for k
	} else { // 2D case
	    if ( nivtx-1 != nicell || njvtx-1 != njcell || nkvtx != 1 ) {
		throw new Error(text("For block[", id, "] we have a mismatch in 2D grid size.",
				     " Have read nivtx=", nivtx, " njvtx=", njvtx,
				     " nkvtx=", nkvtx));
	    }
	    for ( size_t j = jmin; j <= jmax+1; ++j ) {
		for ( size_t i = imin; i <= imax+1; ++i ) {
		    line = byLine.front; byLine.popFront();
		    // Note that the line starts with whitespace.
		    formattedRead(line, " %g %g", &x, &y);
		    auto vtx = get_vtx(i,j);
		    vtx.pos[gtl].refx = x;
		    vtx.pos[gtl].refy = y;
		    vtx.pos[gtl].refz = 0.0;
		} // for i
	    } // for j
	}
    } // end read_grid()

    override void write_grid(string filename, double sim_time, size_t gtl=0)
    {
	if ( GlobalConfig.verbosity_level >= 1 && id == 0 ) {
	    writeln("write_grid(): Start block ", id);
	}
	size_t kmaxrange;
	auto outfile = new GzipOut(filename);
	auto writer = appender!string();
	if ( GlobalConfig.dimensions == 3 ) {
	    formattedWrite(writer, "%d %d %d  # ni nj nk\n", nicell+1, njcell+1, nkcell+1);
	    kmaxrange = kmax + 1;
	} else { // 2D case
	    formattedWrite(writer, "%d %d %d  # ni nj nk\n", nicell+1, njcell+1, nkcell);
	    kmaxrange = kmax;
	}
	outfile.compress(writer.data);
	for ( size_t k = kmin; k <= kmaxrange; ++k ) {
	    for ( size_t j = jmin; j <= jmax+1; ++j ) {
		for ( size_t i = imin; i <= imax+1; ++i ) {
		    auto vtx = get_vtx(i,j,k);
		    writer = appender!string();
		    formattedWrite(writer, "%20.12e %20.12e %20.12e\n", vtx.pos[gtl].x,
				   vtx.pos[gtl].y, vtx.pos[gtl].z);
		    outfile.compress(writer.data);
		} // for i
	    } // for j
	} // for k
	outfile.finish();
    } // end write_grid()

    override double read_solution(string filename)
    // Note that the position data is read into grid-time-level 0
    // by scan_values_from_string(). 
    {
	size_t ni, nj, nk;
	double sim_time;
	if ( GlobalConfig.verbosity_level >= 1 && id == 0 ) {
	    writeln("read_solution(): Start block ", id);
	}
	auto byLine = new GzipByLine(filename);
	auto line = byLine.front; byLine.popFront();
	formattedRead(line, " %g", &sim_time);
	line = byLine.front; byLine.popFront();
	// ignore second line; it should be just the names of the variables
	// [TODO] We should test the incoming strings against the current variable names.
	line = byLine.front; byLine.popFront();
	formattedRead(line, "%d %d %d", &ni, &nj, &nk);
	if ( ni != nicell || nj != njcell || 
	     nk != ((GlobalConfig.dimensions == 3) ? nkcell : 1) ) {
	    throw new Error(text("For block[", id, "] we have a mismatch in solution size.",
				 " Have read ni=", ni, " nj=", nj, " nk=", nk));
	}	
	for ( size_t k = kmin; k <= kmax; ++k ) {
	    for ( size_t j = jmin; j <= jmax; ++j ) {
		for ( size_t i = imin; i <= imax; ++i ) {
		    line = byLine.front; byLine.popFront();
		    get_cell(i,j,k).scan_values_from_string(line);
		} // for i
	    } // for j
	} // for k
	return sim_time;
    }

    // Returns sim_time from file.

    override void write_solution(string filename, double sim_time)
    // Write the flow solution (i.e. the primary variables at the cell centers)
    // for a single block.
    // This is almost Tecplot POINT format.
    {
	if ( GlobalConfig.verbosity_level >= 1 && id == 0 ) {
	    writeln("write_solution(): Start block ", id);
	}
	auto outfile = new GzipOut(filename);
	auto writer = appender!string();
	formattedWrite(writer, "%20.12e\n", sim_time);
	outfile.compress(writer.data);
	writer = appender!string();
	foreach(varname; variable_list_for_cell()) {
	    formattedWrite(writer, " \"%s\"", varname);
	}
	formattedWrite(writer, "\n");
	outfile.compress(writer.data);
	writer = appender!string();
	formattedWrite(writer, "%d %d %d\n", nicell, njcell, nkcell);
	outfile.compress(writer.data);
	for ( size_t k = kmin; k <= kmax; ++k ) {
	    for ( size_t j = jmin; j <= jmax; ++j ) {
		for ( size_t i = imin; i <= imax; ++i ) {
		    outfile.compress(" " ~ get_cell(i,j,k).write_values_to_string() ~ "\n");
		} // for i
	    } // for j
	} // for k
	outfile.finish();
    }

    override void write_history(string filename, double sim_time, bool write_header=false)
    {
	throw new Error("[TODO] Not implemented yet.");
    }

    // to be ported from invs.cxx
    override void inviscid_flux()
    {
	throw new Error("[TODO] Not implemented yet.");
    }

    override void apply_convective_bc(double t)
    {
	bc[north].apply_convective(t);
	bc[east].apply_convective(t);
	bc[south].apply_convective(t);
	bc[west].apply_convective(t);
	if ( GlobalConfig.dimensions == 3 ) {
	    bc[top].apply_convective(t);
	    bc[bottom].apply_convective(t);
	}
    } // end apply_convective_bc()

    override void apply_viscous_bc(double t)
    {
	bc[north].apply_viscous(t);
	bc[east].apply_viscous(t);
	bc[south].apply_viscous(t);
	bc[west].apply_viscous(t);
	if ( GlobalConfig.dimensions == 3 ) {
	    bc[top].apply_viscous(t);
	    bc[bottom].apply_viscous(t);
	}
    } // end apply_convective_bc

} // end class SBlock



/** Indexing of the data in 2D.
 *
 * \verbatim
 * The following figure shows cell [i,j] and its associated
 * vertices and faces. 
 * (New arrangement, planned August 2006, implemented Nov 2006)
 *
 *
 *
 *     Vertex 3         North face           Vertex 2 
 *   vtx[i,j+1]         ifj[i,j+1]           vtx[i+1,j+1]
 *             +--------------x--------------+
 *             |                             |
 *             |                             |
 *             |                             |
 *             |                             |
 *             |                             |
 *   West      |         cell center         |  East 
 *   face      |          ctr[i,j]           |  face
 *   ifi[i,j]  x              o              x  ifi[i+1,j]
 *             |                             |
 *             |                             |
 *             |                             |
 *             |                             |
 *             |                             |
 *             |                             |
 *             |                             |
 *             +--------------x--------------+
 *     Vertex 0           South face         Vertex 1
 *     vtx[i,j]           ifj[i,j]           vtx[i+1,j]
 *
 *
 * Thus...
 * ----
 * Active cells are indexed as ctr[i][i], where
 * imin <= i <= imax, jmin <= j <= jmax.
 *
 * Active east-facing interfaces are indexed as ifi[i][j], where
 * imin <= i <= imax+1, jmin <= j <= jmax.
 *
 * Active north-facing interfaces are indexed as ifj[i][j], where
 * imin <= i <= imax, jmin <= j <= jmax+1.
 *
 * Active vertices are indexed as vtx[i][j], where
 * imin <= i <= imax+1, jmin <= j <= jmax+1.
 *
 * Space for ghost cells is available outside these ranges.
 *
 * Indexing for the 3D data -- see page 8 in 3D CFD workbook
 * \endverbatim
 */