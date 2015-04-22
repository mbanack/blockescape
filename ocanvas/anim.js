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
    temp_rect.bind("mouseenter", function(){canvas.mouse.cursor("pointer"); string2 = "now in rect" + 
              temp_rect.id; string = "restrict y to "+getYOfId(temp_rect.id) + 
              " or restrict x to "+getXOfId(temp_rect.id);})
             .bind("mouseleave", function() {canvas.mouse.cursor("default"); string = ""; string2 = ""});
    rectangle_array.unshift(temp_rect);
    const_array.unshift(ypos);
    counter++;
  }

  make_rect(270,50,100,50);  //horizontal piece
  make_rect(270,150,100,50); //horizontal piece
  make_rect(270,250,100,50); //horizontal piece
  make_rect(270,350,100,50); //horizontal piece
  make_rect(390,350,50,100); //vertical piece
  function getYOfId(temp_id)
  {
    for(i = 0; i < rectangle_array.length; i++)
    {
      if(rectangle_array[i].id == temp_id)
        return rectangle_array[i].y;
    }
    return -1;
  }

  function getXOfId(temp_id)
  {
    for(i = 0; i < rectangle_array.length; i++)
    {
      if(rectangle_array[i].id == temp_id)
        return rectangle_array[i].x;
    }
    return -1;
  }

  function getWOfId(temp_id)
  {
    for(i = 0; i < rectangle_array.length; i++)
    {
      if(rectangle_array[i].id == temp_id)
        return rectangle_array[i].width;
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
  var xholder;
  
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
        {
          yholder = getYOfId((rectangle_array.length-i));
          xholder = getXOfId((rectangle_array.length-i));
        }
        flag = false;
        if(getWOfId((rectangle_array.length-i)) == 100) //this means horizontal piece
        {//restrict to horizontal movement
          this.y = yholder; 
          if(this.x <= 0) //restrict is now tested on movement rather than snap after mouse release
            this.x = 0;
          if((rectangle_array[i].x+rectangle_array[i].width) >= (canvas.width))
            rectangle_array[i].x = (canvas.width - rectangle_array[i].width);
        }
        else //this means vertical piece
        {
          this.x = xholder; 
          if(this.y <= 0) //restrict is now tested on movement rather than snap after mouse release
            this.y = 0;
          if((rectangle_array[i].y+rectangle_array[i].height) >= (canvas.height))
            rectangle_array[i].y = (canvas.height - rectangle_array[i].height);
        }
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