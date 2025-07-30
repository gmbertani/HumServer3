const wsProtocol = location.protocol === "https:" ? "wss" : "ws";
const wsHost = location.hostname;
const wsPort = 12345;

const socket = new WebSocket(`${wsProtocol}://${wsHost}:${wsPort}`);


socket.onopen = () => {
    new QWebChannel(socket, function(channel) {
        window.humBridge = channel.objects.humBridge;

        function updateList() {
            const list = document.getElementById("dataList");
            list.innerHTML = '';
            humBridge.dataList.forEach((val, i) => {
                const li = document.createElement("li");
                li.textContent = val;
                list.appendChild(li);
            });
        }

        updateList();
        humBridge.dataListChanged.connect(updateList);

        humBridge.logSent.connect(function(msg) {
            alert("Risposta da Qt: " + msg);
        });
    });
};

socket.onerror = err => {
    console.error("Errore WebSocket:", err);
};
