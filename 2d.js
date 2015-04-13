(function(window, document, undefined){
    twod_init = function() {
        console.log("2d_init");
        try {
            var canvas = oCanvas.create({
                canvas: "#canvas"
            });

            var image = canvas.display.image({
                x: 177,
                y: 120,
                origin: { x: "center", y: "center" },
                image: "1.jpg"
            });

            canvas.addChild(image);
        } catch(err) {
            console.log("2d_init error: " + err.message);
        }
    }();
}());
