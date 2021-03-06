import QtQuick 2.1
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import "../../controls" as Controls
import "../../Global.js" as Global
import LayersView 1.0

DropArea {
    id : dropArea
    //property MapScrollers mapScrollers
    property alias drawer : renderer
    property alias mouseActions : mouseActions

    function entireMap() {
        renderer.addCommand("setviewextent("+ renderer.viewerId + ",entiremap)");
        renderer.update()
    }

    function transfer(datapanel){
        var layers = datapanel.manager.layers;
        for(var i = layers.length-1; i > 0; --i){  // start at 1 because the first layer is always the global layer, is there by default so we skip it
            var expr = "adddrawer(" + renderer.viewerId + ","+ layers[i].name + ",\"itemid=" + layers[i].id + "\"," + layers[i].typeName + ")"
            renderer.addCommand(expr)
        }
        renderer.update()
    }

    onDropped: {
        var resource = mastercatalog.id2Resource(drag.source.ilwisobjectid, dropArea)
        addDataSource(resource.url, resource.name, resource.typeName)       
    }

    Connections {
        target: mouseActions
        onZoomEnded :{
            zoomEnded(envelope)

            renderer.addCommand("removedrawer(" + renderer.viewerId + ",selectiondrawer, post)")
            renderer.update()

            renderer.addCommand("adddrawer(" + renderer.viewerId + ",selectiondrawer, colorless)")
            renderer.update()
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 30
        LayersView {
            id: renderer
            anchors.top: parent.top
            height : parent.height - 16
            width : parent.width - 16
            anchors.margins: 1
            objectName : "layers_" + uicontext.uniqueName()

            function finalizeDraw(){
                if (!manager.hasSelectionDrawer) {
                    manager.hasSelectionDrawer = true
                    renderer.addCommand("adddrawer(" + renderer.viewerId + ",selectiondrawer, colorless)")
                }                
            }

            Component.onCompleted: {
                renderer.associate(objectName,"drawEnded")
            }

            LayerExtentMouseActions{
                id : mouseActions
                layerManager: manager
                drawer : renderer
                zoomToExtents: true
                hasPermanence: true
                showInfo: true
                panningDirection: Global.panningReverse
                selectiondrawerColor: "basic"
            }           
        }
    }
}

