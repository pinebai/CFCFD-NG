/** \file l_post.cxx
 * \ingroup l1d2
 * \brief Postprocessor for l1d.cxx.
 *
 * This program is used to read the flow solution data 
 * generated by the One-Dimensional Lagrangian code 
 * and then generate either
 *
 * 1. a generic plotting file containing
 *    solution data for a single time instant or
 *
 * 2. A solution file suitable for a simulation restart.
 * 
 * \author PA Jacobs
 *
 * \version 1.0 --  17-Dec-91
 * \version 2.0 --  20-Dec-91, multi-slug version
 * \version 3.0 --  31-Dec-91, include piston dynamics
 * \version 4.0 --  03-Jan-92, include diaphragms
 * \version 4.1 --  08-Jan-92, logarithm options for pressure and density
 * \version 5.0 --  13-Jan-92, histories at specified x-locations
 * \version 6.0 --  30-Mar-95, entropy added (heat transfer from some months back)
 * \version 6.0 --  11-Apr-98, Linux version, more general input
 * \version 6.1 --  13-Apr-98, command-line options, both Generic and Save
 * \version 6.2 --  22-Apr-98, better names for options; GNU Plot outout
 * \version 24-Jul-06, C++ port
 */

/*-----------------------------------------------------------------*/

#include <vector>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../../../lib/util/source/useful.h"
#include "../../../lib/util/source/config_parser.hh"
#include "l_kernel.hh"
#include "l_tube.hh"
#include "l1d.hh"
#include "l_io.hh"

int L_write_GENERIC(std::vector<slug_data> &A, int nslug, FILE *outfile, int nsp);
int take_log_rho(std::vector<slug_data> &A, int nslug);
int take_log_p(std::vector<slug_data> &A, int nslug);

/*-----------------------------------------------------------------*/

int main(int argc, char **argv)
{
    struct simulation_data SD;
    int js, jp, jd;
    std::vector<slug_data> A;               /* several gas slugs        */
    std::vector<piston_data> Pist;          /* room for several pistons */
    std::vector<diaphragm_data> Diaph;      /* diaphragms            */

    double tstop;
    int i, max_sol, echo_input;

    int ilogr, ilogp;   /* postprocessing options       */
    char oname[40], base_file_name[32];
    char pname[40], iname[40];
    char suffix[20];
    FILE *infile,    /* flow solution file           */
        *outfile;   /* plot file                    */
    int command_line_error, found_solution, output_format;

    /*
     * INITIALIZE
     */
    printf("\n------------------------------------");
    printf("\nLagrangian 1D Solution Postprocessor");
    printf("\n------------------------------------");
    printf("\n\n");

    strcpy(base_file_name, "default");
    echo_input = 0;
    max_sol = 10000;
    tstop = 0.0;
    ilogr = 0;
    ilogp = 0;
#   define  PLOT     0
#   define  RESTART  1
#   define  SPLIT    2
    output_format = PLOT;

    /*
     * Decode command line arguments.
     */
    command_line_error = 0;
    if (argc < 2) {
        /* no arguments supplied */
        command_line_error = 1;
        goto usage;
    }   /* end if */

    i = 1;
    while (i < argc) {
        /* process the next command-line argument */

        if (strcmp(argv[i], "-f") == 0) {
            /* Set the base file name. */
            i++;
            if (i >= argc) {
                command_line_error = 1;
                goto usage;
            }
            strcpy(base_file_name, argv[i]);
            i++;
            printf("Setting base_file_name = %s\n", base_file_name);
        } else if (strcmp(argv[i], "-echo") == 0) {
            echo_input = 1;
            i++;
            printf("Will echo input parameters...\n");
        } else if (strcmp(argv[i], "-t") == 0) {
            /* Set the target solution time. */
            i++;
            if (i >= argc) {
                command_line_error = 1;
                goto usage;
            }
            sscanf(argv[i], "%lf", &tstop);
            i++;
            printf("Setting tstop = %e\n", tstop);
        } else if (strcmp(argv[i], "-logrho") == 0) {
            ilogr = 1;
            i++;
            printf("Take logarithm of density.\n");
        } else if (strcmp(argv[i], "-logp") == 0) {
            ilogp = 1;
            i++;
            printf("Take logarithm of pressure.\n");
        } else if (strcmp(argv[i], "-restart") == 0) {
            output_format = RESTART;
            i++;
            printf("Output format is Raw Data for Restart.\n");
        } else if (strcmp(argv[i], "-split") == 0) {
            output_format = SPLIT;
            i++;
            printf("Output format is Raw Data in Individual Blocks.\n");
        } else if (strcmp(argv[i], "-plot") == 0) {
            output_format = PLOT;
            i++;
            printf("Output format is for Plotting.\n");
        } else if (strcmp(argv[i], "-help") == 0) {
            command_line_error = 1;
            printf("Printing usage message...\n\n");
            goto usage;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            command_line_error = 1;
            goto usage;
        }   /* end if */
    }   /* end while */

    /*
     * If the command line arguments are incorrect,
     * write some advice to the console then finish.
     */
    usage:
    if (command_line_error == 1) {
        printf("Purpose:\n");
        printf("Pick up a solution file <base_file_name>.Ls, skip to\n");
        printf("a particular solution time and then either save that\n");
        printf("solution as raw data for a simulation restart, \n");
        printf("or reformatted for a plotting program.\n");
        printf("When reformatting, the density and pressure fields can\n");
        printf("be converted to logarithmic scales.\n");
        printf("\n");

        printf("Command-line options: (defaults are shown in parentheses)\n");
        printf("-f <base_file_name>       (default)\n");
        printf("-echo                     (no echo)\n");
        printf("-t <time>                 (0.0)\n");
        printf("-plot                     (plot)\n");
        printf("-restart                  (plot)\n");
        printf("-split                    (plot)\n");
        printf("-logrho                   (linear)\n");
        printf("-logp                     (linear)\n");
        printf("-help          (print this message)\n");
        exit(1);
    }   /* end if command_line_error */

    /*
     * * Read the input parameter file.
     */
    strcpy(pname, base_file_name);
    strcat(pname, ".Lp");
    printf("parameterfile: %s\n", pname);
    ConfigParser parameterdict = ConfigParser(pname);
    L_set_case_parameters(&SD, parameterdict, echo_input);
    TubeModel tube = TubeModel(pname, echo_input);
    A.resize(SD.nslug);
    Pist.resize(SD.npiston);
    Diaph.resize(SD.ndiaphragm);
    for (jp = 0; jp < SD.npiston; ++jp) {
        set_piston_parameters(&(Pist[jp]), jp, parameterdict, SD.dt_init, echo_input);
        Pist[jp].sim_time = 0.0;
    }
    for (jd = 0; jd < SD.ndiaphragm; ++jd) {
        set_diaphragm_parameters(&(Diaph[jd]), jd, parameterdict, echo_input);
        Diaph[jd].sim_time = 0.0;
    }
    for (js = 0; js < SD.nslug; ++js) {
        L_set_slug_parameters(&(A[js]), js, &SD, parameterdict, echo_input);
        A[js].sim_time = 0.0;
    }   /* end for */
    Gas_model *gmodel = get_gas_model_ptr();
    int nsp = gmodel->get_number_of_species();

    /*
     * Read and discard solutions until the requested solution
     * time is exceeded.
     */
    strcpy(iname, base_file_name);
    strcat(iname, ".Ls");
    printf("infile       : %s\n", iname);
    if ((infile = fopen(iname, "r")) == NULL) {
        printf("\nCould not open %s; BAILING OUT\n", iname);
        exit(1);
    }   /* end if */
    found_solution = 0;
    for (i = 1; i <= max_sol; ++i) {
        printf(".");
        fflush(stdout);
        for (jp = 0; jp < SD.npiston; ++jp)
            read_piston_solution(&(Pist[jp]), infile);
        for (jd = 0; jd < SD.ndiaphragm; ++jd)
            read_diaphragm_solution(&(Diaph[jd]), infile);
        for (js = 0; js < SD.nslug; ++js)
            L_read_solution(&(A[js]), infile);
        if (A[0].sim_time >= tstop) {
            found_solution = 1;
            break;
        }   /* end if */
    }   /* end for i ... */
    printf("\n");
    if (infile != NULL)
        fclose(infile);

    /*
     * If we have found the appropriate solution, write it
     * out in the requested format.
     */
    if (found_solution == 1) {

        if (output_format == PLOT) {
            /* Generic plot format. */
            if (ilogr == 1) take_log_rho(A, SD.nslug);
            if (ilogp == 1) take_log_p(A, SD.nslug);
            strcpy(oname, base_file_name);
            strcat(oname, ".gen");
            printf("outfile      : %s\n", oname);
            if ((outfile = fopen(oname, "w")) == NULL) {
                printf("\nCould not open %s; BAILING OUT\n", oname);
                exit(-1);
            }
            L_write_GENERIC(A, SD.nslug, outfile, nsp);
            if (outfile != NULL)
                fclose(outfile);
        } else if (output_format == RESTART) {
            /* Assume Raw data format -- suitable for a restart. */
            strcpy(oname, base_file_name);
            strcat(oname, ".Lsave");
            printf("outfile      : %s\n", oname);
            if ((outfile = fopen(oname, "w")) == NULL) {
                printf("\nCould not open %s; BAILING OUT\n", oname);
                exit(-1);
            }
            for (jp = 0; jp < SD.npiston; ++jp)
                write_piston_solution(&(Pist[jp]), outfile);
            for (jd = 0; jd < SD.ndiaphragm; ++jd)
                write_diaphragm_solution(&(Diaph[jd]), outfile);
            for (js = 0; js < SD.nslug; ++js)
                L_write_solution(&(A[js]), outfile);
            if (outfile != NULL)
                fclose(outfile);
        } else {
            /* Write out the solution as individual pieces. */
            for (jp = 0; jp < SD.npiston; ++jp) {
                strcpy(oname, base_file_name);
                sprintf(suffix, ".piston.%d", jp);
                strcat(oname, suffix);
                printf("outfile for piston %d    : %s\n", jp, oname);
                if ((outfile = fopen(oname, "w")) == NULL) {
                    printf("\nCould not open %s; BAILING OUT\n", oname);
                    exit(-1);
                }
                write_piston_solution(&(Pist[jp]), outfile);
                if (outfile != NULL)
                    fclose(outfile);
            } /* end for jp */
            for (jd = 0; jd < SD.ndiaphragm; ++jd) {
                strcpy(oname, base_file_name);
                sprintf(suffix, ".dia.%d", jd);
                strcat(oname, suffix);
                printf("outfile for diaphragm %d : %s\n", jd, oname);
                if ((outfile = fopen(oname, "w")) == NULL) {
                    printf("\nCould not open %s; BAILING OUT\n", oname);
                    exit(-1);
                }
                write_diaphragm_solution(&(Diaph[jd]), outfile);
                if (outfile != NULL)
                    fclose(outfile);
            } /* end for jd */
            for (js = 0; js < SD.nslug; ++js) {
                strcpy(oname, base_file_name);
                sprintf(suffix, ".slug.%d", js);
                strcat(oname, suffix);
                printf("outfile for gas slug %d : %s\n", js, oname);
                if ((outfile = fopen(oname, "w")) == NULL) {
                    printf("\nCould not open %s; BAILING OUT\n", oname);
                    exit(-1);
                }
                L_write_solution(&(A[js]), outfile);
                if (outfile != NULL)
                    fclose(outfile);
            } // end for js
        } /* end if */

    } else {
        printf("No solution found at requested time...\n");
        printf("Latest solution found at t = %e\n", A[0].sim_time);
    }   /* end if */

    return SUCCESS;
} // end function main

/*----------------------------------------------------------------*/

int L_write_GENERIC(std::vector<slug_data> &A, int nslug, FILE *outfile, int nsp )
{
    // Write the flow solution (cell centres only) to a GENERIC plotting file.
#   if (DEBUG >= 1)
    printf("\nWriting a PLOT file at t = %e...\n", A[0].sim_time);
#   endif

    fprintf(outfile, "# %e  <== sim_time\n", A[0].sim_time);
    fprintf(outfile, "# %d         <== number of columns\n", (11 + nsp) );
    fprintf(outfile, "# x,m        <== col_1\n");
    fprintf(outfile, "# area,m2    <== col_2\n");
    fprintf(outfile, "# rho,kg/m3  <== col_3\n");
    fprintf(outfile, "# u,m/s      <== col_4\n");
    fprintf(outfile, "# e,J/kg     <== col_5\n");
    fprintf(outfile, "# p,Pa       <== col_6\n");
    fprintf(outfile, "# a,m/s      <== col_7\n");
    fprintf(outfile, "# T,K        <== col_8\n");
    fprintf(outfile, "# tau0,Pa    <== col_9\n");
    fprintf(outfile, "# q,W/m**2   <== col_10\n");
    fprintf(outfile, "# S2,J/kg/K  <== col_11\n");
    for ( int isp = 0; isp < nsp; ++isp ) {
	fprintf(outfile, "# f[%d]       <== col_%0d\n", isp, (11+isp+1) );
    }

    fprintf(outfile, "# %d          <== number of slugs/segments\n", nslug);

    for ( int js = 0; js < nslug; ++js ) {
        /* Use blank lines to separate blocks for GNU Plot */
        fprintf(outfile, "\n");
        fprintf(outfile, "# %d         <== number of cells, this slug\n",
		A[js].nnx);

        for ( int ix = A[js].ixmin; ix <= A[js].ixmax; ++ix ) {
            fprintf(outfile, "%e %e %e %e %e %e %e %e %e %e %e ",
                    0.5 * (A[js].Cell[ix].x + A[js].Cell[ix - 1].x),
                    0.5 * (A[js].Cell[ix].area + A[js].Cell[ix - 1].area),
                    A[js].Cell[ix].gas->rho, A[js].Cell[ix].u,
                    A[js].Cell[ix].gas->e[0], A[js].Cell[ix].gas->p,
                    A[js].Cell[ix].gas->a, A[js].Cell[ix].gas->T[0],
                    A[js].Cell[ix].shear_stress,
                    A[js].Cell[ix].heat_flux, A[js].Cell[ix].entropy);
	    for ( int isp = 0; isp < nsp; ++isp ) {
		fprintf( outfile, "%e ", A[js].Cell[ix].gas->massf[isp] );
	    }
	    fprintf( outfile, "\n" );
        }   /* end for ix */
    }   /* end for (js... */

    fflush(outfile);
    return SUCCESS;
} // end function L_write_GENERIC


int take_log_rho(std::vector<slug_data> &A, int nslug)
{
    // Take the logarithm of the density values.
    for ( int js = 0; js < nslug; ++js ) {
        for ( int ix = A[js].ixmin; ix <= A[js].ixmax; ++ix ) {
            A[js].Cell[ix].gas->rho = log10(A[js].Cell[ix].gas->rho);
        }
    }
    return SUCCESS;
} // end function take_log_rho


int take_log_p(std::vector<slug_data> &A, int nslug)
{
    // Take the logarithm of the pressure values.
    for ( int js = 0; js < nslug; ++js ) {
        for ( int ix = A[js].ixmin; ix <= A[js].ixmax; ++ix ) {
            A[js].Cell[ix].gas->p = log10(A[js].Cell[ix].gas->p);
        }
    }
    return 0;
} // end function take_log_p

/*=============== end of l_post.cxx ==================*/
