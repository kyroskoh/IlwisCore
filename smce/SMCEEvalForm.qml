import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.1
import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1

import LayersView 1.0
import SMCE 1.0
import "../../../qml/Global.js" as Global
import "../../../qml/controls" as Controls

Column {
    id: evalform
    anchors.fill: parent
    Column {
        id: nodecolumn
        spacing: 5
        anchors.margins: 5
        visible: selectedNode != null && selectedNode.type === Node.Group
        Text {
            id: title
            text : qsTr("Weights")
            font.bold : true
        }

        ListView {
            spacing: 5
            anchors.margins: 5
            model: (selectedNode != null && selectedNode.weights !== null) ? selectedNode.weights.directWeights : null
            width: parent.width
            height: childrenRect.height
            delegate: Row {
                spacing: 5
                anchors.margins: 5
                TextField {
                    id : directWeight
                    width : 30
                    text : ""
                    font.pointSize: 8

                    property string oldText

                    style: TextFieldStyle {
                        background: Rectangle {
                            radius: 3
                            width : parent.width
                            height: parent.height
                            border.color: parent.enabled ? Global.edgecolor : "transparent"
                            border.width: directWeight.readOnly ? 0: 1
                            color : "white"
                        }
                    }
                    onTextChanged: {
                        var regex = /^-?\d*(\.\d*)?$/
                        if (!regex.test(text)){
                            text = oldText
                        } else {
                            oldText = text
                            model.directWeight = text
                        }
                    }
                    Component.onCompleted: {
                        text = model.directWeight
                    }
                }

                Text {
                    id : normalizedWeight
                    width: 30
                    text : model.normalizedWeight.toFixed(2).toString()
                    font.pointSize: 8
                }

                Text {
                    id : label
                    width : evalform.width - directWeight.width - normalizedWeight.width
                    text : model.name
                    font.weight: Font.Bold
                    font.pointSize: 8
                    elide: Text.ElideRight
                    clip :true
                }
            }
        }

        Button {
            text : qsTr("Apply")
            onClicked: {
                if (selectedNode != null)
                    selectedNode.weights.apply()
            }
        }
    }

    Column {
        id: standardizationcolumn
        visible: selectedNode != null && selectedNode.type !== Node.Group

        Rectangle {
            id : standardizationlabel
            height : 18
            width: parent.width
            color : Global.palegreen
            Text{
                text : qsTr("Standardization")
                font.bold : true
                //x : 5
                //anchors.verticalCenter: parent.verticalCenter
            }
        }

        Rectangle {
            id : standardizationpanel
            color : Global.palegreen
            border.color: Global.darkgreen
            //anchors.fill: parent
            height : parent.height - 18
            width: parent.width


            //SMCEClassStandardizationTable {
            //    id: classstandardizationtable
            //    height: parent.height
            //}

            /*SMCEClassStandardization {
                    id: classstandardization
                    height: parent.height
                }*/

            Loader {
                id : stdEditor
                width : parent.width
                height : parent.height
            }

        }

        Component.onCompleted: {
            // if ( input indicator is class raster)
            //stdEditor.setSource("SMCEClassStandardization.qml")
            // else
            stdEditor.setSource("SMCEGraphStandardization.qml")
        }
    }

    /*
       Column {
            id: previewcolumn
            width : parent.parent.width / 3
            anchors.left: standardizationcolumn.right

            Rectangle {
                id : previewlabel
                width : parent.width
                height : 18
                color : Global.palegreen
                Text{
                    text : qsTr("Preview")
                    font.bold : true
                    x : 5
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Rectangle {
                id : preview
                width : parent.width
                height : parent.height - 18
                color : Global.palegreen
            }
        }
       */
}
