function init() {
 
  //Block 1
  var canvas = oCanvas.create({
    canvas: "#canvas",
    background: "#ccc"
  });
 
  //Block 2
  var quadrado = canvas.display.rectangle({
    x: 25,
    y: 25,
    width: 20,
    height: 20, 
    fill: "#0aa",
    velocX: 8,
    velocY: 8
  });
  canvas.addChild(quadrado);

  var rectangle = canvas.display.rectangle({
    x: 100,
    y: 250,
    width: 100,
    height: 50, 
    fill: "#f1f",
  });
  canvas.addChild(rectangle);



  //Block 3
  canvas.bind("click tap", function() {
    quadrado.x = canvas.mouse.x;
    quadrado.y = canvas.mouse.y;
  });

  rectangle.dragAndDrop({
    start: function() {
      this.fill = "ff0";
    },
    move: function() {
      this.fill = "0f0";
      this.y = 250; //restrict to horizontal movement only
    },
    end: function() {
      this.fill = "f00";
      if(this.x <= 0)
        this.x = 0;
      if((rectangle.x+rectangle.width) >= (canvas.width))
        rectangle.x = (canvas.width - rectangle.width);
      //this.y = 400; //snap it to this location after release mouse
    }
  });

var text = canvas.display.text({
  x: 177,
  y: 196,
  origin: { x: "center", y: "center" },
  align: "center",
  font: "bold 25px/1.5 sans-serif",
  text: "Mouse Position\nX: 0\nY: 0",
  fill: "#000"
});
canvas.addChild(text);


  //Block 4
  canvas.setLoop(function() {
    quadrado.x += quadrado.velocX;
    quadrado.y += quadrado.velocY;
    quadrado.rotation++;    
    if ((quadrado.x <= 0) || (quadrado.x >= (canvas.width)))  
      quadrado.velocX = -quadrado.velocX;
    if ((quadrado.y < 20) || (quadrado.y > (canvas.height - 20))) 
      quadrado.velocY = -quadrado.velocY;
    
  text.text = "Mouse Position\n" +
      "X: " + canvas.mouse.x + "\n" +
      "Y: " + canvas.mouse.y + "\n" +
      "rectangle.x = " + rectangle.x + "\n" +
      "rectangle.y = " + rectangle.y + "\n";
  }).start();



}