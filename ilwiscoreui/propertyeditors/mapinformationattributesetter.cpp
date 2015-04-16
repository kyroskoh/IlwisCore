#include "kernel.h"
#include "ilwisdata.h"
#include "datadefinition.h"
#include "columndefinition.h"
#include "table.h"
#include "visualattributemodel.h"
#include "mapinformationattributesetter.h"

REGISTER_PROPERTYEDITOR("mapinfopropertyeditor",MapInformationPropertySetter)

MapInformationPropertySetter::MapInformationPropertySetter(QObject *parent) :
    VisualAttributeEditor("mapinfopropertyeditor",TR("Mouse over Info"),QUrl("MapinfoProperties.qml"), parent)
{

}

MapInformationPropertySetter::~MapInformationPropertySetter()
{

}

bool MapInformationPropertySetter::canUse(const IIlwisObject& obj, const QString& name ) const
{
    if (!obj.isValid())
        return false;
    if(!hasType(obj->ilwisType(), itCOVERAGE))
        return false;
    return name == VisualAttributeModel::LAYER_ONLY;

}

VisualAttributeEditor *MapInformationPropertySetter::create()
{
    return new MapInformationPropertySetter();
}

void MapInformationPropertySetter::prepare(CoverageLayerModel *parentLayer, const IIlwisObject &bj, const ColumnDefinition &datadef)
{
    VisualAttributeEditor::prepare(parentLayer,bj,datadef);
}

bool MapInformationPropertySetter::showInfo() const
{
    if ( layer())
        return layer()->showInfo();
    return true;
}

void MapInformationPropertySetter::setShowInfo(bool yesno)
{
    if (!layer())
        return;

    layer()->showInfo(yesno);

}

