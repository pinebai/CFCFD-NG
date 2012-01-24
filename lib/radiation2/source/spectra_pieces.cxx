/** \file spectra_pieces.cxx
 *  \ingroup radiation
 *
 *  \brief Class definitions for line-of-sight calculations
 *
 *  \author Daniel F. Potter
 *  \version 15-Sep-09: initial implementation
 *            
 **/

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <math.h>
#include <iomanip>
#include <algorithm>

#include "../../util/source/useful.h"

#include "spectral_model.hh"
#include "spectra_pieces.hh"
#include "radiation_constants.hh"

using namespace std;

/* ------------ SpectralContainer class ------------ */

SpectralContainer::SpectralContainer() {}

SpectralContainer::SpectralContainer( RadiationSpectralModel * rsm )
{
    // Assuming uniform spectral distribution
    int nnus = rsm->get_spectral_points();
    nu.resize( nnus );
    
    // Uniformally distributed spectral points with constant frequency spacing
    double nu_val = lambda2nu( rsm->get_lambda_max() );
    double dnu = ( lambda2nu( rsm->get_lambda_min() ) - lambda2nu( rsm->get_lambda_max() ) ) 
		/ double ( ( rsm->get_spectral_points() - 1 ) );
    for( int inu=0; inu<rsm->get_spectral_points(); ++inu ){
	nu[inu] = nu_val;
	nu_val+=dnu;
    }
}

SpectralContainer::SpectralContainer(SpectralContainer &C)
: nu( C.nu ) {}

SpectralContainer::~SpectralContainer()
{
    nu.resize(0);
}

double
SpectralContainer::
write_data_to_file( string fname,
    		    vector<double> &Y1, string Y1_label, string Y1_int_label,
    		    vector<double> &Y2, string Y2_label )
{
    /* 0. Determine if Y2 data is present */
    bool with_Y2 = ( Y2.size() > 0 ) ? true : false;
    
    /* 1. Setup the output file. */
    ofstream specfile;
    specfile.open(fname.c_str());
    if( specfile.fail() ) {
	cout << "Error opening file: " << fname << endl;
	cout << "Bailing Out!\n";
	exit(FILE_ERROR);
    }
    
    specfile << setprecision(12) << scientific << showpoint;

    specfile << "# " << fname << endl
    	     << "# Column 1: Wavelength (nm)" << endl
             << "# Column 2: " << Y1_label << endl
             << "# Column 3: " << Y1_int_label << endl;
    if ( with_Y2 ) {
    	specfile << "# Column 4: " << Y2_label << endl;
    }
    
    /* 2. Write data for each frequency interval to file. */
    double Y1_int = 0.0;
    for ( int inu=int(nu.size())-1; inu >= 0; --inu ) {
        // Integration of Y1
        if ( inu < int(nu.size())-1 ) 
	    Y1_int += 0.5 * ( Y1[inu] + Y1[inu+1] ) * fabs(nu[inu] - nu[inu+1]);
	// Write to file
        specfile << setw(20) << nu2lambda( nu[inu] )
                 << setw(20) << Y1[inu] * nu[inu] * nu[inu] / RC_c_SI
                 << setw(20) << Y1_int;
        if ( with_Y2 ) {
            specfile << setw(20) << Y2[inu];
        }
        specfile << endl;
        // move on to next frequency interval
    }
    
    specfile.close();
    
    return Y1_int;
}

/* ------------ CoeffSpectra class ------------ */

CoeffSpectra::CoeffSpectra() {}

CoeffSpectra::CoeffSpectra( RadiationSpectralModel * rsm )
 : SpectralContainer( rsm )
{
    j_nu.resize( nu.size() );
    kappa_nu.resize( nu.size() );
    j_int.resize( nu.size() );
}
    

CoeffSpectra::~CoeffSpectra()
{
    j_nu.resize(0);
    kappa_nu.resize(0);
    j_int.resize(0);
}

CoeffSpectra * CoeffSpectra::clone()
{
    CoeffSpectra * X = new CoeffSpectra();
    X->nu.assign( nu.begin(), nu.end() );
    X->j_nu.assign( j_nu.begin(), j_nu.end() );
    X->kappa_nu.assign( kappa_nu.begin(), kappa_nu.end() );
    X->j_int.assign( j_int.begin(), j_int.end() );
    
    return X;
}

double CoeffSpectra::write_to_file( string fname )
{
    string Y1_label = "Emission coefficient, j_lambda (W/m**3-sr-m)";
    string Y1_int_label = "Integrated emission, j (W/m**3-sr)";
    string Y2_label = "Absorption coefficient, kappa_lambda (1/m)";
    
    return write_data_to_file( fname, j_nu, Y1_label, Y1_int_label, kappa_nu, Y2_label );
}

double CoeffSpectra::integrate_emission_spectra()
{
    double j_total = 0.0;
    for ( int inu=1; inu < int(nu.size()); inu++ ) {
    	j_total += 0.5 * ( j_nu[inu] + j_nu[inu-1] ) * fabs(nu[inu] - nu[inu-1]);
    }

    return j_total;
}

void CoeffSpectra::calculate_cumulative_emission( bool resize )
{
    // 0. (Re)size the vector if requested
    if ( resize )
    	j_int.resize( j_nu.size(), 0.0 );
    
    // 1. Integrate, storing cumulative integrand along the way
    j_int[0] = 0.0;
    for ( int inu=1; inu < int(nu.size()); inu++ ) {
    	j_int[inu] = j_int[inu-1] + 0.5 * ( j_nu[inu] + j_nu[inu-1] ) * fabs(nu[inu] - nu[inu-1]);
    }
    
    return;
}

/* ------------ SpectralIntensity class ------------ */

double eval_Gaussian( double x, double delta_x )
{
    //       x: distance from center of profile
    // delta_x: half-width at half-maximum
    double A = (1.0/delta_x)*sqrt(log(2.0)/M_PI);
    double B = -log(2.0)*(x/delta_x)*(x/delta_x);
    
    return A*exp(B);
}

SpectralIntensity::SpectralIntensity()
: nwidths( 5 ) {}

SpectralIntensity::SpectralIntensity( RadiationSpectralModel * rsm )
 : SpectralContainer( rsm )
{
    I_nu.resize( nu.size() );
    I_int.resize( nu.size() );
    
    nwidths = 5;
}

SpectralIntensity::SpectralIntensity( RadiationSpectralModel * rsm, double T )
 : SpectralContainer( rsm )
{
    I_nu.resize( nu.size() );
    I_int.resize( nu.size() );
    
    nwidths = 5;
    
    for ( size_t inu=0; inu<nu.size(); ++inu ) {
    	I_nu[inu] = planck_intensity( nu[inu], T );
    }
}

SpectralIntensity::SpectralIntensity(SpectralIntensity &S )
 : SpectralContainer( S ), I_nu( S.I_nu ), I_int( S.I_int ), nwidths( S.nwidths ) {}

SpectralIntensity::~SpectralIntensity()
{
    I_nu.resize(0);
    I_int.resize(0);
}

double SpectralIntensity::write_to_file( string filename )
{
    string Y1_label = "Spectral intensity, I_lambda (W/m**2-sr-m)";
    string Y1_int_label = "Integrated intensity, I (W/m**2-sr)";
    
    return write_data_to_file( filename, I_nu, Y1_label, Y1_int_label );
}

void SpectralIntensity::apply_apparatus_function( double delta_x_ang )
{
    /* apply apparatus function (Gaussian distribution with delta_x as HWHM) to
       the provided intensity spectrum (via convolution integral) */
       
    /* FIXME: A smeared spectra can be represented by less points than the 
              original spectra.  Perhaps use LINE_POINTS per delta_x_ang?
	      [Inner (convolution) loop can be reduced also]               */
       
    // Quick exit if delta_x_ang is zero
    if ( delta_x_ang==0.0 ) return;
    
    // A vector to temporarily smeared hold data
    vector<double> I_nu_temp( nu.size() );
       
    // NOTE: delta_x_ang is the HFHM of the spectrometer in units of Angstroms
    int percentage=0;
       
    for( size_t inu=0; inu<nu.size(); inu++) {
	double nu_val = nu[inu];
	double lambda_ang = 10.0 * nu2lambda( nu_val );
	// convert delta_x_ang to delta_x_hz
	double delta_x_hz = delta_x_ang / lambda_ang * nu_val;
	double nu_lower = nu_val - double(nwidths) * delta_x_hz;
	double nu_upper = nu_val + double(nwidths) * delta_x_hz;
	int jnu_start = get_nu_index(nu,nu_lower) + 1;
	int jnu_end = get_nu_index(nu,nu_upper) + 1;
	
	if((double(inu)/double(nu.size())*100.0+1.0)>double(percentage)) {
	    cout << "Smearing spectrum: | " << percentage << "% |, jnu_start = " << jnu_start << ", jnu_end = " << jnu_end << " \r" << flush;
	    percentage += 10;
        }
	
	// Apply convolution integral over this frequency range with trapezoidal method
	double I_nu_conv = 0.0;
	for ( int jnu=jnu_start; jnu<jnu_end; jnu++ ) {
	    if ( jnu>jnu_start ) {
		double x0 = nu[jnu-1] - nu_val;
		double x1 = nu[jnu] - nu_val;
		double f_x0 = eval_Gaussian(x0, delta_x_hz);
		double f_x1 = eval_Gaussian(x1, delta_x_hz);
		I_nu_conv += 0.5 * ( I_nu[jnu-1]*f_x0 + I_nu[jnu]*f_x1 ) * ( nu[jnu] - nu[jnu-1] );
	    }
	}
	
	// Make sure a zero value is not returned if nwidths is too small
	if ( jnu_start==jnu_end ) {
	    cout << "SpectralIntensity::apply_apparatus_function()" << endl
	         << "WARNING: nwidths is too small!" << endl;
	    I_nu_conv = I_nu[inu];
	}
	
	// Save result
	I_nu_temp[inu] = I_nu_conv;
    }
    
    cout << endl;
    
    // Loop again to overwrite old I_nu values in S
    for( size_t inu=0; inu<nu.size(); inu++)
	I_nu[inu] = I_nu_temp[inu];
    
    return;
}

void SpectralIntensity::reverse_data_order()
{
    reverse(I_nu.begin(), I_nu.end());
    reverse(I_int.begin(), I_int.end());
    reverse(nu.begin(), nu.end());
    
    return;
}

void SpectralIntensity::reset_intensity_vector()
{
    for ( size_t inu=0; inu<I_nu.size(); ++inu ) 
    	I_nu[inu] = 0.0;
    
    for ( size_t inu=0; inu<I_int.size(); ++inu )
    	I_int[inu] = 0.0;
    
    return;
}

double SpectralIntensity::integrate_intensity_spectra( double lambda_min, double lambda_max )
{
    // 1. Find spectral indice range to integrate over
    int inu_start = 0;
    if ( lambda_max > 0.0 ) inu_start = get_nu_index(nu, lambda2nu(lambda_max))-1;
    
    int inu_end = nu.size() - 1;
    if ( lambda_min > 0.0 ) inu_end = get_nu_index(nu, lambda2nu(lambda_min))-1;

    double I_total = 0.0;
    for( int inu=inu_start; inu<inu_end; ++inu ) {
    	I_total += 0.5 * ( I_nu[inu] + I_nu[inu+1] ) * ( nu[inu+1] - nu[inu] );
    	I_int[inu] = I_total;
    }

    return I_total;
}

/* ------------ SpectralFlux class ------------ */

SpectralFlux::SpectralFlux() {}

SpectralFlux::SpectralFlux( RadiationSpectralModel * rsm )
 : SpectralContainer( rsm )
{
    q_nu.resize( nu.size() );
}

SpectralFlux::~SpectralFlux()
{
    q_nu.resize(0);
}

double SpectralFlux::write_to_file( string filename )
{
    string Y1_label = "Spectral flux, q_lambda (W/m**2-sr-m)";
    string Y1_int_label = "Integrated flux, q (W/m**2-sr)";
    
    return write_data_to_file( filename, q_nu, Y1_label, Y1_int_label );
}

/* ------------ IntensityProfile class ------------ */

IntensityProfile::IntensityProfile() {}

IntensityProfile::~IntensityProfile() {}

void
IntensityProfile::add_new_point( double x, double I )
{
    x_vec.push_back( x );
    I_vec.push_back( I );

    return;
}

void
IntensityProfile::spatially_smear( double dx_smear )
{
    if ( dx_smear < 0.0 ) return;
    
    if ( x_vec.size()<2 ) {
    	cout << "IntensityProfile::spatially_smear()" << endl
    	     << "Cannot spatially smear as there are only " << x_vec.size()
    	     << " points present.\nBailing out!" << endl;
    	exit( BAD_INPUT_ERROR );
    }
    
    double dx = ( x_vec[1] - x_vec[0] );
    int np_smear = int( dx_smear / dx );
    if ( np_smear < 1 ) {
    	cout << "IntensityProfile::spatially_smear()" << endl
    	     << "dx_smear = " << dx_smear << " need to be >> dx = " << dx << endl
    	     << "Returning unsmeared data." << endl;
    	     return;
    }
    int nps = int(x_vec.size());
    vector<double> x_tmp;
    vector<double> I_tmp;
    
    for ( int ip=0; ip < nps+np_smear; ip++ ) {
    	double s = x_vec[0] + dx * ( double( ip - np_smear) );
    	x_tmp.push_back( s );
    	I_tmp.push_back( 0.0 );
    	for ( int jp=ip-np_smear; jp<ip; ++jp ) {
    	    double I = 0.0;
    	    if ( jp < 0 ) I = 0.0;
    	    else if ( jp >= nps ) I = I_vec[nps-1];
	    else I = I_vec[jp];
	    I_tmp.back() += I;
	}
	// Divide by number of averaging points
	I_tmp.back() /= double(np_smear);
    }
    
    // Replace the original data with the smeared data
    x_vec = x_tmp;
    I_vec = I_tmp;
    
    return;
}

void
IntensityProfile::spatially_smear_for_varying_dx( double dx_smear )
{
    if ( dx_smear < 0.0 ) return;
    
    if ( x_vec.size()<2 ) {
    	cout << "IntensityProfile::spatially_smear_for_varying_dx()" << endl
    	     << "Cannot spatially smear as there are only " << x_vec.size()
    	     << " points present.\nBailing out!" << endl;
    	exit( BAD_INPUT_ERROR );
    }
    
    int nps = int(x_vec.size());
    vector<double> x_tmp;
    vector<double> I_tmp;
    
    for ( int ip=0; ip < nps; ip++ ) {
    	double s = x_vec[ip] - dx_smear;
    	x_tmp.push_back( s );
    	I_tmp.push_back( 0.0 );
    	double dx_weighting = 0.0;
    	for ( int jp=0; jp < nps; jp++ ) {
    	    if ( x_vec[jp] >= s && x_vec[jp] <= s + dx_smear ) {
    	    	double dx = 0.0;
    	    	if ( jp==0 ) dx = fabs(x_vec[jp+1] - x_vec[jp]);
    	    	else dx = fabs(x_vec[jp] - x_vec[jp-1]);
    	    	I_tmp.back() += I_vec[jp] * dx;
    	    	dx_weighting += dx;
    	    }
	}
	// Divide by number of averaging weighting
	I_tmp.back() /= dx_weighting;
    }
    
    double last_dx = x_vec[nps-1] - x_vec[nps-2];
    double s = x_vec[nps-1] - dx_smear + last_dx;
    while ( s <= x_vec[nps-1] ) {
     	x_tmp.push_back( s );
    	I_tmp.push_back( 0.0 );
    	double dx_weighting = 0.0;
    	for ( int jp=0; jp < nps; jp++ ) {
    	    if ( x_vec[jp] >= s && x_vec[jp] <= s + dx_smear ) {
    	    	double dx = 0.0;
    	    	if ( jp==0 ) dx = fabs(x_vec[jp+1] - x_vec[jp]);
    	    	else dx = fabs(x_vec[jp] - x_vec[jp-1]);
    	    	I_tmp.back() += I_vec[jp] * dx;
    	    	dx_weighting += dx;
    	    }
	}
	// Divide by number of averaging weighting
	I_tmp.back() /= dx_weighting;
	// increment s
	s += last_dx;
    }
    
    // Replace the original data with the smeared data
    x_vec = x_tmp;
    I_vec = I_tmp;
    
    return;
}

void
IntensityProfile::write_to_file( string fname )
{
    /* 1. Setup the output file. */
    ofstream ofile;
    ofile.open(fname.c_str());
    if( ofile.fail() ) {
	cout << "Error opening file: " << fname << endl;
	cout << "Bailing Out!\n";
	exit(FILE_ERROR);
    }
    
    ofile << setprecision(12) << scientific << showpoint;

    ofile << "# " << fname << endl
    	  << "# Column 1: x (m)" << endl
          << "# Column 2: Intensity, I (W/m2-sr)" << endl;
    
    /* 2. Write data to file. */
    for ( int ip=0; ip < int(x_vec.size()); ++ip ) {
	// Write to file
        ofile << setw(20) << x_vec[ip] << setw(20) << I_vec[ip] << endl;
    }
    
    ofile.close();
}

/* ------------ SpectralField class ------------ */

SpectralField::SpectralField() {}

SpectralField::~SpectralField() {}

void
SpectralField::add_new_intensity_spectra( double x, SpectralIntensity * S )
{
    x_vec.push_back( x );
    S_vec.push_back( new SpectralIntensity(*S) );
    
    cout << "S_vec.back()->nu.size() = " << S_vec.back()->nu.size() << endl;
    cout << "S->nu.size() = " << S->nu.size() << endl;

    return;
}

IntensityProfile
SpectralField::
extract_intensity_profile( double lambda_l, double lambda_u )
{
    if ( S_vec.size()==0 ) {
	cout << "SpectralField::extract_intensity_profile()" << endl
	     << "No spectra present, bailing out!" << endl;
	exit( BAD_INPUT_ERROR );
    }
    
    // 1. Find spectral indice range to integrate over
    int inu_start = get_nu_index(S_vec[0]->nu, lambda2nu(lambda_u));
    int inu_end = get_nu_index(S_vec[0]->nu, lambda2nu(lambda_l));
    
    // 2. Loop over LOS_points and spectrally integrate
    IntensityProfile IvX;
    for ( size_t ip=0; ip<x_vec.size(); ++ip ) {
    	double I_total = 0.0;
    	for( int inu=inu_start; inu<inu_end; ++inu )
    	    I_total += 0.5 * ( S_vec[ip]->I_nu[inu] + S_vec[ip]->I_nu[inu+1] ) * ( S_vec[ip]->nu[inu+1] - S_vec[ip]->nu[inu] );
    	IvX.add_new_point( x_vec[ip], I_total );
    }
    
    return IvX;
}

/* ------------- Helper functions -------------- */

double nu2lambda( double nu ) 
{
    double lambda_nm = RC_c_SI / nu * 1.0e9;
    
    return lambda_nm;
}

double lambda2nu( double lambda_nm ) 
{
    double nu = RC_c_SI / lambda_nm * 1.0e9;
    
    return nu;
}

double planck_intensity(const double nu, const double T)
{
    double B_nu = 0.0;
    
    // Impose a lower temperature limit for convenience in initialisations
    if ( T > 50.0 ) {
	double top = 2.0 * RC_h_SI * nu * nu * nu;
	double bottom = RC_c_SI * RC_c_SI * ( exp( RC_h_SI * nu / ( RC_k_SI * T ) ) - 1.0 );
	B_nu = top / bottom;
    }

    return B_nu;
}

int get_nu_index( vector<double> &nus, double nu )
{
    // NOTE: this function only works for uniform spectral distribution
    // 0. Firstly check if nu is in range
    int nnu = int ( nus.size() );
    int inu;
    if ( nu < nus.front() ) inu=0;
    else if ( nu > nus.back() ) inu=nnu-1;
    else {
	// 1. nu is in range, so find the appropriate index
#	if SPECTRAL_DISTRIBUTION == UNIFORM
	double dnu = ( nus.back() - nus.front() ) / double ( nnu - 1 );
	inu = int ( ( nu - nus.front() ) / dnu );
#	elif SPECTRAL_DISTRIBUTION == OPTIMISE
	vector<double>::iterator iter = lower_bound( nus.begin(), nus.end(), nu );
	inu = distance( nus.begin(), iter );
#       endif
    }

    // cout << "nu = " << nu << "inu = " << inu << ", nus.front() = " << nus.front() << ", nus.back() = " << nus.back() << endl;
    
    return inu;
}

