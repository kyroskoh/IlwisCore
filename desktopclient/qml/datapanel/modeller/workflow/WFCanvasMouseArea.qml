import QtQuick 2.1
import MasterCatalogModel 1.0
import OperationCatalogModel 1.0
import OperationModel 1.0
import WorkflowModel 1.0
import QtQuick.Dialogs 1.1
import ".." as Modeller
import "../../../Global.js" as Global

MouseArea {
    id: mousearea
    anchors.fill: parent
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    hoverEnabled: true
    property bool positionChanged: false
    property int lastX: width/2;
    property int lastY: height/2;
    property int lastOpX: width/2;
    property int lastOpY: height/2;

    onWheel: {
        var delta = wheel.angleDelta.y / 40
        zoom(delta, false, mouseX, mouseY)
    }
    onPressed: {
        selectThing()
    }
    onDoubleClicked: {
        openWorkflow()
    }
    onPositionChanged: {
        move()
    }
    onReleased: {
        wfCanvas.stopWorkingLine()
        oldx = -1
        oldy = -1
        cursorShape = Qt.ArrowCursor
        if ( currentItem && currentItem.type === "operationitem"){
            if ( currentItem.condition)    {
                currentItem.condition.resize()
            }
        }

        if( workarea.dropCondition){
            var component = Qt.createComponent("ConditionItem.qml");
            if (component.status === Component.Ready){
                var nodeid = workflow.addNode(0,{x : mouseX, y:mouseY, w:360, h:160,type:'conditionnode'})
                currentItem = component.createObject(wfCanvas, {"x": mouseX, "y": mouseY, "h" : 160, "w" : 360,"itemid" : nodeid, "scale": wfCanvas.scale});
                workarea.conditionsList.push(currentItem)
            }
            workarea.dropCondition = false


        }
    }

    function selectThing(){
        if (canvasActive) {
            var operationSelected = -1, highestZ = -1, smallestDistance = 100000, selectedFlow=false
            var alllist = operationsList
            alllist  = alllist.concat(conditionsList)

            for(var i=0; i < alllist.length; ++i){

                var item = alllist[i]
                item.isSelected = false
                currentItem = itemAt(mouseX, mouseY)

                if ( item.type === "operationitem" || item.type == "conditionitem") {
                    for(var j=0; j < item.flowConnections.length; j++)
                    {
                        var flow = item.flowConnections[j];

                        // Retrieve basic X and Y positions of the line
                        var startPoint = flow.attachsource.center();
                        var index = -1
                        if ( flow.target.type !== "conditionitem"){
                            index = flow.attachtarget
                        }
                        var endPoint = flow.target.attachementPoint(wfCanvas,index)
                        var ax = startPoint.x;
                        var ay = startPoint.y;
                        var bx = endPoint.x;
                        var by = endPoint.y;

                        // Calculate distance to check mouse hits a line
                        var distanceAC = Math.sqrt(Math.pow((ax-mouseX), 2) + Math.pow((ay-mouseY), 2));
                        var distanceBC = Math.sqrt(Math.pow((bx-mouseX), 2) + Math.pow((by-mouseY), 2));
                        var distanceAB = Math.sqrt(Math.pow((ax-bx), 2) + Math.pow((ay-by), 2));
                        var distanceLine = distanceAC + distanceBC;


                        // Check if mouse intersects the line with offset of 10
                        if(distanceLine >= distanceAB &&
                                distanceLine < (distanceAB + wfCanvas.scale) &&
                                distanceLine - distanceAB < smallestDistance)
                        {
                            smallestDistance = distanceLine - distanceAB;
                            selectedFlow = flow;
                        }
                        flow.isSelected = false;
                    }
                }
            }
        }

        oldx = mouseX
        oldy = mouseY

        if(!selectedFlow && !currentItem)
        {
            lastX = mouseX;
            lastY = mouseY;
            lastOpX = mouseX;
            lastOpY = mouseY;
        }

        if (selectedFlow && !currentItem) {
            selectedFlow.isSelected = true
        } else if (currentItem) {
            currentItem.isSelected = true
            workarea.showSelectedOperation(currentItem.type === "operationitem")

        }else {
            workarea.showSelectedOperation(false)
        }

        if ( selectedFlow)
            console.debug("a", selectedFlow.source.itemid, selectedFlow.target.itemid, selectedFlow.isSelected)
        wfCanvas.canvasValid = false
    }

    function openWorkflow() {
        var operationSelected = -1, item = 0, highestZ = 0;
        for(var i=0; i < operationsList.length; ++i){

            item = operationsList[i]
            var isContained = mouseX >= item.x && mouseY >= item.y && mouseX <= (item.x + item.width) && mouseY <= (item.y + item.height)

            if ( isContained && item.z > highestZ ) {
                operationSelected = i
                highestZ = item.z
            }
        }
        if (operationSelected > -1) {
            var resource = mastercatalog.id2Resource(item.operation.id, area)
            var filter = "itemid=" + resource.id
            bigthing.newCatalog(filter, "workflow",resource.url,"other")
        }
    }

    function getCursorShape(xrelative, yrelative) {
        var cshape = Qt.SizeAllCursor

        if ( currentItem.type === "conditionitem"){
            if ( xrelative < 10){
                if ( yrelative > currentItem.height * wfCanvas.zoomScale - 10)
                    cshape = Qt.SizeBDiagCursor
                else
                    cshape = Qt.SizeHorCursor
            }else if (xrelative > currentItem.width * wfCanvas.zoomScale - 10){
                if ( yrelative > currentItem.height * wfCanvas.zoomScale - 10)
                    cshape = Qt.SizeFDiagCursor;
                else
                    cshape = Qt.SizeHorCursor
            }else
                if ( yrelative  > currentItem.height * wfCanvas.zoomScale - 10)
                    cshape = Qt.SizeVerCursor
        }

        return cshape

    }

    function scaleMousePosition(num){
        if ( wfCanvas.zoomScale == 1)
            return num;
        var dzoom =  wfCanvas.zoomScale/wfCanvas.oldZoomScale
        var scaledNum = (num + width *(dzoom - 1))
        return scaledNum;
    }

    function move() {
        lastX = mouseX
        lastY = mouseY

        if (attachementForm.state === "invisible") {
            if (workingLineBegin.x !== -1) {
                workingLineEnd = Qt.point(mouseX, mouseY)
                canvasValid = false
            }
            else if (oldx >= 0 && oldy >= 0) {
                if ( currentItem ){
                    var xrelative = scaleMousePosition(mouseX) - currentItem.x
                    var yrelative = scaleMousePosition(mouseY) - currentItem.y
                    //scaleMousePosition(mouseX)
                    cursorShape = getCursorShape(xrelative, yrelative)
                    var prevXCorner = currentItem.x
                    var prevYCorner = currentItem.y
                    if ( cursorShape == Qt.SizeAllCursor){
                        currentItem.x += mouseX - oldx
                        currentItem.y += mouseY - oldy
                        if ( currentItem.type === "conditionitem"){
                            //currentItem.moveContent( mouseX - oldx, mouseY - oldy)
                            canvasValid = false
                        }
                        else if ( currentItem.type === "junctionitem"){
                            canvasValid = false
                        }
                    }else if (cursorShape == Qt.SizeHorCursor ){
                        if ( xrelative < 10){
                            currentItem.x = mouseX
                            currentItem.width = currentItem.width + prevXCorner - currentItem.x
                        }else
                            currentItem.width = Math.max(100,mouseX - currentItem.x)
                    } else if (cursorShape == Qt.SizeBDiagCursor ){
                        currentItem.x = mouseX
                        currentItem.width = currentItem.width + prevXCorner - currentItem.x
                        currentItem.height = Math.max(100,mouseY - currentItem.y)
                    }else if (cursorShape == Qt.SizeFDiagCursor ){
                        currentItem.width = Math.max(100,mouseX - currentItem.x)
                        currentItem.height = Math.max(100,mouseY - currentItem.y)
                    }else if (cursorShape == Qt.SizeVerCursor ){
                        if ( yrelative > currentItem.height - 10){
                            currentItem.height = Math.max(100,mouseY - currentItem.y)
                        }
                    }
                }else {
                    var deltax = oldx - mouseX
                    var deltay = oldy - mouseY
                    workarea.pan(deltax,deltay)
                }
                oldx = mouseX
                oldy = mouseY
            }
        }
    }
}
