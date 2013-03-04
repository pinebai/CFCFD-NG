// bc_catalytic.hh


const int MAX_EQ_WC_TABLE_ENTRIES = 100;

//-----------------------------------------------------------------
// A class to apply the catalytic-wall BC
class CatalyticWallBC {
public:
    CatalyticWallBC( int type_code );
    CatalyticWallBC( CatalyticWallBC &cw );
    virtual ~CatalyticWallBC();
    
    virtual int apply( Gas_data &Q, std::vector<double> &massf ) = 0;
    
public:
    int type_code;
};

class SuperCatalyticWallBC : public CatalyticWallBC {
public:
    SuperCatalyticWallBC( std::vector<double> massf_wall );
    SuperCatalyticWallBC( SuperCatalyticWallBC &cw );
    ~SuperCatalyticWallBC();
    
    int apply( Gas_data &Q, std::vector<double> &massf );
    
private:
    std::vector<double> massf_wall;
};

class EquilibriumCatalyticWallBC : public CatalyticWallBC {
public:
    EquilibriumCatalyticWallBC( std::string fname );
    EquilibriumCatalyticWallBC( EquilibriumCatalyticWallBC &cw );
    ~EquilibriumCatalyticWallBC();
    
    int apply( Gas_data &Q, std::vector<double> &massf );
    
private:
    std::vector<double> fC[MAX_EQ_WC_TABLE_ENTRIES];
    double lpmin, dlp;
    int ipmax;
};
