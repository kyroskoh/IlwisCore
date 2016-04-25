#include <QUrl>
#include <QStandardPaths>
#include "dirent.h"
#include "kernel.h"
#include "ilwisdata.h"
#include "domain.h"
#include "catalog.h"
#include "connectorinterface.h"
#include "symboltable.h"
#include "dataformat.h"
#include "mastercatalog.h"
#include "ilwiscontext.h"
#include "operationExpression.h"

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

Parameter::PathType Parameter::pathType() const
{
    if ( _value.indexOf("http://") == 0 || _value.indexOf("https://") == 0){
        return ptREMOTE;
    }
    if ( _value.indexOf("file://") == 0){
        return ptLOCALOBJECT;
    }
    if ( _value.indexOf("ilwis://") == 0){
        return ptLOCALOBJECT;
    }
    if ( _value.indexOf("?") == 0)
        return ptUNDEFINED;

    if ( hasType(_type, (itSTRING | itNUMBER | itBOOL)))
        return ptIRRELEVANT;

    return ptNONE;
}

IlwisTypes Parameter::determineType(const QString& value, const SymbolTable &symtab) {
    IlwisTypes tp = IlwisObject::findType(value);
    if ( value == "\"?\"")
        tp = itANY;

    if ( tp != itUNKNOWN)
        return tp;

    Symbol sym = symtab.getSymbol(value);
    if ( sym.isValid() && sym._type != itUNKNOWN)
        return sym._type;

    QString s = context()->workingCatalog()->resolve(value);
    IlwisTypes type = IlwisObject::findType(s) ;
    if ( type != itUNKNOWN)
        return type;

    quint64 id = IlwisObject::internalname2id(value);
    if ( id != i64UNDEF){
        ESPIlwisObject obj =  mastercatalog()->get(id);
        if ( obj.get() != 0)
            return obj->ilwisType();
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
    _inParametersMap.clear();
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
    //TODO: other cases of special expressions
    _name = "assignment";
    _inParameters.push_back(Parameter(e.mid(e.indexOf("=")+1),itCOVERAGE, symtab));

}

void OperationExpression::parseFunctionExpression(const QString &txt, const SymbolTable &symtab) {
    QString e = txt;
    int index = e.indexOf("(");
    if ( index == -1)
        specialExpressions(txt,symtab);

    int startQuote = e.indexOf("\"");
    int endQuote = e.indexOf("\"", startQuote+1 );
    int index2 = e.indexOf("=");
    if ( (index2 > startQuote && index2 < endQuote) || index2 > index)
        index2 = -1;
    if ( index != -1) {
        if ( _name == "") {
            if ( index2 != -1)
                _name = e.mid(index2+1, index - index2 - 1);
            else
                _name = e.left(index);
        }
        _isRemote = _name.indexOf("http://") != -1;
    }

    QString start = e.left(index2);
    int blockCount = 0;
    int quoteCount = 0;
    int count = 0;
    int cur = 0;
    std::vector<int> outputParams;
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
                outputParams.push_back(count);
            ++count;
        }
        qint32 shift = 0;
        outputParams.push_back(start.size());
        for(int i =0; i < outputParams.size(); ++i) {
            int index3 = outputParams[i];
            QString part = start.mid(cur, index3 - cur) ;
            part = part.trimmed();
            Parameter parm(part,itUNKNOWN, symtab);
            _outParameters.push_back(parm);
            shift = 1;
            cur = index3 + shift;
        }
    }
    std::map<int, int> inputParams;
    int keyword = 0;
    int paramIndex = -1;
    if (index > 0) {
        int len =  e.size() - index - 2;
        QString rest = e.mid(index + 1 , len);
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

            ++count;
            if ( blockCount == 0 && quoteCount == 0) {
                if (c == ','){
                    if (keyword == 0) {
                        inputParams[++paramIndex] = count;
                    } else if (inputParams.size() == 0){
                        inputParams[keyword] = count;
                    } else {
                        //Probleem
                    }
                } else if ( c == '=' ) {
                    QString value = rest.mid(inputParams[paramIndex], rest.size() - count);
                    // ignore '=' in urls, they are part of the url
                    if ( value.indexOf("://") == -1){
                        keyword = count;
                    }

                }
            }

        }
        cur = 0;
        inputParams[keyword == 0 ? ++paramIndex : keyword] = rest.size() + 1;
        int index4 = 0;
        foreach(const auto& input, inputParams) {
            if (keyword > 0) {
                index4 = input.first;
                QString key = rest.mid(cur, index4 - cur - 1);
                key = key.trimmed();
                cur = index4;
                QString value = rest.mid(cur, input.second - cur - 1).trimmed();
                _inParametersMap[key] = Parameter(value, itUNKNOWN, symtab);
            } else {
                QString value = rest.mid(cur, input.second - cur - 1).trimmed();
                _inParameters.insert(input.first, Parameter(value, itUNKNOWN, symtab));
            }
            cur = input.second;
            index4 = 0;
        }
    }
}

Parameter OperationExpression::parm(int index, bool in) const
{
    if (!inputIsKeyword()) {
        const QList<Parameter>& parameters = in ? _inParameters : _outParameters;
        if ( index < parameters.size()) {
                return parameters[index];
        }
    }
    return Parameter();
}

Parameter OperationExpression::parm(const QString searchValue, bool caseInsensitive, bool in) const
{
    if ( !searchValue.isEmpty() ) {
        QString k1 = caseInsensitive ? searchValue.toLower() : searchValue;
        if (inputIsKeyword() && in) {
            if ( caseInsensitive ){
                foreach(const auto& parm, _inParametersMap) {
                    QString k2 = _type == otFunction ? parm.name(): parm.value();
                    k2 = k2.toLower();
                    if ( k1 == k2) {
                        /*if (parm.value() == EXPREMPTYPARAMETER)
                            parm.value("?");*/
                        return parm;
                    }
                }
            } else {
                auto iter = _inParametersMap.find(searchValue);
                if (iter != _inParametersMap.end()) {
                    return (*iter);
                }
            }
        } else {
            const QList<Parameter>& parameters = in ? _inParameters : _outParameters;
            foreach(const Parameter& parm, parameters) {
                QString k2 = _type == otFunction ? parm.name(): parm.value();
                if ( caseInsensitive){
                    k2 = k2.toLower();
                    if ( k1 == k2)
                        return parm;
                } else {
                    if ( k2 == searchValue)
                        return parm;
                }
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
    if (inputIsKeyword() && in) {
        return _inParametersMap.size();
    }
    const QList<Parameter>& parameters = in ? _inParameters : _outParameters;

    return parameters.size();
}

bool OperationExpression::isValid() const
{
    return !_name.isEmpty();
}

bool OperationExpression::isRemote() const
{
    return _isRemote;
}

QUrl Ilwis::OperationExpression::metaUrl(bool simple) const
{
    QString url = _name;
    if ( _name.indexOf("http://") == -1) // names that are already an url are not in the local ilwis://operations,
        url = "ilwis://operations/" + _name;
    if ( !simple) {
        if (inputIsKeyword()){
            for(auto iter = _inParametersMap.begin(); iter != _inParametersMap.end(); iter++) {
                const auto& parm = *iter;
                if (iter == _inParametersMap.begin())
                    url += "?";
                else
                    url += "&";

                url += QString("pin::%1::type=%2").arg(parm.value()).arg(parm.valuetype());
            }
        } else {
            for(int i=0; i < _inParameters.size(); ++i) {
                if ( i == 0)
                    url += "?";
                else
                    url += "&";

                url += QString("pin::%1::type=%2").arg(i).arg(_inParameters[i].valuetype());
            }
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

QString OperationExpression::toString(bool rightsideonly) const
{
    QString expression;
    if ( _type == otFunction) {
        if ( rightsideonly == false){
            for(const Parameter& parm : _outParameters) {
                if ( expression != "")
                    expression += ", ";
                if ( expression == "" )
                    expression += "( ";

                expression += parm.value();
            }
            if ( expression != "")
                expression += " ) = ";
        }
        expression += _name;
        expression += "( ";
        int count = 0;
        if (inputIsKeyword()){
            for(auto iter = _inParametersMap.begin(); iter != _inParametersMap.end(); iter++) {
                const Parameter& parm = *iter;
                if ( count++ != 0)
                    expression += ", ";
                expression += parm.value() + "=" + parm.value();
            }
        } else {
            for(const Parameter& parm : _inParameters) {
                if ( count++ != 0)
                    expression += ", ";
                expression += parm.value();
            }
        }
        expression += ")"    ;
    }
    //TODO: other cases

    return expression;
}

QString OperationExpression::modifyTableOutputUrl(const QString& output, const QStringList& parms)
{
    QString columnName = output;
    QString firstTable = parms[0];
    if ( firstTable.indexOf("://") != -1){
        int index = firstTable.lastIndexOf("/");
        firstTable = firstTable.mid(index + 1);
        index =  firstTable.indexOf(".");
        if ( index != -1)
            firstTable = firstTable.left(index) ;
    }
    QString internalPath = context()->persistentInternalCatalog().toString();
    QString outpath = internalPath + "/" + output;

    return outpath;

}
OperationExpression OperationExpression::createExpression(quint64 operationid, const QString& parameters, bool acceptIncompleteExpressions){
    if (  parameters == "")
        return OperationExpression();

    Resource operationresource = mastercatalog()->id2Resource(operationid);
    if ( !operationresource.isValid())
        return OperationExpression();



    QString expression;
    QStringList parms = parameters.split("|");
    bool hasMissingParameters = false;
    int maxinputparameters = operationresource["inparameters"].toString().split("|").last().toInt();
    for(int i = 0; i < parms.size(); ++ i){
        if (operationresource.ilwisType() & itWORKFLOW){
            int parm = i + 1;
            if (operationresource[QString("pout_%1_optional").arg(parm)] == "false" && i < operationresource["outparameters"].toInt()) {
                QString value = parms[i + operationresource["inparameters"].toInt()];
                if (value.split("@@")[0].size() == 0) {
                    if ( !acceptIncompleteExpressions){
                        kernel()->issues()->log(TR("Output parameter " + QString::number(i) + " is undefined with name " +  operationresource[QString("pout_%1_name").arg(parm)].toString()));
                        hasMissingParameters = true;
                    }
                }
            }
            if (operationresource[QString("pin_%1_optional").arg(parm)] == "false" && i < operationresource["inparameters"].toInt() && parms[i].size() == 0) {
                if ( !acceptIncompleteExpressions){
                    kernel()->issues()->log(TR("Input parameter " + QString::number(i) + " is undefined with name " +  operationresource[QString("pin_%1_name").arg(parm)].toString()));
                    hasMissingParameters = true;
                }
            }
        }
        if(i < maxinputparameters ){
            if ( expression.size() != 0)
                expression += ",";
            expression += (parms[i] == "" ?  "?input_" + QString::number(i+1) : parms[i]);
        }

    }

    if (hasMissingParameters) return OperationExpression();

    QString allOutputsString;

    bool duplicateFileNames = false;

    QStringList parts = operationresource["outparameters"].toString().split("|");
    int maxparms = parts.last().toInt();
    int count = 1;
    for(int i=(parms.size() - maxparms); i<parms.size(); ++i){
        QString output = parms[i];


        QString pout = QString("pout_%1_type").arg(count++);

        IlwisTypes outputtype = operationresource[pout].toULongLong();
        if ( output.indexOf("@@") != -1 ){
            QString format;
            QStringList parts = output.split("@@");
            output = parts[0];
            if ( output == ""){
                if (!acceptIncompleteExpressions)
                    continue;
                else
                    output = "?output_" + QString::number(i);
            }

            //Check if user didnt put the same output name in another output field
            int occurences = 0;
            for(int j=(parms.size() - maxparms); j<parms.size(); ++j){
                QString compareString = parms[j].split("@@")[0];
                if(output == compareString){
                    occurences++;
                }
            }

            //Add the duplicate name to the list of duplicate names
            if(occurences>1){
                duplicateFileNames = true;
                kernel()->issues()->log(TR("Workflow did not execute, multiple occurences of an output name"));
            }

            QString formatName = parts[1];

            if ( operationresource.ilwisType() & itWORKFLOW) {
                QStringList existingFileNames;

                DIR *directory;

                //If not memory
                QString fileName;

                if(formatName == "Memory" ){
                    //Get all files in the internal catalog
                    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/internalcatalog";
                    directory = opendir(dataLocation.toStdString().c_str());
                }else {
                    //Get all files in the directory
                    QString dataLocation = output;
                    dataLocation = QUrl(dataLocation).toLocalFile();

                    QStringList splitUrl = dataLocation.split("/");

                    fileName = splitUrl.last();

                    QString query = "name='" + formatName + "'";
                    std::multimap<QString, Ilwis::DataFormat>  formats = Ilwis::DataFormat::getSelectedBy(Ilwis::DataFormat::fpNAME, query);
                    if ( formats.size() == 1){
                        QString connector = (*formats.begin()).second.property(DataFormat::fpCONNECTOR).toString();
                        QString code = (*formats.begin()).second.property(DataFormat::fpCODE).toString();

                        QVariantList extensions = Ilwis::DataFormat::getFormatProperties(DataFormat::fpEXTENSION,outputtype, connector, code);

                        fileName += ".";
                        fileName += extensions[0].toString();
                    }

                    splitUrl.removeLast();

                    dataLocation = splitUrl.join("/");

                    directory = opendir(dataLocation.toStdString().c_str());
                }

                struct dirent *file;

                //Put the existing file names in a list for later use
                while ((file = readdir (directory)) != NULL) {
                    existingFileNames.push_back(file->d_name);
                }

                closedir(directory);

                //Check if a file with the same name already exist
                for(int j=0;j<existingFileNames.size();++j){
                    if(formatName == "Memory"){
                        if(existingFileNames[j] == output) {
                            duplicateFileNames = true;
                            kernel()->issues()->log(TR("Workflow did not execute duplicate name: " + output + ". Please change this name."));
                        }
                    }else{
                        if(existingFileNames[j] == fileName){
                            duplicateFileNames = true;
                            kernel()->issues()->log(TR("Workflow did not execute duplicate name: " + fileName + ". Please change this name."));
                        }
                    }
                }
            }

            if ( hasType(outputtype, itCOLUMN)){
                if ( formatName == "Memory"){
                    output = modifyTableOutputUrl(output, parms);
                }else
                    output = parms[0] + "[" + output + "]";
            }
            if ( formatName == "Keep original"){
                IIlwisObject obj;
                obj.prepare(parms[0], operationresource["pin_1_type"].toULongLong());
                if ( obj.isValid()){
                    IlwisTypes type = operationresource[pout].toULongLong();
                    QVariantList values = DataFormat::getFormatProperties(DataFormat::fpCODE,type,obj->provider());
                    if ( values.size() != 0){
                        format = "{format(" + obj->provider() + ",\"" + values[0].toString() + "\")}";
                    }else{
                        kernel()->issues()->log(QString("No valid conversion found for provider %1 and format %2").arg(obj->provider()).arg(IlwisObject::type2Name(type)));
                        return OperationExpression();
                    }
                }
            }
            //overrule the user if he wants to store things in the internalcatalog, then the format is by defintion stream
            if ( context()->workingCatalog()->resource().url() == INTERNAL_OBJECT)
                formatName == "Memory";
            if ( formatName != "Memory"){ // special case
                if ( format == "") {
                    QString query = "name='" + formatName + "'";
                    std::multimap<QString, Ilwis::DataFormat>  formats = Ilwis::DataFormat::getSelectedBy(Ilwis::DataFormat::fpNAME, query);
                    if ( formats.size() == 1){
                        format = "{format(" + (*formats.begin()).second.property(DataFormat::fpCONNECTOR).toString() + ",\"" +
                                (*formats.begin()).second.property(DataFormat::fpCODE).toString() + "\")}";
                    }
                }
                // if there is no path we extend it with a path unless the output is a new column, output is than the "old" table so no new output object
                if ( output.indexOf("://") == -1 )
                    output = context()->workingCatalog()->resource().url().toString() + "/" + output + format;
                else
                    output = output + format;
            }else{
                if ( hasType(outputtype,itRASTER)){
                    format = "{format(stream,\"rastercoverage\")}";
                }else if (hasType(outputtype, itFEATURE)){
                    format = "{format(stream,\"featurecoverage\")}";
                }else if (hasType(outputtype, itTABLE | itCOLUMN)){
                    format = "{format(stream,\"table\")}";
                }else if (hasType(outputtype, itCATALOG)){
                    format = "{format(stream,\"catalog\")}";
                }else if (hasType(outputtype, itDOMAIN)){
                    format = "{format(stream,\"domain\")}";
                }else if (hasType(outputtype, itCOORDSYSTEM)){
                    format = "{format(stream,\"coordinatesystem\")}";
                }else if (hasType(outputtype, itGEOREF)){
                    format = "{format(stream,\"georeference\")}";
                }

                output = output + format;
            }
        }

        if(!allOutputsString.isEmpty()){
            allOutputsString.append(",");
        }
        allOutputsString += output;
    }

    if(!duplicateFileNames){
        if ( allOutputsString == "")
            expression = QString("script %1(%2)").arg(operationresource.name()).arg(expression);
        else
            expression = QString("script %1=%2(%3)").arg(allOutputsString).arg(operationresource.name()).arg(expression);

        OperationExpression opExpr(expression);

        return opExpr;
    }
    return OperationExpression();
}







