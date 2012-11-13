/**
 * \file Analyzer.h
 * \brief Declaration of the Analyzer class and widely-used types and functions
 * \author Julien Henry
 */

/** \mainpage PAGAI
 *
 * \section intro_sec Usage
 *
 * PAGAI is a prototype of static analyser, working on top of the LLVM compiler infrastructure. 
 * It implements various state-of-the-art analysis techniques by abstract interpretation 
 * and decision procedures, and computes numerical invariants over a program expressed 
 * as an LLVM bitcode file. The tool is open source, and downloadable here : 
 * http://forge.imag.fr/projects/pagai/
 *
 * For a list of all available options :
 * \code
 * pagai -h or pagai --help \n 
 * \endcode
 */

#ifndef ANALYZER_H
#define ANALYZER_H

#include <set>
#include <map>
#include <vector>

#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/CFG.h"


enum Apron_Manager_Type {
	BOX,
	OCT,
	PK,
	PKEQ,
	PPL_POLY,
	PPL_POLY_BAGNARA,
	PPL_GRID,
	PKGRID
};

enum Techniques {
	SIMPLE,
	LOOKAHEAD_WIDENING,
	GUIDED,
	PATH_FOCUSING,
	PATH_FOCUSING_INCR,
	LW_WITH_PF,
	COMBINED_INCR,
	LW_WITH_PF_DISJ,
};

enum SMTSolver {
	MATHSAT,
	Z3,
	Z3_QFNRA,
	SMTINTERPOL,
	CVC3, 
	API_Z3,
	API_YICES
};

std::string TechniquesToString(Techniques t);
enum Techniques TechniqueFromString(bool &error, std::string d);

std::string ApronManagerToString(Apron_Manager_Type D);

SMTSolver getSMTSolver();

Techniques getTechnique();

bool compareTechniques();
std::vector<enum Techniques> * getComparedTechniques();

bool compareDomain();

bool compareNarrowing();

bool onlyOutputsRho();

bool useSourceName();
void set_useSourceName(bool b);

bool OutputAnnotatedFile();
char* getAnnotatedFilename();
char* getSourceFilename();

char* getFilename();

int getTimeout();

bool definedMain();
std::string getMain();

bool quiet_mode();
bool log_smt_into_file();

Apron_Manager_Type getApronManager();
Apron_Manager_Type getApronManager(int i);

bool useNewNarrowing();
bool useNewNarrowing(int i);

bool useThreshold();
bool useThreshold(int i);

extern llvm::raw_ostream *Out;

#endif
