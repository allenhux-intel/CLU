/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Usage:
    The wrapper generater is a simple interpreter. It does not
    run or emulate a preprocessor. It has a very rudimentary
    understanding of C grammar. For greatest success, keep your
    kernel definitions as simple as possible.

    1) Use the "kernel" or "__kernel" keywords. DO NOT
         use a substitute, e.g. #define KERNEL kernel
    2) Exactly one token (typically "void") should appear between
       the "kernel" token and the name of the kernel. Comments
       and newlines are OK.
       E.g. this is NOT OK:
           kernel MYDEFINE void MYDEFINE myKernelName(...){...}
         This, however, IS OK:
           kernel MYVOIDDEFINE myKernelName(...){...}
    3) All #includes are expanded, even within comments.
    4) All kernels encountered while processing #includes are also wrapped
    5) The wrapper generator ignores preprocessor directives.
         E.g. kernels and #includes within #if...#endif are interpreted
         hence it's easier to encounter infinite recursive #includes
*/
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <list>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include "string.h"

using namespace std;

struct ParamPair
{
    string m_type;
    string m_name;
    bool   m_isLocal;
};
typedef vector<ParamPair> ParamPairArray;
struct KernelStrings
{
    string m_kernelName;
    ParamPairArray m_parameters;
};
typedef vector<KernelStrings> KernelList;
typedef list<string>          StringList;

#define CLU_PREFIX "clug"
#define CLU_PREFIX_CREATE CLU_PREFIX "Create_"
#define CLU_PREFIX_ENQUEUE CLU_PREFIX "Enqueue_"

#define CLU_MAGIC_BUILD_FLAG "CLU_GENERATED_BUILD" // matching #define in clu_runtime.cpp

// arbitrary maximum recursion depth for #includes
// prevents infinite loops
const int MAX_RECURSION_DEPTH = 256;

bool g_verbose = true;
bool g_lineNumbers = true;
int  g_lineNumber = 1;
bool g_generateCPP = false;

//------------------------------------------------------------------------
// Error routine -- called to exit generator semi-gracefully
//------------------------------------------------------------------------
void ReturnError(const string& in_errString)
{
    cerr << "ERROR: " << in_errString << endl;
    exit(-1);
}

//------------------------------------------------------------------------
// simple tokenizer class that does not modify the input strings.
// step along tokens with NextToken, which returns TOKEN_END if no more tokens
// GetToken(str) copies the current token into str
//------------------------------------------------------------------------
class Tokenizer
{
private:
    const string &m_string;
    const string &m_delimiters;
    int m_index; // current position in source string
    int m_start; // start of current token
    int m_end;   // end of current token

    bool IsDelimiter(const char c)
    {
        bool found = false;
        for (unsigned int i = 0; i < m_delimiters.size(); i++)
        {
            if (c == m_delimiters[i])
            {
                found = true;
                break;
            }
        }
        return found;
    }
    Tokenizer& operator = (const Tokenizer&) {return *this;} // unused, removes compile warning
public:
    Tokenizer(const string& in_string, const string& in_delimiters)
        : m_string(in_string), m_delimiters(in_delimiters) {m_index = 0;}

    enum
    {
        TOKEN_END = -1
    };

    // returns index into input string of start of next token, or TOKEN_END
    int NextToken()
    {
        const int size = m_string.size();
        m_start = TOKEN_END;
        // scan along until we find a character that /isn't/ a delimiter
        while ((m_index < size) && (IsDelimiter(m_string[m_index])))
        {
            m_index++;
        }
        if (size != m_index)
        {
            m_start = m_index; // found start of token
        }

        // scan along until we find a character that /is/ a delimiter
        while ((m_index < size) && (!IsDelimiter(m_string[m_index])))
        {
            m_index++;
        }
        m_end = m_index;

        return m_start;
    }

    // copies token into output string
    void GetToken(string& out_string)
    {
        out_string = m_string.substr(m_start, m_end-m_start);
    }

    // Jump over a sequence of tokens starting with the specified token and
    // ending with the specified delimiter.
    // Particularly useful to skip the '__attribute__' sequence in the kernel declaration.
    int SkipTokens(const string& token, const string& delimiter)
    {
        int bkpIndex = m_index;

        NextToken();

        string localToken;
        GetToken(localToken);

        if (token.compare(localToken) == 0)
        {
            m_index = m_string.find(delimiter, m_index);

            m_start = TOKEN_END;
            m_end   = TOKEN_END;
        }
        else
            m_index = bkpIndex;

        return m_index;
    }

    // change current position within the input string
    void SetStartIndex(int in_index) {m_index = in_index;}
};

//------------------------------------------------------------------------
// From file path (c:\blah\blah\foo.cl) create header (foo_cl)
//------------------------------------------------------------------------
string GetHeader(const string& in_outFileName)
{
    const int NOT_FOUND = -1;

    // find last "\\" or "//"
    int a = in_outFileName.find_last_of("\\");
    if (NOT_FOUND == a) // try forward slash
    {
        a = in_outFileName.find_last_of("/");
    }
    a++;
    string header = in_outFileName.substr(a);
    replace(header.begin(), header.end(), '.', '_');

    return header;
}

//------------------------------------------------------------------------
// make an array containing all the tokens of a parameter definition
// e.g. global int * foo -> {"global","int","*","foo"}
//------------------------------------------------------------------------
void GetParamTokens(const string& param, vector<string>& paramTokens)
{
    // ignore pointer and reference in parameter list:
    const string paramDelimiters("*& ,()\n\r\t");

    Tokenizer tokenizer(param, paramDelimiters);
    int paramIndex;
    while (Tokenizer::TOKEN_END != (paramIndex = tokenizer.NextToken()))
    {
        string token;
        tokenizer.GetToken(token);
        paramTokens.push_back(token);
    }
}

//------------------------------------------------------------------------
// Translate device parameters into host parameters
//    global int* foo    -> cl_mem foo or cl::Buffer & foo in C++ mode
//    local double * foo -> int foo
//    float foo          -> float foo
// Returns the index into the source string after the parameters
//------------------------------------------------------------------------
int GetParameterString(const string& in_src, int in_srcIndex,
    ParamPairArray& out_params)
{
    // find parameters between ( and )
    string::size_type start = in_src.find('(',in_srcIndex);
    if (string::npos == start) return 0; // error condition encountered
    string::size_type end = in_src.find(')',start);
    if (string::npos == end) return 0; // error condition encountered

    // copy parameter string
    string parameters;
    start++; // don't include (
    parameters = in_src.substr(start, end-start);

    // return index into src string where this search ended
    int outIndex = end;

    // find the comma delimited parameters
    const string delimiters(",");
    Tokenizer tokenizer(parameters, delimiters);

    int paramIndex;
    while (Tokenizer::TOKEN_END != (paramIndex = tokenizer.NextToken()))
    {
        string param;
        tokenizer.GetToken(param);
        vector<string> paramTokens;
        GetParamTokens(param, paramTokens);

        // number of tokens excluding the name of the token:
        int numTokens = paramTokens.size() - 1;
        if (numTokens < 1)
        {
            string error = "Error parsing parameters in string: " + parameters;
            ReturnError(error);
        }
        // determine host type of parameter
        enum eParamType
        {
            PARAM_LOCAL,
            PARAM_GLOBAL,
            PARAM_SAMPLER,
            PARAM_DEFAULT
        };
        eParamType paramType = PARAM_DEFAULT;
        // loop over the tokens that describe the type of the parameter
        for (int i = 0; i < (numTokens); i++)
        {
            if (("local" == paramTokens[i]) || ("__local" == paramTokens[i]))
            {
                paramType = PARAM_LOCAL;
                break;
            }
            if (("global" == paramTokens[i]) || ("__global" == paramTokens[i]) ||
                ("constant" == paramTokens[i]) || ("__constant" == paramTokens[i]) ||
                ("image1d_t" == paramTokens[i]) ||
                ("image1d_buffer_t" == paramTokens[i]) ||
                ("image1d_array_t" == paramTokens[i]) ||
                ("image2d_array_t" == paramTokens[i]) ||
                ("image2d_t" == paramTokens[i]) || ("image3d_t" == paramTokens[i])
                )
            {
                paramType = PARAM_GLOBAL;
                break;
            }
            if ("sampler_t" == paramTokens[i])
            {
                paramType = PARAM_SAMPLER;
                break;
            }
        }

        // translate device type to host type
        int size = out_params.size();
        out_params.resize(size+1);
        out_params[size].m_isLocal = false;
        switch (paramType)
        {
        case PARAM_LOCAL:
            out_params[size].m_type = "cl_uint";
            out_params[size].m_isLocal = true;
            break;
        case PARAM_GLOBAL:
            if( g_generateCPP ) {
                out_params[size].m_type = "cl::Buffer &";
            } else {
                out_params[size].m_type = "cl_mem";
            }
            break;
        case PARAM_SAMPLER:
            out_params[size].m_type = "cl_sampler";
            break;
        default: // neither local nor global? just paste entire type
            for (int i = 0; i < numTokens; i++)
            {
                if (i > 0)
                {
                    out_params[size].m_type += " ";
                }
                string t = paramTokens[i];

                // unsigned?
                if ((t == "unsigned") && ((i+1) < numTokens))
                {
                    i++;
                    t = paramTokens[i];
                    if      (t == "int")    t = "cl_uint";
                    else if (t == "char")   t = "cl_uchar";
                    else if (t == "short")  t = "cl_ushort";
                    else if (t == "long")   t = "cl_ulong";
                }
                else // signed
                {
                    if      (t == "int")    t = "cl_int";
                    else if (t == "char")   t = "cl_char";
                    else if (t == "short")  t = "cl_short";
                    else if (t == "long")   t = "cl_long";
                    else if (t == "float")  t = "cl_float";
                    else if (t == "double") t = "cl_double";
                } // end if
                out_params[size].m_type += t;
            }
        };

        // add parameter name
        out_params[size].m_name = paramTokens[numTokens];
    }
    return outIndex;
}

//------------------------------------------------------------------------
// Create a simplified copy of the input string by removing
// comments and function bodies
//------------------------------------------------------------------------
void CopySimplify(string& out_src, const string& in_src)
{
    int size = in_src.size();
    // remove potentially confusing code: comments and function bodies
    // we only care about kernel definitions
    for (int i = 0; i < (size-1); i++)
    {
        // remove comments
        if ('/' == in_src[i]) // may be "//" or "/*"
        {
            if ('/' == in_src[i+1]) // found "//"
            {
                // clear to the end of the line
                while (('\n' != in_src[i]) && (i < size))
                {
                    i++;
                }
            }
            else if ('*' == in_src[i+1])
            {
                // clear until we find "*/" or reach end of file
                while (i < size)
                {
                    if ((i+1 < size) &&
                        ('*' == in_src[i]) && ('/' == in_src[i+1]))
                    {
                        i+=2;
                        break;
                    }
                    i++;
                } // end clearing C-style comment
            }
        }

        // remove code blocks between { and }
        if ('{' == in_src[i])
        {
            int popCount = 1;
            while (i < size)
            {
                // if found the end of a block, decrement popCount
                if ('}' == in_src[i])
                {
                    popCount--;
                }

                i++;

                // if found the start of a block, increment popCount
                if ('{' == in_src[i])
                {
                    popCount++;
                }

                // if popCount is 0 at this point, exit
                if (0 == popCount)
                {
                    break;
                }
            }
        }
        // write non-comment and non-body characters
        out_src += in_src[i];
    }
}

//------------------------------------------------------------------------
// Find all the kernels, plus their parameter names and types
//------------------------------------------------------------------------
void FindKernels(const string& src, KernelList& out_kernels)
{
    const string kernelDelimiters(" ,()\n\r\t");

    // find the kernel names and parameters
    Tokenizer tokenizer(src, kernelDelimiters);

    int srcIndex;
    while (Tokenizer::TOKEN_END != (srcIndex = tokenizer.NextToken()))
    {
        string token;
        tokenizer.GetToken(token);
        // must be of form "kernel SOMETHING myKernelName(...."
        if (("kernel" == token) || ("__kernel" == token))
        {
            // structure to be returned containing kernel name & parameters
            KernelStrings kernelStrings;

            // handle multiple attributes
            while (true)
            {
                int attrIndex = tokenizer.SkipTokens("__attribute__", ")");

                if (attrIndex == srcIndex)
                    break;

                srcIndex = attrIndex;
            }

            int index = GetParameterString(src, srcIndex, kernelStrings.m_parameters);
            if (0 == index)
            {
                break; // error condition encountered, skip this kernel
            }

            srcIndex = tokenizer.NextToken();
            if (Tokenizer::TOKEN_END == srcIndex)
            {
                break; // error condition encountered, skip this kernel
            }

            // this should be the name
            srcIndex = tokenizer.NextToken();
            if (Tokenizer::TOKEN_END == srcIndex)
            {
                break; // error condition encountered, skip this kernel
            }

            // get kernel name
            tokenizer.GetToken(kernelStrings.m_kernelName);
            // add kernel name to list
            out_kernels.push_back(kernelStrings);
            // scoot tokenizer forward past the parameters (we got this index from GetParameterString)
            tokenizer.SetStartIndex(index);
        }
    }
}


//------------------------------------------------------------------------
// recursively search for and include #includes
//------------------------------------------------------------------------
void FindIncludes(string& out_src, stringstream& in_is, const StringList& in_includePaths)
{
    static int recursionDepth = 0;

    recursionDepth++;
    if (recursionDepth > MAX_RECURSION_DEPTH)
    {
        string error = "Reached max #include recursion depth";
        ReturnError(error);
    }
    string tmp;
    while( std::getline(in_is, tmp) )
    {
        if (string::npos != tmp.find("#include"))
        {
            string::size_type start = tmp.find_first_of('"');
            string::size_type end = tmp.find_last_of('"');
            if (string::npos == start)
            {
                start = tmp.find_first_of('<');
                end = tmp.find_last_of('>');
            }
            if (string::npos == start) // neither "" nor <>
            {
                string error = "Could not interpret include directive:\n" + tmp;
                ReturnError(error);
            }
            string incName = tmp.substr(start+1,end-start-1);

            // search all include paths for the file
            bool found = false;
            for(StringList::const_iterator i = in_includePaths.begin(); i != in_includePaths.end(); i++)
            {
                string pathString = *i + incName.c_str();
                const char* path = pathString.c_str();
                ifstream incFile(path);
                if (incFile.good())
                {
                    string src((std::istreambuf_iterator<char>(incFile)), std::istreambuf_iterator<char>());
                    incFile.close();
                    stringstream is;
                    is << src;
                    FindIncludes(out_src, is, in_includePaths);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                string error("File could not be found for #include: ");
                error += incName;
                ReturnError(error);
            }
            out_src += "/* included " + incName + "*/\n";
        }
        else
        {
            out_src += tmp + "\n";
        }
    } // end loop over lines of code
    recursionDepth--;
}

//------------------------------------------------------------------------
// scan through source for kernels and #includes
// recursively search all included files
//------------------------------------------------------------------------
void ProcessSource(const string& in_src, const StringList& in_includePaths,
    StringList& out_sources, KernelList& out_kernels)
{
    // first, incorporate all #includes in incoming source
    // search for #include.  This is recursive.
    string included;
    stringstream is;
    is << in_src;
    FindIncludes(included, is, in_includePaths);

    out_sources.push_back(included);

    // make a very simplified copy of the string that is easy
    // to search for tokens, e.g. "kernel" and "#include"
    string simpleSrc;
    CopySimplify(simpleSrc, included);

    // search for kernels & parameters
    FindKernels(simpleSrc, out_kernels);
}

//------------------------------------------------------------------------
// Write comment listing functions we expect users to use
//------------------------------------------------------------------------
class WriteExports
{
private:
    ofstream& m_outFile;
    WriteExports& operator = (const WriteExports&) {return *this;} // unused, removes compile warning
public:
    WriteExports(ofstream& out_file) : m_outFile(out_file) {}
    void operator() (const KernelStrings& in_kernelStrings)
    {
        const string& name = in_kernelStrings.m_kernelName;
        m_outFile << endl << "    " CLU_PREFIX "_" << name << " " CLU_PREFIX_CREATE << name << "(cl_int*)" << endl;
        m_outFile << "    cl_int " CLU_PREFIX_ENQUEUE << name << "(...)" << endl;
    }
};

//------------------------------------------------------------------------
// Write comment listing functions we expect users to use in C++ mode
//------------------------------------------------------------------------
class WriteCPPExports
{
private:
    ofstream& m_outFile;
    WriteCPPExports& operator = (const WriteCPPExports&) {return *this;} // unused, removes compile warning
public:
    WriteCPPExports(ofstream& out_file) : m_outFile(out_file) {}
    void operator() (const KernelStrings& in_kernelStrings)
    {
        // Output definition of function that returns the std::function interface to the kernel
        const string& name = in_kernelStrings.m_kernelName;
        m_outFile << "    cl::Program " CLU_PREFIX_CREATE << name << "(cl::Context creationContext = cl::Context::getDefault())" << endl;
    }
};

//------------------------------------------------------------------------
// functor that writes kernel wrapper
//------------------------------------------------------------------------
class WriteKernelWrapper
{
private:
    ofstream& m_outFile;
    string&   m_getProgramName;
    WriteKernelWrapper& operator = (const WriteKernelWrapper&) {return *this;} // unused, removes compile warning
public:
    WriteKernelWrapper(ofstream& outFile, string& in_getProgramName) :
      m_outFile(outFile), m_getProgramName(in_getProgramName) {}
    void operator() (const KernelStrings& in_kernelStrings)
    {
        const string& kernelName = in_kernelStrings.m_kernelName;
        const ParamPairArray& kernelParams = in_kernelStrings.m_parameters;

        string structName  = CLU_PREFIX "_" + kernelName;
        string createName  = CLU_PREFIX_CREATE + kernelName;
        string enqueueName = CLU_PREFIX_ENQUEUE + kernelName;

        // definition of custom structure for kernel
        m_outFile <<
            "/* object that associates specific cl_kernel with its wrapper code */" << endl <<
            "typedef struct _" << structName << endl <<
            "{" << endl <<
            "    cl_kernel  m_kernel;  /* application should clReleaseKernel */" << endl <<
            "    cl_program m_program; /* DO NOT clReleaseProgram */" << endl <<
            "} " << structName << ";" << endl << endl;

        // custom function to get "object" containing structure and pointer to enqueue function
        m_outFile <<
            "/* function to initialize structure and create kernel from source */" << endl <<
            "INLINE " << structName << " " << createName << "(cl_int * errcode_ret)" << endl <<
            "{"                                                      << endl <<
            "    cl_int status;"                                     << endl <<
            "    " << structName << " s;"                            << endl <<
            "    s.m_program = " << m_getProgramName << "(&status);" << endl <<
            "    if (CL_SUCCESS == status)"                          << endl <<
            "    {"                                                  << endl <<
            "        s.m_kernel = clCreateKernel(s.m_program, \"" << kernelName << "\", &status);" << endl <<
            "    }"                                                  << endl <<
            "    if (errcode_ret)"                                   << endl <<
            "    {"                                                  << endl <<
            "        *errcode_ret = status;"                         << endl <<
            "    }"                                                  << endl <<
            "    return s;"                                          << endl <<
            "}"                                                      << endl << endl;

        // custom function for enqueue of kernel
        // create parameter string
        string parameters = "(" + structName + " s, clu_enqueue_params* params";
        for (unsigned int i = 0; i < kernelParams.size(); i++)
        {
            parameters += ", " + kernelParams[i].m_type + " " + kernelParams[i].m_name;
        }
        parameters += ")";

        m_outFile <<
            "INLINE cl_int " << enqueueName << parameters << endl <<
            "{" << endl <<
            "    cl_uint status = CL_SUCCESS;" << endl;

        for (unsigned int i = 0; i < kernelParams.size(); i++)
        {
            m_outFile <<
                "    status = clSetKernelArg(s.m_kernel, " << i << ", ";
            if (true == kernelParams[i].m_isLocal)
            {
                m_outFile << kernelParams[i].m_name << ", 0);" << endl;
            }
            else
            {
                m_outFile << "sizeof(" << kernelParams[i].m_type << "), &" << kernelParams[i].m_name << ");" << endl;
            }
            m_outFile << "    if (CL_SUCCESS != status) return status;" << endl;
        }

        m_outFile <<
            "    status = cluEnqueue(s.m_kernel, params);" << endl <<
            "    return status;" << endl <<
            "}" << endl << endl;

    } // end functor operator ()
};

//------------------------------------------------------------------------
// functor that writes a C++ kernel wrapper
//------------------------------------------------------------------------
class WriteCPPKernelWrapper
{
private:
    ofstream& m_outFile;
    string&   m_getProgramName;
    WriteCPPKernelWrapper& operator = (const WriteCPPKernelWrapper&) {return *this;} // unused, removes compile warning
public:
    WriteCPPKernelWrapper(ofstream& outFile, string& in_getProgramName) :
      m_outFile(outFile), m_getProgramName(in_getProgramName) {}
    void operator() (const KernelStrings& in_kernelStrings)
    {
        const string& kernelName = in_kernelStrings.m_kernelName;
        const ParamPairArray& kernelParams = in_kernelStrings.m_parameters;
        
        string structName  = CLU_PREFIX "_" + kernelName;
        string createName  = CLU_PREFIX_CREATE + kernelName;

        // Output function to return std::function wrapper
        m_outFile << 
            "std::function<cl::Event (";
        m_outFile << endl << "    cl::EnqueueArgs&";
        // Output param type list
        for (unsigned int i = 0; i < kernelParams.size(); i++)
        {
            std::string paramType(kernelParams[i].m_type);
            m_outFile << ", " << endl << "    " << paramType;
        }
        m_outFile << endl <<
            "    )> " << endl << createName << "(cl::Context creationContext = cl::Context::getDefault()) " << endl << "{" << endl;
        m_outFile << 
            "    std::function<cl::Event (";
        m_outFile << endl << "        cl::EnqueueArgs&";
        // Output param type list
        for (unsigned int i = 0; i < kernelParams.size(); i++)
        {
            std::string paramType(kernelParams[i].m_type);
            m_outFile << ", " << endl << "        " << paramType;
        }
        m_outFile << ")> " << endl << "        kernelFunctor(cl::make_kernel<";
        // Output param type list for the make_kernel entity.
        for (unsigned int i = 0; i < kernelParams.size(); i++)
        {
            std::string paramType(kernelParams[i].m_type);
            m_outFile << endl << "            " << paramType;
            if( i < (kernelParams.size() - 1) ) {
                m_outFile << ", ";
            }
        }
        m_outFile << ">" << endl << "            (" << m_getProgramName << "(creationContext), \"" << kernelName << "\"));" << endl; 
        m_outFile << "    return kernelFunctor;" << endl << "}" << endl << endl;

        // custom function for enqueue of kernel
        // create parameter string
        string parameters = "(" + structName + " s, clu_enqueue_params* params";
        for (unsigned int i = 0; i < kernelParams.size(); i++)
        {
            parameters += ", " + kernelParams[i].m_type + " " + kernelParams[i].m_name;
        }
        parameters += ")";

    } // end functor operator ()
};

//------------------------------------------------------------------------
// fix quotes and newlines in stringified string
//------------------------------------------------------------------------
class WriteSourceString
{
private:
    ofstream& m_outFile;
    WriteSourceString& operator = (const WriteSourceString&) {return *this;} // unused, removes compile warning

public:
    WriteSourceString(ofstream& out_file) : m_outFile(out_file) {}
    void operator () (const string& in_src)
    {
        stringstream is;
        string tmp;

        bool empty_string = true; // workaround case where string is pruned to nothing

        is << in_src;
        while( std::getline(is, tmp) )
        {
            // DO NOT skip empty lines: need them for debugging to work
            //if (0 == tmp.size()) continue; // skip empty lines

            // start of line
            m_outFile << "\n    ";
            if (g_lineNumbers)
            {
                m_outFile << "/* " << g_lineNumber++ << "*/ ";
            }
            m_outFile << "\"";

            // fix problem characters (quotes and backslashes)
            for (size_t i = 0; i < tmp.size(); i ++)
            {
                char c = tmp[i];
                switch (c)
                {
                case '"': m_outFile << '\\'; break;
                case '\\': m_outFile << '\\'; break;
                }
                m_outFile << c;
            } // end loop over line
            m_outFile << "\\n\""; // end of line
            empty_string = false;
        } // end loop over lines of code

        // workaround case where string is pruned to nothing
        if (empty_string)
        {
            m_outFile << endl << "    \"\"";
        }

        m_outFile << ","; // end of string
    }
};

//------------------------------------------------------------------------
// write stringified cl source wrapped in function that also builds it
//------------------------------------------------------------------------
void GenerateWrappers(const string& in_inFileName, const string& in_outFileName,
    const StringList& in_includePaths)
{
    // read the whole source file in as a single string
    ifstream inFile(in_inFileName.c_str());
    if (!inFile.is_open())
    {
        string error("File not found: ");
        error += in_inFileName;
        ReturnError(error);
    }
    string src((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    StringList sources; // list of source strings, the original and all #includes
    KernelList kernels; // kernels and their arguments

    // scan through source for kernels and #includes
    // recursively search all included files
    ProcessSource(src, in_includePaths, sources, kernels);

    string header = GetHeader(in_outFileName);
    string getProgramName = CLU_PREFIX "Get_" + header;

    ofstream outFile(in_outFileName.c_str());
    if (!outFile.is_open())
    {
        string error("File could not be opened for writing: ");
        error += in_outFileName;
        ReturnError(error);
    }

    // helpful comments
    if( g_generateCPP ) 
    {
        outFile <<
            "/* CLU GENERATED */" << endl <<
            "/* All structures and functions generated by CLU start with " << CLU_PREFIX << " */" << endl <<
            "/*" << endl <<
            "Typical Usage:" << endl << 
            "    auto kernelFunctorName = clugCreate_MyKernelName();" << endl <<
            "    kernelFunctorName(" << endl <<
            "        cl::EnqueueArgs(cl::NDRange(numElements), cl::NDRange(numElements)), ...);" << endl << endl <<
            "Exports:" << endl;
        for_each(kernels.begin(), kernels.end(), WriteCPPExports(outFile));
    }
    else 
    {
        outFile <<
            "/* CLU GENERATED */" << endl <<
            "/* All structures and functions generated by CLU start with " << CLU_PREFIX << " */" << endl <<
            "/*" << endl <<
            "Typical Usage:" << endl << endl <<
            "    " CLU_PREFIX "MyKernelName s = " CLU_PREFIX_CREATE "MyKernelName(0);" << endl <<
            "    clu_enqueue_params params = CLU_DEFAULT_PARAMS;" << endl <<
            "    params.nd_range = CLU_ND1(bufferLength);" << endl <<
            "    status = " CLU_PREFIX_ENQUEUE "MyKernel(s, &params, ...);" << endl << endl <<
            "Exports:" << endl;
            for_each(kernels.begin(), kernels.end(), WriteExports(outFile));
    }


    outFile <<
        "*/" << endl << endl;

    
    // #pragma once
    if( g_generateCPP ) 
    {
        outFile <<
            "#include <CL/cl.hpp>" << endl <<
            "#include <functional>" << endl <<
            "#ifndef __" << header.c_str() << endl <<
            "#define __" << header.c_str() << endl << endl;

        // function to build program from stringified sources
        outFile <<
            "cl::Program " << getProgramName << "(const cl::Context &creationContext)" << endl <<
            "{" << endl <<
            "    static const char* src[" << sources.size() << "] = {";

        for_each(sources.begin(), sources.end(), WriteSourceString(outFile));

        outFile << endl <<
            "    };" << endl;
            // TODO: Expand to make this accept more than one source string

        outFile << "    cl::Program::Sources sourceList;" << endl;
        for( unsigned int i = 0; i < sources.size(); ++i ) {
            outFile << "    sourceList.push_back(" << endl <<
                "        std::pair<const char*, ::size_t>(src[" << i << "], strlen(src[" << i << "]))";
            if( i < (sources.size() - 1) ) {
                outFile << ", " << endl;
            }
        }
        outFile << ");" << endl;
        outFile <<
            "    cl::Program program = cl::Program(creationContext, sourceList);" << endl <<
            "    program.build();" << endl <<
            "    return program;" << endl <<
            "}" << endl << endl;

        for_each(kernels.begin(), kernels.end(), WriteCPPKernelWrapper(outFile, getProgramName));

        outFile << "#endif" << endl << endl;
    }
    else 
    {
        outFile <<
            "#include <clu.h>" << endl <<
            "#ifndef __" << header.c_str() << endl <<
            "#define __" << header.c_str() << endl << endl;

        // function to build program from stringified sources
        outFile <<
            "/* This function is shared by all " CLU_PREFIX_CREATE "* functions below */" << endl <<
            "INLINE cl_program " << getProgramName << "(cl_int* out_pStatus)" << endl <<
            "{" << endl <<
            "    static const char* src[" << sources.size() << "] = {";

        for_each(sources.begin(), sources.end(), WriteSourceString(outFile));

        outFile << endl <<
            "    };" << endl <<
            "    /* CLU will only build this program the first time */" << endl <<
            "    /* CLU will release this program upon shutdown (cluRelease) */" << endl <<
            // pass a flag to the build API so it will know to manage the lifetime of the resulting cl_program
            "    cl_program program = cluBuildSourceArray(" << sources.size() << ", src, 0, \"" << CLU_MAGIC_BUILD_FLAG << "\", out_pStatus);" << endl <<
            "    return program;" << endl <<
            "}" << endl << endl;

        for_each(kernels.begin(), kernels.end(), WriteKernelWrapper(outFile, getProgramName));

        outFile << "#endif" << endl << endl;
    }


    outFile.close();

    if (g_verbose)
    {
        cerr << "kernels found: ";
        for (KernelList::iterator i = kernels.begin(); i != kernels.end(); i++)
        {
            if (i != kernels.begin()) cerr << ", ";
            cerr << i->m_kernelName;
        }
        cerr << endl;
    }
}

//------------------------------------------------------------------------
// display command-line parameters
//------------------------------------------------------------------------
void Usage()
{
    cerr <<
        "The syntax of this command is:" << endl << endl <<
        "generator input_file_name" << endl <<
        "-i -I include_path" << endl <<
        "-o -O output_file_name (defaults to input_file_name.h)" << endl <<
        "-q -Q quiet mode" << endl <<
        "-n -N do not show line numbers" << endl <<
        "-cpp output the header in C++ mode to work with cl.hpp" << endl;
}

//************************************************************************
// main: parse command-line arguments and generate wrapper file
//************************************************************************
int main(int argc, char *argv[])
{
    StringList includePaths;
    string inFileName;
    string outFileName;

    // process command line
    if (0 == argc)
    {
        Usage();
        return -1;
    }

    int arg = 1;
    while (argc > arg)
    {
        //Args here
        if ((!strcmp(argv[arg], "-I")) || (!strcmp(argv[arg], "-i")))
        {
            arg++;
            char* str = argv[arg];
            includePaths.push_back(str);
            int n = strlen(str);
            if ((str[n-1] != '/') && (str[n-1] != '\\'))
            {
                string err = "Include paths must have trailing slash: ";
                err += str;
                ReturnError(err);
                return -1;
            }
        }
        else if (!strcmp(argv[arg], "-h"))
        {
            Usage();
            return -1;
        }
        else if ((!strcmp(argv[arg], "-O")) || (!strcmp(argv[arg], "-o")))
        {
            arg++;
            outFileName = argv[arg];
        }
        else if ((!strcmp(argv[arg], "-Q")) || (!strcmp(argv[arg], "-q")))
        {
            g_verbose = false;
        }
        else if ((!strcmp(argv[arg], "-N")) || (!strcmp(argv[arg], "-n")))
        {
            g_lineNumbers = false;
        }
        else if ((!strcmp(argv[arg], "-cpp")))
        {
            g_generateCPP = true;
        }
        else // default, undecorated argument assumed to be name
        {
            inFileName = argv[arg];
            //printf("unknown command line argument: %s\n", argv[arg]);
            //PrintUsage(argv[0]);
            //exit(1);
        }
        arg++;
    }

    if (0 == inFileName.size())
    {
        Usage();
        return -1;
    }

    if (0 == outFileName.size())
    {
        outFileName = inFileName + ".h";
    }

    // first path is current path
    includePaths.push_front("");

    // open input & output files, write output file
    GenerateWrappers(inFileName, outFileName, includePaths);

    return 0;
}
