#include <QString>
#include <QVector>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

#include "kernel.h"
#include "ilwis.h"
#include "angle.h"
#include "point.h"
#include "box.h"
#include "ilwisobject.h"
#include "ilwisdata.h"
#include "ellipsoid.h"
#include "geodeticdatum.h"
#include "projection.h"
#include "domain.h"
#include "numericrange.h"
#include "numericdomain.h"
#include "coordinatesystem.h"
#include "valuedefiner.h"
#include "columndefinition.h"
#include "table.h"
#include "containerstatistics.h"
#include "coverage.h"
#include "georeference.h"
#include "connectorinterface.h"
#include "grid.h"
#include "gridcoverage.h"
#include "pixeliterator.h"
#include "resource.h"


quint64 Ilwis::GridBlockInternal::_blockid = 0;

using namespace Ilwis;

GridCoverage::GridCoverage()
{
}

GridCoverage::GridCoverage(const Resource& res) : Coverage(res){

}

GridCoverage::~GridCoverage()
{
}

const IGeoReference& GridCoverage::georeference() const
{
    return _georef;
}

void GridCoverage::setGeoreference(const IGeoReference &grf)
{
    _georef = grf;
    if ( _grid.isNull() == false) { // remove the current grid, all has become uncertain
        _grid.reset(0);

    }
    if ( _georef.isValid()) {
        _georef->compute();
        setCoordinateSystem(grf->coordinateSystem()); // mandatory
    }
}

IlwisTypes GridCoverage::ilwisType() const
{
    return itGRIDCOVERAGE;
}


void GridCoverage::setDomain(const IDomain& dom){
    ValueDefiner::setDomain(dom);

}


void GridCoverage::copyBinary(const IGridCoverage& gc, int index) {
    IGridCoverage gcNew;
    gcNew.set(this);
    Size inputSize =  gc->size();
    Size sz(inputSize.xsize(),inputSize.ysize(), 1);
    gcNew->georeference()->size(sz);
    PixelIterator iterIn(gc, Box3D<>(Voxel(0,0,index), Voxel(inputSize.xsize(), inputSize.ysize(), index + 1)));
    PixelIterator iterOut(gcNew, Box3D<>(Size(inputSize.xsize(), inputSize.ysize(), 1)));
    for_each(iterOut, iterOut.end(), [&](double& v){
         v = *iterIn;
        ++iterIn;
    });
}

bool GridCoverage::storeBinaryData() {
    if (!connector(cmOUTPUT).isNull())
        return connector(cmOUTPUT)->storeBinaryData(this);
    return ERROR1(ERR_NO_INITIALIZED_1,"connector");
}

Size GridCoverage::size() const
{
    if (!_grid.isNull())
        return _grid->size();
    if ( _georef.isValid())
        return _georef->size();

    return Size();

}

void GridCoverage::size(const Size &sz)
{
    // size must always be positive or undefined
    if (sz.xsize() > 0 && sz.ysize() > 0) {
        if (!_grid.isNull())
            _grid->setSize(sz);
        if (_georef.isValid())
            _georef->size(sz);
    }


}





