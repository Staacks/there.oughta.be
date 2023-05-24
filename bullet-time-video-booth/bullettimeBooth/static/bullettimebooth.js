var ajaxRequestQueue = [];
var ajaxRequestRunning = false;

var vidNormal, vidFlipped, countdown, footer;
var boothStatus = -1;

function ajax(url, success, fail, always, binary) {
    var request = new XMLHttpRequest();
    if (binary === true)
        request.responseType = "arraybuffer";
    request.open('GET', url, true);
    request.timeout = 3000;

    request.onreadystatechange = function() {
        if (request.readyState == 4) {
            var next = ajaxRequestQueue.shift();
            if (next) {
                setTimeout(function(){next.send()}, 10);
            } else
                ajaxRequestRunning = false;
            if (request.status >= 200 && request.status < 400) {
                if (typeof success !== 'undefined')
                    if (binary === true)
                        success(new Uint8Array(request.response));
                    else
                        success(JSON.parse(request.responseText));
            } else {
                if (typeof fail !== 'undefined')
                    fail();
            }
            if (typeof always !== 'undefined')
                always();
        }
    };

    if (ajaxRequestRunning)
        ajaxRequestQueue.push(request);
    else {
        ajaxRequestRunning = true;
        request.send();
    }
}

function pollStatus() {
    ajax("/status?" + (new Date()).getTime(),
    function(data) { //success
        newStatus = (boothStatus != data["status"]);
        document.body.classList.remove("status"+boothStatus);
        boothStatus = data["status"];
        document.body.classList.add("status"+boothStatus);
        switch(boothStatus) {
            case 0: //Idle
                if (newStatus) {
                    countdown.classList.remove("active");
                    vidNormal.classList.remove("blocked");
                    vidFlipped.classList.remove("blocked");
                    showRandom();
                    footer.innerHTML = "<span class=\"green\">Grün</span> drücken um zu starten.";
                }
                break;
            case 1: //Countdown
                if (newStatus) {
                    footer.innerHTML = "Macht euch bereit...";
                    countdown.classList.add("active");
                    vidNormal.classList.remove("active");
                    vidFlipped.classList.remove("active");
                    vidNormal.classList.add("blocked");
                    vidFlipped.classList.add("blocked");
                }
                countdown.innerHTML = Math.ceil(data["timeRef"] - Date.now()/1000.0);
                break;
            case 2: //Recording
                if (newStatus) {
                    footer.innerHTML = "Aufnahme läuft!";
                    countdown.classList.add("active");
                }
                countdown.innerHTML = Math.ceil(data["timeRef"] + data["duration"] - Date.now()/1000.0);
                break;
            case 3: //Processing
                if (newStatus) {
                    footer.innerHTML = "Vorschau wird erzeugt...";
                    countdown.classList.add("active");
                }
                countdown.innerHTML = Math.ceil(data["timeRef"] + data["finish"] - Date.now()/1000.0);
                break;
            case 4: //Decision
                if (newStatus) {
                    vidNormal.classList.remove("blocked");
                    vidFlipped.classList.remove("blocked");
                    showPreview();
                    footer.innerHTML = "<span class=\"green\">grün</span>: Aufnahme behalten<br /><span class=\"red\">rot</span>: Aufnahme verwerfen";
                    countdown.classList.remove("active");
                }
                break;
            case 5: //Error
                if (newStatus) {
                    footer.innerHTML = "Sorry, es gab ein Problem und ich versuche es selbst zu beheben. Wenn es immer wieder auftaucht oder diese Meldung in einer Minute nicht verschwindet, müsst ihr wohl Sebastian holen...";
                    countdown.classList.remove("active");
                    vidNormal.classList.remove("active");
                    vidFlipped.classList.remove("active");
                    vidNormal.classList.add("blocked");
                    vidFlipped.classList.add("blocked");
                }
                break;
            default:
                console.log("Dafuck?");
        }
    },
    function () { //fail
    },
    function () { //always
        setTimeout(pollStatus, 100);
    });
}

function flipPreview(nextVideo, oldVideo) {
    nextVideo.currentTime = 0;
    nextVideo.classList.add("active");
    oldVideo.classList.remove("active");
    nextVideo.play();
}

function flipRandom(nextVideo, oldVideo) {
    nextVideo.classList.add("active");
    oldVideo.classList.remove("active");
    nextVideo.play();
    oldVideo.src = "/random?i=" + (new Date()).getTime();
    oldVideo.load()
}

function vidNormalEnded() {
    if (boothStatus == 0) {//idle, show randoms
        flipRandom(vidFlipped, vidNormal);
    } else if (boothStatus == 4) {//Decision, showing preview
        flipPreview(vidFlipped, vidNormal);
    }
}

function vidFlippedEnded() {
    if (boothStatus == 0) {//idle, show randoms
        flipRandom(vidNormal, vidFlipped);
    } else if (boothStatus == 4) {//Decision, showing preview
        flipPreview(vidNormal, vidFlipped);
    }
}

function showPreview() {
    url = "/preview?" + (new Date()).getTime();
    vidNormal.src = url;
    vidNormal.load();
    vidFlipped.src = url;
    vidFlipped.load();
    vidNormal.classList.add("active");
    vidFlipped.classList.remove("active");
    vidNormal.play();
}

function showRandom() {
    vidNormal.src = "/random?i=2" + (new Date()).getTime();
    vidNormal.load();
    vidFlipped.src = "/random?i=1" + (new Date()).getTime();
    vidFlipped.load();
    vidNormal.classList.add("active");
    vidFlipped.classList.remove("active");
    vidNormal.play();
}

function onKey(event) {
    if (event.key == " ") {
        ajax('/control?cmd=ok');
    } else if (event.key === "Escape") {
        ajax('/control?cmd=abort');
    }
}

function ready(fn) {
    if (document.attachEvent ? document.readyState === "complete" : document.readyState !== "loading") {
        fn();
    } else {
        document.addEventListener('DOMContentLoaded', fn);
    }
}

ready(function() {
    vidNormal = document.getElementById("vidNormal");
    vidFlipped = document.getElementById("vidFlipped");
    vidNormal.addEventListener('ended', vidNormalEnded, false);
    vidFlipped.addEventListener('ended', vidFlippedEnded, false);
    countdown = document.getElementById("countdown");
    footer = document.getElementById("footer");
    pollStatus();
    document.onkeydown = onKey;
});
