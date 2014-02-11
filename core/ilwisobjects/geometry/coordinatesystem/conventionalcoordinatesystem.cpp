#include "kernel.h"
#include "geometries.h"
#include "ilwisdata.h"
#include "ellipsoid.h"
#include "geodeticdatum.h"
#include "projection.h"
#include "coordinatesystem.h"
#include "conventionalcoordinatesystem.h"
#include "proj4parameters.h"

using namespace Ilwis;

ConventionalCoordinateSystem::ConventionalCoordinateSystem() : _unit("meter")
{
}

ConventionalCoordinateSystem::ConventionalCoordinateSystem(const Resource &resource) : CoordinateSystem(resource)
{
}

ConventionalCoordinateSystem::~ConventionalCoordinateSystem()
{
    _projection.set(0);
    _ellipsoid.set(0);
}

Coordinate ConventionalCoordinateSystem::coord2coord(const ICoordinateSystem &sourceCs, const Coordinate &crdSource) const
{

    if (sourceCs->id() == id()) //  the 'real'isEqual test is too expensive here, as this method can be called 100000's of times (resample)
        return crdSource;
    LatLon ll = sourceCs->isLatLon() ? LatLon(crdSource.y,crdSource.x) : sourceCs->coord2latlon(crdSource);
    if ( ll.isValid()) {
        return latlon2coord(ll);
    }
    return Coordinate();
}

LatLon ConventionalCoordinateSystem::coord2latlon(const Coordinate &crdSource) const
{
    LatLon pl = _projection->coord2latlon(crdSource);
    if (!pl.isValid())
        return llUNDEF;
    if (abs(pl.lon()) > 90)
        return llUNDEF;
    return pl;
}

Coordinate ConventionalCoordinateSystem::latlon2coord(const LatLon &ll) const
{
    Coordinate xy = _projection->latlon2coord(ll);
    if (xy == crdUNDEF)
        return crdUNDEF;
    return xy;

}

bool ConventionalCoordinateSystem::isEqual(const IlwisObject *obj) const
{
    if ( !obj || !hasType(obj->ilwisType(), itCONVENTIONALCOORDSYSTEM))
        return false;

    if(id() == obj->id())
        return true;

    const ConventionalCoordinateSystem *csy = static_cast<const ConventionalCoordinateSystem *>(obj);
    if ( !csy->isValid())
        return false;

    if ( ellipsoid().isValid() && ellipsoid()->isEqual(csy->ellipsoid())) {
        if ( csy->projection().isValid() && projection().isValid()){ //  special case; in general datums will be checked first
            if (csy->projection()->code() == "longlat" && projection()->code() == "longlat")
                return true;
        }
        if ( datum() && csy->datum()) {
            if  (datum()->code() == csy->datum()->code())
                    return true;
        } else {
            if ( csy->projection().isValid() && projection().isValid()) {
                return csy->projection()->isEqual(projection().ptr());
            }
        }
    }


    return false;
}

const std::unique_ptr<GeodeticDatum>& ConventionalCoordinateSystem::datum() const
{
    return _datum;
}

void ConventionalCoordinateSystem::setDatum(GeodeticDatum *datum)
{
    _datum.reset(datum);
}

IEllipsoid ConventionalCoordinateSystem::ellipsoid() const
{
    return _ellipsoid;
}

void ConventionalCoordinateSystem::setEllipsoid(const IEllipsoid &ell)
{
    _ellipsoid = ell;
}

bool ConventionalCoordinateSystem::isLatLon() const
{
    return _unit == "degrees";
}

IlwisTypes ConventionalCoordinateSystem::ilwisType() const
{
    return itCONVENTIONALCOORDSYSTEM;
}

bool ConventionalCoordinateSystem::isValid() const
{
    bool ok1 =  _projection.isValid();
    bool ok2 =_ellipsoid.isValid();

    return ok1 && ok2;
}



void ConventionalCoordinateSystem::setProjection(const IProjection &proj)
{
    _projection = proj;
    if ( proj->code().contains("longlat") || proj->code().contains("latlon"))
        _unit = "degrees";
}

IProjection ConventionalCoordinateSystem::projection() const
{
    return _projection;
}

bool ConventionalCoordinateSystem::prepare()
{
    return CoordinateSystem::prepare();
}

bool ConventionalCoordinateSystem::prepare(const QString &parms)
{
    Proj4Parameters proj4(parms);

    QString ell = proj4["ellps"];
    _ellipsoid = new Ellipsoid();
    if ( ell != sUNDEF) {
        _ellipsoid.prepare("code=" + ell);
    } else {
        QString laxis = proj4["a"];
        if ( laxis != sUNDEF) {
            QString saxis = proj4["b"];
            bool ok;
            double a = laxis.toDouble(&ok);
            if (!ok) return ERROR2(ERR_INVALID_PROPERTY_FOR_2, "ellipsoid", name());
            double b = saxis.toDouble(&ok);
            if (!ok) return ERROR2(ERR_INVALID_PROPERTY_FOR_2, "ellipsoid", name());

            double f = (a - b) / a;
            _ellipsoid->setEllipsoid(a, f);
            _ellipsoid->setName("Custom Ellipsoid for " + name());
        }
    }
    if ( proj4.hasDatum()) {
        _datum.reset(new GeodeticDatum());
        if ( proj4["dx"] != sUNDEF) {
            _datum->set7TransformationParameters(proj4["dx"].toDouble(),
                                                 proj4["dy"].toDouble(),
                                                 proj4["dz"].toDouble(),
                                                 proj4["rx"].toDouble(),
                                                 proj4["ry"].toDouble(),
                                                 proj4["rz"].toDouble(),
                                                 proj4["dscale"].toDouble());
        }
    }
    QString code = proj4["proj"];

    if ( code == sUNDEF) {
        kernel()->issues()->log(TR(ERR_INVALID_PROPERTY_FOR_2).arg("projection name", name()));
        return false;
    }
    code = "code=" + code;
    Resource prj = mastercatalog()->name2Resource(code, itPROJECTION);
    if ( !prj.isValid()) {
         return ERROR1(ERR_COULDNT_CREATE_OBJECT_FOR_1,"projection " + code );
    }
    bool ok =_projection.prepare(prj.id());
    _projection->setCoordinateSystem(this);
    if ( _projection.isValid())  {
        ok = _projection->prepare(parms);
    }
    if ( ok && (_projection->code().contains("longlat") || _projection->code().contains("latlon")))
        _unit = "degrees";


    return ok;
}


