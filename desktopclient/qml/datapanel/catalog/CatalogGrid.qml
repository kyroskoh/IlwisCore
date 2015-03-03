import QtQuick 2.2
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import QtQuick.Dialogs 1.0
import MasterCatalogModel 1.0
import CatalogModel 1.0
import ResourceModel 1.0
import UIContextModel 1.0

import "../../Global.js" as Global

FocusScope {
    anchors.fill: parent
    Rectangle {
        anchors.fill :parent
        color : "white"

        Image {
            anchors.fill: parent
            source : "../../images/remoteBig.png"
            fillMode: Image.Stretch
            opacity: 0.07

        }

        ScrollView {
            id : catalogGrid
            property color background1 : "#EAECEE"
            property color background2 : "#FFF9E3"

            anchors.fill : parent
            anchors.margins: 2

            GridView {
                id : itemgrid

                function setSelected(objectid){
                    if ( uicontext.currentKey === 0)    {
                        console.debug(currentCatalog.resources.length)
                        for(var i = 0; i < currentCatalog.resources.length; ++i){
                            if (currentCatalog.resources[i].isSelected && currentCatalog.resources[i].id !== objectid)
                                currentCatalog.resources[i].isSelected=false
                        }
                    }
                }

                model : setResources()
                delegate: CatalogGridDelegate{}
                cellWidth: 170
                cellHeight: 18
                clip : true
                cacheBuffer: 1000
            }
        }
    }
}



