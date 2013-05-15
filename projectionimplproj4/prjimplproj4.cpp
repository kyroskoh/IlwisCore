#include <QString>
#include <functional>

#include "kernel.h"
#include "ilwis.h"
#include "angle.h"
#include "point.h"
#include "ilwisobject.h"
#include "ilwisdata.h"
#include "ellipsoid.h"
#include "geodeticdatum.h"
#include "projection.h"
#include "ProjectionImplementation.h"
#include "proj_api.h"
#include "prjimplproj4.h"
#include "proj4parameters.h"
#include "coordinatesystem.h"
#include "conventionalcoordinatesystem.h"


using namespace Ilwis;

ProjectionImplementation *ProjectionImplementationProj4::create(const Resource& item) {
    return new ProjectionImplementationProj4(item);
}

void ProjectionImplementationProj4::setParameter(Projection::ProjectionParamValue type, const QVariant &v)
{
    ProjectionImplementation::setParameter(type, v);
    QString value = v.toString();
    switch(type) {
    case Projection::pvX0:
        _targetDef += " +x_0=" + value;break;
    case Projection::pvY0:
        _targetDef += " +y_0=" + value; break;
    case Projection::pvLON0:
        _targetDef += " +lon_0=" + value; break;
    case Projection::pvLAT0:
        _targetDef += " +lat_0=" + value; break;
    case Projection::pvLAT1:
        _targetDef += " +lat_1=" + value; break;
    case Projection::pvLAT2:
        _targetDef += " +lat_2=" + value; break;
    case Projection::pvLATTS:
        _targetDef += " +lat_ts=" + value; break;
    case Projection::pvZONE:
        _targetDef += " +zone=" + value; break;
    case Projection::pvK0:
        _targetDef += " +k0=" + value; break;
    case Projection::pvELLCODE:
        _targetDef += " " + value; break;
    case Projection::pvNORTH:
        _targetDef += value == "No" ? "+south" : ""; break;
    default:
        _targetDef += "";
    }
    if ( _pjBase)
        pj_free(_pjBase);

    _pjBase = pj_init_plus(_targetDef.toLatin1());
}

ProjectionImplementationProj4::ProjectionImplementationProj4(const Resource &item)
{
    QString cd = item.code();
    _outputIsLatLon = cd == "latlong" || cd == "longlat";
    _targetDef = QString("+proj=%1").arg(cd);
    _pjLatlon =  pj_init_plus("+proj=latlong +ellps=WGS84");
    _pjBase = pj_init_plus(_targetDef.toLatin1());
}

ProjectionImplementationProj4::~ProjectionImplementationProj4()
{
    pj_free(_pjLatlon);
    pj_free(_pjBase);
}

bool ProjectionImplementationProj4::prepare(const QString &parms)
{
    if ( parms != "") {
        Proj4Parameters proj4(parms);
        //std::function<void(double)> assign; // = [&](double v)  -> void {}

        std::map<QString,Projection::ProjectionParamValue>  alias = { { "lat_1",Projection::pvLAT1}, { "lat_2",Projection::pvLAT2},{ "lat_0",Projection::pvLAT0}, { "k",Projection::pvK0},
                                                     { "lon_0",Projection::pvLON0}, { "x_0",Projection::pvX0}, { "y_0",Projection::pvY0}, { "zone",Projection::pvZONE}};
        auto assign = [&](const QString& proj4Name)->void
        {
            QString sv = proj4[proj4Name];
            if ( sv != sUNDEF) {
                Projection::ProjectionParamValue v = alias[proj4Name];
                setParameter(v, sv.toDouble());
            }
        };
        QString ellps = proj4["ellps"];
        if ( ellps != sUNDEF) {
            _targetDef += " +ellps=" + ellps;
        }
        QString a = proj4["a"];
        if ( a != sUNDEF) {
            _targetDef += " +a=" + a;
        }
        QString b = proj4["b"];
        if ( b != sUNDEF) {
            _targetDef += " +b=" + b;
        }
        QString shifts = proj4["towgs84"];
        if ( shifts != sUNDEF) {
            _targetDef += " +towgs84=" + shifts;
        }

        for(auto iter= alias.begin(); iter != alias.end(); ++iter) {
            assign(iter->first);
        }
        if ( _pjBase)
            pj_free(_pjBase);

        _pjBase = pj_init_plus(_targetDef.toLatin1());
        return true;
    }
    else if ( _coordinateSystem != 0) {
        if ( _coordinateSystem->ellipsoid().isValid()) {
            _targetDef += " " + _coordinateSystem->ellipsoid()->code();
        }
        if ( _coordinateSystem->datum() && _coordinateSystem->datum()->isValid()) {
            _targetDef += " " + _coordinateSystem->datum()->code();
        }
        if ( _pjBase)
            pj_free(_pjBase);

        _pjBase = pj_init_plus(_targetDef.toLatin1());
        return true;
    }
    return ERROR1(ERR_NO_INITIALIZED_1,"Projection");
}

Coordinate ProjectionImplementationProj4::latlon2coord(const LatLon &ll) const
{
    if ( _pjBase == 0 || _pjLatlon == 0) {
        int *err = pj_get_errno_ref();
        if (err != 0){
            QString error(pj_strerrno(*err));
            error = "projection error:" + error;
            kernel()->issues()->log(error);
        }
        return Coordinate();
    }

    double x = ll.lon(Angle::uRADIANS);
    double y = ll.lat(Angle::uRADIANS);
    int err = pj_transform(_pjLatlon, _pjBase, 1, 1, &x, &y, NULL );
    if ( err != 0) {
        QString error(pj_strerrno(err));
        error = "projection error:" + error;
        kernel()->issues()->log(error);
        return Coordinate();
    }

    if ( _outputIsLatLon)
        return Coordinate(Degrees(y,false).degrees(),Degrees(x, false).degrees());
    return Coordinate(x,y);
}

LatLon ProjectionImplementationProj4::coord2latlon(const Coordinate &crd) const
{

    if ( _pjBase == 0 || _pjLatlon == 0){
        int *err = pj_get_errno_ref();
        if (err != 0){
            QString error(pj_strerrno(*err));
            error = "projection error:" + error;
            kernel()->issues()->log(error);
        }
        return LatLon();
    }

    double x = crd.x();
    double y = crd.y();
    int err = pj_transform(_pjBase,_pjLatlon, 1, 1, &x, &y, NULL );
    if ( err != 0) {
        QString error(pj_strerrno(err));
        error = "projection error:" + error;
        kernel()->issues()->log(error);
        return LatLon();
    }

    return LatLon(Degrees(y,false),Degrees(x, false));
}


