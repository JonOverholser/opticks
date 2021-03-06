/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "PointAdapter.h"

using namespace std;

PointAdapter::PointAdapter(PlotViewImp* pPlot, bool bPrimary, bool bQuiet) :
   PointImp(pPlot, bPrimary, bQuiet)
{
}

PointAdapter::PointAdapter(PlotViewImp* pPlot, bool bPrimary, LocationType point, bool bQuiet) :
   PointImp(pPlot, bPrimary, point, bQuiet)
{
}

PointAdapter::PointAdapter(PlotViewImp* pPlot, bool bPrimary, double dX, double dY, bool bQuiet) :
   PointImp(pPlot, bPrimary, dX, dY, bQuiet)
{
}

PointAdapter::~PointAdapter()
{
   notify(SIGNAL_NAME(Subject, Deleted));
}

const string& PointAdapter::getObjectType() const
{
   static string type("PointAdapter");
   return type;
}

bool PointAdapter::isKindOf(const string& className) const
{
   if ((className == getObjectType()) || (className == "Point"))
   {
      return true;
   }

   return PointImp::isKindOf(className);
}
