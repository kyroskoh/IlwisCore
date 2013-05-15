#include <QString>
#include <typeinfo>
#include "kernel.h"
#include "range.h"
#include "domainitem.h"
#include "itemrange.h"

using namespace Ilwis;

QHash<QString, CreateItemFunc> ItemRange::_createItem;

DomainItem *ItemRange::create(const QString& type){
    auto iter = _createItem.find(type);
    if ( iter != _createItem.end())
        return iter.value()(type);
    return 0;
}

void ItemRange::addCreateItem(const QString& type, CreateItemFunc func){
    _createItem[type] = func;
}
