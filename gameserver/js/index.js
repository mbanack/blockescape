var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);

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
});

http.listen(9002, function(){
  console.log('listening on *:9002');
});
