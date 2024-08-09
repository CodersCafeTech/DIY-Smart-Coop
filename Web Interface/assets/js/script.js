/*  clock */
const clocks = document.querySelectorAll('.clock');

clock = () => {
    let today = new Date();
    let h = today.getHours() % 12 + today.getMinutes() / 59; // 22 % 12 = 10pm
    let m = today.getMinutes(); // 0 - 59
    let s = today.getSeconds(); // 0 - 59

    h *= 30; // 12 * 30 = 360deg
    m *= 6;
    s *= 6; // 60 * 6 = 360deg

    clocks.forEach(clock => {
        const hours = clock.querySelector('.hours');
        const minutes = clock.querySelector('.minutes');
        const seconds = clock.querySelector('.seconds');

        rotation(hours, h);
        rotation(minutes, m);
        rotation(seconds, s);
    });

    // call every second
    setTimeout(clock, 500);
};

rotation = (target, val) => {
    target.style.transform = `rotate(${val}deg)`;
};

window.onload = clock();

function toggleDiv() {
    $('.components').toggle();
    $('.components2').toggle();
}

function toggleDoorSchedule() {
    $('.components').toggle();
    $('.components3').toggle();
}

const firebaseConfig = {
    apiKey: "AIzaSyAdFC7gjn3ziTbYW0-cXi6g6daN66fiNFM",
    authDomain: "fishfeeder-6476e.firebaseapp.com",
    databaseURL: "https://fishfeeder-6476e-default-rtdb.firebaseio.com",
    projectId: "fishfeeder-6476e",
    storageBucket: "fishfeeder-6476e.appspot.com",
    messagingSenderId: "608481250071",
    appId: "1:608481250071:web:a75950105cb2db58798552"
  };
firebase.initializeApp(firebaseConfig);

var countRef = firebase.database().ref('count');
countRef.on('value', function(snapshot) {
    count = snapshot.val()
    console.log(count)
});

function feednow() {
    firebase.database().ref().update({
        feednow: 1
    });
}

var doorRef = firebase.database().ref('doorStatus');
doorRef.on('value', function(snapshot) {
    doorStatus = snapshot.val()
    console.log(doorStatus)
    $('#doorStatus').text(doorStatus+' Door');
});

function toggleDoorStatus() {
    // Get the current status
    doorRef.once('value').then(function(snapshot) {
        var currentStatus = snapshot.val();
        var newStatus = currentStatus === 'Open' ? 'Close' : 'Open';
        $('#doorStatus').text(doorStatus+' Door');
        
        doorRef.set(newStatus, function(error) {
            if (error) {
                console.log("Status could not be updated: " + error.message);
            } else {
                console.log("Status successfully updated to: " + newStatus);
            }
        });
    });
}

$(document).ready(function() {
    $('#timepicker').mdtimepicker(); //Initializes the time picker
    addDiv();
});

$('#timepicker').mdtimepicker().on('timechanged', function(e) {
    console.log(e.time)
    addStore(count, e);
    count = count + 1
    firebase.database().ref().update({
        count: parseInt(count),
    });
});

function addStore(count, e) {
    firebase.database().ref('timers/timer' + count).set({
        time: e.time
    });
    addDiv();
}

$('#openingTime').mdtimepicker().on('timechanged', function(e) {
    console.log(e.time)
    addOpeningTime( e);
});

function addOpeningTime( e) {
    firebase.database().ref('openingTime').set({
        time: e.time
    });
}

$('#closingTime').mdtimepicker().on('timechanged', function(e) {
    console.log(e.time)
    addClosingTime( e);
});

function addClosingTime( e) {
    firebase.database().ref('closingTime').set({
        time: e.time
    });
}

function showShort(id) {
    var idv = $(id)[0]['id']
    $("#time_" + idv).toggle();
    $("#short_" + idv).toggle();

}

function removeDiv(id) {
    var idv = $(id)[0]['id']
    firebase.database().ref('timers/' + idv).remove();
    if (count >= 0) {
        count = count - 1;
    }

    firebase.database().ref().update({
        count: parseInt(count),
    });
    $(id).fadeOut(1, 0).fadeTo(500, 0)
}

function addDiv() {
    var divRef = firebase.database().ref('timers');
    divRef.on('value', function(snapshot) {
        var obj = snapshot.val()
        var i = 0;
        $('#wrapper').html('')
        while (i <= count) {
            var propertyValues = Object.entries(obj);
            let ts = propertyValues[i][1]['time']
                //var timeString = "12:04";
            var H = +ts.substr(0, 2);
            var h = (H % 12) || 12;
            h = (h < 10) ? ("0" + h) : h; // leading 0 at the left for 1 digit hours
            var ampm = H < 12 ? " AM" : " PM";
            ts = h + ts.substr(2, 3) + ampm;
            console.log(ts)

            const x = `
            <div id=${propertyValues[i][0]}>
                <div class="btn2 btn__secondary2" onclick=showShort(${propertyValues[i][0]}) id="main_${propertyValues[i][0]}">
                <div id="time_${propertyValues[i][0]}">
                ${ts}
                </div>
                <div class="icon2" id="short_${propertyValues[i][0]}" onclick=removeDiv(${propertyValues[i][0]})>
                    <div class="icon__add">
                        <ion-icon name="trash"></ion-icon>
                    </div>
                </div>
                </div>
                
                
            </div>`

            $('#wrapper').append(x);
            i++;
        }
    });
}