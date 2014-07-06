/**
 * \file Analyzer.cc
 * \brief Implementation of the Analyzer class (main class)
 * \author Julien Henry
 */
#include <string>
#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <boost/lexical_cast.hpp>

#include "Execute.h" 
#include "Analyzer.h" 
#include "Debug.h" 

SMTSolver Solver;
Techniques technique;
bool compare;
bool compare_Domain;
bool compare_Narrowing;
bool onlyrho;
bool quiet;
bool bagnara_widening;
bool defined_main;
bool use_source_name;
bool printAll;
bool oflcheck;
bool svcomp;
bool force_old_output;
bool output_annotated;
bool log_smt;
bool skipnonlinear;
bool has_timeout;
bool optimize;
bool withinlining;
bool onlydumpll;
std::string main_function;
Apron_Manager_Type ap_manager[2];
bool Narrowing[2];
bool Threshold[2];
llvm::raw_ostream *Out;
char* filename;
char* annotatedFilename;
std::string annotatedBCFilename;
int npass;
int timeout;
std::map<Techniques,int> Passes;
std::vector<enum Techniques> TechniquesToCompare;

void show_help() {
        std::cout << "Usage :\n \
\tpagai -h OR pagai [options]  -i <filename> \n \
--help (-h) : help\n \
--main <name> (-m) : only analyze the function <name>\n \
--no-undefined-check : do not verify integer overflows\n \
--svcomp : SV-Comp mode\n \
--dump-ll : only dump the analyzed .ll\n \
--optimize (-O) : optimize input with -O3 before analysis\n \
--noinline : do not inline functions\n \
--output-bc <filename> (-b) : create an annotated bc file called <filename>\n \
--force-old-output : force to use the old output\n \
--skipnonlinear : enforce rough abstraction of nonlinear arithmetic\n \
--quiet (-q) : quiet mode : does not print anything \n \
--domain (-d) : abstract domain\n \
         possible abstract domains:\n \
		   * box (Apron boxes)\n \
		   * oct (Octagons)\n \
		   * pk (NewPolka strict polyhedra), default\n \
		   * pkeq (NewPolka linear equalities)\n";
#ifdef PPL_ENABLED
		std::cout << 
"		   * ppl_poly (PPL strict polyhedra)\n \
		   * ppl_poly_bagnara (ppl_poly + widening from Bagnara & al, SAS’2003)\n \
		   * ppl_grid (PPL grids)\n \
		   * pkgrid (Polka strict polyhedra + PPL grids)\n";
#endif
		std::cout << 
"		 example: pagai -i <file> --domain box\n \
--input (-i) : input file\n \
--technique (-t) : use a specific technique\n \
		possible techniques:\n \
		  * lw (Lookahead Widening, Gopan & Reps, SAS'06)\n \
		  * g (Guided Static Analysis, Gopan & Reps, SAS'07)\n \
		  * pf (Path Focusing, Monniaux & Gonnord, SAS'11)\n \
		  * lw+pf (Henry, Monniaux & Moy, SAS'12), default\n \
		  * s (simple abstract interpretation)\n \
		  * dis (lw+pf, using disjunctive invariants)\n \
		  * pf_incr (s followed by pf, results of s are injected in pf)\n \
		  * incr (s followed by lw+pf, results of s are injected in lw+pf)\n \
		example: pagai -i <file> --technique pf\n \
--solver (-s) : select SMT Solver\n \
		  * z3 \n \
		  * z3_qfnra\n \
		  * mathsat\n \
		  * smtinterpol\n \
		  * cvc3\n \
		  * cvc4\n";
#ifdef HAS_Z3 
		std::cout << 
"		  * z3_api (default)\n";
#endif
#ifdef HAS_YICES
		std::cout << 
"		  * yices_api (deprecated)\n";
#endif
		std::cout << 
"		example: pagai -i <file> --technique pf --solver cvc4\n \
-n : new version of narrowing (only for s technique)\n \
-T : apply widening with threshold instead of classical widening\n \
-M : compare the two versions of narrowing (only for s technique)\n \
--annotated : specify the name of the outputted annotated source file\n \
--timeout : set a timeout for the SMT queries (available only for z3 solvers)\n \
--log-smt : SMT-lib2 queries are logged in files\n \
--compare (-c) : use this argument to compare techniques\n \
		example: pagai -i <file> -c pf -c s  will compare the two techniques pf and s\n \
--comparedomains (-C) : compare two abstract domains using the same technique\n \
          example: ./pagai -i <filename> --comparedomains --domain box --domain2 pkeq --technique pf\n \
--printformula (-f) : only outputs the SMTpass formula\n \
";
}

SMTSolver getSMTSolver() {
	return Solver;
}

Techniques getTechnique() {
	return technique;
}

bool compareTechniques() {
	return compare;
}

bool compareDomain() {
	return compare_Domain;
}

bool compareNarrowing() {
	return compare_Narrowing;
}

bool onlyOutputsRho() {
	return onlyrho;
}

bool skipNonLinear() {
	return skipnonlinear;
}

bool useSourceName() {
	return use_source_name;
}

bool printAllInvariants() {
	return printAll;
}

bool check_overflow() {
	return oflcheck;
}

bool inline_functions() {
	return withinlining;
}

bool dumpll() {
	return onlydumpll;
}

bool generateMetadata() {
	return annotatedBCFilename.size();
}

std::string getAnnotatedBCFilename() {
	return annotatedBCFilename;
}

void set_useSourceName(bool b) {
	use_source_name = (b && !force_old_output);
}

enum outputs preferedOutput() {
	if (force_old_output)
		return LLVM_OUTPUT;
	else
		return C_OUTPUT;
}

bool OutputAnnotatedFile() {
	return output_annotated;
}

char* getAnnotatedFilename() {
	return annotatedFilename;
}

int getTimeout() {
	return timeout;
}

bool hasTimeout() {
	return has_timeout;
}

char* getFilename() {
	return filename;
}

bool SVComp() {
	return svcomp;
}

Apron_Manager_Type getApronManager() {
	return ap_manager[0];
}

Apron_Manager_Type getApronManager(int i) {
	return ap_manager[i];
}

bool useNewNarrowing() {
	return Narrowing[0];
}

bool useNewNarrowing(int i) {
	return Narrowing[i];
}

bool useThreshold() {
	return Threshold[0];
}

bool useThreshold(int i) {
	return Threshold[i];
}

std::string TechniquesToString(Techniques t) {
	switch (t) {
		case LOOKAHEAD_WIDENING:
			return "LOOKAHEAD WIDENING";
		case PATH_FOCUSING: 
			return "PATH FOCUSING";
		case PATH_FOCUSING_INCR: 
			return "PATH FOCUSING INCR";
		case LW_WITH_PF:
			return "COMBINED";
		case COMBINED_INCR:
			return "COMBINED INCR";
		case SIMPLE:
			return "CLASSIC";
		case GUIDED:
			return "GUIDED";
		case LW_WITH_PF_DISJ:
			return "DISJUNCTIVE";
		default:
			abort();
	}
}

std::vector<enum Techniques> * getComparedTechniques() {
	return &TechniquesToCompare;
}

bool setApronManager(char * domain, int i) {
	std::string d;
	d.assign(domain);
	
	if (!d.compare("box")) {
		ap_manager[i] = BOX;
	} else if (!d.compare("oct")) {
		ap_manager[i] = OCT;
	} else if (!d.compare("pk")) {
		ap_manager[i] = PK;
	} else if (!d.compare("pkeq")) {
		ap_manager[i] = PKEQ;
#ifdef PPL_ENABLED
	} else if (!d.compare("ppl_poly_bagnara")) {
		ap_manager[i] = PPL_POLY_BAGNARA;
	} else if (!d.compare("ppl_poly")) {
		ap_manager[i] = PPL_POLY;
	} else if (!d.compare("ppl_grid")) {
		ap_manager[i] = PPL_GRID;
	} else if (!d.compare("pkgrid")) {
		ap_manager[i] = PKGRID;
#endif
	} else {
		std::cout << "Wrong parameter defining the abstract domain\n";
		return 1;
	}
	return 0;
}

std::string ApronManagerToString(Apron_Manager_Type D) {
	switch (D) {
		case BOX:
			return "BOX";
		case OCT:
			return "OCT";
		case PK:
			return "PK";
		case PKEQ:
			return "PKEQ";
#ifdef PPL_ENABLED
		case PPL_POLY:
			return "PPL_POLY";
		case PPL_POLY_BAGNARA:
			return "PPL_POLY_BAGNARA";
		case PPL_GRID:
			return "PPL_GRID";
		case PKGRID:
			return "PKGRID";
#endif
	}
}

enum Techniques TechniqueFromString(bool &error, std::string d) {
	error = false;
	if (!d.compare("lw")) {
		return LOOKAHEAD_WIDENING;
	} else if (!d.compare("pf")) {
		return PATH_FOCUSING;
	} else if (!d.compare("pf_incr")) {
		return PATH_FOCUSING_INCR;
	} else if (!d.compare("lw+pf")) {
		return LW_WITH_PF;
	} else if (!d.compare("s")) {
		return SIMPLE;
	} else if (!d.compare("g")) {
		return GUIDED;
	} else if (!d.compare("dis")) {
		return LW_WITH_PF_DISJ;
	} else if (!d.compare("incr")) {
		return COMBINED_INCR;
	}
	error = true;
	return SIMPLE;
}

bool setTechnique(char * t) {
	std::string d;
	d.assign(t);
	bool error;
	enum Techniques r = TechniqueFromString(error,d);
	technique = r;
	if (error) {
		std::cout << "Wrong parameter defining the technique you want to use\n";
		return 1;
	}
	return 0;
}

bool setSolver(char * t) {
	std::string d;
	d.assign(t);
	if (!d.compare("z3")) {
		Solver = Z3;
	} else if (!d.compare("z3_qfnra")) {
		Solver = Z3_QFNRA;
	} else if (!d.compare("mathsat")) {
		Solver = MATHSAT;
	} else if (!d.compare("smtinterpol")) {
		Solver = SMTINTERPOL;
	} else if (!d.compare("cvc3")) {
		Solver = CVC3;
	} else if (!d.compare("cvc4")) {
		Solver = CVC4;
#ifdef HAS_Z3 
	} else if (!d.compare("z3_api")) {
		Solver = API_Z3;
#endif
#ifdef HAS_YICES
	} else if (!d.compare("yices_api")) {
		Solver = API_YICES;
#endif
	} else {
		std::cout << "Wrong parameter defining the solver\n";
		return 1;
	}
	return 0;
}

bool setMain(const char * m) {
	main_function.assign(m);
	defined_main = true;
	return 0;
}

bool setTimeout(char * t) {
	std::string d;
	d.assign(t);
	has_timeout = true;
	bool error = false;
	try {
		timeout = boost::lexical_cast< int >( d );
	} catch( const boost::bad_lexical_cast & ) {
		//unable to convert
		error = true;
	}
	try {
		double timeout_double = boost::lexical_cast< double >( d );
		TIMEOUT_LIMIT = sys::TimeValue(timeout_double);
	} catch( const boost::bad_lexical_cast & ) {
		//unable to convert
		error = true;
	}
	return error;
}

bool definedMain() {
	return defined_main;
}

std::string getMain() {
	return main_function;
}

bool isMain(llvm::Function * F) {
	if (!definedMain()) return false;
	return (main_function.compare(F->getName().str()) == 0);
}

bool quiet_mode() {
	return quiet;
} 

bool log_smt_into_file() {
	return log_smt;
}
	
bool optimizeBC() {
	return optimize;
}

int main(int argc, char* argv[]) {

	execute run;
    int o;
    bool help = false;
    bool bad_use = false;
    const char* outputname="";
	char* arg;
	bool debug = false;

	filename=NULL;
#ifdef HAS_Z3 
	Solver = API_Z3;
#else
	Solver = Z3;
#endif
	ap_manager[0] = PK;
	ap_manager[1] = PK;
	Narrowing[0] = false;
	Narrowing[1] = false;
	Threshold[0] = false;
	Threshold[1] = false;
	technique = LW_WITH_PF;
	compare = false;
	compare_Domain = false;
	compare_Narrowing = false;
	onlyrho = false;
	skipnonlinear = false;
	defined_main = false;
	quiet = false;
	use_source_name = false;
	printAll = false;
	force_old_output = false;
	output_annotated = false;
	oflcheck = true;
	svcomp = false;
	log_smt = false;
	n_totalpaths = 0;
	n_paths = 0;
	npass = 0;
	timeout = 0;
	has_timeout = false;
	optimize = false;
	withinlining = true;
	onlydumpll = false;
	annotatedFilename = NULL;
	annotatedBCFilename = "";

	static struct option long_options[] =
		{
			{"help", no_argument,       0, 'h'},
			{"debug",   no_argument,       0, 'D'},
			{"compare",     required_argument,       0, 'c'},
			{"technique",  required_argument,       0, 't'},
			{"comparedomains",  required_argument,       0, 'C'},
			{"domain",  required_argument, 0, 'd'},
			{"domain2",  required_argument, 0, 'e'},
			{"input",  required_argument, 0, 'i'},
			{"main",  required_argument, 0, 'm'},
			{"output",    required_argument, 0, 'o'},
			{"solver",    required_argument, 0, 's'},
			{"printformula",    no_argument, 0, 'f'},
			{"printall",    no_argument, 0, 'p'},
			{"skipnonlinear",    no_argument, 0, 'L'},
			{"quiet",    no_argument, 0, 'q'},
			{"no-undefined-check",    no_argument, 0, 'v'},
			{"force-old-output",    no_argument, 0, 'S'},
			{"annotated",    required_argument, 0, 'a'},
			{"timeout",    required_argument, 0, 'k'},
			{"log-smt",    no_argument, 0, 'l'},
			{"output-bc",  required_argument, 0, 'b'},
			{"svcomp",  no_argument, 0, 'V'},
			{"optimize",  no_argument, 0, 'O'},
			{"noinline",  no_argument, 0, 'I'},
			{"dump-ll",  no_argument, 0, 'z'},
			{NULL, 0, 0, 0}
		};
	/* getopt_long stores the option index here. */
	int option_index = 0;

	 while ((o = getopt_long(argc, argv, "qa:ShDi:o:s:c:Cft:d:e:nNMTm:k:pvb:VOIz",long_options,&option_index)) != -1) {
        switch (o) {
			case 0:
				assert(false);
				break;
			case 'q':
				quiet = true;
        	    break;
			case 'V':
				{ 
					svcomp = true;
					technique = PATH_FOCUSING;
				}
        	    break;
			case 'l':
				log_smt = true;
        	    break;
        	case 'S':
        	    force_old_output = true;
        	    break;
        	case 'h':
        	    help = true;
        	    break;
        	case 'D':
        	    debug = true;
        	    break;
        	case 'v':
        	    oflcheck = false;
        	    break;
        	case 'I':
        	    withinlining = false;
        	    break;
        	case 'z':
				onlydumpll = true;
        	    break;
        	case 'b':
        	    arg = optarg;
				annotatedBCFilename.assign(arg);
        	    break;
        	case 'c':
        	    compare = true;
        	    arg = optarg;
				{ 
					std::string d;
					d.assign(arg);
					enum Techniques technique = TechniqueFromString(bad_use,d);
					TechniquesToCompare.push_back(technique);
				}
        	    break;
        	case 'C':
        	    compare_Domain = true;
        	    break;
        	case 't':
        	    arg = optarg;
				if (setTechnique(arg)) {
					bad_use = true;
				}
        	    break;
        	case 'm':
        	    arg = optarg;
				if (setMain(arg)) {
					bad_use = true;
				}
        	    break;
        	case 'k':
				if (setTimeout(optarg)) {
					bad_use = true;
				}
        	    break;
        	case 'd':
        	    arg = optarg;
				if (setApronManager(arg,0)) {
					bad_use = true;
				}
        	    break;
        	case 'e':
        	    arg = optarg;
				if (setApronManager(arg,1)) {
					bad_use = true;
				}
        	    break;
        	case 'i':
        	    filename = optarg;
        	    break;
        	case 'n':
				Narrowing[0] = true;
        	    break;
        	case 'N':
				Narrowing[1] = true;
        	    break;
        	case 'T':
				Threshold[0] = true;
        	    break;
        	case 'M':
				Narrowing[0] = true;
				Narrowing[1] = false;
				compare_Narrowing = true;
        	    break;
        	case 'o':
        	    outputname = optarg;
        	    break;
        	case 'O':
        	    optimize = true;
        	    break;
        	case 'a':
				output_annotated = true;
        	    use_source_name = true;
        	    annotatedFilename = optarg;
        	    break;
        	case 's':
        	    arg = optarg;
				if (setSolver(arg)) {
					bad_use = true;
				}
        	    break;
        	case 'f':
        	    onlyrho = true;
        	    break;
        	case 'L':
				skipnonlinear = true;
        	    break;
			case 'p':
				printAll = true;
				break;
        	case '?':
        	    std::cout << "Error : Unknown option " << optopt << "\n";
        	    bad_use = true;
        }   
    }
    if (!help) {
        if (!filename) {
            std::cout << "No input file specified\n";
            bad_use = true;
        }

        if (bad_use) {
            std::cout << "Bad use\n";
            show_help();
            exit(EXIT_FAILURE);
        }
    } else {
        show_help();
        exit(EXIT_SUCCESS);
    }

	run.exec(filename,outputname);
	
	return 0;
}

