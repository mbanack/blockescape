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
  });
  socket.on('mouse input', function(msg){
    io.emit('mouse input', msg);
  });
  socket.on('draw pieces', function(msg){
    io.emit('draw pieces', msg);
  });
  socket.on('assign me id', function(msg){
    var message = newid + ' ';
    io.emit('assign id html', message);
    newid++;
  });
});

http.listen(9002, function(){
  console.log('listening on *:9002');
});
