#include <QString>
#include <QUrl>
#include "kernel.h"
#include "ilwisdata.h"
#include "ilwisobject.h"
#include "domain.h"
#include "catalog.h"
#include "connectorinterface.h"
#include "containerconnector.h"
#include "symboltable.h"
#include "mastercatalog.h"
#include "ilwiscontext.h"
#include "OperationExpression.h"

using namespace Ilwis;

Parameter::Parameter() : _key(sUNDEF), _value(sUNDEF), _type(itUNKNOWN), _domain(sUNDEF) {
}

Parameter::Parameter(const QString &key, const QString &value, IlwisTypes tp, const SymbolTable &symtab) :
    Identity(key),
    _value(value),
    _domain(sUNDEF)
{
    _type = tp;
    if ( _type == itUNKNOWN)
        _type = Parameter::determineType(value, symtab);
}

Parameter::Parameter(const QString &value, IlwisTypes tp, const SymbolTable &symtab) : _domain(sUNDEF)
{
    _value = value;
    _type = tp;
    if ( _type == itUNKNOWN)
        _type = Parameter::determineType(_value, symtab);
}

Ilwis::Parameter::~Parameter()
{
}

QString Parameter::value() const
{
    return _value;
}

QString Parameter::domain() const
{
    return _domain;
}

void Parameter::domain(const QString &dom)
{
    _domain = dom;
}

IlwisTypes Parameter::valuetype() const
{
    return _type;
}

bool Parameter::isEqual(const Parameter &parm) const
{
    return parm.name() == name() && parm.value() == _value;
}

bool Parameter::isValid() const
{
    return !_value.isEmpty();
}

IlwisTypes Parameter::determineType(const QString& value, const SymbolTable &symtab) {
    IlwisTypes tp = IlwisObject::findType(value);
    if ( tp != itUNKNOWN)
        return tp;

    Symbol sym = symtab.getSymbol(value);
    if ( sym.isValid() && sym._type != itUNKNOWN)
        return sym._type;

    QString s = context()->workingCatalog()->resolve(value);
    IlwisTypes type = IlwisObject::findType(s) ;
    if ( type != itUNKNOWN)
        return type;

    if ( value.left(11) == ANONYMOUS_PREFIX) {
        QString sid = value.mid(12);
        bool ok;
        quint64 id = sid.toLongLong(&ok);
        if ( ok) {
            ESPIlwisObject obj =  mastercatalog()->get(id);
            if ( obj.get() != 0)
                return obj->ilwisType();
        }
    }

    tp = Domain::ilwType(value);

    return tp == itUNKNOWN ? itSTRING : tp;

}



//----------------------------------------

OperationExpression::OperationExpression()
{
}

Ilwis::OperationExpression::~OperationExpression()
{
}

OperationExpression::OperationExpression(const QString &e, const SymbolTable &symtab)
{
    setExpression(e, symtab);
}

void OperationExpression::setExpression(const QString &e, const SymbolTable &symtab) {
    _name = "";
    _inParameters.clear();
    _outParameters.clear();
    _type = otFunction;
    QString dummy = e;
    int index1 = e.indexOf(" ");
    int index2 = e.indexOf("=");
    int index3 = e.indexOf("[");
    bool isCommand = index1 != -1 && ( index1 < index2 || index2 == -1);
    if ( isCommand) {
        _type = otCommand;
        for(int i = 0; i < index1; ++i){
            if ( !(e[i].isDigit() || e[i].isLetter() || e[i] == '-')){
                _type = otFunction;
                break;
            }
        }
    }
    else if ( index3 != -1)
        _type = otSelection;

    if ( _type == otFunction) {
        parseFunctionExpression(e, symtab);
    } else if ( _type == otCommand) {
        parseCommandExpression(e, symtab);
    } else if ( _type == otSelection) {
        parseSelectors(e, symtab);
    }
}

void OperationExpression::parseCommandExpression(const QString &e, const SymbolTable &symtab) {
    if ( e.left(6) == "script"){ // special case , all after tyhe script statement is the script text
        _name = "script";
        QString rest = e.mid(7).trimmed();
        _inParameters.push_back(Parameter(rest, itSTRING, symtab));
        return;
    }
    int blockCount = 0;
    int quoteCount = 0;
    int count = 0;
    QList<int> indexes;
    foreach(const QChar& cu, e) {
        char c = cu.toLatin1(); // eessions are not internatiolized, so its allowed
        if ( c == '(' && quoteCount == 0)
            blockCount++;
        if ( c == ')' && quoteCount == 0)
            blockCount--;
        if ( c == '"' && quoteCount == 0)
            quoteCount++;
        else if ( c == '"' && quoteCount != 0)
            quoteCount--;

        if ( c == ' ' && blockCount == 0 && quoteCount == 0)
            indexes.push_back(count++);
        ++count;
    }
    indexes.push_back(count - 1);
    int current = 0;
    foreach(int index, indexes) {
        QString part = e.mid(current, index - current) ;
        part = part.trimmed();
        if ( current == 0)
            _name = part;
        else
            _inParameters.push_back(Parameter(part, itUNKNOWN, symtab));
        current = index;
    }
}

void  OperationExpression::parseSelectors(const QString& e, const SymbolTable &symtab) {
    int index = e.indexOf("[");
    int index2 = e.indexOf("=");
    int index3 = e.indexOf("{");
    QString selectPart = e.mid(index+1, e.size() - index - 2);
    QString inputMap = e.mid(index2+1, index - index2 - 1);
    _inParameters.push_back(Parameter(inputMap, itUNKNOWN, symtab));
    QString outputPart =  index3 == -1 ? e.left(index2) : e.left(index3);
    IlwisTypes valueType =  hasType(_inParameters.back().valuetype(), itCOVERAGE) ? itCOVERAGE : itTABLE;
    _outParameters.push_back(Parameter(outputPart, valueType, symtab));
    int index4=selectPart.indexOf(",");
    if ( index4 == -1) { //either id or number
        bool ok;
        int layer = selectPart.toUInt(&ok);
        if ( ok) {
            _inParameters.push_back(Parameter(QString("\"layer=%1\"").arg(layer),itSTRING,symtab));
        } else {
            _inParameters.push_back(Parameter(QString("\"attribute=%1\"").arg(selectPart), itSTRING, symtab));
        }
    } else {
        QStringList parts = selectPart.split(",");
        if ( parts.size() == 2) {
            _inParameters.push_back(Parameter(QString("\"box=%1\"").arg(selectPart),itSTRING, symtab));
        } else {
           _inParameters.push_back(Parameter(QString("\"polygon=%1\"").arg(selectPart), itSTRING, symtab));
        }
    }
    _name = "selection";

}

void OperationExpression::specialExpressions(const QString &e, const SymbolTable &symtab) {
    //TODO other cases of special expressions
    _name = "assignment";
    _inParameters.push_back(Parameter(e.mid(e.indexOf("=")+1),itCOVERAGE, symtab));

}

void OperationExpression::parseFunctionExpression(const QString &txt, const SymbolTable &symtab) {
    QString e = txt;
    int index = e.indexOf("(");
    if ( index == -1)
        specialExpressions(txt,symtab);

    int index2 = e.indexOf("=");
    if ( index != -1) {
        if ( _name == "") {
            if ( index2 != -1)
                _name = e.mid(index2+1, index - index2 - 1);
            else
                _name = e.left(index);
        }
    }

    QString start = e.left(index2);
    int blockCount = 0;
    int quoteCount = 0;
    int count = 0;
    int cur = 0;
    std::vector<int> indexes;
    if ( index2 != -1) {
        foreach(const QChar& cu, start) {
            char c = cu.toLatin1();
            if ( c == '{' && quoteCount == 0){
                blockCount++;
            }
            if ( c == '}' && quoteCount == 0){
                blockCount--;
            }

            if ( c == ',' || blockCount == 1)
                indexes.push_back(count++);
            ++count;
        }
        qint32 shift = 0;
        indexes.push_back(start.size());
        for(int i =0; i < indexes.size(); ++i) {
            int index3 = indexes[i];
            QString part = start.mid(cur, index3 - cur) ;
            part = part.trimmed();
            Parameter parm(part,itUNKNOWN, symtab);
            _outParameters.push_back(parm);
            shift = 1;
            cur = index3 + shift;
        }
    }
    if (index > 0) {
        int len =  e.size() - index - 2;
        QString rest = e.mid(index + 1 , len);
        //indexes.push_back(0);
        indexes.clear();
        blockCount = quoteCount = count = 0;
        foreach(const QChar& cu, rest) {
            char c = cu.toLatin1(); // eessions are not internatiolized, so its allowed
            if ( c == '(' && quoteCount == 0)
                blockCount++;
            if ( c == ')' && quoteCount == 0)
                blockCount--;
            if ( c == '"' && quoteCount == 0)
                quoteCount++;
            else if ( c == '"' && quoteCount != 0)
                quoteCount--;

            if ( c == ',' && blockCount == 0 && quoteCount == 0)
                indexes.push_back(++count);
            else
                ++count;
        }
        cur = 0;
        indexes.push_back(rest.size() + 1);
        for(int i =0; i < indexes.size(); ++i) {
            int index4 = indexes[i];
            QString part = rest.mid(cur, index4 - cur - 1) ;
            part = part.trimmed();
            _inParameters.push_back(Parameter(part,itUNKNOWN, symtab));
            cur = index4;
        }
    }
}

Parameter OperationExpression::parm(int index, bool in) const
{
    const QList<Parameter>& parameters = in ? _inParameters : _outParameters;
    if ( index < parameters.size())
        return parameters[index];

    return Parameter();
}

Parameter OperationExpression::parm(const QString searchValue, bool caseInsensitive, bool in) const
{
    const QList<Parameter>& parameters = in ? _inParameters : _outParameters;
    if ( !searchValue.isEmpty() ) {
        foreach(const Parameter& parm, parameters) {
            QString k2 = _type == otFunction ? parm.name(): parm.value();
            if ( caseInsensitive){
                QString k1 = searchValue.toLower();
                k2 = k2.toLower();
                if ( k1 == k2)
                    return parm;
            } else {
                if ( k2 == searchValue)
                return parm;
            }
        }
    }
    return Parameter();
}

QString OperationExpression::name(bool caseInsensitive) const
{
    if ( caseInsensitive)
        return _name.toLower();
    return  _name;
}

bool OperationExpression::matchesParameterCount(const QString& match, bool in) const {
    int count = parameterCount(in);
    if ( match.right(1) == "+") {
        QString n = match.left(match.size() - 1);
        return count >= n.toInt();
    }
    QStringList parts = match.split("|");
    foreach(const QString& part, parts) {
        bool ok;
        int index = part.toInt(&ok);
        if (!ok) {
            return ERROR0("Illegal metdata definition");
        }
        if ( index == count)
            return true;
    }
    return false;


}

int OperationExpression::parameterCount(bool in) const
{
    const QList<Parameter>& parameters = in ? _inParameters : _outParameters;

    return parameters.size();
}

bool OperationExpression::isValid() const
{
    return !_name.isEmpty();
}

QUrl Ilwis::OperationExpression::metaUrl(bool simple) const
{
    QString url = "ilwis://operations/" + _name;
    if ( !simple) {
        for(int i=0; i < _inParameters.size(); ++i) {
            if ( i == 0)
                url += "?";
            else
                url += "&";

            url += QString("pin::%1::type=%2").arg(i).arg(_outParameters[i].valuetype());
        }
        for(int i=0; i < _outParameters.size(); ++i) {
            if ( i == 0)
                url += "?";
            else
                url += "&";

            url += QString("pout::%1::type=%2").arg(i).arg(_outParameters[i].valuetype());
        }
    }
    return QUrl(url);

}

QString OperationExpression::toString() const
{
    QString expression;
    if ( _type == otFunction) {
        for(const Parameter& parm : _outParameters) {
            if ( expression != "")
                expression += ", ";
            if ( expression == "" )
                expression += "( ";

            expression += parm.value();

        }
        if ( expression != "")
            expression += " ) = ";
        expression += _name;
        expression += "( ";
        int count = 0;
        for(const Parameter& parm : _inParameters) {
            if ( count++ != 0)
                expression += ", ";
            expression += parm.value();
        }
        expression += ")"    ;
    }
    //TODO other cases

    return expression;
}






