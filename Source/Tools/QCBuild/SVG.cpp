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
// Purpose: Functions to generate SVG vector graphics, ready to
//          embeded on www directly.
//          We use it to generate component diagrams.
//

#include "SVG.h"

//
// Begin SVG document. This function writes to stdout
// header SVG header.
//
// f      - C stream (e.g. stdout), where to write generated SVG data (IN).
// width  - width of SVG document in pixels (IN).
// height - height of SVG document in pixels (IN).
//

void SvgBegin(FILE *f, int width, int height)
{
  fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\\"
             "version=\"1.1\" width=\"%dpx\" height=\"%dpx\">\n",
             width, height);
}

//
// End SVG document. This function writes end of SVG tag
// on stdout.
//
// f - C stream (e.g. stdout), where to write generated SVG data (IN).
//

void SvgEnd(FILE *f)
{
  fprintf(f, "</svg>\n");
}

//
// Generate SVG code to draw rectangle and write it on stdout.
//
// f      - C stream (e.g. stdout), where to write generated SVG data (IN).
// x, y   - position of top, left corner of rectangle in pixels (IN).
// w, h   - width and height of rectangle in pixels (IN).
// color  - html color name (e.g. 'red' or 'RGB(r,g,b)' (IN/OPT).
// border - html border's color (IN/OPT).
//

void SvgRect(FILE *f, int x, int y, int w, int h,
                 const char *color, const char *border)
{
  fprintf(f, "<rect x='%d' y='%d' width='%d' height='%d'style='fill:%s;stroke:%s;' />\n",
             x, y, w, h, color, border);
}

//
// Generate SVG code to draw line beetwen (x1,y1) and (x2,y2) points
// and print it on stdout.
//
// f     - C stream (e.g. stdout), where to write generated SVG data (IN).
// x1,y1 - coordinates of line's begin in pixels (IN).
// x2,y2 - coordinates of line's end in pixels (IN).
// color - html color (e.g. 'red' or 'RGB(r,g,b)' (IN/OPT).
// width - width of line if pixels (IN/OPT).
//

void SvgLine(FILE *f, int x1, int y1, int x2, int y2, const char *color, int width)
{
  fprintf(f, "<line x1='%d' y1='%d' x2='%d' y2='%d' style='stroke:%s;stroke-width:%d' />\n",
             x1, y1, x2, y2, color, width);
}

//
// Generate SVG code to draw text and print it on stdout.
//
// f     - C stream (e.g. stdout), where to write generated SVG data (IN).
// x,y   - coordinates, where to begin to draw in pixels (IN).
// text  - ASCIZ message to write (e.g. 'some text') (IN).
// color - html color name (e.g. 'red' or 'RGB(r,g,b)') (IN/OPT).
//

void SvgText(FILE *f, int x, int y, const char *text, const char *color)
{
  fprintf(f, "<text x='%d' y='%d' fill='%s'>%s</text>\n", x, y, color, text);
}

//
// Generate SVG code to create text link and print it on stdout.
//
// f     - C stream (e.g. stdout), where to write generated SVG data (IN).
// x,y   - coordinates, where to begin to draw in pixels (IN).
// url   - target url (IN).
// text  - ASCIZ message to write (e.g. 'some text') (IN).
// color - html color name (e.g. 'red' or 'RGB(r,g,b)') (IN/OPT).
//

void SvgLink(FILE *f, int x, int y, const char *url, const char *text, const char *color)
{
  fprintf
  (
    f,
    "<a xlink:href='%s'>"
    "<text x='%d' y='%d' fill='%s'>%s</text>"
    "</a>",
    url,
    x,
    y,
    color,
    text
  );
}
