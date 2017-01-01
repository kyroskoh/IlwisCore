import QtQuick 2.2
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import MasterCatalogModel 1.0
import OperationCatalogModel 1.0
import OperationModel 1.0
import ApplicationFormExpressionParser 1.0
import "../../../workbench" as Bench
import "../../../Global.js" as Global

Rectangle  {
    width : parent.width
    height : parent.height
    color : "white"
    ScrollView{
        id: runFormScrollView
        width : parent.width
        height : parent.height - 30
        property var operationid;


        Bench.ApplicationForm{
            id : appFrame
            y : 20
            width : runFormScrollView.width - 20
            height : parent.height
            showTitle: false
        }


    }

    function fillAppFrame() {
        workflowView.workflow.createMetadata()
        var form = formbuilder.index2Form(workflowView.workflow.id, true)
        appFrame.formQML = ""
        appFrame.formQML = form
        appFrame.formTitle = ""
        appFrame.opacity = 1
    }

    function executeRunForm(runparms) {
        operations.executeoperation(runparms.runid,appFrame.currentAppForm.formresult, runparms)
        return appFrame.currentAppForm.formresult
    }
}
