
var gateway = `ws://${window.location.hostname}/wbskt`;
var websocket;
var reinitWebSocketWaitTime = 2000;
var maxNumCliExCode = 4050;
var tooManyClientsConnected = false;
// Init web socket when the page loads
window.addEventListener('load', onload);

// Establish websocket connection to ESP8266 device
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onload(event) {
    initWebSocket();
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Closing code is: ' + event.code);
    if (event.code === maxNumCliExCode || tooManyClientsConnected) {
       document.getElementById('temp').innerHTML = 'CONN_DENIED';
       document.getElementById('humd').innerHTML = 'CONN_DENIED';
       console.log('Closing connection permanently due to too many clients connected...');
    } else {
       console.log('Connection closed, retrying to connect...');
       setTimeout(initWebSocket, reinitWebSocketWaitTime);
    }
}

// Function that receives the message from the ESP8266 with the readings
function onMessage(event) {
    console.log(event.data);
    let eventDataDict = JSON.parse(event.data);
    let eventDataKeys = Object.keys(eventDataDict);
    for (const key of eventDataKeys){
        if (eventDataDict[key] === 'CONN_DENIED') {
             tooManyClientsConnected = true;
             break;
        }
        document.getElementById(key).innerHTML = eventDataDict[key];
    }
}
