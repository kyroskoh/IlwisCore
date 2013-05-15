#ifndef BLOCKITERATOR_H
#define BLOCKITERATOR_H

#include "ilwis.h"
#include "boost/geometry.hpp"

namespace Ilwis {

class BlockIterator;

class GridBlock {
public:
    GridBlock(BlockIterator& biter);
    double& operator()(quint32 x, quint32 y, quint32 z=0);
private:
    BlockIterator& _iterator;
    std::vector<qint32> _internalBlockNumber;
    std::vector<qint32> _offsets;
};

class BlockIterator : private PixelIterator {
public:
    friend class GridBlock;

    BlockIterator( IGridCoverage raster, const Size& sz, const Box3D<>& box=Box3D<>() , double step=rUNDEF);

    GridBlock& operator*() {
        return _block;
    }
    BlockIterator& operator++();
    BlockIterator& operator--();
private:
    GridBlock _block;
    Size _blocksize;
    double _outside=rILLEGAL;

};


}

#endif // BLOCKITERATOR_H
