var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var newid = 0;

app.get('/', function(req, res){
  res.sendFile(__dirname + '/index.html');
});

io.on('connection', function(socket){
  socket.on('num pieces', function(msg){
    io.emit('num pieces', msg);
    //io.sockets.to(id).emit('num pieces', msg);
  });
  socket.on('mouse input', function(msg){
    var substr = msg.substring(0,2);
    console.log('got emit "mouse input" from client ' + substr);
    io.emit('mouse input', msg);
  });
  socket.on('draw pieces', function(msg){
    console.log(msg);
    io.emit('draw pieces', msg);
  });
  socket.on('assign me id', function(){
    io.emit('assign id to html', newid.toString());
    newid++;
  });
  socket.on('output id', function(x){
      console.log('client ' + x + ' joined');
  });
});

http.listen(9002, function(){
  console.log('listening on *:9002');
});
