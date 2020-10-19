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
// Purpose: Pop single chars from given C stream and tokenize them
//          into known token (e.g. number, indentifier, C macro,
//          C comment etc.).
//
//

#include "Scan.h"

using namespace Tegenaria;

//
// Global variables.
// Internal use only.
//

static int ScanBuf = 0;

static FILE *ScanFile = stdin;

//
// Initialize scanner. MUST be called once time before
// any ScanXXX function used.
//
// f - pointer to opened C stream file (IN).
//

void ScanInit(FILE *f)
{
  ScanFile = f;
  ScanBuf  = fgetc(ScanFile);
}

//
// Pop next byte from stream.
//
// RETURNS: Next character in scanner stream or
//          value < 0 if EOF reached.
//

const int ScanPop()
{
  int ret = ScanBuf;

  ScanBuf = fgetc(ScanFile);

  DBG_MSG3("ScanNextChar : Popped [%c] from stream.\n", ScanBuf);

  return ret;
}

//
// Check next byte in scanner, but do NOT pop it.
//
// RETURNS: Next character in scanner stream or
//          value < 0 if EOF reached.
//

const int ScanPeek()
{
  DBG_MSG3("ScanPeek : Peeked [%c].\n", ScanBuf);

  return ScanBuf;
}

//
// Skip one byte in scanner stream.
//

void ScanEat()
{
  DBG_MSG3("ScanNextChar : Eated [%c] from stream.\n", ScanBuf);

  ScanBuf = fgetc(ScanFile);
}

//
// Convert token ID into human readable string.
// See Scan.h for possible token IDs.
//
// id - one of token's ID defined in Scan.h (IN).
//
// RETURNS: Name of token ID
//          or 'TOKEN_UNKNOWN" if given ID not recognised.
//

const char *GetTokenName(int id)
{
  switch(id)
  {
    case TOKEN_ERROR        : return "TOKEN_ERROR";
    case TOKEN_EOF          : return "TOKEN_EOF";
    case TOKEN_EOL          : return "TOKEN_EOL";
    case TOKEN_NUMBER       : return "TOKEN_NUMBER";
    case TOKEN_CLASS        : return "TOKEN_CLASS";
    case TOKEN_STRUCT       : return "TOKEN_STRUCT";
    case TOKEN_ENUM         : return "TOKEN_ENUM";
    case TOKEN_IDENT        : return "TOKEN_IDENT";
    case TOKEN_MACRO        : return "TOKEN_MACRO";
    case TOKEN_BLOCK_BEGIN  : return "TOKEN_BLOCK_BEGIN";
    case TOKEN_BLOCK_END    : return "TOKEN_BLOCK_END";
    case TOKEN_PARENT_BEGIN : return "TOKEN_PARENT_BEGIN";
    case TOKEN_PARENT_END   : return "TOKEN_PARENT_END";
    case TOKEN_OPERATOR     : return "TOKEN_OPERATOR";
    case TOKEN_COMMENT      : return "TOKEN_COMMENT";
    case TOKEN_WHITE        : return "TOKEN_WHITE";
    case TOKEN_DELIM        : return "TOKEN_DELIM";
    case TOKEN_SEMICOLON    : return "TOKEN_SEMICOLON";
    case TOKEN_STRING       : return "TOKEN_STRING";
    case TOKEN_CHAR         : return "TOKEN_CHAR";
    case TOKEN_INDEX_BEGIN  : return "TOKEN_INDEX_BEGIN";
    case TOKEN_INDEX_END    : return "TOKEN_INDEX_END";
  }

  return "TOKEN_UNKNOWN";
}

//
// Pop next token from stream.
// We pop next bytes from stream until we recognize one of
// known token. For list of known tokens see Scan.h.
//
// token - reference to C++ string, where to store retrieved token
//         (e.g. '#define N 1234') (OUT).
//
// RETURNS: ID of recognized token (e.g. TOKEN_MACRO)
//          or TOKEN_ERROR if error.
//

int ScanPopToken(string &token)
{
  DBG_ENTER3("ScanPopToken");

  int tokenId = TOKEN_ERROR;

  int goOn = 1;

  token.clear();

  while(goOn)
  {
    int c = ScanPeek();

    switch(c)
    {
      //
      // EOF.
      //

      case -1:
      case 0:
      {
        DBG_MSG2("[EOF]\n");

        tokenId = TOKEN_EOF;

        goOn = 0;

        break;
      }

      //
      // White and control.
      // Ignore.
      //

      case 1  ... 9:
      case 11 ... 12:
      case 14 ... 32:
      {
        DBG_MSG2("[TOKEN_WHITE]");

        tokenId = TOKEN_WHITE;

        while(ScanPeek() > 0 && ScanPeek() <= 32 && ScanPeek() != 13 && ScanPeek() != 10)
        {
          ScanEat();
        }

        token = " ";

        goOn = 0;

        break;
      }

      //
      // End of line.
      // Ignore.
      //

      case 10:
      case 13:
      {
        DBG_MSG2("[TOKEN_EOL]");

        tokenId = TOKEN_EOL;

        if (c == 13 && ScanPeek() == 10)
        {
          ScanEat();
        }

        ScanEat();

        token = " ";

        goOn = 0;

        break;
      }

      case ',':
      {
        DBG_MSG2("[TOKEN_DELIM]");

        tokenId = TOKEN_DELIM;

        token = ScanPop();

        goOn = 0;
      }

      //
      // Identifier.
      //

      case 'a'...'z':
      case 'A'...'Z':
      case '_':
      {
        DBG_MSG2("[IDENT]\n");

        tokenId = TOKEN_IDENT;

        while(isalnum(ScanPeek()) || ScanPeek() == '_' || ScanPeek() == ':' || ScanPeek() == '~')
        {
          token += ScanPop();
        }

        goOn = 0;

        break;
      }

      //
      // Number.
      //

      case '0'...'9':
      {
        DBG_MSG2("[NUMBER]\n");

        tokenId = TOKEN_NUMBER;

        while(isdigit(ScanPeek()) || ScanPeek() == '.')
        {
          token += ScanPop();
        }

        goOn = 0;

        break;
      }

      //
      // C Macro.
      // Skip.
      //

      case '#':
      {
        DBG_MSG2("[MACRO]\n");

        int insideMacro = 1;
        int joined      = 0;

        tokenId = TOKEN_MACRO;

        while(insideMacro)
        {
          token += ScanPop();

          switch(ScanPeek())
          {
            case 10:
            case 13:
            {
              if (joined == 0)
              {
                insideMacro = 0;
              }
              else
              {
                joined = 0;
              }

              break;
            }

            case '\\':
            {
              joined = 1;

              break;
            }
          }
        }

        goOn = 0;

        break;
      }

      case '{':
      case '}':
      case '(':
      case ')':
      {
        token = ScanPop();

        goOn = 0;

        switch(c)
        {
          case '{': tokenId = TOKEN_BLOCK_BEGIN;   break;
          case '}': tokenId = TOKEN_BLOCK_END;     break;
          case '(': tokenId = TOKEN_PARENT_BEGIN;  break;
          case ')': tokenId = TOKEN_PARENT_END;    break;
        }

        break;
      }

      case '+':
      case '-':
      case '/':
      case '=':
      case '*':
      case '&':
      case '|':
      case '%':
      case '<':
      case '>':
      {
        goOn = 0;

        token += ScanPop();

        //
        // '//' - One line comment.
        //

        if (c == '/' && ScanPeek() == '/')
        {
          DBG_MSG2("[COMMENT]\n");

          tokenId = TOKEN_COMMENT;

          while(ScanPeek() != 10 && ScanPeek() != 13)
          {
            token += ScanPop();
          }

          if (ScanPeek() == 13)
          {
            ScanEat();

            if (ScanPeek() == 10)
            {
              ScanEat();
            }
          }
          else if (ScanPeek() == 10)
          {
            ScanEat();
          }
        }

        //
        // '/*' - multiple line comment.
        //

        else if (c == '/' && ScanPeek() == '*')
        {
          DBG_MSG2("[LONG_COMMENT]");

          tokenId = TOKEN_COMMENT;

          int insideComment = 1;

          while(insideComment)
          {
            token += ScanPop();

            int len = token.size();

            if (len > 2 && strcmp(&token[len - 2], "*/") == 0)
            {
              insideComment = 0;
            }
          }

          while(ScanPeek() == 10 || ScanPeek() == 13)
          {
            ScanEat();
          }
        }

        //
        // Operator.
        //

        else
        {
          DBG_MSG2("[OPERATOR]\n");

          tokenId = TOKEN_OPERATOR;
        }

        break;
      }

      //
      // ';' - end of instruction.
      //

      case ';':
      {
        DBG_MSG2("[SEMICOLON]\n");

        tokenId = TOKEN_SEMICOLON;

        token = ScanPop();

        goOn = 0;

        break;
      }

      //
      // '"' Begin of string.
      //

      case '"':
      {
        DBG_MSG2("[STRING]\n");

        tokenId = TOKEN_STRING;

        token += ScanPop();

        while(ScanPeek() != '"' || ScanPeek() == '\\')
        {
          if (ScanPeek() == '\\')
          {
            token += ScanPop();
            token += ScanPop();
          }
          else
          {
            token += ScanPop();
          }
        }

        token += ScanPop();

        goOn = 0;

        break;
      }

      //
      // ' Begin of char.
      //

      case '\'':
      {
        DBG_MSG2("[CHAR]\n");

        tokenId = TOKEN_CHAR;

        token += ScanPop();

        while(ScanPeek() != '\'' || ScanPeek() == '\\')
        {
          if (ScanPeek() == '\\')
          {
            token += ScanPop();
            token += ScanPop();
          }
          else
          {
            token += ScanPop();
          }
        }

        token += ScanPop();

        goOn = 0;

        break;
      }

      //
      // Index brackets [].
      //

      case '[':
      {
        DBG_MSG2("[TOKEN_INDEX_BEGIN]\n");

        tokenId = TOKEN_INDEX_BEGIN;

        token += ScanPop();

        goOn = 0;

        break;
      }

      case ']':
      {
        DBG_MSG2("[TOKEN_INDEX_END]\n");

        tokenId = TOKEN_INDEX_END;

        token += ScanPop();

        goOn = 0;

        break;
      }

      default:
      {
        ScanEat();
      }
    }
  }

  fail:

  DBG_LEAVE3("ScanPopToken");

  return tokenId;
}
