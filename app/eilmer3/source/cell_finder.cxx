/** \file cell_finder.cxx
 *  \ingroup eilmer3
 *  \brief Definitions for the cell finder class used for ray-tracing.
 **/

#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_get_max_threads() 1
#endif

#include "../../../lib/util/source/useful.h"
#include "../../../lib/radiation2/source/ray_tracing_pieces.hh"

#include "cell_finder.hh"
#include "bc.hh"
#include "bc_defs.hh"
#include "kernel.hh"

using namespace std;

CellFinder::CellFinder( int nvertices )
: nvertices_( nvertices ) {}

CellFinder::~CellFinder() {}

CellFinder2D::CellFinder2D( int nvertices )
: CellFinder( nvertices )
{
    // initialise rp_, a_ and dc_ arrays for each thread
    int nthreads = omp_get_max_threads();

    // cout << "CellFinder2D::CellFinder2D()" << endl
    //      << "nthreads = " << nthreads << endl;

    rp_.resize( nthreads );
    a_.resize( nthreads );
    dc_.resize( nthreads );
    
    for ( int ithread=0; ithread < nthreads; ++ithread ) {
    	rp_[ithread].resize( nvertices_ );
    	a_[ithread].resize( nvertices_ );
    	dc_[ithread] = new int[2];
    }
}

CellFinder2D::~CellFinder2D()
{
    for ( size_t ithread=0; ithread < dc_.size(); ++ithread )
    	delete [] dc_[ithread];
}

int
CellFinder2D::
find_cell( const Vector3 &p, int &ib, int &ic, int &jc, int &kc )
{
    // &p -> address of spatial point we are looking for
    // ib -> block indice
    // ic, jc, kc -> cell indices
    
    int ithread = omp_get_thread_num();
    
    // Get pointers to the block and cell
    Block * A = get_block_data_ptr(ib);
    FV_Cell * cell = A->get_cell(ic,jc,kc);
    
    // Pointer for the neighbour block
    Block * B;
    
    // Search for the containing cell starting with the given guess
    dc_[ithread][0] = 1; dc_[ithread][1] = 1;
    int count = 0;
    int status = INSIDE_GRID; 
    while ( abs(dc_[ithread][0]) + abs(dc_[ithread][1]) != 0 ) {
    	// cout << "ib = " << ib << ", ic = " << ic << ", jc = " << jc << ", A->imax = " << A->imax << ", A->imin = " << A->imin << ", A->jmax = " << A->jmax << ", A->jmin = " << A->jmin << endl;
	// Check that predicted cell is inside the block domain
	// FIXME: Is this (if's and else's) approach the most efficient?
	if ( ic < A->imin ) {
	    // cout << "Outside the block: West" << endl;
	    if ( A->bcp[WEST]->type_code == ADJACENT ) {
	    	// set the block index to the neighbour
	    	ib = A->bcp[WEST]->neighbour_block;
	    	B = get_block_data_ptr( ib );
	    	// make sure jc is back in-range
	    	jc -= dc_[ithread][1];
	    	if ( A->bcp[WEST]->neighbour_face==EAST ) {
	    	    // jc stays the same, ic goes to new imax
	    	    ic = B->imax;
	    	}
	    	else if ( A->bcp[WEST]->neighbour_face==SOUTH ) {
	    	    // ic goes to jc, jc goes to new jmin
	    	    ic = jc; jc = B->jmin;
	    	}
	    	else if ( A->bcp[WEST]->neighbour_face==WEST ) {
	    	    // ic goes to new imin, jc goes to new jmax - jc
	    	    ic = B->imin; jc = B->jmax - jc;
	    	}
	    	else if ( A->bcp[WEST]->neighbour_face==NORTH ) {
	    	    // ic goes to new imax - jc, jc goes to new jmax
	    	    ic = B->imax - jc; jc = B->jmax;
	    	}
	    }
	    else {
	    	// step back to boundary indices
	    	ic -= dc_[ithread][0]; 
	    	jc -= dc_[ithread][1];
	    	// set status flag to the exit direction
	    	status = WEST;
	    }
	}
	else if ( ic > A->imax ) {
	    // cout << "Outside the block: East" << endl;
	    if ( A->bcp[EAST]->type_code == ADJACENT ) {
	    	// set the block index to the neighbour
	    	ib = A->bcp[EAST]->neighbour_block;
	    	B = get_block_data_ptr( ib );
	    	// make sure jc is back in-range
	    	jc -= dc_[ithread][1];
	    	if ( A->bcp[EAST]->neighbour_face==WEST ) {
	    	    // jc stays the same, ic goes to new imin
	    	    ic = B->imin;
	    	}
	    	else if ( A->bcp[EAST]->neighbour_face==NORTH ) {
	    	    // ic goes to jc, jc goes to new jmax
	    	    ic = jc; jc = B->jmax;
	    	}
	    	else if ( A->bcp[EAST]->neighbour_face==EAST ) {
	    	    // ic goes to new imax, jc goes to new jmax - jc
	    	    ic = B->imax; jc = B->jmax - jc;
	    	}
	    	else if ( A->bcp[EAST]->neighbour_face==SOUTH ) {
	    	    // ic goes to new jmax - jc, jc goes to new jmin
	    	    ic = B->imax - jc; jc = B->jmin;
	    	}
	    }
	    else {
	    	// step back to boundary indices
	    	ic -= dc_[ithread][0]; 
	    	jc -= dc_[ithread][1];
	    	// set status flag to the exit direction
	    	status = EAST;
	    }
	}
	else if ( jc < A->jmin ) {
	    // cout << "Outside the block: South" << endl;
	    if ( A->bcp[SOUTH]->type_code == ADJACENT ) {
	    	// set the block index to the neighbour
	    	ib = A->bcp[SOUTH]->neighbour_block;
	    	B = get_block_data_ptr( ib );
	    	// make sure ic is back in-range
	    	ic -= dc_[ithread][0];
	    	if ( A->bcp[SOUTH]->neighbour_face==NORTH ) {
	    	    // ic stays the same, jc goes to new jmax
	    	    jc = B->jmax;
	    	}
	    	else if ( A->bcp[SOUTH]->neighbour_face==EAST ) {
	    	    // ic goes to new imax, jc goes to new jmax - ic
	    	    ic = B->imax; jc = B->jmax - ic;
	    	}
	    	else if ( A->bcp[SOUTH]->neighbour_face==SOUTH ) {
	    	    // ic goes to new imax - ic, jc goes to new jmin
	    	    ic = B->imax - ic; jc = B->jmin;
	    	}
	    	else if ( A->bcp[SOUTH]->neighbour_face==WEST ) {
	    	    // ic goes to new imin, jc goes to ic
	    	    ic = B->imin; jc = B->jmin;
	    	}
	    }
	    else {
	    	// step back to boundary indices
	    	ic -= dc_[ithread][0]; 
	    	jc -= dc_[ithread][1];
	    	// set status flag to the exit direction
	    	status = SOUTH;
	    }
	}
	else if ( jc > A->jmax ) {
	    // cout << "Outside the block: North" << endl;
	    if ( A->bcp[NORTH]->type_code == ADJACENT ) {
	    	// set the block index to the neighbour
	    	ib = A->bcp[NORTH]->neighbour_block;
	    	B = get_block_data_ptr( ib );
	    	// make sure ic is back in-range
	    	ic -= dc_[ithread][0];
	    	if ( A->bcp[NORTH]->neighbour_face==SOUTH ) {
	    	    // ic stays the same, jc goes to new jmin
	    	    jc = B->jmin;
	    	}
	    	else if ( A->bcp[NORTH]->neighbour_face==WEST ) {
	    	    // ic goes to new imin, jc goes to new jmax - ic
	    	    ic = B->imin; jc = B->jmax - ic;
	    	}
	    	else if ( A->bcp[NORTH]->neighbour_face==NORTH ) {
	    	    // ic goes to new imax - ic, jc goes to new jmax
	    	    ic = B->imax - ic; jc = B->jmax;
	    	}
	    	else if ( A->bcp[NORTH]->neighbour_face==EAST ) {
	    	    // ic goes to new imax, jc goes to ic
	    	    ic = B->imax; jc = ic;
	    	}
	    }
	    else {
	    	// step back to boundary indices
	    	ic -= dc_[ithread][0]; 
	    	jc -= dc_[ithread][1];
	    	// set status flag to the exit direction
	    	status = NORTH;
	    }
	}
	// else -> do nothing, retain INSIDE_GRID status
	
	A = get_block_data_ptr( ib );
	cell = A->get_cell( ic, jc, kc );
	
	// if ( ic < A->imin || ic > A->imax || jc < A->jmin || jc > A->jmax ) {
	//     cout << "ib = " << ib << ", ic = " << ic << ", jc = " << jc << ", A->imax = " << A->imax << ", A->imin = " << A->imin << ", A->jmax = " << A->jmax << ", A->jmin = " << A->jmin << endl;
	//     cout << "dc[0] = " << dc_[ithread][0] << ", dc[1] = " << dc_[ithread][1] << endl;
	//     exit( FAILURE );
	// }
	
	// break if we are no longer inside the grid
	if ( status != INSIDE_GRID ) {
	    break;
	}

	// Otherwise test the predicted cell
	test_cell( cell, p, dc_[ithread] );
	ic += dc_[ithread][0]; jc += dc_[ithread][1];

	// increment search counter
	++count;
	// A conservative check to see that we are not in an 'infinite' loop
	if ( count > (A->imax + A->jmax) ) {
	    cout << "Cell search seems to be looping." << endl;
	    status = ERROR;
	    break;
	}
    }
    
    return status;
}

void
CellFinder2D::
test_cell( const FV_Cell * cell, const Vector3 &p, int *dc )
{
    // NOTE: this function should be applicable to both tri and quad 2D cells
    
    int ithread = omp_get_thread_num();
    
    // define the r vectors
    for ( int iv=0; iv<nvertices_; ++iv )
	rp_[ithread][iv] = cell->vtx[iv]->pos - p;
    
    // calculate the cross products
    int iv_p1;
    for ( int iv=0; iv<nvertices_; iv++ ) {
	iv_p1 = iv+1;
	if ( iv_p1 == nvertices_ ) iv_p1 = 0;
	a_[ithread][iv] = cross(rp_[ithread][iv], cell->vtx[iv_p1]->pos - cell->vtx[iv]->pos );
    }
    
    // if all cross product 'z' components are positive, then point is inside cell
    // NOTE: Cross-product of 2 2D vectors always results in k component only.
    //       We are assuming on the line is as good as in the cell.
    //       Assuming eilmer3 will be be consistent with vertex indices.

    dc[0] = 0; dc[1] = 0;
    
    if ( a_[ithread][0].z < 0.0 && a_[ithread][2].z > 0.0 ) dc[1] = -1;
    else if ( a_[ithread][0].z > 0.0 && a_[ithread][2].z < 0.0 ) dc[1] = 1;
    
    if ( a_[ithread][1].z < 0.0 && a_[ithread][3].z > 0.0 ) dc[0] = 1;
    else if ( a_[ithread][1].z > 0.0 && a_[ithread][3].z < 0.0 ) dc[0] = -1;
    
    // cout << "dc[0] = " << dc[0] << ", dc[1] = " << dc[1] << endl;
    
    return;
}

CellFinder3D::CellFinder3D( int nvertices )
: CellFinder( nvertices )
{
    // initialise vp_, a_ and dc_ arrays for each thread
    int nthreads = omp_get_max_threads();

    cout << "CellFinder3D::CellFinder3D()" << endl
         << "nthreads = " << nthreads << endl;
         
    vp_.resize( nthreads );
    a_.resize( nthreads );
    dc_.resize( nthreads );
    
    for ( int ithread=0; ithread<nthreads; ++ithread ) {
	vp_[ithread].resize( 3 );	// need vectors to 3 points to test a plane
	if ( nvertices==8 ) {
	    // hexahedron
	    a_[ithread].resize( 6 );
	}
	else if ( nvertices==4 ) {
	    // tetrahedron
	    cout << "CellFinder3D::CellFinder3D()" << endl
		 << "CellFinder3D is not yet able to handle unstructured grids" << endl
		 << "Exiting program." << endl;
	    exit( BAD_INPUT_ERROR );
	}
	
	dc_[ithread] = new int[3];	// cell stepping has 3 degrees of freedom
    }
}

CellFinder3D::~CellFinder3D()
{
    for ( size_t ithread=0; ithread<dc_.size(); ++ithread )
    	delete [] dc_[ithread];
}

int
CellFinder3D::
find_cell( const Vector3 &p, int &ib, int &ic, int &jc, int &kc )
{
    // &p -> address of spatial point we are looking for
    // ib -> block indice
    // ic, jc, kc -> cell indices
    
    // NOTE: assuming single block for the moment
    
    int ithread = omp_get_thread_num();
    
    // Get pointers to the block and cell
    Block * A = get_block_data_ptr(ib);
    FV_Cell * cell = A->get_cell(ic,jc,kc);
    
    // Pointer for the neighbour block
    // Block * B;
    
    // Search for the containing cell starting with the given guess
    dc_[ithread][0] = 1; dc_[ithread][1] = 1; dc_[ithread][2] = 1;
    int count = 0;
    int status = INSIDE_GRID;    
    while ( abs(dc_[ithread][0]) + abs(dc_[ithread][1]) + abs(dc_[ithread][2]) != 0 ) {
	// Check that predicted cell is inside the block domain
	if ( ic < A->imin ) {
	    // cout << "Outside the block: West" << endl;
	    if ( A->bcp[WEST]->type_code == ADJACENT ) {
	    	cout << "Multiple blocks in 3D not yet supported." << endl;
	    	exit( FAILURE );
	    }
	    else {
	    	// the ray is leaving the grid
	    	// step back inside the grid
	    	ic-=dc_[ithread][0]; 
	    	jc-=dc_[ithread][1];
	    	kc-=dc_[ithread][2];
	    	// set status flag to the exit direction
	    	status = WEST;
	    }
	}
	else if ( ic > A->imax ) {
	    // cout << "Outside the block: EAST" << endl;
	    if ( A->bcp[EAST]->type_code == ADJACENT ) {
	    	cout << "Multiple blocks in 3D not yet supported." << endl;
	    	exit( FAILURE );
	    }
	    else {
	    	// the ray is leaving the grid
	    	// step back inside the grid
	    	ic-=dc_[ithread][0]; 
	    	jc-=dc_[ithread][1];
	    	kc-=dc_[ithread][2];
	    	// set status flag to the exit direction
	    	status = EAST;
	    }
	}
	else if ( jc < A->jmin ) {
	    // cout << "Outside the block: SOUTH" << endl;
	    if ( A->bcp[SOUTH]->type_code == ADJACENT ) {
	    	cout << "Multiple blocks in 3D not yet supported." << endl;
	    	exit( FAILURE );
	    }
	    else {
	    	// the ray is leaving the grid
	    	// step back inside the grid
	    	ic-=dc_[ithread][0]; 
	    	jc-=dc_[ithread][1];
	    	kc-=dc_[ithread][2];
	    	// set status flag to the exit direction
	    	status = SOUTH;
	    }
	}
	else if ( jc > A->jmax ) {
	    // cout << "Outside the block: NORTH" << endl;
	    if ( A->bcp[NORTH]->type_code == ADJACENT ) {
	    	cout << "Multiple blocks in 3D not yet supported." << endl;
	    	exit( FAILURE );
	    }
	    else {
	    	// the ray is leaving the grid
	    	// step back inside the grid
	    	ic-=dc_[ithread][0]; 
	    	jc-=dc_[ithread][1];
	    	kc-=dc_[ithread][2];
	    	// set status flag to the exit direction
	    	status = NORTH;
	    }
	}
	else if ( kc < A->kmin ) {
	    // cout << "Outside the block: BOTTOM" << endl;
	    if ( A->bcp[BOTTOM]->type_code == ADJACENT ) {
	    	cout << "Multiple blocks in 3D not yet supported." << endl;
	    	exit( FAILURE );
	    }
	    else {
	    	// the ray is leaving the grid
	    	// step back inside the grid
	    	ic-=dc_[ithread][0]; 
	    	jc-=dc_[ithread][1];
	    	kc-=dc_[ithread][2];
	    	// set status flag to the exit direction
	    	status = BOTTOM;
	    }
	}
	else if ( kc > A->kmax ) {
	    // cout << "Outside the block: TOP" << endl;
	    if ( A->bcp[TOP]->type_code == ADJACENT ) {
	    	cout << "Multiple blocks in 3D not yet supported." << endl;
	    	exit( FAILURE );
	    }
	    else {
	    	// the ray is leaving the grid
	    	// step back inside the grid
	    	ic-=dc_[ithread][0]; 
	    	jc-=dc_[ithread][1];
	    	kc-=dc_[ithread][2];
	    	// set status flag to the exit direction
	    	status = TOP;
	    }
	}
	// else -> do nothing, retain INSIDE_GRID status
	
	A = get_block_data_ptr( ib );
	cell = A->get_cell( ic, jc, kc );
	
	// break if we are no longer inside the grid
	if ( status != INSIDE_GRID ) break;

	// Otherwise test the predicted cell
	test_cell( cell, p, dc_[ithread] );
	ic += dc_[ithread][0]; jc += dc_[ithread][1]; kc += dc_[ithread][2];

	// increment search counter
	++count;
	// A conservative check to see that we are not in an 'infinite' loop
	if ( count > (A->imax + A->jmax + A->kmax) ) {
	    cout << "Cell search seems to be looping." << endl;
	    status = ERROR;
	    break;
	}
    }
    
    return status;
}

static int hex_vertex_indices[6][3] = { { 2, 6, 7 },
					{ 5, 6, 2 },
					{ 0, 4, 5 },
					{ 0, 3, 7 },
					{ 7, 6, 5 },
				  	{ 0, 1, 2 } };

void
CellFinder3D::
test_cell( const FV_Cell * cell, const Vector3 &p, int *dc )
{
    // NOTE: this function should be applicable to all polyhedral cells
    
    int ithread = omp_get_thread_num();
    
    // calculate 'a' for all faces
    for ( size_t iface=0; iface<a_[ithread].size(); ++iface ) {
    	for ( size_t ivtx=0; ivtx<vp_[ithread].size(); ++ ivtx ) {
    	    // get ordered (clockwise) vertex indices from hex_vertex_indices
    	    vp_[ithread][ivtx] = cell->vtx[ hex_vertex_indices[iface][ivtx] ]->pos - p;
    	}
    	a_[ithread][iface] = dot( vp_[ithread][0], cross( vp_[ithread][1], vp_[ithread][2] ) );
    }
    
    // if an a_ value is negative, it is on the 'correct' side of that face
    // if all a_ values are negative, then point is inside cell
    // NOTE: - we are assuming on the line is as good as in the cell.
    //       - and that eilmer3 will be be consistent with vertex indices.
    
    dc[0] = 0; dc[1] = 0; dc[2] = 0;
    
    // i direction
    if ( a_[ithread][EAST] > 0.0 && a_[ithread][WEST] < 0.0 ) dc[0] = 1;
    else if ( a_[ithread][EAST] < 0.0 && a_[ithread][WEST] > 0.0 ) dc[0] = -1;
    
    // j direction
    if ( a_[ithread][NORTH] > 0.0 && a_[ithread][SOUTH] < 0.0 ) dc[1] = 1;
    else if ( a_[ithread][NORTH] < 0.0 && a_[ithread][SOUTH] > 0.0 ) dc[1] = -1;
    
    // k direction
    if ( a_[ithread][TOP] > 0.0 && a_[ithread][BOTTOM] < 0.0 ) dc[2] = 1;
    else if ( a_[ithread][TOP] < 0.0 && a_[ithread][BOTTOM] > 0.0 ) dc[2] = -1;
    
    return;
}

