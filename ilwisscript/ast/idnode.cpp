#include <QRegExp>
#include "ilwis.h"
#include "kernel.h"
#include "catalog.h"
#include "mastercatalog.h"
#include "ilwiscontext.h"
#include "symboltable.h"
#include "astnode.h"
#include "idnode.h"


IDNode::IDNode(char *name) : _text(name), _isreference(false)
{
}

void IDNode::setType(int ty)
{
    _type = ty;
}

QString IDNode::nodeType() const
{
    return "ID";
}

quint64 IDNode::type() const {
    return _type;
}

QString IDNode::id() const {
    return _text;
}

bool IDNode::isReference() const {
    return _isreference;
}


bool IDNode::evaluate(SymbolTable& symbols, int scope) {


    bool ok;
    symbols.get(id(), scope, ok);
    if ( ok) {
        _isreference = true;
        return true;
    }

    _type = Ilwis::IlwisObject::findType(id());
    if ( _type != itUNKNOWN){
        _text = Ilwis::context()->workingCatalog()->resolve(_text);
        return true;
    }

    // see if it is an old style reference
    _type = tentativeFileType(".mpr");
    if ( _type != itUNKNOWN)
        return true;
    _type = tentativeFileType(".tbt");
    if ( _type != itUNKNOWN)
        return true;

    return false;
}

IlwisTypes IDNode::tentativeFileType(const QString& ext) const{
    QString name = id() + ext;
    if ( !name.contains(QRegExp("\\\\|/"))) {
        name = Ilwis::context()->workingCatalog()->resolve(name);
    }
    QFileInfo inf(name);
    if ( inf.exists())
        return Ilwis::IlwisObject::name2Type(name);

    return itUNKNOWN;

}
