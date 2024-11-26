const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');
const colorInput = document.getElementById('color');

// 캔버스 크기를 확대하여 표시 (픽셀 확대 효과)
canvas.style.width = '500px';
canvas.style.height = '500px';

// 서버에 연결
const ws = new WebSocket('ws://localhost:8080', 'rplace-protocol');
ws.binaryType = 'arraybuffer';

// 캔버스 초기화
let pixelData = new Uint8Array(100 * 100 * 4); // RGBA 형식

// 캔버스 초기화: 모든 픽셀을 흰색으로 설정
for (let i = 0; i < pixelData.length; i += 4) {
    pixelData[i] = 255;     // R
    pixelData[i + 1] = 255; // G
    pixelData[i + 2] = 255; // B
    pixelData[i + 3] = 255; // A (불투명도)
}
updateCanvas();

// 서버로부터 메시지를 수신하면 캔버스를 업데이트
ws.onmessage = function(event) {
    const data = new Uint8Array(event.data);
    for (let i = 0; i < data.length; i += 3) {
        const x = data[i];
        const y = data[i + 1];
        const color = data[i + 2];

        // 색상을 적용하여 픽셀 데이터 업데이트
        const index = (y * 100 + x) * 4;
        const rgbColor = colorToRGB(color);
        pixelData[index] = rgbColor.r;
        pixelData[index + 1] = rgbColor.g;
        pixelData[index + 2] = rgbColor.b;
        pixelData[index + 3] = 255; // 불투명도
    }
    updateCanvas();
};

// 캔버스에 클릭 이벤트 리스너 추가
canvas.addEventListener('click', function(event) {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    const x = Math.floor((event.clientX - rect.left) * scaleX);
    const y = Math.floor((event.clientY - rect.top) * scaleY);

    const color = parseInt(colorInput.value);

    // 서버로 픽셀 정보 전송
    const message = new Uint8Array([x, y, color]);
    ws.send(message.buffer);
});

// 캔버스 업데이트 함수
function updateCanvas() {
    const imageData = new ImageData(new Uint8ClampedArray(pixelData), 100, 100);
    ctx.putImageData(imageData, 0, 0);
}

// 색상 인덱스를 RGB로 변환하는 함수 (8가지 색상 사용)
function colorToRGB(colorIndex) {
    const colors = [
        { r: 255, g: 255, b: 255 }, // 흰색
        { r: 0, g: 0, b: 0 },       // 검은색
        { r: 255, g: 0, b: 0 },     // 빨강
        { r: 0, g: 255, b: 0 },     // 초록
        { r: 0, g: 0, b: 255 },     // 파랑
        { r: 255, g: 255, b: 0 },   // 노랑
        { r: 0, g: 255, b: 255 },   // 청록
        { r: 255, g: 0, b: 255 },   // 자홍
    ];
    return colors[colorIndex % colors.length];
}
