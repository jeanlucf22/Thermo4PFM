// Copyright (c) 2018, Lawrence Livermore National Security, LLC and
// UT-Battelle, LLC.
// Produced at the Lawrence Livermore National Laboratory and
// the Oak Ridge National Laboratory
// Written by M.R. Dorr, J.-L. Fattebert and M.E. Wickett
// LLNL-CODE-747500
// All rights reserved.
// This file is part of AMPE.
// For details, see https://github.com/LLNL/AMPE
// Please also read AMPE/LICENSE.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the disclaimer below.
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the disclaimer (as noted below) in the
//   documentation and/or other materials provided with the distribution.
// - Neither the name of the LLNS/LLNL nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY,
// LLC, UT BATTELLE, LLC,
// THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include "CALPHADFreeEnergyFunctionsTernary.h"
#include "CALPHADFunctions.h"
#include "PhysicalConstants.h"
#include "functions.h"

#include <boost/property_tree/json_parser.hpp>

#include <cmath>
#include <iomanip>
#include <string>

namespace pt = boost::property_tree;

void readLmixBinaryParameters(pt::ptree& Lmix_db, double Lmix[4][2])
{
    {
        int i = 0;
        for (pt::ptree::value_type& v : Lmix_db.get_child("L0"))
        {
            Lmix[0][i] = v.second.get_value<double>();
            std::cout << Lmix[0][i] << std::endl;
            i++;
        }
    }
    {
        int i = 0;
        for (pt::ptree::value_type& v : Lmix_db.get_child("L1"))
        {
            Lmix[1][i] = v.second.get_value<double>();
            std::cout << Lmix[1][i] << std::endl;
            i++;
        }
    }
    {
        auto child = Lmix_db.get_child_optional("L2");
        if (child)
        {
            int i = 0;
            for (pt::ptree::value_type& v : Lmix_db.get_child("L2"))
            {
                Lmix[2][i] = v.second.get_value<double>();
                i++;
            }
        }
        else
        {
            Lmix[2][0] = 0.0;
            Lmix[2][1] = 0.0;
        }
    }
    {
        auto child = Lmix_db.get_child_optional("L3");
        if (child)
        {
            int i = 0;
            for (pt::ptree::value_type& v : Lmix_db.get_child("L3"))
            {
                Lmix[3][i] = v.second.get_value<double>();
                i++;
            }
        }
        else
        {
            Lmix[3][0] = 0.0;
            Lmix[3][1] = 0.0;
        }
    }
}

void readLmixTernaryParameters(pt::ptree& Lmix_db, double LmixABC[3][2])
{
    // L0
    {
        auto child = Lmix_db.get_child_optional("L0");
        if (child)
        {
            int i = 0;
            for (pt::ptree::value_type& v : Lmix_db.get_child("L0"))
            {
                LmixABC[0][i] = v.second.get_value<double>();
                i++;
            }
        }
    }
    // L1
    {
        auto child = Lmix_db.get_child_optional("L1");
        if (child)
        {
            int i = 0;
            for (pt::ptree::value_type& v : Lmix_db.get_child("L1"))
            {
                LmixABC[1][i] = v.second.get_value<double>();
                i++;
            }
        }
    }
    // L2
    {
        auto child = Lmix_db.get_child_optional("L2");
        if (child)
        {
            int i = 0;
            for (pt::ptree::value_type& v : Lmix_db.get_child("L2"))
            {
                LmixABC[2][i] = v.second.get_value<double>();
                i++;
            }
        }
    }
}
CALPHADFreeEnergyFunctionsTernary::CALPHADFreeEnergyFunctionsTernary(
    pt::ptree& calphad_db, boost::optional<pt::ptree&> newton_db,
    const EnergyInterpolationType energy_interp_func_type,
    const ConcInterpolationType conc_interp_func_type)
    : d_energy_interp_func_type(energy_interp_func_type),
      d_conc_interp_func_type(conc_interp_func_type)
{
    d_fA[0] = std::nan("");
    d_fA[1] = std::nan("");
    d_fB[0] = std::nan("");
    d_fB[1] = std::nan("");
    d_fC[0] = std::nan("");
    d_fC[1] = std::nan("");

    d_L_AB_L[0] = std::nan("");
    d_L_AB_L[1] = std::nan("");
    d_L_AC_L[0] = std::nan("");
    d_L_AC_L[2] = std::nan("");
    d_L_BC_L[0] = std::nan("");

    d_L_AB_S[0] = std::nan("");
    d_L_BC_S[0] = std::nan("");
    d_L_BC_S[3] = std::nan("");

    d_L_ABC_L[0] = std::nan("");
    d_L_ABC_L[1] = std::nan("");
    d_L_ABC_L[2] = std::nan("");
    d_L_ABC_S[0] = std::nan("");
    d_L_ABC_S[1] = std::nan("");
    d_L_ABC_S[2] = std::nan("");

    d_fenergy_diag_filename = "energy.vtk";

    d_ceq_l[0] = -1;
    d_ceq_l[1] = -1;
    d_ceq_s[0] = -1;
    d_ceq_s[1] = -1;

    readParameters(calphad_db);

    setupSolver(newton_db);
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::setupSolver(
    boost::optional<pt::ptree&> newton_db)
{
    std::clog << "CALPHADFreeEnergyFunctionsTernary::setupSolver()..."
              << std::endl;
    d_solver = new CALPHADConcentrationSolverTernary();

    if (newton_db) readNewtonparameters(newton_db.get());
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::readNewtonparameters(
    pt::ptree& newton_db)
{
    double tol         = newton_db.get<double>("tol", 1.e-8);
    double alpha       = newton_db.get<double>("alpha", 1.);
    int maxits         = newton_db.get<int>("max_its", 20);
    const bool verbose = newton_db.get<bool>("verbose", false);

    d_solver->SetTolerance(tol);
    d_solver->SetMaxIterations(maxits);
    d_solver->SetDamping(alpha);
    d_solver->SetVerbose(verbose);
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::readParameters(pt::ptree& calphad_db)
{
    pt::ptree& species0_db = calphad_db.get_child("SpeciesA");
    std::string name       = species0_db.get<std::string>("name", "unknown");
    std::string dbnameL("PhaseL");
    d_g_species_phaseL[0].initialize(name, species0_db.get_child(dbnameL));
    std::string dbnameA("PhaseA");
    d_g_species_phaseA[0].initialize(name, species0_db.get_child(dbnameA));

    pt::ptree& speciesB_db = calphad_db.get_child("SpeciesB");
    name                   = speciesB_db.get<std::string>("name", "unknown");
    d_g_species_phaseL[1].initialize(name, speciesB_db.get_child(dbnameL));
    d_g_species_phaseA[1].initialize(name, speciesB_db.get_child(dbnameA));

    pt::ptree& speciesC_db = calphad_db.get_child("SpeciesC");
    name                   = speciesC_db.get<std::string>("name", "unknown");
    d_g_species_phaseL[2].initialize(name, speciesC_db.get_child(dbnameL));
    d_g_species_phaseA[2].initialize(name, speciesC_db.get_child(dbnameA));

    // read Lmix coefficients

    // AB
    {
        pt::ptree& Lmix0_db = calphad_db.get_child("LmixABPhaseL");
        readLmixBinaryParameters(Lmix0_db, d_LmixABPhaseL);

        pt::ptree& Lmix1_db = calphad_db.get_child("LmixABPhaseA");
        readLmixBinaryParameters(Lmix1_db, d_LmixABPhaseA);
    }

    // AC
    {
        pt::ptree& Lmix0_db = calphad_db.get_child("LmixACPhaseL");
        readLmixBinaryParameters(Lmix0_db, d_LmixACPhaseL);

        pt::ptree& Lmix1_db = calphad_db.get_child("LmixACPhaseA");
        readLmixBinaryParameters(Lmix1_db, d_LmixACPhaseA);
    }

    // BC
    {
        pt::ptree& Lmix0_db = calphad_db.get_child("LmixBCPhaseL");
        readLmixBinaryParameters(Lmix0_db, d_LmixBCPhaseL);

        pt::ptree& Lmix1_db = calphad_db.get_child("LmixBCPhaseA");
        readLmixBinaryParameters(Lmix1_db, d_LmixBCPhaseA);
    }

    // ABC
    {
        // default values
        d_LmixABCPhaseL[0][0] = 0.0;
        d_LmixABCPhaseL[0][1] = 0.0;
        d_LmixABCPhaseL[1][0] = 0.0;
        d_LmixABCPhaseL[1][1] = 0.0;
        d_LmixABCPhaseL[2][0] = 0.0;
        d_LmixABCPhaseL[2][1] = 0.0;

        std::string dbnamemixL("LmixABCPhaseL");
        if (calphad_db.get_child_optional(dbnamemixL))
        {
            pt::ptree& Lmix0_db = calphad_db.get_child(dbnamemixL);
            readLmixTernaryParameters(Lmix0_db, d_LmixABCPhaseL);
        }

        assert(d_LmixABCPhaseL[0][0] == d_LmixABCPhaseL[0][0]);
        assert(d_LmixABCPhaseL[0][1] == d_LmixABCPhaseL[0][1]);
        assert(d_LmixABCPhaseL[1][0] == d_LmixABCPhaseL[1][0]);
        assert(d_LmixABCPhaseL[1][1] == d_LmixABCPhaseL[1][1]);
        assert(d_LmixABCPhaseL[2][0] == d_LmixABCPhaseL[2][0]);
        assert(d_LmixABCPhaseL[2][1] == d_LmixABCPhaseL[2][1]);

        std::string dbnamemixA("LmixABCPhaseA");
        // default values
        d_LmixABCPhaseA[0][0] = 0.0;
        d_LmixABCPhaseA[0][1] = 0.0;
        d_LmixABCPhaseA[1][0] = 0.0;
        d_LmixABCPhaseA[1][1] = 0.0;
        d_LmixABCPhaseA[2][0] = 0.0;
        d_LmixABCPhaseA[2][1] = 0.0;
        if (calphad_db.get_child_optional(dbnamemixA))
        {
            pt::ptree& Lmix1_db = calphad_db.get_child(dbnamemixA);
            readLmixTernaryParameters(Lmix1_db, d_LmixABCPhaseA);
        }

        assert(d_LmixABCPhaseA[0][0] == d_LmixABCPhaseA[0][0]);
        assert(d_LmixABCPhaseA[0][1] == d_LmixABCPhaseA[0][1]);
        assert(d_LmixABCPhaseA[1][0] == d_LmixABCPhaseA[1][0]);
        assert(d_LmixABCPhaseA[1][1] == d_LmixABCPhaseA[1][1]);
        assert(d_LmixABCPhaseA[2][0] == d_LmixABCPhaseA[2][0]);
        assert(d_LmixABCPhaseA[2][1] == d_LmixABCPhaseA[2][1]);
    }

    // print database just read
    std::clog << "CALPHAD database..." << std::endl;
    pt::write_json(std::clog, calphad_db);
}

//-----------------------------------------------------------------------

double CALPHADFreeEnergyFunctionsTernary::computeFreeEnergy(
    const double temperature, const double* conc, const PhaseIndex pi,
    const bool gp)
{
    const double conc0 = conc[0];
    const double conc1 = conc[1];

    double lAB[4]
        = { lmix0ABPhase(pi, temperature), lmix1ABPhase(pi, temperature),
              lmix2ABPhase(pi, temperature), lmix3ABPhase(pi, temperature) };
    double lAC[4]
        = { lmix0ACPhase(pi, temperature), lmix1ACPhase(pi, temperature),
              lmix2ACPhase(pi, temperature), lmix3ACPhase(pi, temperature) };
    double lBC[4]
        = { lmix0BCPhase(pi, temperature), lmix1BCPhase(pi, temperature),
              lmix2BCPhase(pi, temperature), lmix3BCPhase(pi, temperature) };

    double lABC[3] = { lmix0ABCPhase(pi, temperature),
        lmix1ABCPhase(pi, temperature), lmix2ABCPhase(pi, temperature) };

    CALPHADSpeciesPhaseGibbsEnergy* g_species;

    switch (pi)
    {
        case PhaseIndex::phaseL:
            g_species = &d_g_species_phaseL[0];
            break;
        case PhaseIndex::phaseA:
            g_species = &d_g_species_phaseA[0];
            break;
        default:
            std::cout << "CALPHADFreeEnergyFunctionsTernary::"
                         "computeFreeEnergy(), undefined phase!!!"
                      << std::endl;
            abort();
            return 0.;
    }

    double conc2 = 1. - conc0 - conc1;
    double fe    = conc0 * g_species[0].fenergy(temperature)
                + conc1 * g_species[1].fenergy(temperature)
                + conc2 * g_species[2].fenergy(temperature)
                + CALPHADcomputeFMixTernary(lAB, lAC, lBC, lABC, conc0, conc1)
                + CALPHADcomputeFIdealMixTernary(
                      gas_constant_R_JpKpmol * temperature, conc0, conc1);

    // subtract -mu*c to get grand potential
    if (gp)
    {
        double deriv[2];
        computeDerivFreeEnergy(temperature, conc, pi, deriv);
        fe -= deriv[0] * conc0;
        fe -= deriv[1] * conc1;
    }

    return fe;
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::computeDerivFreeEnergy(
    const double temperature, const double* const conc, const PhaseIndex pi,
    double* deriv)
{
    double lAB[4]
        = { lmix0ABPhase(pi, temperature), lmix1ABPhase(pi, temperature),
              lmix2ABPhase(pi, temperature), lmix3ABPhase(pi, temperature) };
    double lAC[4]
        = { lmix0ACPhase(pi, temperature), lmix1ACPhase(pi, temperature),
              lmix2ACPhase(pi, temperature), lmix3ACPhase(pi, temperature) };
    double lBC[4]
        = { lmix0BCPhase(pi, temperature), lmix1BCPhase(pi, temperature),
              lmix2BCPhase(pi, temperature), lmix3BCPhase(pi, temperature) };
    double lABC[3] = { lmix0ABCPhase(pi, temperature),
        lmix1ABCPhase(pi, temperature), lmix2ABCPhase(pi, temperature) };

    CALPHADSpeciesPhaseGibbsEnergy* g_species;

    switch (pi)
    {
        case PhaseIndex::phaseL:
            g_species = &d_g_species_phaseL[0];
            break;
        case PhaseIndex::phaseA:
            g_species = &d_g_species_phaseA[0];
            break;
        default:
            std::cout << "CALPHADFreeEnergyFunctionsTernary::"
                         "computeFreeEnergy(), undefined phase!!!"
                      << std::endl;
            abort();
            return;
    }

    CALPHADcomputeFMix_derivTernary(
        lAB, lAC, lBC, lABC, conc[0], conc[1], deriv);

    deriv[0] += g_species[0].fenergy(temperature);
    deriv[0] -= g_species[2].fenergy(temperature);

    deriv[1] += g_species[1].fenergy(temperature);
    deriv[1] -= g_species[2].fenergy(temperature);

    double tmp[2];
    CALPHADcomputeFIdealMix_derivTernary(
        gas_constant_R_JpKpmol * temperature, conc[0], conc[1], tmp);
    deriv[0] += tmp[0];
    deriv[1] += tmp[1];
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::computeSecondDerivativeFreeEnergy(
    const double temp, const double* const conc, const PhaseIndex pi,
    std::vector<double>& d2fdc2)
{
    assert(conc[0] >= 0.);
    assert(conc[0] <= 1.);
    assert(conc[1] >= 0.);
    assert(conc[1] <= 1.);

    double lAB[4]   = { lmix0ABPhase(pi, temp), lmix1ABPhase(pi, temp),
        lmix2ABPhase(pi, temp), lmix3ABPhase(pi, temp) };
    double lAC[4]   = { lmix0ACPhase(pi, temp), lmix1ACPhase(pi, temp),
        lmix2ACPhase(pi, temp), lmix3ACPhase(pi, temp) };
    double lBC[4]   = { lmix0BCPhase(pi, temp), lmix1BCPhase(pi, temp),
        lmix2BCPhase(pi, temp), lmix3BCPhase(pi, temp) };
    double lABC[3]  = { lmix0ABCPhase(pi, temp), lmix1ABCPhase(pi, temp),
        lmix2ABCPhase(pi, temp) };
    const double rt = gas_constant_R_JpKpmol * temp;

    double deriv1[4];
    CALPHADcomputeFIdealMix_deriv2Ternary(rt, conc[0], conc[1], &deriv1[0]);

    double deriv2[4];
    CALPHADcomputeFMix_deriv2Ternary(
        lAB, lAC, lBC, lABC, conc[0], conc[1], &deriv2[0]);

    d2fdc2[0] = deriv1[0] + deriv2[0];
    d2fdc2[1] = deriv1[1] + deriv2[1];
    d2fdc2[2] = deriv1[2] + deriv2[2];
    d2fdc2[3] = deriv1[3] + deriv2[3];
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::setupValuesL(const double temperature)
{
    d_L_AB_L[0]  = lmix0ABPhaseL(temperature);
    d_L_AB_L[1]  = lmix1ABPhaseL(temperature);
    d_L_AB_L[2]  = lmix2ABPhaseL(temperature);
    d_L_AB_L[3]  = lmix3ABPhaseL(temperature);
    d_L_AC_L[0]  = lmix0ACPhaseL(temperature);
    d_L_AC_L[1]  = lmix1ACPhaseL(temperature);
    d_L_AC_L[2]  = lmix2ACPhaseL(temperature);
    d_L_AC_L[3]  = lmix3ACPhaseL(temperature);
    d_L_BC_L[0]  = lmix0BCPhaseL(temperature);
    d_L_BC_L[1]  = lmix1BCPhaseL(temperature);
    d_L_BC_L[2]  = lmix2BCPhaseL(temperature);
    d_L_BC_L[3]  = lmix3BCPhaseL(temperature);
    d_L_ABC_L[0] = lmix0ABCPhaseL(temperature);
    d_L_ABC_L[1] = lmix1ABCPhaseL(temperature);
    d_L_ABC_L[2] = lmix2ABCPhaseL(temperature);
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::setupValuesS(const double temperature)
{
    d_L_AB_S[0]  = lmix0ABPhaseA(temperature);
    d_L_AB_S[1]  = lmix1ABPhaseA(temperature);
    d_L_AB_S[2]  = lmix2ABPhaseA(temperature);
    d_L_AB_S[3]  = lmix3ABPhaseA(temperature);
    d_L_AC_S[0]  = lmix0ACPhaseA(temperature);
    d_L_AC_S[1]  = lmix1ACPhaseA(temperature);
    d_L_AC_S[2]  = lmix2ACPhaseA(temperature);
    d_L_AC_S[3]  = lmix3ACPhaseA(temperature);
    d_L_BC_S[0]  = lmix0BCPhaseA(temperature);
    d_L_BC_S[1]  = lmix1BCPhaseA(temperature);
    d_L_BC_S[2]  = lmix2BCPhaseA(temperature);
    d_L_BC_S[3]  = lmix3BCPhaseA(temperature);
    d_L_ABC_S[0] = lmix0ABCPhaseA(temperature);
    d_L_ABC_S[1] = lmix1ABCPhaseA(temperature);
    d_L_ABC_S[2] = lmix2ABCPhaseA(temperature);
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::setupValuesForTwoPhasesSolver(
    const double temperature, const PhaseIndex pi0, const PhaseIndex pi1)
{
    PhaseIndex pis[2] = { pi0, pi1 };

    for (short i = 0; i < 2; i++)
    {
        switch (pis[i])
        {

            case PhaseIndex::phaseL:
                d_fA[i] = d_g_species_phaseL[0].fenergy(temperature);
                d_fB[i] = d_g_species_phaseL[1].fenergy(temperature);
                d_fC[i] = d_g_species_phaseL[2].fenergy(temperature);

                setupValuesL(temperature);

                break;

            case PhaseIndex::phaseA:
                d_fA[i] = d_g_species_phaseA[0].fenergy(temperature);
                d_fB[i] = d_g_species_phaseA[1].fenergy(temperature);
                d_fC[i] = d_g_species_phaseA[2].fenergy(temperature);

                setupValuesS(temperature);

                break;

            default:
                std::cerr << "CALPHADFreeEnergyFunctionsTernary::"
                             "setupValuesForTwoPhasesSolver: Undefined phase"
                          << std::endl;
        }
    }
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::setup(const double temperature)
{
    d_fA[0] = d_g_species_phaseL[0].fenergy(temperature);
    d_fA[1] = d_g_species_phaseA[0].fenergy(temperature);

    d_fB[0] = d_g_species_phaseL[1].fenergy(temperature);
    d_fB[1] = d_g_species_phaseA[1].fenergy(temperature);

    d_fC[0] = d_g_species_phaseL[2].fenergy(temperature);
    d_fC[1] = d_g_species_phaseA[2].fenergy(temperature);

    setupValuesL(temperature);
    setupValuesS(temperature);
}

//=======================================================================

// compute equilibrium concentrations in various phases for given temperature
bool CALPHADFreeEnergyFunctionsTernary::computeCeqT(const double temperature,
    const PhaseIndex pi0, const PhaseIndex pi1, double* ceq, const int maxits,
    const bool verbose)
{
    if (verbose)
        std::cout << "CALPHADFreeEnergyFunctionsTernary::computeCeqT()"
                  << std::endl;
    assert(temperature > 0.);

    setupValuesForTwoPhasesSolver(temperature, pi0, pi1);

    assert(d_L_ABC_L[0] == d_L_ABC_L[0]);

    double RTinv = 1.0 / (gas_constant_R_JpKpmol * temperature);
    CALPHADEqConcentrationSolverTernary eq_solver;
    eq_solver.SetMaxIterations(maxits);

    int ret = eq_solver.ComputeConcentration(ceq, RTinv, d_L_AB_L, d_L_AC_L,
        d_L_BC_L, d_L_AB_S, d_L_AC_S, d_L_BC_S, d_L_ABC_L, d_L_ABC_S, d_fA,
        d_fB, d_fC);

    if (ret >= 0)
    {
        if (verbose)
        {
            std::cout << "CALPHAD, c0 phase0=" << ceq[0] << std::endl;
            std::cout << "CALPHAD, c1 phase0=" << ceq[1] << std::endl;
            std::cout << "CALPHAD, c0 phase1=" << ceq[2] << std::endl;
            std::cout << "CALPHAD, c1 phase1=" << ceq[3] << std::endl;
        }

        d_ceq_l[0] = ceq[0];
        d_ceq_l[1] = ceq[1];
        d_ceq_s[0] = ceq[2];
        d_ceq_s[1] = ceq[3];
    }
    else
    {
        std::cout << "CALPHADFreeEnergyFunctionsTernary, WARNING: ceq "
                     "computation did not converge"
                  << std::endl;
    }

    return (ret >= 0);
}

//=======================================================================

bool CALPHADFreeEnergyFunctionsTernary::computeCeqT(const double temperature,
    const PhaseIndex pi0, const PhaseIndex pi1, const double c0,
    const double c1, double* ceq, const int maxits, const bool verbose)
{
    assert(temperature > 0.);

    setupValuesForTwoPhasesSolver(temperature, pi0, pi1);

    double RTinv = 1.0 / (gas_constant_R_JpKpmol * temperature);
    CALPHADEqPhaseConcentrationSolverTernary eq_solver(c0, c1);
    eq_solver.SetMaxIterations(maxits);

    int ret = eq_solver.ComputeConcentration(ceq, RTinv, d_L_AB_L, d_L_AC_L,
        d_L_BC_L, d_L_AB_S, d_L_AC_S, d_L_BC_S, d_L_ABC_L, d_L_ABC_S, d_fA,
        d_fB, d_fC);

    if (ret >= 0)
    {
        if (verbose)
        {
            std::cout << "CALPHAD, c0 phase0=" << ceq[0] << std::endl;
            std::cout << "CALPHAD, c1 phase0=" << ceq[1] << std::endl;
            std::cout << "CALPHAD, c0 phase1=" << ceq[2] << std::endl;
            std::cout << "CALPHAD, c1 phase1=" << ceq[3] << std::endl;
        }

        d_ceq_l[0] = ceq[0];
        d_ceq_l[1] = ceq[1];
        d_ceq_s[0] = ceq[2];
        d_ceq_s[1] = ceq[3];
    }
    else
    {
        std::cout << "CALPHADFreeEnergyFunctionsTernary, WARNING: ceq "
                     "computation did not converge"
                  << std::endl;
    }

    return (ret >= 0);
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::computePhasesFreeEnergies(
    const double temperature, const double hphi, const double conc0,
    const double conc1, double& fl, double& fa)
{
    // std::cout<<"CALPHADFreeEnergyFunctionsTernary::computePhasesFreeEnergies()"<<endl;

    double cauxilliary[4] = { conc0, conc1, conc0, conc1 };

    // std::cout<<"d_ceq_l="<<d_ceq_l<<endl;
    // std::cout<<"d_ceq_a="<<d_ceq_a<<endl;
    if (d_ceq_l[0] >= 0.) cauxilliary[0] = d_ceq_l[0];
    if (d_ceq_l[1] >= 0.) cauxilliary[1] = d_ceq_l[1];
    if (d_ceq_s[0] >= 0.) cauxilliary[2] = d_ceq_s[0];
    if (d_ceq_s[1] >= 0.) cauxilliary[3] = d_ceq_s[1];

    setup(temperature);

    assert(d_fC[0] == d_fC[0]);

    double RTinv = 1.0 / (gas_constant_R_JpKpmol * temperature);
    int ret = d_solver->ComputeConcentration(cauxilliary, conc0, conc1, hphi,
        RTinv, d_L_AB_L, d_L_AC_L, d_L_BC_L, d_L_AB_S, d_L_AC_S, d_L_BC_S,
        d_L_ABC_L, d_L_ABC_S, d_fA, d_fB, d_fC);

    if (ret < 0)
    {
        std::cerr << "ERROR in "
                     "CALPHADFreeEnergyFunctionsTernary::"
                     "computePhasesFreeEnergies() "
                     "---"
                  << "conc0=" << conc0 << ", conc1=" << conc1
                  << ", hphi=" << hphi << std::endl;
        abort();
    }

    assert(conc0 >= 0.);
    double concl[2] = { cauxilliary[0], cauxilliary[1] };
    fl = computeFreeEnergy(temperature, &concl[0], PhaseIndex::phaseL, false);

    assert(conc1 >= 0.);
    double conca[2] = { cauxilliary[2], cauxilliary[3] };
    fa = computeFreeEnergy(temperature, &conca[0], PhaseIndex::phaseA, false);
}

//-----------------------------------------------------------------------
// output: x
int CALPHADFreeEnergyFunctionsTernary::computePhaseConcentrations(
    const double temperature, const double* const conc, const double phi,
    const double eta, double* x)

{
    (void)eta;

    assert(conc[0] == conc[0]);
    assert(conc[1] == conc[1]);
    assert(x[0] >= 0.);
    assert(x[1] >= 0.);
    assert(x[0] <= 1.);
    assert(x[1] <= 1.);

    const double conc0 = conc[0];
    const double conc1 = conc[1];

    const double RTinv = 1.0 / (gas_constant_R_JpKpmol * temperature);

    d_fA[0] = getFenergyPhaseL(0, temperature);
    d_fA[1] = getFenergyPhaseA(0, temperature);

    d_fB[0] = getFenergyPhaseL(1, temperature);
    d_fB[1] = getFenergyPhaseA(1, temperature);

    d_fC[0] = getFenergyPhaseL(2, temperature);
    d_fC[1] = getFenergyPhaseA(2, temperature);
    assert(d_fC[0] == d_fC[0]);

    d_L_AB_L[0] = lmix0ABPhaseL(temperature);
    d_L_AB_L[1] = lmix1ABPhaseL(temperature);
    d_L_AB_L[2] = lmix2ABPhaseL(temperature);
    d_L_AB_L[3] = lmix3ABPhaseL(temperature);

    d_L_AB_S[0] = lmix0ABPhaseA(temperature);
    d_L_AB_S[1] = lmix1ABPhaseA(temperature);
    d_L_AB_S[2] = lmix2ABPhaseA(temperature);
    d_L_AB_S[3] = lmix3ABPhaseA(temperature);

    d_L_AC_L[0] = lmix0ACPhaseL(temperature);
    d_L_AC_L[1] = lmix1ACPhaseL(temperature);
    d_L_AC_L[2] = lmix2ACPhaseL(temperature);
    d_L_AC_L[3] = lmix3ACPhaseL(temperature);

    d_L_AC_S[0] = lmix0ACPhaseA(temperature);
    d_L_AC_S[1] = lmix1ACPhaseA(temperature);
    d_L_AC_S[2] = lmix2ACPhaseA(temperature);
    d_L_AC_S[3] = lmix3ACPhaseA(temperature);

    d_L_BC_L[0] = lmix0BCPhaseL(temperature);
    d_L_BC_L[1] = lmix1BCPhaseL(temperature);
    d_L_BC_L[2] = lmix2BCPhaseL(temperature);
    d_L_BC_L[3] = lmix3BCPhaseL(temperature);

    d_L_BC_S[0] = lmix0BCPhaseA(temperature);
    d_L_BC_S[1] = lmix1BCPhaseA(temperature);
    d_L_BC_S[2] = lmix2BCPhaseA(temperature);
    d_L_BC_S[3] = lmix3BCPhaseA(temperature);

    d_L_ABC_L[0] = lmix0ABCPhaseL(temperature);
    d_L_ABC_L[1] = lmix1ABCPhaseL(temperature);
    d_L_ABC_L[2] = lmix2ABCPhaseL(temperature);

    d_L_ABC_S[0] = lmix0ABCPhaseA(temperature);
    d_L_ABC_S[1] = lmix1ABCPhaseA(temperature);
    d_L_ABC_S[2] = lmix2ABCPhaseA(temperature);

    const char interp_func_type = concInterpChar(d_conc_interp_func_type);
    const double hphi           = interp_func(phi, interp_func_type);

    double heta = 0.0;
    // std::cout<<"d_ceq_a="<<d_ceq_a<<endl;
    // x[0] = ( d_ceq_l>=0. ) ? d_ceq_l : 0.5;
    // x[1] = ( d_ceq_a>=0. ) ? d_ceq_a : 0.5;

    // conc could be outside of [0.,1.] in a trial step
    double c0 = conc0 >= 0. ? conc0 : 0.;
    c0        = c0 <= 1. ? c0 : 1.;
    double c1 = conc1 >= 0. ? conc1 : 0.;
    c1        = c1 <= 1. ? c1 : 1.;

    int ret = d_solver->ComputeConcentration(x, c0, c1, hphi, RTinv, d_L_AB_L,
        d_L_AC_L, d_L_BC_L, d_L_AB_S, d_L_AC_S, d_L_BC_S, d_L_ABC_L, d_L_ABC_S,
        d_fA, d_fB, d_fC);
    if (ret == -1)
    {
        std::cerr << "ERROR, "
                     "CALPHADFreeEnergyFunctionsTernary::"
                     "computePhaseConcentrations() "
                     "failed for conc0="
                  << conc0 << ", conc1=" << conc1 << ", hphi=" << hphi
                  << ", heta=" << heta << std::endl;
        abort();
    }

    return ret;
}

//-----------------------------------------------------------------------

void CALPHADFreeEnergyFunctionsTernary::energyVsPhiAndC(
    const double temperature, const double* const ceq, const bool found_ceq,
    const double phi_well_scale, const int npts_phi, const int npts_c)
{
    std::clog << "CALPHADFreeEnergyFunctionsTernary::energyVsPhiAndC()..."
              << std::endl;

    const double* const ceqL = &ceq[0];
    const double* const ceqS = &ceq[2];
    // std::clog<<"Input:
    // "<<ceq[0]<<","<<ceq[1]<<","<<ceq[2]<<","<<ceq[3]<<endl;

    double slopec = 0.;
    double fc0    = 0.;
    double fc1    = 0.;
    if (found_ceq)
    {
        // compute slope of f between equilibrium concentrations
        // to add slopec*conc to energy later on

        fc0    = computeFreeEnergy(temperature, &ceqL[0], PhaseIndex::phaseL);
        fc1    = computeFreeEnergy(temperature, &ceqS[0], PhaseIndex::phaseA);
        slopec = -(fc1 - fc0) / (ceqL[1] - ceqL[0]);
    }
    std::clog << std::setprecision(8) << "fc0: " << fc0 << "..."
              << ", fc1: " << fc1 << "..." << std::endl;
    std::clog << "CALPHADFreeEnergyFunctionsTernary: Use slope: " << slopec
              << "..." << std::endl;

    {
        // reset cmin, cmax, deltac
        double c0min = std::min(ceqL[0], ceqS[0]);
        double c0max = std::max(ceqL[0], ceqS[0]);

        double dc0     = c0max - c0min;
        c0min          = std::max(0.25 * c0min, c0min - 0.25 * dc0);
        c0max          = std::min(1. - 0.25 * (1. - c0max), c0max + 0.25 * dc0);
        c0max          = std::max(c0max, c0min + dc0);
        double deltac0 = (c0max - c0min) / (npts_c - 1);

        double c1min = std::min(ceqL[1], ceqS[1]);
        double c1max = std::max(ceqL[1], ceqS[1]);

        double dc1     = c1max - c1min;
        c1min          = std::max(0.25 * c1min, c1min - 0.25 * dc1);
        c1max          = std::min(1. - 0.25 * (1. - c1max), c1max + 0.25 * dc1);
        c1max          = std::max(c1max, c1min + dc1);
        double deltac1 = (c1max - c1min) / (npts_c - 1);

        std::clog << "Range for c0: " << c0min << " to " << c0max << std::endl;
        std::clog << "Range for c1: " << c1min << " to " << c1max << std::endl;

        std::ofstream tfile(d_fenergy_diag_filename.data(), std::ios::out);

        printEnergyVsPhiHeader(temperature, npts_phi, npts_c, npts_c, c0min,
            c0max, c1min, c1max, tfile);

        for (int i0 = 0; i0 < npts_c; i0++)
        {
            int i1    = (1. - c0min - deltac0 * i0 - c1min) / deltac1;
            int i1max = i1 < npts_c ? i1 : npts_c;
            for (int i1 = 0; i1 < i1max; i1++)
            {
                double c[2] = { c0min + deltac0 * i0, c1min + deltac1 * i1 };
                printEnergyVsPhi(
                    c, temperature, phi_well_scale, npts_phi, tfile);
            }
        }
    }
}

// Print out free energy as a function of phase
// for given composition and temperature
// File format: ASCII VTK, readble with Visit
void CALPHADFreeEnergyFunctionsTernary::printEnergyVsPhiHeader(
    const double temperature, const int nphi, const int nc0, const int nc1,
    const double c0min, const double c0max, const double c1min,
    const double c1max, std::ostream& os) const
{
    os << "# vtk DataFile Version 2.0" << std::endl;
    os << "Free energy [J/mol] at T=" << temperature << std::endl;
    os << "ASCII" << std::endl;
    os << "DATASET STRUCTURED_POINTS" << std::endl;

    os << "DIMENSIONS   " << nphi << " " << nc0 << " " << nc1 << std::endl;
    double asp_ratio_c0 = (nc0 > 1) ? (c0max - c0min) / (nc0 - 1) : 1.;
    double asp_ratio_c1 = (nc1 > 1) ? (c1max - c1min) / (nc1 - 1) : 1.;
    os << "ASPECT_RATIO " << 1. / (nphi - 1) << " " << asp_ratio_c0 << " "
       << asp_ratio_c1 << std::endl;
    os << "ORIGIN        0. " << c0min << " " << c1min << std::endl;
    os << "POINT_DATA   " << nphi * nc0 * nc1 << std::endl;
    os << "SCALARS energy float 1" << std::endl;
    os << "LOOKUP_TABLE default" << std::endl;
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::printEnergyVsPhi(const double* conc,
    const double temperature, const double phi_well_scale, const int npts,
    std::ostream& os)
{
    // std::cout << "CALPHADFreeEnergyFunctionsTernary::printEnergyVsPhi()..."
    // << std::endl;
    const double dphi = 1.0 / (double)(npts - 1);
    const double eta  = 0.0;

    // os << "# phi     f(phi)     for c=" << conc
    //           << " eta=" << eta
    //           << " and T=" << temperature << std::endl;
    for (int i = 0; i < npts; i++)
    {
        const double phi = i * dphi;

        double e       = fchem(phi, eta, conc, temperature);
        const double w = phi_well_scale * well_func(phi);

        os << e + w << std::endl;
    }
    // os << std::endl;
}

//=======================================================================
// compute free energy in [J/mol]
double CALPHADFreeEnergyFunctionsTernary::fchem(const double phi,
    const double eta, const double* const conc, const double temperature)
{
    const double conc0 = conc[0];
    const double conc1 = conc[1];

    (void)eta;

    const char interp_func_type = concInterpChar(d_conc_interp_func_type);
    const double hcphi          = interp_func(phi, interp_func_type);

    const double tol = 1.e-8;
    double fl        = 0.;
    double fa        = 0.;
    if ((phi > tol) & (phi < 1. - tol))
    {
        computePhasesFreeEnergies(temperature, hcphi, conc0, conc1, fl, fa);
    }
    else
    {
        // don't solve for phases concentrations, just compute energy
        // in either phase
        double conc[2] = { conc0, conc1 };
        if (phi <= tol)
        {
            fl = computeFreeEnergy(temperature, &conc[0], PhaseIndex::phaseL);
        }
        else
        {
            fa = computeFreeEnergy(temperature, &conc[0], PhaseIndex::phaseA);
        }
    }

    const char interpf = energyInterpChar(d_energy_interp_func_type);
    const double hfphi = interp_func(phi, interpf);

    return (1.0 - hfphi) * fl + hfphi * fa;
}

//=======================================================================

void CALPHADFreeEnergyFunctionsTernary::printEnergyVsComposition(
    const double temperature, const int npts)
{
    const double dc = 1.0 / (double)(npts - 1);

    std::string filename1("Fl");
    filename1 += d_g_species_phaseL[0].name();
    filename1 += d_g_species_phaseL[2].name();
    filename1 += ".dat";
    std::ofstream os1(filename1.c_str(), std::ios::out);
    os1 << "#phi=0, c1=0, temperature=" << temperature << std::endl;
    for (int i = 0; i < npts; i++)
    {
        double conc[2];
        conc[0] = i * dc;
        conc[1] = 0.;

        double e = fchem(0., 0., conc, temperature);
        os1 << conc[0] << "\t" << e << std::endl;
    }
    os1 << std::endl;

    std::string filename2("Fs");
    filename2 += d_g_species_phaseA[0].name();
    filename2 += d_g_species_phaseA[2].name();
    filename2 += ".dat";
    std::ofstream os2(filename2.c_str(), std::ios::out);
    os2 << "#phi=1, c1=0, temperature=" << temperature << std::endl;
    for (int i = 0; i < npts; i++)
    {
        double conc[2];
        conc[0] = i * dc;
        conc[1] = 0.;

        double e = fchem(1., 0., conc, temperature);
        os2 << conc[0] << "\t" << e << std::endl;
    }
    os2 << std::endl;

    std::string filename3("Fl");
    filename3 += d_g_species_phaseL[1].name();
    filename3 += d_g_species_phaseL[2].name();
    filename3 += ".dat";
    std::ofstream os3(filename3.c_str(), std::ios::out);
    os3 << "#phi=0, c0=0, temperature=" << temperature << std::endl;
    for (int i = 0; i < npts; i++)
    {
        double conc[2];
        conc[0] = 0.;
        conc[1] = i * dc;

        double e = fchem(0., 0., conc, temperature);
        os3 << conc[1] << "\t" << e << std::endl;
    }
    os3 << std::endl;

    std::string filename4("Fs");
    filename4 += d_g_species_phaseA[1].name();
    filename4 += d_g_species_phaseA[2].name();
    filename4 += ".dat";
    std::ofstream os4(filename4.c_str(), std::ios::out);
    os4 << "#phi=1, c0=0, temperature=" << temperature << std::endl;
    for (int i = 0; i < npts; i++)
    {
        double conc[2];
        conc[0] = 0.;
        conc[1] = i * dc;

        double e = fchem(1., 0., conc, temperature);
        os4 << conc[1] << "\t" << e << std::endl;
    }
    os4 << std::endl;

    std::string filename5("Fl");
    filename5 += d_g_species_phaseL[0].name();
    filename5 += d_g_species_phaseL[1].name();
    filename5 += ".dat";
    std::ofstream os5(filename5.c_str(), std::ios::out);
    os5 << "#phi=0, temperature=" << temperature << std::endl;
    for (int i = 0; i < npts; i++)
    {
        double conc[2];
        conc[0] = i * dc;
        conc[1] = 1. - i * dc;

        double e = fchem(0., 0., conc, temperature);
        os5 << conc[0] << "\t" << e << std::endl;
    }
    os5 << std::endl;

    std::string filename6("Fs");
    filename6 += d_g_species_phaseA[0].name();
    filename6 += d_g_species_phaseA[1].name();
    filename6 += ".dat";
    std::ofstream os6(filename6.c_str(), std::ios::out);
    os6 << "#phi=1, temperature=" << temperature << std::endl;
    for (int i = 0; i < npts; i++)
    {
        double conc[2];
        conc[0] = i * dc;
        conc[1] = 1. - i * dc;

        double e = fchem(1., 0., conc, temperature);
        os6 << conc[0] << "\t" << e << std::endl;
    }
    os6 << std::endl;
}