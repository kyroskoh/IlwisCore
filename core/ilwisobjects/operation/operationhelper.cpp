#include "kernel.h"
#include "raster.h"
#include "connectorinterface.h"
#include "ilwisoperation.h"

using namespace Ilwis;

OperationHelper::OperationHelper()
{
}

Box3D<qint32> OperationHelper::initialize(const IGridCoverage &inputGC, IGridCoverage &outputGC, const Parameter& parm, quint64 what)
{
    Resource resource(itGRIDCOVERAGE);
    Size sz = inputGC->size();
    Box3D<qint32> box(sz);

    if ( what & itGRIDSIZE) {
        QString range = parm.modifier("range");
        if (range != sUNDEF) {
            box = Box3D<qint32>(range);
            if ( box.zlength() == 0) {
                box.max_corner().z(box.min_corner().z() + 1);
            }
        }
        resource.addProperty("size", IVARIANT(box.size()));
    }
    if ( what & itENVELOPE) {
        if ( box.isNull() || !box.isValid()) {
            sz = inputGC->size();
            box  = Box3D<qint32>(sz);
        }
        Box2D<double> bounds = inputGC->georeference()->pixel2Coord(box);
        resource.addProperty("envelope", IVARIANT(bounds));
    }
    if ( what & itCOORDSYSTEM) {
        resource.addProperty("coordinatesystem", IVARIANT(inputGC->coordinateSystem()));
    }
    if ( what & itGEOREF) {
        if ( box.isNull() || !box.isValid()) {
            sz = inputGC->size();
            box  = Box3D<qint32>(sz);
        }
        if ( sz.xsize() == box.xlength() && sz.ysize() == box.ylength())
            resource.addProperty("georeference", IVARIANT(inputGC->georeference()));
    }
    if ( what & itDOMAIN) {
        resource.addProperty("domain", IVARIANT(inputGC->datadef().domain()));
    }
    resource.prepare();

    outputGC.prepare(resource);
   //mastercatalog()->addItems({resource});

    return box;
}

int OperationHelper::subdivideTasks(const IGridCoverage& gcov, const Box3D<qint32> &bnds, std::vector<Box3D<qint32> > &boxes)
{
    if ( !gcov.isValid() || gcov->size().isNull() || gcov->size().ysize() == 0) {
        return ERROR1(ERR_NO_INITIALIZED_1, "Grid size");
        return iUNDEF;
    }

    int cores = std::min(QThread::idealThreadCount(),gcov->size().ysize());
    if (gcov->size().totalSize() < 10000)
        cores = 1;

    boxes.clear();
    boxes.resize(cores);
    Box3D<qint32> bounds = bnds;
    if ( bounds.isNull())
        bounds = Box3D<qint32>(gcov->size());
    int left = 0; //bounds.min_corner().x();
    int right = bounds.size().xsize();
    int top = bounds.size().ysize();
    int step = bounds.size().ysize() / cores;
    int currentY = 0;

    for(int i=0 ; i < cores; ++i){
        Box3D<qint32> smallBox(Pixel(left, currentY), Pixel(right, std::min(top,currentY + step)) );
        boxes[i] = smallBox;
        currentY = currentY + step + 1 ;
    }
    return cores;
}

