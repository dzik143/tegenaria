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

#include "Diagram.h"

using namespace std;

struct ComponentBox
{
  string title_;
  string desc_;
  string purpose_;
  string depends_;

  int x_;
  int y_;
  int w_;
  int h_;

  void randomize()
  {
    x_ = rand() % 800;
    y_ = rand() % 600;

    w_ = 150;
    h_ = 32;
  }

  ComponentBox(const char *title, const char *desc,
                   const char *purpose, const char *depends)
  {
    title_   = title;
    desc_    = desc;
    purpose_ = purpose;
    depends_ = depends;

    randomize();
  }

  ComponentBox()
  {
  }

  void render(FILE *f)
  {
    string url = string("#comp_") + title_.c_str();

    SvgRect(f, x_, y_, w_, h_);
    SvgLink(f, x_ + 16, y_ + 16, url.c_str(), title_.c_str(), "blue");
    SvgLine(f, x_, y_ + 32, x_ + w_, y_ + 32);

    //SvgText(x_ + 16, y_ + 64, purpose_.c_str());
  }
};

//
// Sum all distance beetwen every boxes pair.
// Used to set boxes position.
//
// boxes - vector of boxes to analyze (IN).
//
// RETURNS: Sum of distances beeten all pairs in table.
//

double SumDist(vector<ComponentBox *> &boxes)
{
  double sum = 0.0;

  double sumDist = 0.0;

  int count = 0;

  for (int i = 0; i < boxes.size(); i++)
  {
    string depends = boxes[i] -> depends_;

    char *token = strtok(&depends[0], " ");

    while (token)
    {
      for (int j = 0; j < boxes.size(); j++)
      {
        int xi = boxes[i] -> x_;
        int yi = boxes[i] -> y_;

        int xj = boxes[j] -> x_;
        int yj = boxes[j] -> y_;

        if (boxes[j] -> title_ == token)
        {
          sumDist += sqrt((xi-xj)*(xi-xj) + (yi-yj)*(yi-yj));

          count ++;
        }
      }

      token = strtok(NULL, " ");
    }

    for (int j = 0; j < boxes.size(); j++)
    {
      int xi = boxes[i] -> x_;
      int yi = boxes[i] -> y_;

      int xj = boxes[j] -> x_;
      int yj = boxes[j] -> y_;

      int x1 = max(xi, xj);
      int y1 = max(yi, yj);

      int x2 = min(xi + boxes[i] -> w_, xj + boxes[j] -> w_);
      int y2 = min(yi + boxes[i] -> h_, yj + boxes[j] -> h_);

      if (x1 < x2 && y1 < y2)
      {
        sum += fabs(x1 - x2) * fabs(y1 - y2);
      }
    }
  }

  sum += fabs(sumDist / count - 320) * boxes.size() * boxes.size();
  sum += sumDist;

  return sum;
}

//
// Generate SVG code to render diagram of connections
// beetwen all components inside project.
//
// f           - C stream (eg. stdout), where to write generated SVG data (IN).
// si          - list of all components to draw (IN).
//
// centerIndex - index in si[] table containing center element. If specified
//               diagram is generated around one seleected component only.
//               Use -1 to draw all components (IN/OPT).
//
// RETURNS: 0 if OK.
//

int CreateComponentDiagram(FILE *f, vector<SourceInfo> &si, int centerIndex)
{
  srand(time(0));

  vector<ComponentBox *> boxes;

  //
  // Create diagram around one selected component only if
  // center component specified.
  //

  if (centerIndex > 0)
  {
    //
    // Add center element to diagram.
    //

    SourceInfo *center = &si[centerIndex];

    const char *centerTitle   = center -> get("TITLE");
    const char *centerDepends = center -> get("DEPENDS");

    boxes.push_back(new ComponentBox(centerTitle, center -> get("DESC"),
                                         center -> get("PURPOSE"),
                                             centerDepends));

    //
    // Add rest of components, but only if one of below happen:
    //
    // - Component depends on center component.
    // - Component is used by center component.
    //

    for (int i = 0; i < si.size(); i++)
    {
      const char *depends = si[i].get("DEPENDS");
      const char *title   = si[i].get("TITLE");

      //
      // Skip examples.
      //

      if (strstr(title, "-example"))
      {
        continue;
      }

      if (strstr(centerDepends, title) || strstr(depends, centerTitle))
      {
        boxes.push_back(new ComponentBox(title, si[i].get("DESC"),
                                             si[i].get("PURPOSE"), depends));
      }
    }
  }

  //
  // Create diagram boxes for every project's component.
  //
  else
  {
    for (int i = 0; i < si.size(); i++)
    {
      //
      // Skip examples.
      //

      if (strstr(si[i].get("TITLE"), "-example"))
      {
        continue;
      }

      //
      // Add component to diagram.
      //

      boxes.push_back(new ComponentBox(si[i].get("TITLE"), si[i].get("DESC"),
                                           si[i].get("PURPOSE"), si[i].get("DEPENDS")));
    }
  }

  //
  // Localize boxes.
  //

  vector<ComponentBox> best(boxes.size());

  double minDist = 1e32;

  int width  = 1024;
  int height = 768;

  for (int i = 0; i < 512; i++)
  {
    for (int i = 0; i < boxes.size(); i++)
    {
      boxes[i] -> randomize();
    }

    int dist = SumDist(boxes);

    if (dist < minDist)
    {
      minDist = dist;

      for (int i = 0; i < boxes.size(); i++)
      {
        best[i].x_ = boxes[i] -> x_;
        best[i].y_ = boxes[i] -> y_;
      }
    }
  }

  for (int i = 0; i < boxes.size(); i++)
  {
    boxes[i] -> x_ = best[i].x_;
    boxes[i] -> y_ = best[i].y_;
  }

  //
  // Align boxes to left, top corner.
  //

  int minX = 10000;
  int minY = 10000;
  int maxX = 0;
  int maxY = 0;

  for (int i = 0; i < boxes.size(); i++)
  {
    minX = min(boxes[i] -> x_, minX);
    minY = min(boxes[i] -> y_, minY);
  }

  for (int i = 0; i < boxes.size(); i++)
  {
    boxes[i] -> x_ -= minX;
    boxes[i] -> y_ -= minY - 32;

    maxX = max(boxes[i] -> x_ + boxes[i] -> w_, maxX);
    maxY = max(boxes[i] -> y_ + boxes[i] -> h_, maxY);
  }

  //
  // Render arrows.
  //

  SvgBegin(f, maxX + 32, maxY + 32);

  SvgText(f, 16, 16, "This file was automatically generated by qcbuild --diagram.\n");

  for (int i = 0; i < boxes.size(); i++)
  {
    string depends = boxes[i] -> depends_;

    char *token = strtok(&depends[0], " ");

    while (token)
    {
      int found = 0;

      for (int j = 0; found == 0 && j < boxes.size(); j++)
      {
        if (boxes[j] -> title_ == token)
        {
          char color[32];

          int xi = boxes[i] -> x_ + boxes[i] -> w_ / 2;
          int yi = boxes[i] -> y_ + boxes[i] -> h_ / 2;

          int xj = boxes[j] -> x_ + boxes[j] -> w_ / 2;
          int yj = boxes[j] -> y_ + boxes[j] -> h_ / 2;

          snprintf(color, 32, "RGB(%d,%d,%d)", rand()%255, rand() % 255, rand() % 255);

          SvgLine(f, xi, yi, xj, yj, color);
        }
      }

      token = strtok(NULL, " ");
    }
  }

  //
  // Render boxes.
  //

  for (int i = 0; i < boxes.size(); i++)
  {
    boxes[i] -> render(f);
  }

  SvgEnd(f);

  //
  // Clean up.
  //

  for (int i = 0; i < boxes.size(); i++)
  {
    delete boxes[i];
  }

  return 0;
}
