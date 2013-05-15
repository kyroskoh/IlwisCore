#include "kernel.h"
#include "ilwis.h"
#include "ilwisobject.h"
#include "ilwisdata.h"
#include "domain.h"
#include "range.h"
#include "valuedefiner.h"
#include "columndefinition.h"
#include "connectorinterface.h"
#include "basetable.h"
#include "flattable.h"

using namespace Ilwis;

FlatTable::FlatTable()
{
}

FlatTable::FlatTable(const Resource& res) : BaseTable(res){

}

FlatTable::~FlatTable()
{
    _datagrid.clear();
}

bool FlatTable::createTable()
{
    if(!BaseTable::createTable())
        return false;
    for(unsigned int i=0; i < _rows; ++i)
        _datagrid.push_back(std::vector<QVariant>(_columnDefinitionsByIndex.size()));
    return true;
}

bool FlatTable::prepare()
{
    return Table::prepare();
}

bool FlatTable::isValid() const
{
    return Table::isValid();
}

bool FlatTable::addColumn(const QString &name, const IDomain &domain)
{
    bool ok = BaseTable::addColumn(name, domain);
    if(!ok)
        return false;
    for(std::vector<QVariant>& row : _datagrid) {
        row.push_back(QVariant());
    }
    return true;

}

bool FlatTable::addColumn(const ColumnDefinition &def)
{
    bool ok = BaseTable::addColumn(def);
    if(!ok)
        return false;
    for(std::vector<QVariant>& row : _datagrid) {
        row.push_back(QVariant());
    }
    return true;
}

QVariantList FlatTable::column(const QString &nme) const
{
    if (!const_cast<FlatTable *>(this)->initLoad())
        return QVariantList();

    QVariantList data;

    quint32 index = columnIndex(nme);
    if ( !isColumnIndexValid(index))
        return QVariantList();
    for(quint32 i=0; i < _rows; ++i) {
        data << _datagrid[i][index];
    }
    return data;
}

void FlatTable::column(const QString &nme, const QVariantList &vars, quint32 offset)
{
    if (!const_cast<FlatTable *>(this)->initLoad())
        return ;

    quint32 index = columnIndex(nme);
    if ( !isColumnIndexValid(index))
        return ;
    quint32 rec = offset;
    for(const QVariant& var : vars) {
        if ( rec < _rows)
            _datagrid[rec++][index] = var;
        else {
            _datagrid.push_back(std::vector<QVariant>(_columnDefinitionsByIndex.size()));
            _datagrid[rec++][index] = var;
            _rows = _datagrid.size();
        }
    }

}

QVariantList FlatTable::record(quint32 rec) const
{
    if (!const_cast<FlatTable *>(this)->initLoad())
        return QVariantList();
    QVariantList data;

    if ( rec < _rows && _datagrid.size() != 0) {
        for(const QVariant& var : _datagrid[rec])
            data << var;
    }else
        kernel()->issues()->log(TR(ERR_INVALID_RECORD_SIZE_IN).arg(name()),IssueObject::itWarning);
    return data;
}

void FlatTable::record(quint32 rec, const QVariantList &vars, quint32 offset)
{
    if (!const_cast<FlatTable *>(this)->initLoad())
        return ;
    if ( rec >= _rows ) {
        _datagrid.push_back(std::vector<QVariant>(_columnDefinitionsByIndex.size()));
        _rows = _datagrid.size();
        rec = _rows - 1;
    }

    quint32 col = offset;
    for(const QVariant& var : vars) {
        if ( col < _columns)
            _datagrid[rec][col++] = var;
    }

}

QVariant FlatTable::cell(const QString& col, quint32 rec) const {
    if (!const_cast<FlatTable *>(this)->initLoad())
        return QVariantList();

    quint32 index = columnIndex(col);
    if ( !isColumnIndexValid(index))
        return QVariantList();
    if ( rec < _rows)
        return _datagrid[rec][index];
    kernel()->issues()->log(TR(ERR_INVALID_RECORD_SIZE_IN).arg(name()),IssueObject::itWarning);
    return QVariant();
}

void FlatTable::cell(const QString &col, quint32 rec, const QVariant &var)
{
    if (!const_cast<FlatTable *>(this)->initLoad())
        return ;

    quint32 index = columnIndex(col);
    if ( !isColumnIndexValid(index))
        return;
    if ( rec < _rows)
        _datagrid[rec][index] = var;
}

IlwisTypes FlatTable::ilwisType() const
{
    return itFLATTABLE;
}
