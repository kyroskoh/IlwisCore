#ifndef ProjectionImplFactoryProj4PORJ4_H
#define ProjectionImplFactoryProj4PORJ4_H

namespace Ilwis {
class ProjectionImplFactoryProj4 : public ProjectionFactory
{
public:
    ProjectionImplFactoryProj4();

    ProjectionImplementation *create(const Ilwis::Resource &item) const;
    ProjectionImplementation *create(const QString&) const;
    virtual bool canUse(const Ilwis::Resource &resource) const;
    virtual bool prepare();
private:
    QString _knownProjections;

};
}

#endif // ProjectionImplFactoryProj4PORJ4_H
