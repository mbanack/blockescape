/*  global variables    */
var canvas,
    currentShape="circle",
    isPaused = true,
    isInitialized = false;
 
/*  math and positioning    */
var shapes = ["circle","tricuspoid","tetracuspoid","epicycloid","epicycloid 2","epicycloid 3","lissajous","lemniscate","butterfly"];
var w = 800;
var h = 480;
var centerX = 240;
var centerY = 240;
var radius_x = 150;
var radius_y = 150;
var theta = 0;
var objects = [];
var numObjects = 0;
var r2d = 180/Math.PI;
var d2r = Math.PI/180;
var orbitSteps = 180;
var orbitSpeed = Math.PI*2/orbitSteps;
var objectInterval;
var objectPosition;
var direction = 1;
var index = 0;
var xVar1 = 0;
var xVar2;
var xVar3;
var xVar4;
var startingObjects = 100;
var newX;
var newY;
 
onload = init;
 
function init() {
    canvas = oCanvas.create({
        canvas: "#myCanvas",
        background: "#ffffff"
    });
     
    initInterface();
    initObjects();
     
     
    onEnterFrame();
    isInitialized = true;
    canvas.setLoop(onEnterFrame).start();
}
 
function initInterface() {
    var xOff = 625;
    var yOff = 25;
    for(var i=0;i<shapes.length;i++) {
        var b = canvas.display.rectangle({
            x:xOff,
            y:yOff,
            width:150,
            height:20,
            fill:'#ededed',
            stroke:'1px outside #808080',
            shapeName:shapes[i]
        });
        var txt = canvas.display.text({
            x:35,
            y:4,
            align:'center',
            font:'12px courier,monospace',
            text:shapes[i],
            fill:'#000000'
        });
        b.addChild(txt);
        b.bind('mouseenter',function(){
            document.getElementById("myCanvas").style.cursor="pointer";
            this.fill = '#ffffff';
        });
        b.bind('mouseleave',function(){
            document.getElementById("myCanvas").style.cursor="auto";
            this.fill = '#ededed';
        });
        b.bind('click',function(){
            currentShape = this.shapeName;
            isInitialized = false;
            onEnterFrame();
            isInitialized = true;
        });
        yOff += 23;
        canvas.addChild(b);
    }
    yOff += 23;
    var pauseButton = canvas.display.rectangle({
        x:xOff,
        y:yOff,
        width:150,
        height:20,
        fill:'#ededed',
        stroke:'1px outside #808080'
    });
    var pauseButtonText = canvas.display.text({
        x:35,
        y:4,
        align:'center',
        font:'12px courier,monospace',
        text:"PLAY/PAUSE",
        fill:'#000000'
    });
    pauseButton.addChild(pauseButtonText);
    pauseButton.bind('mouseenter',function(){
        document.getElementById("myCanvas").style.cursor="pointer";
        this.fill = '#ffffff';
    });
    pauseButton.bind('mouseleave',function(){
        document.getElementById("myCanvas").style.cursor="auto";
        this.fill = '#ededed';
    });
    pauseButton.bind('click',function() {
        isPaused = !isPaused;
    });
    canvas.addChild(pauseButton);
 
}
function initObjects() {
    for(var i=0;i<startingObjects;i++) {
        addObject();
    }
}
function addObject() {
var obj = canvas.display.ellipse({
        x:centerX,
        y:centerY,
        radius_x:5,
        radius_y:5,
        stroke:"1px #000000",
        fill:"#"+randomRGB()+randomRGB()+randomRGB()
    });
    objects.push(obj);
    numObjects = objects.length;
    objectInterval = orbitSteps/numObjects;
    canvas.addChild(obj);
}
function removeObject() {
    numObjects = objects.length;
    objectInterval = orbitSteps/numObjects;
}
 
function randomRGB(){
    var s = Math.floor(Math.random()*256).toString(16);
    if(s.length==1) s = "0"+s;
    return s;
}
function onEnterFrame() {
    if(isPaused==true && isInitialized==true) return;
    for(var i = 0; i < numObjects; i++) {
        objectPosition = orbitSpeed*objectInterval*i;    //    each object is individually updated
        switch(currentShape) {
            case "circle":
                newX = centerX + radius_x * Math.cos(theta + objectPosition);
                newY = centerY + radius_y * Math.sin(theta + objectPosition);
                break;
            case "tricuspoid":
                newX = centerX + (radius_x*.5) * ((2 * Math.cos(theta + objectPosition)) + Math.cos(2 * (theta + objectPosition)));
                newY = centerY + (radius_y*.5) * ((2 * Math.sin(theta + objectPosition)) - Math.sin(2 * (theta + objectPosition)));
                break;
            case "tetracuspoid":
                newX = centerX + radius_x * Math.pow((Math.cos(theta + objectPosition)),3);
                newY = centerY + radius_y * Math.pow((Math.sin(theta + objectPosition)),3);
                break;
            case "epicycloid":
                newX = centerX + (radius_x*.4) * Math.cos(theta + objectPosition) - radius_x*1*(Math.cos((radius_x/radius_x + 1) * (theta + objectPosition)));
                newY = centerY + (radius_y*.4) * Math.sin(theta + objectPosition) - radius_y*1*(Math.sin((radius_y/radius_y + 1) * (theta + objectPosition)));
                break;
            case "epicycloid 2":
                newX = centerX + (radius_x*.4) * Math.cos(theta + objectPosition) - radius_x*1*(Math.cos((radius_x/radius_x + 2) * (theta + objectPosition)));
                newY = centerY + (radius_y*.4) * Math.sin(theta + objectPosition) - radius_y*1*(Math.sin((radius_y/radius_y + 2) * (theta + objectPosition)));
                break;
            case "epicycloid 3":
                newX = centerX + (radius_x*.4) * Math.cos(theta + objectPosition) - radius_x*1*(Math.cos((radius_x/radius_x + 3) * (theta + objectPosition)));
                newY = centerY + (radius_y*.4) * Math.sin(theta + objectPosition) - radius_y*1*(Math.sin((radius_y/radius_y + 3) * (theta + objectPosition)));
                break;
            case "lissajous":
                newX = centerX + radius_x * (Math.sin(3 * (theta + objectPosition) + xVar1));
                newY = centerY + radius_y * Math.sin(theta + objectPosition);
                xVar1 += .002;
                break;
            case "lemniscate":
                newX = centerX + (radius_x*1.2) * ((Math.cos(theta + objectPosition)/(1 + Math.pow(Math.sin(theta + objectPosition),2))));
                newY = centerY + (radius_y*1.2) * (Math.sin(theta + objectPosition) * (Math.cos(theta + objectPosition)/(1 + Math.pow(Math.sin(theta + objectPosition),2))));
                break;
            case "butterfly":
                newX = centerX + (radius_x*.4) * (Math.cos(theta + objectPosition) * (Math.pow(5,Math.cos(theta+objectPosition)) - 2 * Math.cos(4 * (theta+objectPosition)) - Math.pow(Math.sin((theta+objectPosition)/12),4)));
                newY = centerY + (radius_y*.4) * (Math.sin(theta + objectPosition) * (Math.pow(5,Math.cos(theta+objectPosition)) - 2 * Math.cos(4 * (theta+objectPosition)) - Math.pow(Math.sin((theta+objectPosition)/12),4)));
                break;
            default:
                break;
     
        }
        objects[i].x = newX;
        objects[i].y = newY;
    }
    theta += (orbitSpeed*direction);
}