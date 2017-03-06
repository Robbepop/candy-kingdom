/****************************************************************************************[Dimacs.h]
 Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 Copyright (c) 2007-2010, Niklas Sorensson

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef Glucose_Dimacs_h
#define Glucose_Dimacs_h

#include "utils/ParseUtils.h"
#include "core/SolverTypes.h"

#include <vector>

namespace Candy {

class CNFProblem {

    For problem;
    unsigned int maxVars = 0;

    unsigned int headerVars = 0;
    unsigned int headerClauses = 0;

    bool occurenceNormalized = false;
    std::vector<double> literalOccurence;

public:

    CNFProblem() {
    }

    For& getProblem() {
        return problem;
    }

    const For& getProblem() const {
        return problem;
    }

    unsigned int nVars() const {
        return maxVars;
    }

    unsigned int nClauses() const {
        return problem.size();
    }

    double getRelativeOccurence(Lit lit) {
        if (!occurenceNormalized) {
            double max = *std::max_element(literalOccurence.begin(), literalOccurence.end());
            for (double& occ : literalOccurence) {
                occ = occ / max;
            }
            occurenceNormalized = true;
        }
        return literalOccurence[lit];
    }

    bool readDimacsFromStdout() {
        gzFile in = gzdopen(0, "rb");
        if (in == NULL) {
            printf("ERROR! Could not open file: <stdin>");
            return false;
        }
        parse_DIMACS(in);
        gzclose(in);
        return true;
    }

    bool readDimacsFromFile(const char* filename) {
        gzFile in = gzopen(filename, "rb");
        if (in == NULL) {
            printf("ERROR! Could not open file: %s\n", filename);
            return false;
        }
        parse_DIMACS(in);
        gzclose(in);
        return true;
    }

    inline int newVar() {
        literalOccurence.push_back(0);
        literalOccurence.push_back(0);
        return maxVars++;
    }

    inline void readClause(Cl& in) {
        Cl* lits = new Cl(in);
        for (Lit lit : *lits) {
            while (var(lit) >= (int) maxVars) {
                newVar();
            }
            literalOccurence[lit] += 1.0 / lits->size();
            occurenceNormalized = false;
        }
        problem.push_back(lits);
    }

    void readClause(Lit plit) {
        readClause({ plit });
    }

    void readClause(Lit plit1, Lit plit2) {
        readClause({ plit1, plit2 });
    }

    void readClause(std::initializer_list<Lit> list) {
        Cl clause(list);
        readClause(clause);
    }

private:

    void readClause(Glucose::StreamBuffer& in) {
        Cl lits;
        for (int plit = parseInt(in); plit != 0; plit = parseInt(in)) {
            lits.push_back(Glucose::mkLit(abs(plit) - 1, plit < 0));
        }
        readClause(lits);
    }

    void parse_DIMACS(gzFile input_stream) {
        Glucose::StreamBuffer in(input_stream);
        skipWhitespace(in);
        while (*in != EOF) {
            if (*in == 'p') {
                if (eagerMatch(in, "p cnf")) {
                    headerVars = parseInt(in);
                    headerClauses = parseInt(in);
                    problem.reserve(headerClauses);
                } else {
                    printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
                }
            } else if (*in == 'c' || *in == 'p') {
                skipLine(in);
            } else {
                readClause(in);
            }
            skipWhitespace(in);
        }
        if (headerVars != maxVars) {
            fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of variables.\n");
        }
        if (headerClauses != problem.size()) {
            fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
        }
    }

};

}

#endif
