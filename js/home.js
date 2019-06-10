$(function () {
    // Buy me a foo
    var things = ["Beer", "Coffee", "Drink", "Pizza"];
    $("#thing").text (things[Math.floor(Math.random()*things.length)]);
});