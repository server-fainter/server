const WebSocket = require('ws');
const net = require('net');

const TCP_PORT = 8080; // C 서버 포트
const WS_PORT = 9090;  // WebSocket 서버 포트

// TCP 서버 연결
function createTcpConnection(ws) {
    const client = new net.Socket();

    client.connect(TCP_PORT, 'localhost', () => {
        console.log('Connected to TCP server');
        // 초기 캔버스 요청
        client.write('INIT_CANVAS');
    });

    client.on('data', (data) => {
        // TCP 서버로부터 데이터 수신 → WebSocket 클라이언트로 전달
        ws.send(data);
    });

    client.on('close', () => {
        console.log('TCP connection closed');
        ws.close();
    });

    client.on('error', (err) => {
        console.error('TCP error:', err);
        ws.close();
    });

    return client;
}

// WebSocket 서버 생성
const wss = new WebSocket.Server({ port: WS_PORT });

wss.on('connection', (ws) => {
    console.log('WebSocket client connected');
    const tcpClient = createTcpConnection(ws);

    ws.on('message', (message) => {
        // WebSocket 클라이언트로부터 데이터 수신 → TCP 서버로 전달
        tcpClient.write(message);
    });

    ws.on('close', () => {
        console.log('WebSocket client disconnected');
        tcpClient.end();
    });

    ws.on('error', (err) => {
        console.error('WebSocket error:', err);
        tcpClient.end();
    });
});

console.log(`WebSocket proxy server is running on ws://localhost:${WS_PORT}`);
