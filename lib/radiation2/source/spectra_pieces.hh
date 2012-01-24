/** \file spectra_pieces.hh
 *  \ingroup radiation2
 *
 *  \brief Functions, classes and structures for spectral calculations
 *
 *  \author Daniel F. Potter
 *  \version 15-Sep-09: initial implementation
 *
 **/

#ifndef SPECTRA_PIECES_HH
#define SPECTRA_PIECES_HH

#include <string>
#include <vector>

// Forward declaration of RadiationSpectralModel
class RadiationSpectralModel;

static std::vector<double> zero_vec;

class SpectralContainer {
public:
    /// \brief Minimal Constructor 
    SpectralContainer();
    
    /// \brief Constructor 
    SpectralContainer( RadiationSpectralModel * rsm );
    
    /// \brief Copy constructor 
    SpectralContainer( SpectralContainer &C );
    
    /// \brief Deconstructor
    virtual ~SpectralContainer() = 0;
    
public:    
    virtual double write_to_file( std::string fname ) = 0;
    
    double write_data_to_file( std::string fname,
    		    	       std::vector<double> &Y1, std::string Y1_label, std::string Y1_int_label,
    		    	       std::vector<double> &Y2 = zero_vec, std::string Y2_label = "" );
    
public:
    std::vector<double> nu;
};

class CoeffSpectra : public SpectralContainer {
public:
    /// \brief Minimal Constructor 
    CoeffSpectra();
    
    /// \brief Constructor 
    CoeffSpectra( RadiationSpectralModel * rsm  );
    
    /// \brief Deconstructor
    ~CoeffSpectra();
    
public:
    /* Spectral properties storage */
    std::vector<double> j_nu;
    std::vector<double> kappa_nu;
    
    /* Cumulative emission vector */
    std::vector<double> j_int;
    
public:
    /// \brief Clone function
    CoeffSpectra * clone();
    
    /// \brief Write CoeffSpectra class data to file
    double write_to_file( std::string fname );
    
    /// \brief Integrate the emission coefficient spectrum
    double integrate_emission_spectra();
    
    /// \brief Calculate and store the cumulative emission coefficient
    void calculate_cumulative_emission( bool resize=false );
};

double eval_Gaussian( double x, double delta_x );

class SpectralIntensity : public SpectralContainer {
public:
    /// \brief Minimal Constructor 
    SpectralIntensity();
    
    /// \brief Constructor from rsm
    SpectralIntensity( RadiationSpectralModel * rsm  );
    
    /// \brief Constructor from rsm with plank function evaluated at T
    SpectralIntensity( RadiationSpectralModel * rsm, double T );
    
    /// \brief Copy constructor
    SpectralIntensity( SpectralIntensity &S  );
    
    /// \brief Deconstructor
    ~SpectralIntensity();
    
public:
    double write_to_file( std::string fname );
    
    void apply_apparatus_function( double delta_x_ang );
    
    void reverse_data_order();
    
    void reset_intensity_vector();
    
    double integrate_intensity_spectra( double lambda_min=-1.0, double lambda_max=-1.0 );
    
public:
    std::vector<double> I_nu;
    std::vector<double> I_int;
    int nwidths;
};

class SpectralFlux : public SpectralContainer {
public:
    /// \brief Minimal Constructor 
    SpectralFlux();
    
    /// \brief Constructor 
    SpectralFlux( RadiationSpectralModel * rsm  );
    
    /// \brief Deconstructor
    ~SpectralFlux();
    
public:
    double write_to_file( std::string fname );
    
public:
    std::vector<double> q_nu;
};

class IntensityProfile {
public:
    /// \brief Minimal Constructor 
    IntensityProfile();
    
    /// \brief Deconstructor
    ~IntensityProfile();
    
public:
    void add_new_point( double x, double I );
    
    void spatially_smear( double dx_smear );
    
    void spatially_smear_for_varying_dx( double dx_smear );
    
    void write_to_file( std::string fname );
    
public:
    std::vector<double> x_vec;
    std::vector<double> I_vec;
};

class SpectralField {
public:
    /// \brief Minimal Constructor 
    SpectralField();
    
    /// \brief Deconstructor
    ~SpectralField();
    
public:
    void add_new_intensity_spectra( double x, SpectralIntensity * S );
    
    IntensityProfile extract_intensity_profile( double lambda_l, double lambda_u );
    
    SpectralIntensity * last_spectra()
    { return S_vec.back(); }
    
public:
    std::vector<double> x_vec;
    std::vector<SpectralIntensity*> S_vec;
};

/* Some other useful functions */

/// \brief Convert from frequency (Hz) to wavelength (nm)
double nu2lambda(double nu);

/// \brief Convert from wavelength (nm) to frequency (Hz)
double lambda2nu(double lambda_nm);

/// \brief Calculate Planck (blackbody) intensity at frequency nu (Hz) and temperature T (K)
double planck_intensity(const double nu, const double T);

/// \brief Get the frequency index in 'nus' that is just below 'nu'
int get_nu_index( std::vector<double> &nus, double nu );

#endif
