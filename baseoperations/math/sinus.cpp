#include <functional>
#include <future>
#include "kernel.h"
#include "raster.h"
#include "ilwisoperation.h"
#include "pixeliterator.h"
#include "unarymath.h"
#include "sinus.h"

using namespace Ilwis;
using namespace BaseOperations;

Sinus::Sinus() {

}

Sinus::Sinus(quint64 metaid,const Ilwis::OperationExpression& expr) : UnaryMath(metaid, expr)
{

}

bool Sinus::execute(ExecutionContext *ctx)
{
    if (_prepState == sNOTPREPARED)
        if((_prepState = prepare()) != sPREPARED)
            return false;

    QVariant value;
    if ( _spatialCase) {
        UnaryFunction fun = sin;
        return UnaryMath::execute(fun, ctx);


    } else {
        double v = sin(_number);
        value.setValue<double>(v);
    }
    if ( ctx && value.isValid())
        ctx->_results.push_back(value);

    return true;
}

OperationImplementation *Sinus::create(quint64 metaid, const Ilwis::OperationExpression &expr)
{
    return new Sinus(metaid,expr);
}

OperationImplementation::State Sinus::prepare()
{
    OperationImplementation::State state = UnaryMath::prepare();
    if ( state != sPREPARED)
        return state;

    IDomain dom;
    if(!dom.prepare("min1to1"))
        return sPREPAREFAILED;

    _outputGC->datadef().domain(dom);
    return sPREPARED;
}

quint64 Sinus::createMetadata() {

    QString url = QString("ilwis://operations/sin");
    Resource res(QUrl(url), itOPERATIONMETADATA);
    UnaryMath::populateMetadata(res);
    res.addProperty("longname","sinus");
    res.addProperty("pin_1_desc",TR("input gridcoverage with a numerical domain; values are considered to be radians"));
    res.addProperty("outparameters",1);
    res.addProperty("pout_1_type", itGRIDCOVERAGE);
    res.addProperty("pout_1_domain","min1to1");
    res.addProperty("pout_1_desc",TR("output gridcoverage with a numerical domain; values are between -1 and 1"));

    mastercatalog()->addItems({res});
    return res.id();
}


