function init() {
  //working on the file on git?
  var rectangle;
  var rectangle2;
  var rectangle_array = [];  
  var const_array = [];
  var counter = 0;
  var string = "lalalala";
  var string2 = "";

  //Block 1
  var canvas = oCanvas.create({
    canvas: "#canvas",
    background: "#ccc"
  });
 
  function make_rect(xpos,ypos,w,h){
    var temp_rect = canvas.display.rectangle({
      x: xpos,
      y: ypos,
      width: w,
      height: h,
      fill: "#f1f"
    });
    temp_rect.bind("mouseenter", function(){canvas.mouse.cursor("pointer"); string2 = "now in rect" + temp_rect.id; string = "restrict y to "+getYOfId(temp_rect.id);})
                       .bind("mouseleave", function() {canvas.mouse.cursor("default"); string2 = ""});
    rectangle_array.unshift(temp_rect);
    const_array.unshift(ypos);
    counter++;
  }

  make_rect(270,50,100,50);
  make_rect(270,150,100,50);
  make_rect(270,250,100,50);
  make_rect(270,350,100,50);

  function getYOfId(temp_id)
  {
    for(i = 0; i < rectangle_array.length; i++)
    {
      if(rectangle_array[i].id == temp_id)
        return rectangle_array[i].y;
    }
    return -1;
  }


  // for(j = const_array.length-1; j >= 0 ; j--)
  // {
  //   string = string + " " + const_array[j];
  // }
  // for(j = 0; j < const_array.length; j++)
  // {
  //   string = string + " " + const_array[j];
  // }
  
  var flag = true;
  var yholder;
  
  for(i = 0; i < rectangle_array.length; i++)
  {
    canvas.addChild(rectangle_array[i]);
    
    rectangle_array[i].dragAndDrop({
      start: function() {
        this.fill = "ff0";
      },
      move: function() {
        this.fill = "0f0";
        if(flag == true) 
          yholder = getYOfId((rectangle_array.length-i));
        flag = false;
        this.y = yholder; //restrict to horizontal movement only
        if(this.x <= 0) //restrict is now tested on movement rather than snap after mouse release
          this.x = 0;
        if((rectangle_array[0].x+rectangle_array[0].width) >= (canvas.width))
          rectangle_array[0].x = (canvas.width - rectangle_array[0].width);
      },
      end: function() {
       this.fill = "f00";
       flag = true;
        //this.y = 400; //snap it to this location after release mouse
      }
    });
  }

  var text = canvas.display.text({
    x: 177,
    y: 196,
    origin: { x: "center", y: "center" },
    align: "center",
    font: "bold 12px/1.5 sans-serif",
    text: "Mouse Position\nX: 0\nY: 0",
    fill: "#000"
  });
  canvas.addChild(text);

  // //Block 4
  canvas.setLoop(function() {  
    text.text = "Mouse Position\n" +
      "X: " + canvas.mouse.x + "\n" +
      "Y: " + canvas.mouse.y + "\n" + 
      rectangle_array.length + "\n" + 
      string + "\n" + string2
  });


  canvas.timeline.start();

}