#ifndef ILWIS3CONNECTOR_GLOBAL_H
#define ILWIS3CONNECTOR_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(ILWIS3CONNECTOR_LIBRARY)
#  define ILWIS3CONNECTORSHARED_EXPORT Q_DECL_EXPORT
#else
#  define ILWIS3CONNECTORSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // ILWIS3CONNECTOR_GLOBAL_H
