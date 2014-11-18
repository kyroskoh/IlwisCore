#include "kernel.h"
#include "simpledrawer.h"
#include "rootdrawer.h"

using namespace Ilwis;
using namespace Geodrawer;

SimpleDrawer::SimpleDrawer(const QString& nme, DrawerInterface *parentDrawer, RootDrawer *rootdrawer) : BaseDrawer(nme, parentDrawer, rootdrawer)
{
}

bool SimpleDrawer::isSimple() const
{
    return true;
}

std::vector<DrawPosition> &SimpleDrawer::drawPositions()
{
    return _positions;
}

std::vector<DrawColor> &SimpleDrawer::drawColors()
{
    return _colors;
}

