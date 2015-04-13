// consulted mdn ws docs https://developer.mozilla.org/en-US/docs/WebSockets/Writing_WebSocket_client_applications

var be = be || {};
be.net = (function(window, document, undefined){
    var lib = {};

    lib._check_sock = function(timeout) {
        if (timeout == 0) {
            return false;
        }

        switch (lib._sock.readyState) {
            case lib._sock.CONNECTING:
                setTimeout(lib._check_sock, 100, timeout - 1);
                break;
            case lib._sock.OPEN:
                lib._socket_helo();
                break;
            default:
                console.log("unknown websocket state: " + lib._sock.readyState);
                return false;
                break;
        }
    };

    lib._socket_helo = function() {
        // TODO: tell the server who we are, etc
        lib._sock.send("[\"echo echo charlie bravo delta\"]\n");

        // set up the event handler for "new data from server"
        lib._sock.onmessage = function(event) {
            console.log("net._sock.onmessage");
            lib._game_json = event.data;
            var msg = JSON.parse(event.data);
            lib._game_state = msg;
        };
    }

    lib.open_socket = function() {
        lib._sock = new WebSocket("ws://localhost:4097");

        setTimeout(lib._check_sock, 100, 127);
    };

    lib.init = function() {
        console.log("net_init");
        if (lib.open_socket()) {

        } else {
            console.log("net error: couldn't open socket to server");
        }

        console.log("TODO: repeated setTimeout");
    };

    // get current gamestate as parsed object
    lib.get_gamestate = function() {
        // TODO: check for not-set-yet?
        return lib._game_state;
    };
    // get current gamestate as json
    lib.get_gamestate_json = function() {
        return lib._game_json;
    };

    lib.init();
    return lib;
}());
