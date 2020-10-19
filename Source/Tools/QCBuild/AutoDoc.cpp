/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2010, 2014 Sylwester Wysocki <sw143@wp.pl>                   */
/*                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a    */
/* copy of this software and associated documentation files (the "Software"), */
/* to deal in the Software without restriction, including without limitation  */
/* the rights to use, copy, modify, merge, publish, distribute, sublicense,   */
/* and/or sell copies of the Software, and to permit persons to whom the      */
/* Software is furnished to do so, subject to the following conditions:       */
/*                                                                            */
/* The above copyright notice and this permission notice shall be included in */
/* all copies or substantial portions of the Software.                        */
/*                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    */
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        */
/* DEALINGS IN THE SOFTWARE.                                                  */
/*                                                                            */
/******************************************************************************/

//
// Purpose: Generate HTML documentation from comments
//          inside source code.
//

#include "AutoDoc.h"
#include "Diagram.h"
#include <Tegenaria/Str.h>

#ifdef WIN32
# include <io.h>
#endif

using namespace Tegenaria;

int ListComponents(SourceInfo *pi, vector<SourceInfo> &comps);

#define HTML_PRE_STYLE "width:720px;"        \
                       "background:#eee;"    \
                       "border-style:solid;" \
                       "border-color:#888;"  \
                       "border-width:1px;"   \
                       "padding: 8px;"
//
//
//

static string AutoDocHighlightCode(string code)
{
  string oldStr;
  string newStr;

  const char *types[] =
  {
    "int", "double", "float", "vector", "string", "set", "map", "void",
    "stack", "const", "unsigned", "signed", "char", "int64_t", "int32_t",
    "int8_t", "uint64_t", "uint32_t", "uint16_t", "uint8_t", "int16_t",
    "bool", "DWORD", "HANDLE", "BOOL", NULL
  };

  code = " " + code + " ";

  for (int i = 0; types[i]; i++)
  {
    oldStr = string(" ") + types[i] + string(" ");
    newStr = string(" <span style='color:green'>") + types[i] + string("</span> ");
    StrReplaceString(code, oldStr.c_str(), newStr.c_str());
  }

  return code.substr(1, code.size() - 1);
}

//
//
//

static string AutoDocFormatFunctionProto(FunctionInfo *fi)
{
  string rv = "";

  vector<string> args;
  vector<string> argTypes;
  vector<string> argNames;

  int argTypeLengthMax = 0;

  rv += StrNormalizeWhiteSpaces(fi -> return_);

  if (!rv.empty())
  {
    rv += " ";
  }

  rv += StrNormalizeWhiteSpaces(fi -> name_);
  rv += "\n";
  rv += "(\n";

  StrTokenize(args, fi -> args_, ",()");

  //
  // Remove empty tokens from args[].
  //

  for (vector<string>::iterator it = args.begin(); it != args.end(); )
  {
    (*it) = StrNormalizeWhiteSpaces(*it);

    if (it -> empty())
    {
      it = args.erase(it);
    }
    else
    {
      it ++;
    }
  }

  //
  // Split args[] into type and variable name.
  //

  for (int i = 0; i < args.size(); i++)
  {
    string type;
    string name;

    //
    // Find position of last whitespace.
    //

    int lastSpaceIdx = -1;

    for (int j = 0; j < args[i].size(); j++)
    {
      if (isspace(args[i][j]))
      {
        lastSpaceIdx = j;
      }
    }

    if (lastSpaceIdx == -1)
    {
      //
      // Only typename found.
      //

      type = args[i];
      name = "";
    }
    else
    {
      //
      // Found typename and variable name.
      // Split them.
      //

      while(lastSpaceIdx < args[i].size() -1 &&
                (args[i][lastSpaceIdx] == '*') || isspace(args[i][lastSpaceIdx]))
      {
        lastSpaceIdx ++;
      }

      type = StrNormalizeWhiteSpaces(args[i].substr(0, lastSpaceIdx));
      name = StrNormalizeWhiteSpaces(args[i].substr(lastSpaceIdx));
    }

    //
    // Save splitted type and variable name.
    //

    argTypes.push_back(type);
    argNames.push_back(name);

    //
    // Count maximum type length.
    //

    if (type.size() > argTypeLengthMax)
    {
      argTypeLengthMax = type.size();
    }
  }

  //
  // Put args list.
  //

  for (int i = 0; i < args.size(); i++)
  {
    rv += "  ";
    rv += argTypes[i];

    for (int j = 0; j < argTypeLengthMax - argTypes[i].size() + 1; j++)
    {
      rv += " ";
    }

    rv += argNames[i];

    if (i < args.size() - 1)
    {
      rv += ",\n";
    }
    else
    {
      rv += "\n";
    }
  }

  rv += ");";

  rv = StrEncodeToHtml(rv, 0); //STR_HTML_NON_BREAK_SPACES);
  rv = AutoDocHighlightCode(rv);

  return rv;
}

//
// Parse #pragma qcbuild_xxx(parameter) macro.
//
// TODO: Handle multiple parameters.
//

static int AutoDocParseQCBuildPragma(QCBuildPragma *pragmaInfo, string line)
{
  int exitCode = -1;

  vector<string> args;

  char *buf = (char *) line.c_str();

  char *cmd       = NULL;
  char *argsBegin = NULL;
  char *argsEnd   = NULL;
  char *argsRaw   = NULL;

  cmd       = strstr(buf, "qcbuild_");
  argsBegin = strchr(buf, '(');

  for (int i = 0; buf[i]; i++)
  {
    if (buf[i] == ')')
    {
      argsEnd = buf + i;
    }
  }

  FAILEX(cmd == NULL,       "ERROR: Missing 'qcbuild_' prefix in #pragma macro '%s'.\n", line.c_str());
  FAILEX(argsBegin == NULL, "ERROR: Missing '(' in #pragma macro '%s'.\n", line.c_str());
  FAILEX(argsEnd == NULL,   "ERROR: Missing ')' in #pragma macro '%s'.\n", line.c_str());

  argsBegin[0] = 0;
  argsEnd[0]   = 0;
  argsRaw      = argsBegin + 1;

  StrTokenize(args, argsRaw, ",", "\"");

  pragmaInfo -> cmd_ = cmd;
  pragmaInfo -> args_.resize(1);
  pragmaInfo -> args_ = args;

  DBG_MSG("Decoded [%s][%s] pragma command.\n", cmd, argsRaw);

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  return exitCode;
}

//
// Compare source files by number of functions inside.
//

static bool CompareSourceByNumberOfFunctions(const SourceFile &sf1,
                                                 const SourceFile &sf2)
{
  return sf1.functions_.size() > sf2.functions_.size();
}

//
// Generate list of functions inside given C/C++ source file.
//
// sourceFile - pointer to SourceFile struct to collect parsed info (OUT).
// fname      - name of C/C++ file to search (IN).
//
// RETURNS: 0 if OK.
//

int CollectFunctionsFromCSource(SourceFile *sourceFile, const char *fname, int privateMode)
{
  DBG_ENTER("CollectFunctionsFromCSource");

  int exitCode = -1;

  int goOn = 1;

  int deep = 0;

  int tokenId     = -1;
  int lastTokenId = -1;

  int eolTriggered = 0;

  vector<FunctionInfo> &fi = sourceFile -> functions_;

  string namespaceAtDeep[MAX_DEEP + 1];
  string token;
  string lastToken;
  string lastComment;
  string lastArgs;

  string lastIdent;
  string lastLastIdent;

  string returnType;
  string args;

  string currentNamespace;

  int privateAtDeep[MAX_DEEP + 1]    = {0};
  int functionIdAtDeep[MAX_DEEP + 1] = {0};

  FunctionInfo fiEntry;

  DBG_MSG("Assigning [%s]...\n", fname);

  sourceFile -> name_ = fname;

  fiEntry.fname_ = fname;

  //
  // Open source file.
  //

  FILE *f = fopen(fname, "rt");

  FAILEX(f == NULL, "ERROR: Cannot open file '%s'.\n", fname);

  //
  // Init functionIdAtDeep[] array.
  // We're not inside any function at the begin.
  //

  for (int i = 0; i < MAX_DEEP; i++)
  {
    functionIdAtDeep[i] = -1;
  }

  //
  // Scan file.
  //

  DEBUG3("CollectFunctionsFromCSource : Initing scanner...\n");

  ScanInit(f);

  while(goOn)
  {
    int visible = 1;

    tokenId = ScanPopToken(token);

    //
    // Hide content when inside private mode.
    //

    if (privateAtDeep[deep] == 1)
    {
      visible = 0;
    }

    //
    // Determine are we inside top level deep in current namespace.
    // For example:
    //
    // namespace X
    // {
    //   // top level for X namespace (level 0)
    //
    //   void f()
    //   {
    //     // level 1
    //     struct MyData =
    //     {
    //       // level 2
    //     }
    //   }
    // }
    //
    //
    //

    int isTopLevel = 0;

    if (deep == 0 || !namespaceAtDeep[deep].empty())
    {
      isTopLevel = 1;
    }

    DBG_MSG("[%d][%s][%s]\n", deep, GetTokenName(tokenId), token.c_str());

    if (!args.empty())
    {
      args += token;
    }

    switch(tokenId)
    {
      //
      // End of file.
      //

      case TOKEN_EOF:
      {
        goOn = 0;

        break;
      }

      //
      // TOKEN_IDENT
      //

      case TOKEN_IDENT:
      {
        lastLastIdent = lastIdent;
        lastIdent     = token;

        break;
      }

      //
      // Single EOL.
      // Reset comments.
      //

      case TOKEN_EOL:
      {
        eolTriggered = 1;

        break;
      }

      //
      // Comment. Collect comment.
      //

      case TOKEN_COMMENT:
      {
        if (eolTriggered)
        {
          eolTriggered = 0;

          lastComment.clear();
        }

        if (lastTokenId == TOKEN_COMMENT
                || lastTokenId == TOKEN_WHITE
                    || lastTokenId == TOKEN_EOL)
        {
          lastComment += "\n";
          lastComment += ' ';
          lastComment += token.substr(2);
        }
        else
        {
          lastComment += ' ';
          lastComment += token.substr(2);
        }

        break;
      }

      //
      // '('.
      // Probably begin of function arguments.
      //

      case TOKEN_PARENT_BEGIN:
      {
        if (isTopLevel)
        {
          if (fiEntry.return_.empty() && fiEntry.name_.empty())
          {
            fiEntry.return_ = returnType;
            fiEntry.name_   = lastToken;

            DBG_MSG("Argument's begin. Saved identifier [%s].\n", lastToken.c_str());
            DBG_MSG("Argument's begin. Saved return [%s].\n", returnType.c_str());

            returnType = "";
            args       = "(";
          }
          else
          {
            DBG_MSG("Skipped false function args. Probably constructor init list found.\n");
          }
        }

        break;
      }

      //
      // ')'.
      // End of arguments.
      //

      case TOKEN_PARENT_END:
      {
        DBG_MSG("Argument's end. Saved args [%s].\n", args.c_str());

        if (fiEntry.args_.empty())
        {
          fiEntry.args_ = args;
        }

        args.clear();

        break;
      }

      //
      // Block begin.
      // If found in global space it's probably function declaration.
      //

      case TOKEN_BLOCK_BEGIN:
      {
        //
        // Namespace body begin.
        //

        if (lastLastIdent == "namespace")
        {
          DBG_MSG("Entered namespace [%s].\n", lastIdent.c_str());

          namespaceAtDeep[deep + 1] = lastIdent;

          currentNamespace = lastIdent;
        }

        //
        // Function body begin.
        // Function can be started at top level in given namespace.
        //

        else if (isTopLevel && fiEntry.name_.empty() == 0 && fiEntry.args_.empty() == 0)
        {
          if (visible)
          {
            fiEntry.comment_ = StrEncodeToHtml(lastComment, STR_HTML_NON_BREAK_SPACES);

            //
            // Push recognized function to list.
            //

            DBG_MSG("Pushed function [%s][%s][%s]...\n",
                        fiEntry.return_.c_str(), fiEntry.name_.c_str(),
                            fiEntry.args_.c_str());

            fi.push_back(fiEntry);

            functionIdAtDeep[deep + 1] = fi.size() - 1;
          }

          //
          // Clear function's description struct.
          // We're collecting another function from zero.
          //

          lastComment.clear();

          fiEntry.return_.clear();
          fiEntry.args_.clear();
          fiEntry.comment_.clear();
          fiEntry.name_.clear();
        }

        deep ++;

        returnType = "";

        break;
      }

      //
      // '}'.
      //
      // Control deep level. Function declarations are possible
      // only in deep=0 (global space) or in deep=1 as inline class/struct.
      //

      case TOKEN_BLOCK_END:
      {
        lastComment.clear();
        returnType.clear();

        functionIdAtDeep[deep] = -1;

        if (!namespaceAtDeep[deep].empty())
        {
          DBG_MSG("Leaved namespace [%s].\n", namespaceAtDeep[deep].c_str());

          namespaceAtDeep[deep].clear();
        }

        for (int i = deep; i < MAX_DEEP; i++)
        {
          privateAtDeep[i] = 0;
        }

        deep --;

        break;
      }

      //
      // ';' or C macro.
      //

      case TOKEN_SEMICOLON:
      case TOKEN_MACRO:
      {
        DBG_MSG2("[TOKEN_SEMICOLON]: Clearing return type...\n");

        fiEntry.name_.clear();
        fiEntry.args_.clear();
        fiEntry.return_.clear();

        args.clear();
        returnType.clear();
        lastComment.clear();

        if (tokenId == TOKEN_MACRO
                && strstr(token.c_str(), "#pragma")
                    && strstr(token.c_str(), "qcbuild_"))
        {
          QCBuildPragma pragmaInfo;

          FAIL(AutoDocParseQCBuildPragma(&pragmaInfo, token));

          //
          // Dispatch to proper command.
          //

          if (pragmaInfo.cmd_ == "qcbuild_set_private")
          {
            //
            // qcbuild_set_private(bool)
            // Mark current namespace as private (not visible in public doc).
            // If called inside global space it makes whole source file
            // unvisible for public.
            //

            if ((pragmaInfo.args_.size() == 1) && (pragmaInfo.args_[0] == "1"))
            {
              for (int i = deep; i < MAX_DEEP; i++)
              {
                privateAtDeep[i] = 1;

                //
                // Remove current function if any.
                //

                if ((functionIdAtDeep[deep] != -1)
                        && (functionIdAtDeep[deep] == (fi.size() - 1)))
                {
                  FunctionInfo fiEntry = fi[fi.size() - 1];

                   DBG_MSG("Rolled back function [%s][%s][%s]...\n",
                               fiEntry.return_.c_str(), fiEntry.name_.c_str(),
                                   fiEntry.args_.c_str());

                   fi.resize(fi.size() - 1);
                }
              }
            }
          }
          else if (pragmaInfo.cmd_ == "qcbuild_set_file_title")
          {
            //
            // qcbuild_set_file_title(title)
            // Set title for current source file.
            //

            if (pragmaInfo.args_.size() == 1)
            {
              sourceFile -> title_ = pragmaInfo.args_[0];

              DBG_MSG("Set file title to [%s] for file [%s].\n",
                          sourceFile -> title_.c_str(), fname);
            }
          }
          else
          {
            Fatal("ERROR: Unknown pragma command at '%s'.\n", token.c_str());
          }
        }

        break;
      }
    }

    //
    // Collect return type before function prototype.
    //

    if (lastTokenId != TOKEN_COMMENT
            && lastTokenId != TOKEN_SEMICOLON
                && tokenId != TOKEN_SEMICOLON
                    && lastTokenId != TOKEN_BLOCK_END
                        && lastTokenId != TOKEN_BLOCK_BEGIN
                            && lastTokenId != TOKEN_MACRO)
    {
      returnType += lastToken;
    }

    //
    // Save last token.
    //

    lastToken   = token;
    lastTokenId = tokenId;
  }

  //
  // Generate title from source file name in public mode.
  //

  if (privateMode == 0 && sourceFile -> title_.empty())
  {
    int dotIdx = sourceFile -> name_.find('.');

    if (dotIdx != string::npos)
    {
      sourceFile -> title_ = sourceFile -> name_.substr(0, dotIdx);
    }
    else
    {
      sourceFile -> title_ = sourceFile -> name_;
    }
  }

  //
  // Clean up.
  //

  exitCode = 0;

  fail:

  if (exitCode)
  {
    Error("ERROR: Cannot collect functions from '%s' source.\n"
              "Error code is : %d.\n", fname, GetLastError());
  }

  if (f)
  {
    fclose(f);
  }

  DBG_LEAVE("CollectFunctionsFromCSource");

  return exitCode;
}

//
// Generate HTML code to render 2 columns row:
//
// <tr>
//   <td>lvalue</td>
//   <td>rvalue</td>
// </tr>
//
// f      - C stream, where to write generated HTML code (e.g. stdout) (IN).
// lvalue - content for left side column ('legend') (IN).
// rvalue - content for right side column ('legend') (IN).
//

void HtmlPrintRow(FILE *f, const char *lvalue, const char *rvalue)
{
  static int rowSwap = 1;

  string rvalueHtml = StrEncodeToHtml(rvalue);

  fprintf
  (
    f,

    /* Begin of row - put >>row color<< here*/
    "<tr style='background:%s;'>"

      /* First column  - put >>lvalue<< here */
      "<td style='border: 1px solid black; width: 128px;'>"
        "<b>%s</b>"
      "</td>"

      /* Second column - put >>rvalue<< here*/
      "<td style='border: 1px solid black;'>"
        "%s"
      "</td>"

    /* End of row */
    "</tr>",

    rowSwap ? "#ddd" : "#eee",
    lvalue,
    rvalueHtml.c_str()
  );

  rowSwap = !rowSwap;
}

//
// Load content of given file and put it as HTML data
// on given C stream.
// If file doesn't exist no data written to stream.
//
// f     - C stream, where to write HTML data (e.g. stdout) (IN).
// fname - file name to load (e.g. README) (IN).
// title - optional title to put at top of html data (IN/OPT).
//
// RETURNS: 0 if OK.
//

int HtmlPutFile(FILE *f, const char *fname, const char *title = NULL)
{
  int exitCode = -1;

  char *content = NULL;

  string contentHtml;

  //
  // Check args.
  //

  FAILEX(f == NULL, "ERROR: 'f' stream cannot be NULL in HtmlPutFile.\n");
  FAILEX(fname == NULL, "ERROR: 'fname' cannot be NULL in HtmlPutFile.\n");

  //
  // Load file.
  //

  content = FileLoad(fname);

  FAIL(content == NULL);

  //
  // Change CR+LF to LF.
  //

  StrRemoveChar(content, 13);

  //
  // Encode special chars into html.
  //

  contentHtml = content;

  StrReplaceString(contentHtml, "<", "&lt");
  StrReplaceString(contentHtml, ">", "&gt");

  //
  // Put encoded content to output C stream.
  //

  if (title)
  {
    fprintf(f, "<h4>%s</h4>\n", title);
  }

  fprintf(f, "<pre style='%s'>%s</pre>\n", HTML_PRE_STYLE, contentHtml.c_str());

  //
  // Clean up.
  //

  exitCode = 0;

  fail:

  if (content)
  {
    free(content);
  }

  return exitCode;
}

//
// Collect info about every source files inside component.
//
// sourceFiles - generated table with descriptions for every source files
//               inside comopnent. (OUT).
//
// si          - description of one component to analyze (IN).
// privateMode - show private code too if set to 1 (IN).
//
// RETURNS: 0 if OK.
//

int CollectSourceFiles(vector<SourceFile> &sourceFiles, SourceInfo *si, int privateMode)
{
  DBG_ENTER("CollectSourceFiles");

  int exitCode = -1;

  vector<string> files;

  //
  // Tokenize CXXSRC variable into list of source files.
  //

  DEBUG3("CollectSourceFiles : Tokenizing CXXSRC for [%s].\n", si -> get("TITLE"));

  StrTokenize(files, si -> get("CXXSRC"));

  //
  // Collect all functions in every source files.
  //

  DEBUG3("CollectSourceFiles : Parsing source files for [%s].\n", si -> get("TITLE"));

  for (int i = 0; i < files.size(); i++)
  {
    SourceFile sf;

    DEBUG3("CollectSourceFiles : Parsing file [%s].\n", files[i].c_str());

    CollectFunctionsFromCSource(&sf, files[i].c_str(), privateMode);

    sourceFiles.push_back(sf);
  }

  //
  // Sort source files order by number of functions.
  //

  if (sourceFiles.size() > 0)
  {
    DEBUG3("CollectSourceFiles : Sorting source files for [%s].\n", si -> get("TITLE"));

    sort(sourceFiles.begin(), sourceFiles.end(), CompareSourceByNumberOfFunctions);
  }

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  DBG_LEAVE("CollectSourceFiles");

  return exitCode;
}

//
// Generate HTML documentation for given component.
//
// f  - C stream, where to print generated HTML (eg. stdout) (IN).
// si - Source info struct describing given component (IN).
//
// RETURNS: 0 if OK.
//

int GenerateHtmlFromSourceInfo(FILE *f, SourceInfo *si, int privateMode)
{
  int exitCode = -1;

  vector<SourceFile> sourceFiles;

  const char *title = NULL;

  //
  // Check args.
  //

  FAILEX(f == NULL, "ERROR: File stream cannot be NULL in GenerateHtmlFromSourceInfo.\n");
  FAILEX(si == NULL, "ERROR: Source info cannot be NULL in GenerateHtmlFromSourceInfo.\n");

  //
  // Print header table first.
  //

  fprintf(f, "<table style='border: 1px solid black; width: 640px;border-collapse:collapse;'>");
  {
    HtmlPrintRow(f, "Title:",       si -> get("TITLE"));
    HtmlPrintRow(f, "Type:",        si -> get("TYPE"));
    HtmlPrintRow(f, "Author(s):",   si -> get("AUTHOR"));
    HtmlPrintRow(f, "Copyright:",   si -> get("COPYRIGHT"));
    HtmlPrintRow(f, "License:",     si -> get("LICENSE"));
    HtmlPrintRow(f, "Purpose:",     si -> get("PURPOSE"));
    HtmlPrintRow(f, "Description:", si -> get("DESC"));

    if (privateMode)
    {
      HtmlPrintRow(f, "Sources:", si -> get("CXXSRC"));
    }

    HtmlPrintRow(f, "Dependences:", si -> get("DEPENDS"));
    HtmlPrintRow(f, "Used by:    ", si -> get("USED_BY"));
  }
  fprintf(f, "</table>");

  //
  // Collect all functions in every source files.
  //

  FAIL(CollectSourceFiles(sourceFiles, si, privateMode));

  //
  // Generate table of content with list of functions:
  //
  // <a href=...>function1</a>
  // <a href=...>function2</a>
  // <a href=...>function3</a>
  // ...
  //

  if (sourceFiles.size() > 0)
  {
    fprintf(f, "<div style='width:100%%; float: left; margin-bottom: 32px;'>");

    fprintf(f, "<h3>Functions list:</h3>\n");

    title = si -> get("TITLE");

    for (int i = 0; i < sourceFiles.size(); i++)
    {
      vector<FunctionInfo> &fi = sourceFiles[i].functions_;

      if (fi.size() > 0)
      {
        fprintf(f, "<div style='float: left; margin: 4px; padding: 4px; background: #eee;'>");
        {
          //
          // Source file div (container).
          //

          if (privateMode)
          {
            fprintf(f, "<b>%s</b><br/>\n", sourceFiles[i].name_.c_str());

            if (!sourceFiles[i].title_.empty())
            {
              fprintf(f, "<i>(%s)</i><br/>\n", sourceFiles[i].title_.c_str());
            }
          }
          else
          {
            fprintf(f, "<b>%s</b><br/>\n", sourceFiles[i].title_.c_str());
          }

          for (int j = 0; j < fi.size(); j++)
          {
            fprintf(f, "<a href='#%s,%s,%d'>%s</a><br/>\n", title, fi[j].name_.c_str(), i, fi[j].name_.c_str());
          }
        }
        fprintf(f, "</div>");
      }
    }

    fprintf(f, "</div>");
  }

  //
  // Put README file if exists.
  //

  HtmlPutFile(f, "README", "README:");
  HtmlPutFile(f, "TODO", "TODO:");
  HtmlPutFile(f, "CHANGELOG", "CHANGELOG:");

  //
  // Put function's descriptions retrieved from
  // comments before every function.
  //

  if (sourceFiles.size() > 0)
  {
    fprintf(f, "<h3>Functions protos:</h3>\n");

    for (int i = 0; i < sourceFiles.size(); i++)
    {
      vector<FunctionInfo> &fi = sourceFiles[i].functions_;

      if (fi.size() > 0)
      {
        const char *fname = sourceFiles[i].name_.c_str();

        fprintf(f, "<a name='%s,%s'></a><h3>%s</h3>\n", title, fname, fname);

        for (int j = 0; j < fi.size(); j++)
        {
          //
          // Function name.
          //

          fprintf(f, "<a name='%s,%s,%d'></a>\n", title, fi[j].name_.c_str(), i);

          //
          // HTML tag to link it from table of content.
          //

          fprintf(f, "<h3><a href='#%s,%s,%d'>%s</a></h3>\n", title, fi[j].name_.c_str(), i, fi[j].name_.c_str());

          //
          // Function prototype.
          //

          fprintf(f, "<pre style='%s'>%s</pre>",
                      HTML_PRE_STYLE, AutoDocFormatFunctionProto(&fi[j]).c_str());

          //
          // Function description retrieved from comment in source code.
          //

          if (fi[j].comment_.size() > 16)
          {
            string comment = fi[j].comment_;

//            comment = StrReplaceString(comment, "TIP", "<img src='tip.png' style='padding-top: 16px; width: 16px'/><span style='color:green'>TIP</span>");
//            comment = StrReplaceString(comment, "WARNING", "<img src='warning.png' style='padding-top: 16px; width: 16px'/><span style='color:red'>WARNING</span>");

            fprintf(f, "<pre style='%s'>%s</pre>\n", HTML_PRE_STYLE, comment.c_str());
          }
          else
          {
            fprintf(f, "<pre style='%s'><p style='color: #f00;'>Don't be sucker! Describe your function in source code.</p></pre>", HTML_PRE_STYLE);
          }

          //
          // Horizontal line before next function.
          //

          fprintf(f, "<hr/>");
        }
      }
    }
  }

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  if (exitCode)
  {
    Error("ERROR: Cannot generate html documentation.\n");
  }

  return exitCode;
}

//
// Generate HTML table of content for given component.
// Used to generate menu on the left side, where are
// links to components and functions inside components.
//
// It may generate list of links eg:
//
// * LibFile
//   * FileLoad
//   * FileSave
//   * ...
//
//
// f  - C stream, where to print generated HTML (eg. stdout) (IN).
// si - Source info struct describing given component (IN).
//
// RETURNS: 0 if OK.
//

int GenerateHtmlTableOfContent(FILE *f, SourceInfo *si, int privateMode)
{
  DBG_ENTER("GenerateHtmlTableOfContent");

  int exitCode = -1;

  vector<SourceFile> sourceFiles;

  const char *title = si -> get("TITLE");

  //
  // Collect source files inside project.
  //

  FAIL(CollectSourceFiles(sourceFiles, si, privateMode));

  //
  // Generate table of content.
  //

  fprintf(f, "<div id='TOC_%s' style='display:none;'><ul>", title);

  for (int i = 0; i < sourceFiles.size(); i++)
  {
    const char *fname = sourceFiles[i].name_.c_str();

    vector<FunctionInfo> &fi = sourceFiles[i].functions_;

    fprintf(f, "<li><a target='content' href='content.html#%s,%s'>%s</a></li>\n", title, fname, fname);

    fprintf(f, "<ul>");

    for (int j = 0; j < fi.size(); j++)
    {
      fprintf(f, "<li><a target='content' href='content.html#%s,%s,%d'>%s</a><br/>\n", title, fi[j].name_.c_str(), i, fi[j].name_.c_str());
    }

    fprintf(f, "</ul>");
  }

  fprintf(f, "</ul></div>");

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  DBG_LEAVE("GenerateHtmlTableOfContent");

  return exitCode;
}

//
// Generate documentation for whole project and put it to 'doc' directory.
//
// privateMode - show internal data too if set to 1 (IN).
//
// RETURNS: 0 if OK.
//

int AutoDocGenerate(int privateMode)
{
  int exitCode = -1;

  SourceInfo proj;

  vector<SourceInfo> comps;

  map<string, vector<SourceInfo> > compsInSubdir;

  map<string, vector<SourceInfo> >::iterator it;

  char currentDir[MAX_PATH] = {0};

  FILE *f    = NULL;
  FILE *ftoc = NULL;

  const char *docRootPath        = NULL;
  const char *docTocHtmlPath     = NULL;
  const char *docContentHtmlPath = NULL;
  const char *docIndexHtmlPath   = NULL;

  int skipExamples = 0;

  //
  // Save current directory.
  //

  FileGetDir(currentDir, sizeof(currentDir));

  //
  // Load all components from tree.
  //

  FAIL(ListComponents(&proj, comps));

  //
  // Assume directories depending on private/public mode.
  //

  if (privateMode)
  {
    docRootPath        = "doc/private";
    docTocHtmlPath     = "doc/private/toc.html";
    docContentHtmlPath = "doc/private/content.html";
    docIndexHtmlPath   = "doc/private/index.html";
  }
  else
  {
    docRootPath        = "doc/public";
    docTocHtmlPath     = "doc/public/toc.html";
    docContentHtmlPath = "doc/public/content.html";
    docIndexHtmlPath   = "doc/public/index.html";
  }

  //
  // Remove private components in public mode.
  //

  skipExamples = proj.getInt("AUTODOC_SKIP_EXAMPLES");

  if (privateMode == 0)
  {
    for (vector<SourceInfo>::iterator it = comps.begin(); it != comps.end(); )
    {
      if ((it -> getInt("AUTODOC_PRIVATE") == 1)
              || proj.doesMatchPrivateDir(&(*it))
                  || (skipExamples && stristr(it -> get("TITLE"), "example")))
      {
        printf("Skipped private component '%s'.\n", it -> get("TITLE"));

        it = comps.erase(it);
      }
      else
      {
        it ++;
      }
    }
  }

  //
  // Create 'doc' subdirectory if not exists.
  // We will store all docuimentation here.
  //

  #ifdef WIN32
  mkdir("doc");
  mkdir(docRootPath);
  #else
  mkdir("doc", S_IRWXU | S_IRGRP | S_IXGRP);
  mkdir(docRootPath, S_IRWXU | S_IRGRP | S_IXGRP);
  #endif

  //
  // Table of content on left frame.
  //

  f = fopen(docTocHtmlPath, "wt+");

  FAILEX(f == NULL, "ERROR: Cannot create 'doc/toc.html' file.\n");

  fprintf(f, "<html><body style='max-width:640px'>\n");

  //
  // Javascript code to open/close toc subfolders.
  //

  fprintf
  (
    f,
    "<script>"
      "function TocToggle(id)"
      "{"
        "if (document.getElementById('TOC_'+id).style.display == 'none')"
        "{"
          "document.getElementById('TOC_'+id).style.display = 'block';"
          "document.getElementById('TOC_PLUS_'+id).innerHTML = '[-]';"
        "}"
        "else"
        "{"
          "document.getElementById('TOC_'+id).style.display = 'none';"
          "document.getElementById('TOC_PLUS_'+id).innerHTML = '[+]';"
        "}"
      "}"
    "</script>\n"
  );

  //
  // Project title.
  //

  fprintf(f, "<h1>%s Project</h1>\n", proj.get("TITLE"));

  //
  // Main components first.
  //

  fprintf(f, "<h2><a target='content' href='content.html#chap_components_list'>I. Overview</a></h2>\n");
  fprintf(f, "<h2><a target='content' href='content.html#chap_components_diagram'>II. Diagram</a></h2>\n");
  fprintf(f, "<h2><a target='content' href='content.html#chap_components_specification'>III. Specifications</a></h2>\n");

  fprintf(f, "<h3>Main components:</h3>\n");

  fprintf(f, "<ul>\n");

  for (int i = 0; i < comps.size(); i++)
  {
    if (strstr(comps[i].get("TITLE"), "example") == NULL)
    {
      FileSetDir(currentDir);

      FileSetDir(comps[i].get("COMP_DIR"));

      fprintf
      (
        f,
        "<hr/>"
        "<li>"
          "<a style='color:blue; cursor:pointer;' id='TOC_PLUS_%s' onclick=\"TocToggle('%s\')\">[+]</a>"
          "<a target='content' href='content.html#comp_%s'>%s</a>"
        "</li>\n",
        comps[i].get("TITLE"),
        comps[i].get("TITLE"),
        comps[i].get("TITLE"),
        comps[i].get("TITLE")
      );

      GenerateHtmlTableOfContent(f, &comps[i], privateMode);
    }
  }

  fprintf(f, "</ul>");

  //
  // Examples after.
  //

  fprintf(f, "<ul>\n");
  fprintf(f, "<h3>Examples:</h3>\n");

  for (int i = 0; i < comps.size(); i++)
  {
    if (strstr(comps[i].get("TITLE"), "example"))
    {
      FileSetDir(currentDir);

      FileSetDir(comps[i].get("COMP_DIR"));

      fprintf(f, "<hr/><li><a target='content' href='content.html#comp_%s'>%s</a></li>\n",
                  comps[i].get("TITLE"), comps[i].get("TITLE"));

      GenerateHtmlTableOfContent(f, &comps[i], privateMode);
    }
  }

  fprintf(f, "</ul>");
  fprintf(f, "</body></html>");

  FileSetDir(currentDir);

  fclose(f);

  //
  // Dump content.
  //

  f = fopen(docContentHtmlPath, "wt+");

  FAILEX(f == NULL, "ERROR: Cannot create 'doc/content.html' file.\n");

  fprintf(f, "<html><body>");

  fprintf(f, "<p style='color: #f00;'>"
             " DO NOT EDIT! This file was generated automatically"
             " by 'qcbuild --makedoc' tool.\n</p>");

  fprintf(f, "<h1>%s project - technical documentation</h1>\n", proj.get("TITLE"));

  //
  // Table of content on TOP.
  //

  fprintf(f, "<h2>Table of content:<h2>");

  fprintf(f, "<ul>");
    fprintf(f, "<li><a href='#chap_components_list'>I. Overview</a></li>");
    fprintf(f, "<li><a href='#chap_components_diagram'>II. Components diagram</a></li>");
    fprintf(f, "<li><a href='#chap_components_specification'>III. Components specification</a></li>");
  fprintf(f, "</ul>");

  //
  // Segregate components in order to most top subdir e.g.
  // Lib, Client, Server etc.
  //

  for (int i = 0; i < comps.size(); i++)
  {
    compsInSubdir[comps[i].get("TOP_SUBDIR")].push_back(comps[i]);
  }

  if (compsInSubdir.size() == 1)
  {
    DBG_MSG("All sources are inside single directory. Going one level deeper...");

    //
    // All modules are inside single directory.
    // Go one level down.
    //

    compsInSubdir.clear();

    for (int i = 0; i < comps.size(); i++)
    {
      string subTop = FilePathGetDirAtDeep(comps[i].get("COMP_DIR"), 1);

      compsInSubdir[subTop].push_back(comps[i]);
    }
  }

  //
  // I. List of components.
  // Print list of components grouped by most top subdir,
  // but exclude example comopnents.
  //

  fprintf(f, "<a name='chap_components_list'></a>");
  fprintf(f, "<h2>I. Overview:</h2>");
  fprintf(f, "<div style='width:100%%; float: left; margin-bottom: 32px;'>");

  for (it = compsInSubdir.begin(); it != compsInSubdir.end(); it ++)
  {
    vector<SourceInfo> &comps = it -> second;

    fprintf(f, "<div style='float: left; margin: 4px; padding: 4px; background: #eee;'>");

    fprintf(f, "<b>%s</b></b><br/>\n", it -> first.c_str());

    for (int i = 0; i < comps.size(); i++)
    {
      if (strstr(comps[i].get("TITLE"), "example") == NULL)
      {
        fprintf(f, "<a href='#comp_%s'>%s</a><br/>\n",
                    comps[i].get("TITLE"), comps[i].get("TITLE"));
      }
    }

    fprintf(f, "</div>");
  }

  fprintf(f, "</div>\n");

  //
  // Print list of examples grouped by most top subdir,
  //

  fprintf(f, "<div style='width:100%%; float: left; margin-bottom: 32px;'>");
  fprintf(f, "<h3>Examples:</h3>");

  for (it = compsInSubdir.begin(); it != compsInSubdir.end(); it ++)
  {
    vector<SourceInfo> &comps = it -> second;

    fprintf(f, "<div style='float: left; margin: 4px; padding: 4px; background: #eee;'>");

    fprintf(f, "<b>%s</b></b><br/>\n", it -> first.c_str());

    for (int i = 0; i < comps.size(); i++)
    {
      if (strstr(comps[i].get("TITLE"), "example"))
      {
        fprintf(f, "<a href='#comp_%s'>%s</a><br/>\n",
                    comps[i].get("TITLE"), comps[i].get("TITLE"));
      }
    }

    fprintf(f, "</div>");
  }

  fprintf(f, "</div>\n");

  //
  // II. Components diagram.
  //

  fprintf(f, "<a name='chap_components_diagram'></a>");
  fprintf(f, "<h2>II. Components diagram:</h2>");

  FAIL(CreateComponentDiagram(f, comps));

  //
  // III. Generate descriptions for every component.
  //

  fprintf(f, "<a name='chap_components_specification'></a>");
  fprintf(f, "<h2>III. Components specifications:</h2>");

  //
  // Dump components specifications.
  //

  for (int i = 0; i < comps.size(); i++)
  {
    printf("Processing '%s'...\n", comps[i].get("TITLE"));

    //
    // Dump component.
    //

    FileSetDir(currentDir);

    FileSetDir(comps[i].get("COMP_DIR"));

    fprintf(f, "<a name='comp_%s'></a>", comps[i].get("TITLE"));
    fprintf(f, "<h2>%s component</h2>", comps[i].get("TITLE"));

    GenerateHtmlFromSourceInfo(f, &comps[i], privateMode);
  }

  //
  // End content.
  //

  fprintf(f, "</body></html>");

  fclose(f);

  //
  // Restore project's root.
  //

  FileSetDir(currentDir);

  //
  // Main index.html including two frames:
  //
  // - toc.html on left
  // - content.html on right
  //

  f = fopen(docIndexHtmlPath, "wt+");

  FAILEX(f == NULL, "ERROR: Cannot create 'doc/index.html' file.\n");

  fprintf
  (
    f,
    "<html>"
      "<head>"
        "<meta http-equiv='content-type' content='text/html; charset=iso-8859-1'>"
      "</head>"
      "<frameset cols='30%%, *'>"
        "<frame src='toc.html' name='toc'>"
        "<frame src='content.html' name='content'>"
      "</frameset>"
    "</html>"
  );

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  return exitCode;
}
